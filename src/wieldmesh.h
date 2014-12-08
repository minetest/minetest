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

#include "irrlichttypes_extrabloated.h"
#include <string>

struct ItemStack;
class IGameDef;
class ITextureSource;
struct TileSpec;

/*
	Wield item scene node, renders the wield mesh of some item
*/
class WieldMeshSceneNode: public scene::ISceneNode
{
public:
	WieldMeshSceneNode(scene::ISceneNode *parent, scene::ISceneManager *mgr,
			s32 id = -1, bool lighting = false);
	virtual ~WieldMeshSceneNode();

	void setCube(const TileSpec tiles[6],
			v3f wield_scale, ITextureSource *tsrc);
	void setExtruded(const std::string &imagename,
			v3f wield_scale, ITextureSource *tsrc);
	void setItem(const ItemStack &item, IGameDef *gamedef);

	// Sets the vertex color of the wield mesh.
	// Must only be used if the constructor was called with lighting = false
	void setColor(video::SColor color);

	virtual void render();

	virtual const core::aabbox3d<f32>& getBoundingBox() const
	{ return m_bounding_box; }

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

	// Bounding box culling is disabled for this type of scene node,
	// so this variable is just required so we can implement
	// getBoundingBox() and is set to an empty box.
	core::aabbox3d<f32> m_bounding_box;
};

#endif
