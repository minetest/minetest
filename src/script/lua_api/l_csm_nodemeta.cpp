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
	CSMS2CNodeMetaClear send;
	send.pos = m_p;
	CSM_IPC(exchange(send));
}

bool CSMNodeMetadata::contains(const std::string &name) const
{
	CSMS2CNodeMetaContains header;
	header.pos = m_p;
	std::string send = std::string((char *)&header, sizeof(header)) + name;
	CSM_IPC(exchange(send.size(), send.data()));
	bool contains;
	sanity_check(csm_recv_size() >= sizeof(contains));
	memcpy(&contains, csm_recv_data(), sizeof(contains));
	return contains;
}

bool CSMNodeMetadata::setString(const std::string &name, const std::string &var)
{
	std::ostringstream os(std::ios::binary);
	CSMS2CNodeMetaSetString header;
	header.pos = m_p;
	os << std::string((char *)&header, sizeof(header))
			<< serializeString16(name) << serializeString32(var);
	std::string send = os.str();
	CSM_IPC(exchange(send.size(), send.data()));
	bool modified;
	sanity_check(csm_recv_size() >= sizeof(modified));
	memcpy(&modified, csm_recv_data(), sizeof(modified));
	return modified;
}

const StringMap &CSMNodeMetadata::getStrings(StringMap *place) const
{
	CSMS2CNodeMetaGetStrings send;
	send.pos = m_p;
	CSM_IPC(exchange(send));
	std::istringstream is(std::string((char *)csm_recv_data(), csm_recv_size()),
			std::ios::binary);
	place->clear();
	u32 n_fields = readU32(is);
	place->reserve(n_fields);
	for (u32 i = 0; i < n_fields; i++) {
		std::string key = deSerializeString16(is);
		(*place)[std::move(key)] = deSerializeString32(is);
	}
	return *place;
}

const std::vector<std::string> &CSMNodeMetadata::getKeys(std::vector<std::string> *place) const
{
	CSMS2CNodeMetaGetKeys send;
	send.pos = m_p;
	CSM_IPC(exchange(send));
	std::istringstream is(std::string((char *)csm_recv_data(), csm_recv_size()),
			std::ios::binary);
	place->clear();
	u32 n_keys = readU32(is);
	place->reserve(n_keys);
	for (u32 i = 0; i < n_keys; i++)
		place->push_back(deSerializeString16(is));
	return *place;
}

const std::string *CSMNodeMetadata::getStringRaw(const std::string &name,
		std::string *place) const
{
	CSMS2CNodeMetaGetString header;
	header.pos = m_p;
	std::string send = std::string((char *)&header, sizeof(header)) + name;
	CSM_IPC(exchange(send.size(), send.data()));
	if (csm_recv_size() > 0) {
		place->assign((char *)csm_recv_data(), csm_recv_size());
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
