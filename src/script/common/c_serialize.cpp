/*
Minetest
Copyright (C) 2022 sfan5

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

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include "c_serialize.h"
#include "c_internal.h"
#include "log.h"
#include "debug.h"
#include "threading/mutex_auto_lock.h"

extern "C" {
#include <lauxlib.h>
}

// convert negative index to absolute position on Lua stack
#define absidx(_L, _idx) (assert((_idx) < 0), lua_gettop(_L) + (_idx) + 1)

// does the type put anything into PackedInstr::sdata?
static inline bool uses_sdata(int type)
{
	switch (type) {
		case LUA_TSTRING:
		case LUA_TFUNCTION:
		case LUA_TUSERDATA:
			return true;
		default:
			return false;
	}
}

// does the type put anything into PackedInstr::<union>?
static inline bool uses_union(int type)
{
	switch (type) {
		case LUA_TNIL:
		case LUA_TSTRING:
		case LUA_TFUNCTION:
			return false;
		default:
			return true;
	}
}

static inline bool can_set_into(int ktype, int vtype)
{
	switch (ktype) {
		case LUA_TNUMBER:
			return !uses_union(vtype);
		case LUA_TSTRING:
			return !uses_sdata(vtype);
		default:
			return false;
	}
}

// is the key suitable for use with set_into?
static inline bool suitable_key(lua_State *L, int idx)
{
	if (lua_type(L, idx) == LUA_TSTRING) {
		// strings may not have a NULL byte
		size_t len;
		const char *str = lua_tolstring(L, idx, &len);
		return strlen(str) == len;
	} else {
		assert(lua_type(L, idx) == LUA_TNUMBER);
		// numbers must fit into an s32 and be integers
		lua_Number n = lua_tonumber(L, idx);
		return std::floor(n) == n && n >= S32_MIN && n <= S32_MAX;
	}
}

namespace {
	// checks if you left any values on the stack, for debugging
	class StackChecker {
		lua_State *L;
		int top;
	public:
		StackChecker(lua_State *L) : L(L), top(lua_gettop(L)) {}
		~StackChecker() {
			assert(lua_gettop(L) >= top);
			if (lua_gettop(L) > top) {
				rawstream << "Lua stack not cleaned up: "
					<< lua_gettop(L) << " != " << top
					<< " (false-positive if exception thrown)" << std::endl;
			}
		}
	};

	// Since an std::vector may reallocate this is the only safe way to keep
	// a reference to a particular instruction.
	template <typename T>
	class VectorRef {
		std::vector<T> *vec;
		size_t idx;
		VectorRef(std::vector<T> *vec, size_t idx) : vec(vec), idx(idx) {}
	public:
		static VectorRef<T> front(std::vector<T> &vec) {
			return VectorRef(&vec, 0);
		}
		static VectorRef<T> back(std::vector<T> &vec) {
			return VectorRef(&vec, vec.size() - 1);
		}
		T &operator*() { return (*vec)[idx]; }
		T *operator->() { return &(*vec)[idx]; }
	};

	struct Serializer {
		PackInFunc fin;
		PackOutFunc fout;
	};

	typedef std::pair<std::string, Serializer> SerializerTuple;
}

static inline auto emplace(PackedValue &pv, s16 type)
{
	pv.i.emplace_back();
	auto ref = VectorRef<PackedInstr>::back(pv.i);
	ref->type = type;
	if (type == LUA_TTABLE) {
		ref->uidata1 = 0;
		ref->uidata2 = 0;
	} else if (type == INSTR_POP) {
		ref->sidata2 = 0;
	}
	return ref;
}

/**************/

static std::unordered_map<std::string, Serializer> serializers;
static std::mutex serializers_lock;

