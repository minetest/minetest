/*
Minetest
Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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
#include <vector>
#include "irrlichttypes.h"
#include <ITexture.h>
#include <SMesh.h>

class Client;
struct ItemStack;

/*!
 * Holds color information of an item mesh's buffer.
 */
struct ItemPartColor
{
	/*!
	 * If this is false, the global base color of the item
	 * will be used instead of the specific color of the
	 * buffer.
	 */
	bool override_base = false;

	/*!
	 * The color of the buffer.
	 */
	video::SColor color = 0;

	ItemPartColor() = default;

	ItemPartColor(bool override, video::SColor color) :
			override_base(override), color(color)
	{
	}
};

struct ItemMesh
{
	scene::IMesh *mesh = nullptr;

	/*!
	 * Stores the color of each mesh buffer.
	 */
	std::vector<ItemPartColor> buffer_colors;

	/*!
	 * If false, all faces of the item should have the same brightness.
	 * Disables shading based on normal vectors.
	 */
	bool needs_shading = true;

	ItemMesh() = default;
};

class ItemMeshSource
{
public:
	ItemMeshSource(Client *_client) : client(_client) {}
	virtual ~ItemMeshSource() = default;

	ItemMesh createInventoryItemMesh(const ItemStack &stack);
	ItemMesh createWieldItemMesh(const ItemStack &stack);

	virtual scene::SMesh *createExtrusionMesh(
			video::ITexture *texture, video::ITexture *overlay_texture) = 0;
	virtual scene::SMesh *createFlatMesh(
			video::ITexture *texture, video::ITexture *overlay_texture) = 0;

protected:
	Client *client = nullptr;
};
