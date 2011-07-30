/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef CONTENT_INVENTORY_HEADER
#define CONTENT_INVENTORY_HEADER

#include "common_irrlicht.h" // For u8, s16
#include <string>
#include "mapnode.h" // For content_t

class InventoryItem;
class ServerActiveObject;
class ServerEnvironment;

bool           item_material_is_cookable(content_t content);
InventoryItem* item_material_create_cook_result(content_t content);

std::string         item_craft_get_image_name(const std::string &subname);
ServerActiveObject* item_craft_create_object(const std::string &subname,
		ServerEnvironment *env, u16 id, v3f pos);
s16                 item_craft_get_drop_count(const std::string &subname);
bool                item_craft_is_cookable(const std::string &subname);
InventoryItem*      item_craft_create_cook_result(const std::string &subname);
bool                item_craft_is_eatable(const std::string &subname);
s16                 item_craft_eat_hp_change(const std::string &subname);

#endif