void script_register_serializer(lua_State *L, const char *regname,
	PackInFunc fin, PackOutFunc fout)
{
	// Save serialization functions
	{
		MutexAutoLock autolock(serializers_lock);
		auto it = serializers.find(regname);
		if (it == serializers.end()) {
			auto &ref = serializers[regname];
			ref.fin = fin;
			ref.fout = fout;
		} else {
			FATAL_ERROR_IF(it->second.fin != fin || it->second.fout != fout,
				"Serializer registered twice with inconsistent values");
		}
	}

	// Save metatable so we can identify instances later
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_METATABLE_MAP);
	if (lua_isnil(L, -1)) {
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_rawseti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_METATABLE_MAP);
	}

	luaL_getmetatable(L, regname);
	FATAL_ERROR_IF(lua_isnil(L, -1), "No metatable registered with that name");

	// CUSTOM_RIDX_METATABLE_MAP contains { [metatable] = "regname", ... }
	// check first
	lua_pushstring(L, regname);
	lua_rawget(L, -3);
	if (!lua_isnil(L, -1)) {
		FATAL_ERROR_IF(lua_topointer(L, -1) != lua_topointer(L, -2),
				"Serializer registered twice with inconsistent metatable");
	}
	lua_pop(L, 1);
	// then set
	lua_pushstring(L, regname);
	lua_rawset(L, -3);

	lua_pop(L, 1);
}

static bool find_serializer(const char *regname, SerializerTuple &out)
{
	MutexAutoLock autolock(serializers_lock);
	auto it = serializers.find(regname);
	if (it == serializers.end())
		return false;
	// copy data for thread safety
	out.first = it->first;
	out.second = it->second;
	return true;
}

static bool find_serializer(lua_State *L, int idx, SerializerTuple &out)
{
#ifndef NDEBUG
	StackChecker checker(L);
#endif

	// retrieve metatable of the object
	if (lua_getmetatable(L, idx) != 1)
		return false;

	// use our global table to map it to the registry name
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_METATABLE_MAP);
	assert(lua_istable(L, -1));
	lua_pushvalue(L, -2);
	lua_rawget(L, -2);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 3);
		return false;
	}

	// load the associated data
	bool found = find_serializer(lua_tostring(L, -1), out);
	FATAL_ERROR_IF(!found, "Inconsistent internal state");
	lua_pop(L, 3);
	return true;
}

/**************/

// recursively goes through the structure and ensures there are no circular references
static void pack_validate(lua_State *L, int idx, std::unordered_set<const void*> &seen)
{
#ifndef NDEBUG
	StackChecker checker(L);
	assert(idx > 0);
#endif

	if (lua_type(L, idx) != LUA_TTABLE)
		return;

	const void *ptr = lua_topointer(L, idx);
	assert(ptr);

	if (seen.find(ptr) != seen.end())
		throw LuaError("Circular references cannot be serialized (yet)");
	seen.insert(ptr);

	lua_checkstack(L, 5);
	lua_pushnil(L);
	while (lua_next(L, idx) != 0) {
		// key at -2, value at -1
		pack_validate(L, absidx(L, -2), seen);
		pack_validate(L, absidx(L, -1), seen);

		lua_pop(L, 1);
	}

	seen.erase(ptr);
}

