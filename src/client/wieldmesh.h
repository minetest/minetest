/*
Minetest
Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <string>
#include <vector>
#include "irrlichttypes_extrabloated.h"
#include "client/meshgen/item.h"

struct ItemStack;
class Client;
class ITextureSource;
struct ContentFeatures;

/*!
 * Wield item scene node, renders the wield mesh of some item
 */
class WieldMeshSceneNode : public scene::ISceneNode
{
public:
	WieldMeshSceneNode(scene::ISceneManager *mgr, s32 id = -1);
	~WieldMeshSceneNode() override;
	void render() override;

	const aabb3f &getBoundingBox() const override { return m_bounding_box; }

	void setItem(const ItemStack &item, Client *client);

	/*!
	 * Sets the vertex color of the wield mesh.
	 * Must only be used if the constructor was called with lighting = false
	 */
	void setColor(video::SColor color);

	scene::IMesh *getMesh() { return m_meshnode->getMesh(); }

private:
	/*!
	 * Child scene node with the current wield mesh
	 */
	scene::IMeshSceneNode *const m_meshnode;

	/*!
	 * Stores the colors of the mesh's mesh buffers.
	 * This does not include lighting.
	 */
	std::vector<ItemPartColor> m_colors;

	/*!
	 * The base color of this mesh. This is the default
	 * for all mesh buffers.
	 */
	video::SColor m_base_color;

	/*!
	 * Bounding box culling is disabled for this type of scene node,
	 * so this variable is just required so we can implement
	 * getBoundingBox() and is set to an empty box.
	 */
	aabb3f m_bounding_box;
};
