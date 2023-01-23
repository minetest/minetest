/*
Minetest
Copyright (C) 2023 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "csm/csm_message.h"
#include "csm/csm_script_ipc.h"
#include "lua_api/l_csm_nodemeta.h"
#include "lua_api/l_internal.h"
#include "util/serialize.h"
#include <sstream>
#include <utility>

void CSMNodeMetadata::clear()
{
	CSMS2CNodeMetaClear send(CSM_S2C_NODE_META_CLEAR, m_p);
	csm_exchange_msg(send);
}

bool CSMNodeMetadata::contains(const std::string &name) const
{
	CSMS2CNodeMetaContains send(CSM_S2C_NODE_META_CONTAINS, m_p, name);
	csm_exchange_msg(send);
	return csm_deserialize_msg<bool>();
}

bool CSMNodeMetadata::setString(const std::string &name, const std::string &var)
{
	CSMS2CNodeMetaSetString send(CSM_S2C_NODE_META_SET_STRING, m_p, name, var);
	csm_exchange_msg(send);
	return csm_deserialize_msg<bool>();
}

const StringMap &CSMNodeMetadata::getStrings(StringMap *place) const
{
	CSMS2CNodeMetaClear send(CSM_S2C_NODE_META_GET_STRINGS, m_p);
	csm_exchange_msg(send);
	return *place = csm_deserialize_msg<StringMap>();
}

const std::vector<std::string> &CSMNodeMetadata::getKeys(std::vector<std::string> *place) const
{
	CSMS2CNodeMetaClear send(CSM_S2C_NODE_META_GET_STRINGS, m_p);
	csm_exchange_msg(send);
	return *place = csm_deserialize_msg<std::vector<std::string> >();
}

const std::string *CSMNodeMetadata::getStringRaw(const std::string &name,
		std::string *place) const
{
	CSMS2CNodeMetaContains send(CSM_S2C_NODE_META_GET_STRING, m_p, name);
	csm_exchange_msg(send);
	auto recv = csm_deserialize_msg<std::string>();
	if (recv.size() > 0) {
		*place = std::move(recv);
		return place;
	} else {
		return nullptr;
	}
}

void CSMNodeMetaRef::create(lua_State *L, v3s16 p)
{
	CSMNodeMetaRef **ud = (CSMNodeMetaRef **)lua_newuserdata(L, sizeof(*ud));
	*ud = new CSMNodeMetaRef(p);
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

IMetadata *CSMNodeMetaRef::getmeta(bool auto_create)
{
	return &m_meta;
}

void CSMNodeMetaRef::clearMeta()
{
	m_meta.clear();
}

const char CSMNodeMetaRef::className[] = "NodeMetaRef";

void CSMNodeMetaRef::Register(lua_State *L)
{
	registerMetadataClass(L, className, methods);
}

const luaL_Reg CSMNodeMetaRef::methods[] = {
	luamethod(MetaDataRef, contains),
	luamethod(MetaDataRef, get),
	luamethod(MetaDataRef, get_string),
	luamethod(MetaDataRef, set_string),
	luamethod(MetaDataRef, get_int),
	luamethod(MetaDataRef, set_int),
	luamethod(MetaDataRef, get_float),
	luamethod(MetaDataRef, set_float),
	luamethod(MetaDataRef, get_keys),
	luamethod(MetaDataRef, to_table),
	luamethod(MetaDataRef, from_table),
	luamethod(MetaDataRef, equals),
	{0,0}
};
