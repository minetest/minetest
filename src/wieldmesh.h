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

#ifndef WIELDMESH_HEADER
#define WIELDMESH_HEADER

#include <string>
#include "irrlichttypes_extrabloated.h"

struct ItemStack;
class Client;
class ITextureSource;
struct TileSpec;

struct ItemMesh
{
	scene::IMesh *mesh;
	/*!
	 * Stores the color of each mesh buffer.
	 * If the boolean is true, the color is fixed, else
	 * palettes can modify it.
	 */
	std::vector<std::pair<bool, video::SColor> > buffer_colors;

	ItemMesh() : mesh(NULL), buffer_colors() {}
};

/*
	Wield item scene node, renders the wield mesh of some item
*/
class WieldMeshSceneNode : public scene::ISceneNode
{
public:
	WieldMeshSceneNode(scene::ISceneNode *parent, scene::ISceneManager *mgr,
			s32 id = -1, bool lighting = false);
	virtual ~WieldMeshSceneNode();

	void setCube(const TileSpec tiles[6], v3f wield_scale, ITextureSource *tsrc);
	void setExtruded(const std::string &imagename, v3f wield_scale,
			ITextureSource *tsrc, u8 num_frames);
	void setItem(const ItemStack &item, Client *client);

	// Sets the vertex color of the wield mesh.
	// Must only be used if the constructor was called with lighting = false
	void setColor(video::SColor color);

	scene::IMesh *getMesh() { return m_meshnode->getMesh(); }

	virtual void render();

	virtual const aabb3f &getBoundingBox() const { return m_bounding_box; }

private:
	void changeToMesh(scene::IMesh *mesh);

	// Child scene node with the current wield mesh
	scene::IMeshSceneNode *m_meshnode;
	video::E_MATERIAL_TYPE m_material_type;

	// True if EMF_LIGHTING should be enabled.
	bool m_lighting;

	bool m_enable_shaders;
	bool m_anisotropic_filter;
	bool m_bilinear_filter;
	bool m_trilinear_filter;
	/*!
	 * Stores the colors of the mesh's mesh buffers.
	 * This does not include lighting.
	 */
	std::vector<video::SColor> m_colors;

	// Bounding box culling is disabled for this type of scene node,
	// so this variable is just required so we can implement
	// getBoundingBox() and is set to an empty box.
	aabb3f m_bounding_box;
};

void getItemMesh(Client *client, const ItemStack &item, ItemMesh *result);

scene::IMesh *getExtrudedMesh(ITextureSource *tsrc, const std::string &imagename);
#endif
