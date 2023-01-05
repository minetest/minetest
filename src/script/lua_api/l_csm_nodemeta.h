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

#pragma once

#include "irr_v3d.h"
#include "metadata.h"
#include "lua_api/l_base.h"
#include "lua_api/l_metadata.h"

class CSMNodeMetadata : public IMetadata
{
public:
	CSMNodeMetadata(v3s16 p): m_p(p) {}

	~CSMNodeMetadata() = default;

	void clear() override;

	bool contains(const std::string &name) const override;

	bool setString(const std::string &name, const std::string &var) override;

	const StringMap &getStrings(StringMap *place) const override;

	const std::vector<std::string> &getKeys(std::vector<std::string> *place) const override;

protected:
	const std::string *getStringRaw(const std::string &name,
			std::string *place) const override;

private:
	v3s16 m_p;
};

class CSMNodeMetaRef : public MetaDataRef
{
public:
	CSMNodeMetaRef(v3s16 p): m_meta(p) {}

	~CSMNodeMetaRef() = default;

	static void create(lua_State *L, v3s16 p);

	static const char className[];

	static void Register(lua_State *L);

protected:
	IMetadata* getmeta(bool auto_create) override;

	void clearMeta() override;

private:
	CSMNodeMetadata m_meta;

	static const luaL_Reg methods[];
};
