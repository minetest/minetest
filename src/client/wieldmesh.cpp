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

#include "wieldmesh.h"
#include "settings.h"
#include "shader.h"
#include "inventory.h"
#include "client.h"
#include "itemdef.h"
#include "nodedef.h"
#include "mesh.h"
#include "content_mapblock.h"
#include "mapblock_mesh.h"
#include "client/meshgen/collector.h"
#include "client/tile.h"
#include "log.h"
#include "util/numeric.h"
#include <map>
#include <IMeshManipulator.h>

WieldMeshSceneNode::WieldMeshSceneNode(scene::ISceneManager *mgr, s32 id):
	scene::ISceneNode(mgr->getRootSceneNode(), mgr, id),
	m_meshnode(SceneManager->addMeshSceneNode(nullptr, this, -1, {}, {}, {1.f,1.f,1.f}, true))
{
	// Disable bounding box culling for this scene node
	// since we won't calculate the bounding box.
	setAutomaticCulling(scene::EAC_OFF);

	// Setup the child scene node
	m_meshnode->setReadOnlyMaterials(false);
	m_meshnode->setVisible(false);
}

WieldMeshSceneNode::~WieldMeshSceneNode()
{
	m_meshnode->remove();
}

void WieldMeshSceneNode::setItem(const ItemStack &item, Client *client)
{
	IItemDefManager *idef = client->getItemDefManager();
	m_base_color = idef->getItemstackColor(item, client);
	ItemMesh m = createWieldItemMesh(client, item);
	m_colors = std::move(m.buffer_colors);
	m_meshnode->setMesh(m.mesh);
	m_meshnode->setVisible(bool(m.mesh));
}

void WieldMeshSceneNode::setColor(video::SColor c)
{
	scene::IMesh *mesh = m_meshnode->getMesh();
	if (!mesh)
		return;

	u8 red = c.getRed();
	u8 green = c.getGreen();
	u8 blue = c.getBlue();
	u32 mc = mesh->getMeshBufferCount();
	for (u32 j = 0; j < mc; j++) {
		video::SColor bc(m_base_color);
		if ((m_colors.size() > j) && (m_colors[j].override_base))
			bc = m_colors[j].color;
		video::SColor buffercolor(255,
			bc.getRed() * red / 255,
			bc.getGreen() * green / 255,
			bc.getBlue() * blue / 255);
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		colorizeMeshBuffer(buf, &buffercolor);
	}
}

void WieldMeshSceneNode::render()
{
	// note: if this method is changed to actually do something,
	// you probably should implement OnRegisterSceneNode as well
}
