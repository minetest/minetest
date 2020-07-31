/*
Minetest
Copyright (C) 2010-2018 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "hud.h"
#include "network/networkpacket.h"
#include "script/common/c_content.h"
#include "script/common/c_converter.h"

const struct EnumString es_HudElementType[] =
{
	{HUD_ELEM_IMAGE,     "image"},
	{HUD_ELEM_TEXT,      "text"},
	{HUD_ELEM_STATBAR,   "statbar"},
	{HUD_ELEM_INVENTORY, "inventory"},
	{HUD_ELEM_WAYPOINT,  "waypoint"},
	{HUD_ELEM_IMAGE_WAYPOINT, "image_waypoint"},
	{0, NULL},
};

const struct EnumString es_HudElementStat[] =
{
	{HUD_STAT_POS,    "position"},
	{HUD_STAT_POS,    "pos"}, /* Deprecated, only for compatibility's sake */
	{HUD_STAT_NAME,   "name"},
	{HUD_STAT_SCALE,  "scale"},
	{HUD_STAT_TEXT,   "text"},
	{HUD_STAT_NUMBER, "number"},
	{HUD_STAT_ITEM,   "item"},
	{HUD_STAT_DIR,    "direction"},
	{HUD_STAT_ALIGN,  "alignment"},
	{HUD_STAT_OFFSET, "offset"},
	{HUD_STAT_WORLD_POS, "world_pos"},
	{HUD_STAT_SIZE,    "size"},
	{HUD_STAT_Z_INDEX, "z_index"},
	{HUD_STAT_TEXT2,   "text2"},
	{0, NULL},
};

const struct EnumString es_HudBuiltinElement[] =
{
	{HUD_FLAG_HOTBAR_VISIBLE,        "hotbar"},
	{HUD_FLAG_HEALTHBAR_VISIBLE,     "healthbar"},
	{HUD_FLAG_CROSSHAIR_VISIBLE,     "crosshair"},
	{HUD_FLAG_WIELDITEM_VISIBLE,     "wielditem"},
	{HUD_FLAG_BREATHBAR_VISIBLE,     "breathbar"},
	{HUD_FLAG_MINIMAP_VISIBLE,       "minimap"},
	{HUD_FLAG_MINIMAP_RADAR_VISIBLE, "minimap_radar"},
	{0, NULL},
};


HudElement *HudElement::create(HudElementType type)
{
	HudElement *e;

	switch (type){
	case HUD_ELEM_IMAGE:
		e = new HudElementImage();
		break;
	case HUD_ELEM_TEXT:
		e = new HudElementText();
		break;
	case HUD_ELEM_STATBAR:
		e = new HudElementStatbar();
		break;
	case HUD_ELEM_INVENTORY:
		e = new HudElementInventory();
		break;
	case HUD_ELEM_WAYPOINT:
		e = new HudElementWaypoint();
		break;
	case HUD_ELEM_IMAGE_WAYPOINT:
		e = new HudElementWaypoint();
		break;
	default:
		e = new HudElement();
		break;
	}

	e->m_type = type;
	return e;
}

void HudElement::pushLua(lua_State *L)
{
	push_v2f(L, pos);
	lua_setfield(L, -2, "position");

	push_v2f(L, offset);
	lua_setfield(L, -2, "offset");

	push_v2f(L, align);
	lua_setfield(L, -2, "alignment");

	lua_pushnumber(L, z_index);
	lua_setfield(L, -2, "z_index");
}

void HudElement::readLua(lua_State *L)
{
	lua_getfield(L, 2, "position");
	pos = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "offset");
	offset = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "align");
	align = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	z_index = MYMAX(S16_MIN, MYMIN(S16_MAX,
			getintfield_default(L, 2, "z_index", 0)));
}

void HudElementImage::deSerialize(NetworkPacket *pkt, u16 proto_ver)
{
	u32 u32_;
	v3f v3f_;
	v2s32 v2s32_;
	std::string str_;

	*pkt >> pos >> str_ >> scale
		>> texture >> u32_ >> u32_ // text, number, item
		>> u32_ >> align >> offset >> v3f_ // dir, world_pos
		>> v2s32_ >> z_index >> str_; // size, text2
}

void HudElementImage::serialize(NetworkPacket *pkt, u16 proto_ver)
{
	std::string str_;

	*pkt << pos << str_ << scale
		<< texture << (u32)0 << (u32)0 // text, number, item
		<< (u32)0<< align << offset << v3f() // dir, world_pos
		<< v2s32()<< z_index << str_;  // size, text2
}

void HudElementImage::pushLua(lua_State *L)
{
	HudElement::pushLua(L);

	lua_pushstring(L, text.c_str());
	lua_setfield(L, -2, "text");

	push_v2f(L, scale);
	lua_setfield(L, -2, "scale");
}

void HudElementImage::readLua(lua_State *L)
{
	HudElement::readLua(L);

	texture = getstringfield_default(L, 2, "text", "");

	lua_getfield(L, 2, "scale");
	scale = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);
}
