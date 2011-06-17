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

#ifndef CONTENT_CRAFT_HEADER
#define CONTENT_CRAFT_HEADER

class InventoryItem;
class Player;

/*
	items: actually *items[9]
	return value: allocates a new item, or returns NULL.
*/
InventoryItem *craft_get_result(InventoryItem **items);

void craft_set_creative_inventory(Player *player);

// Called when give_initial_stuff setting is used
void craft_give_initial_stuff(Player *player);

#endif