static VectorRef<PackedInstr> pack_inner(lua_State *L, int idx, int vidx, PackedValue &pv)
{
#ifndef NDEBUG
	StackChecker checker(L);
	assert(idx > 0);
	assert(vidx > 0);
#endif

	switch (lua_type(L, idx)) {
		case LUA_TNONE:
		case LUA_TNIL:
			return emplace(pv, LUA_TNIL);
		case LUA_TBOOLEAN: {
			auto r = emplace(pv, LUA_TBOOLEAN);
			r->bdata = lua_toboolean(L, idx);
			return r;
		}
		case LUA_TNUMBER: {
			auto r = emplace(pv, LUA_TNUMBER);
			r->ndata = lua_tonumber(L, idx);
			return r;
		}
		case LUA_TSTRING: {
			auto r = emplace(pv, LUA_TSTRING);
			size_t len;
			const char *str = lua_tolstring(L, idx, &len);
			assert(str);
			r->sdata.assign(str, len);
			return r;
		}
		case LUA_TTABLE:
			break; // execution continues
		case LUA_TFUNCTION: {
			auto r = emplace(pv, LUA_TFUNCTION);
			call_string_dump(L, idx);
			size_t len;
			const char *str = lua_tolstring(L, -1, &len);
			assert(str);
			r->sdata.assign(str, len);
			lua_pop(L, 1);
			return r;
		}
		case LUA_TUSERDATA: {
			SerializerTuple ser;
			if (!find_serializer(L, idx, ser))
				throw LuaError("Cannot serialize userdata object");
			pv.contains_userdata = true;
			auto r = emplace(pv, LUA_TUSERDATA);
			r->sdata = ser.first;
			r->ptrdata = ser.second.fin(L, idx);
			return r;
		}
		default: {
			std::string err = "Cannot serialize type ";
			err += lua_typename(L, lua_type(L, idx));
			throw LuaError(err);
		}
	}

	// LUA_TTABLE
	lua_checkstack(L, 5);

	auto rtable = emplace(pv, LUA_TTABLE);
	const int vi_table = vidx++;

	lua_pushnil(L);
	while (lua_next(L, idx) != 0) {
		// key at -2, value at -1
		const int ktype = lua_type(L, -2), vtype = lua_type(L, -1);
		if (ktype == LUA_TNUMBER)
			rtable->uidata1++; // narr
		else
			rtable->uidata2++; // nrec

		// check if we can use a shortcut
		if (can_set_into(ktype, vtype) && suitable_key(L, -2)) {
			// push only the value
			auto rval = pack_inner(L, absidx(L, -1), vidx, pv);
			rval->pop = vtype != LUA_TTABLE;
			// and where to put it:
			rval->set_into = vi_table;
			if (ktype == LUA_TSTRING)
				rval->sdata = lua_tostring(L, -2);
			else
				rval->sidata1 = lua_tointeger(L, -2);
			// pop tables after the fact
			if (!rval->pop) {
				auto ri1 = emplace(pv, INSTR_POP);
				ri1->sidata1 = vidx;
			}
		} else {
			// push the key and value
			auto rkey = pack_inner(L, absidx(L, -2), vidx, pv);
			vidx++;
			auto rval = pack_inner(L, absidx(L, -1), vidx, pv);
			vidx++;
			// push an instruction to set them
			auto ri1 = emplace(pv, INSTR_SETTABLE);
			ri1->set_into = vi_table;
			ri1->sidata1 = vidx - 2;
			ri1->sidata2 = vidx - 1;
			ri1->pop = true;
			vidx -= 2;
		}

		lua_pop(L, 1);
	}

	assert(vidx == vi_table + 1);
	return rtable;
}

PackedValue *script_pack(lua_State *L, int idx)
{
	if (idx < 0)
		idx = absidx(L, idx);

	std::unordered_set<const void*> seen;
	pack_validate(L, idx, seen);
	assert(seen.size() == 0);

	// Actual serialization
	PackedValue pv;
	pack_inner(L, idx, 1, pv);

	return new PackedValue(std::move(pv));
}

/**************/

