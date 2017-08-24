/*
Minetest
Copyright (C) 2010-2017 Vincent Glize, Dumbeldor <vincent.glize@live.fr>

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
#include "content_sao.h"

class NpcSAO : public LuaEntitySAO
{
public:
	NpcSAO(ServerEnvironment *env, v3f pos,
				 const std::string &name, const std::string &state);
	virtual ~NpcSAO();
	ActiveObjectType getType() const
	{ return ACTIVEOBJECT_TYPE_NPC; }
	ActiveObjectType getSendType() const
	{ return ACTIVEOBJECT_TYPE_GENERIC; }
	virtual void addedToEnvironment(u32 dtime_s);

	static ServerActiveObject *create(ServerEnvironment *env, v3f pos,
									  const std::string &data);
};

