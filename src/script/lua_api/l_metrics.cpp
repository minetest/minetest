/*
Minetest
Copyright (C) 2021  rubenwardy

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "lua_api/l_internal.h"
#include "common/c_content.h"
#include "lua_api/l_metrics.h"
#include "server.h"

#if USE_PROMETHEUS

class MetricRef : public ModApiBase
{
	MetricPtr metric;

	explicit MetricRef(MetricPtr metric) : metric(metric) {}

	// increment(v)
	static int l_increment(lua_State *L)
	{
		auto metric = checkMetric(L, 1);

		double number = readParam<double>(L, 2, 1.0);
		metric->increment(number);
		return 0;
	}

	// decrement(v)
	static int l_decrement(lua_State *L)
	{
		auto metric = checkMetricGauge(L, 1);

		double number = readParam<double>(L, 2, 1.0);
		metric->decrement(number);
		return 0;
	}

	// set(v)
	static int l_set(lua_State *L)
	{
		auto metric = checkMetricGauge(L, 1);

		double number = luaL_checknumber(L, 2);
		metric->set(number);
		return 0;
	}

	// get()
	static int l_get(lua_State *L)
	{
		auto metric = checkMetric(L, 1);

		lua_pushnumber(L, metric->get());
		return 1;
	}

	// garbage collector
	static int gc_object(lua_State *L)
	{
		MetricRef *o = *(MetricRef **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

	static MetricRef *checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);

		void *ud = luaL_checkudata(L, narg, className);
		if (!ud)
			luaL_typerror(L, narg, className);

		return *(MetricRef **)ud; // unbox pointer
	}

	static MetricPtr checkMetric(lua_State *L, int narg)
	{
		auto ref = checkobject(L, narg);
		return ref->metric;
	}

	static MetricGauge *checkMetricGauge(lua_State *L, int narg)
	{
		auto ref = checkobject(L, narg);
		auto ret = dynamic_cast<MetricGauge *>(ref->metric.get());
		if (!ret)
			throw LuaError("Expected gauge metric");
		return ret;
	}

	static const char className[];
	static const luaL_Reg methods[];

public:
	static void create(lua_State *L, MetricPtr metric)
	{
		MetricRef *o = new MetricRef(metric);
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
	}

	static void Register(lua_State *L)
	{
		lua_newtable(L);
		int methodtable = lua_gettop(L);
		luaL_newmetatable(L, className);
		int metatable = lua_gettop(L);

		lua_pushliteral(L, "__metatable");
		lua_pushvalue(L, methodtable);
		lua_settable(L, metatable); // hide metatable from lua getmetatable()

		lua_pushliteral(L, "__index");
		lua_pushvalue(L, methodtable);
		lua_settable(L, metatable);

		lua_pushliteral(L, "__gc");
		lua_pushcfunction(L, gc_object);
		lua_settable(L, metatable);

		lua_pop(L, 1); // Drop metatable

		luaL_openlib(L, 0, methods, 0); // fill methodtable
		lua_pop(L, 1);			// Drop methodtable
	}
};

const char MetricRef::className[] = "MetricsGaugeRef";
const luaL_Reg MetricRef::methods[] = {
		luamethod(MetricRef, increment),
		luamethod(MetricRef, decrement),
		luamethod(MetricRef, set),
		luamethod(MetricRef, get),
		{0, 0},
};

// create_metric(type, name, help)
int ModApiMetrics::l_create_metric(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	std::string type = luaL_checkstring(L, 1);
	std::string name = luaL_checkstring(L, 2);
	std::string help = luaL_checkstring(L, 3);

	name.insert(0, "minetest_");

	auto metrics = getServer(L)->getMetrics();
	if (type == "counter") {
		MetricRef::create(L, metrics->addCounter(name, help));
	} else if (type == "gauge") {
		MetricRef::create(L, metrics->addGauge(name, help));
	} else {
		throw LuaError(std::string("Unknown metric type ") + type);
	}
	return 1;
}

void ModApiMetrics::Initialize(lua_State *L, int top)
{
	MetricRef::Register(L);

	API_FCT(create_metric);
}

#endif
