/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "cpp_api/s_base.h"
#include "cpp_api/s_nodemeta.h"
#include "util/string.h"

struct MapNode;
class ServerActiveObject;

class ScriptApiNode
		: virtual public ScriptApiBase,
		  public ScriptApiNodemeta
{
public:
	ScriptApiNode() = default;
	virtual ~ScriptApiNode() = default;

	bool node_on_punch(v3s16 p, MapNode node,
			ServerActiveObject *puncher, const PointedThing &pointed);
	bool node_on_dig(v3s16 p, MapNode node,
			ServerActiveObject *digger);
	void node_on_construct(v3s16 p, MapNode node);
	void node_on_destruct(v3s16 p, MapNode node);
	bool node_on_flood(v3s16 p, MapNode node, MapNode newnode);
	void node_after_destruct(v3s16 p, MapNode node);
	bool node_on_timer(v3s16 p, MapNode node, f32 dtime);
	void node_on_receive_fields(v3s16 p,
			const std::string &formname,
			const StringMap &fields,
			ServerActiveObject *sender);
public:
	static struct EnumString es_DrawType[];
	static struct EnumString es_ContentParamType[];
	static struct EnumString es_ContentParamType2[];
	static struct EnumString es_LiquidType[];
	static struct EnumString es_LiquidMoveType[];
	static struct EnumString es_NodeBoxType[];
	static struct EnumString es_TextureAlphaMode[];
};