void script_unpack(lua_State *L, PackedValue *pv)
{
	const int top = lua_gettop(L);
	int ctr = 0;

	for (auto &i : pv->i) {
		if (!i.pop && (ctr++) >= 5) {
			lua_checkstack(L, 10);
			ctr = 0;
		}

		/* Instructions */
		switch (i.type) {
			case INSTR_SETTABLE:
				lua_pushvalue(L, top + i.sidata1); // key
				lua_pushvalue(L, top + i.sidata2); // value
				lua_rawset(L, top + i.set_into);
				if (i.pop) {
					if (i.sidata1 != i.sidata2) {
						// removing moves indices so pop higher index first
						lua_remove(L, top + std::max(i.sidata1, i.sidata2));
						lua_remove(L, top + std::min(i.sidata1, i.sidata2));
					} else {
						lua_remove(L, top + i.sidata1);
					}
				}
				continue;
			case INSTR_POP:
				lua_remove(L, top + i.sidata1);
				if (i.sidata2 > 0)
					lua_remove(L, top + i.sidata2);
				continue;
			default:
				break;
		}

		/* Lua types */
		switch (i.type) {
			case LUA_TNIL:
				lua_pushnil(L);
				break;
			case LUA_TBOOLEAN:
				lua_pushboolean(L, i.bdata);
				break;
			case LUA_TNUMBER:
				lua_pushnumber(L, i.ndata);
				break;
			case LUA_TSTRING:
				lua_pushlstring(L, i.sdata.data(), i.sdata.size());
				break;
			case LUA_TTABLE:
				lua_createtable(L, i.uidata1, i.uidata2);
				break;
			case LUA_TFUNCTION:
				luaL_loadbuffer(L, i.sdata.data(), i.sdata.size(), nullptr);
				break;
			case LUA_TUSERDATA: {
				SerializerTuple ser;
				FATAL_ERROR_IF(!find_serializer(i.sdata.c_str(), ser),
					"Can't find serializer (data corruption?)");
				ser.second.fout(L, i.ptrdata);
				i.ptrdata = nullptr; // ownership taken by callback
				break;
			}
			default:
				assert(0);
				break;
		}

		if (i.set_into) {
			if (!i.pop)
				lua_pushvalue(L, -1);
			if (uses_sdata(i.type))
				lua_rawseti(L, top + i.set_into, i.sidata1);
			else
				lua_setfield(L, top + i.set_into, i.sdata.c_str());
		} else {
			if (i.pop)
				lua_pop(L, 1);
		}
	}

	// as part of the unpacking process we take ownership of all userdata
	pv->contains_userdata = false;
	// leave exactly one value on the stack
	lua_settop(L, top+1);
}

/**************/

PackedValue::~PackedValue()
{
	if (!contains_userdata)
		return;
	for (auto &i : this->i) {
		if (i.type == LUA_TUSERDATA && i.ptrdata) {
			SerializerTuple ser;
			if (find_serializer(i.sdata.c_str(), ser)) {
				// tell it to deallocate object
				ser.second.fout(nullptr, i.ptrdata);
			} else {
				assert(false);
			}
		}
	}
}

/**************/

#ifndef NDEBUG
void script_dump_packed(const PackedValue *val)
{
	printf("instruction stream: [\n");
	for (const auto &i : val->i) {
		printf("\t(");
		switch (i.type) {
			case INSTR_SETTABLE:
				printf("SETTABLE(%d, %d)", i.sidata1, i.sidata2);
				break;
			case INSTR_POP:
				printf(i.sidata2 ? "POP(%d, %d)" : "POP(%d)", i.sidata1, i.sidata2);
				break;
			case LUA_TNIL:
				printf("nil");
				break;
			case LUA_TBOOLEAN:
				printf(i.bdata ? "true" : "false");
				break;
			case LUA_TNUMBER:
				printf("%f", i.ndata);
				break;
			case LUA_TSTRING:
				printf("\"%s\"", i.sdata.c_str());
				break;
			case LUA_TTABLE:
				printf("table(%d, %d)", i.uidata1, i.uidata2);
				break;
			case LUA_TFUNCTION:
				printf("function(%d byte)", i.sdata.size());
				break;
			case LUA_TUSERDATA:
				printf("userdata %s %p", i.sdata.c_str(), i.ptrdata);
				break;
			default:
				printf("!!UNKNOWN!!");
				break;
		}
		if (i.set_into) {
			if (i.type >= 0 && uses_sdata(i.type))
				printf(", k=%d, into=%d", i.sidata1, i.set_into);
			else if (i.type >= 0)
				printf(", k=\"%s\", into=%d", i.sdata.c_str(), i.set_into);
			else
				printf(", into=%d", i.set_into);
		}
		if (i.pop)
			printf(", pop");
		printf(")\n");
	}
	printf("]\n");
}
#endif
