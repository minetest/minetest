/*
Minetest
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
#include <map>
#include "item_mesh_source.h"

/*
	Caches extrusion meshes so that only one of them per resolution
	is needed. Also caches one cube (for convenience).

	E.g. there is a single extrusion mesh that is used for all
	16x16 px images, another for all 256x256 px images, and so on.

	WARNING: Not thread safe. This should not be a problem since
	rendering related classes (such as WieldMeshSceneNode) will be
	used from the rendering thread only.
*/
class ExtrusionMeshCache : public IItemMeshSource
{
public:
	ExtrusionMeshCache();
	~ExtrusionMeshCache() override;
	scene::SMesh *createExtrusionMesh(video::ITexture *texture,
			video::ITexture *overlay_texture) override;
	scene::SMesh *createFlatMesh(video::ITexture *texture,
			video::ITexture *overlay_texture) override;

private:
	static scene::SMesh *create(int resolution_x, int resolution_y);

	// Get closest extrusion mesh for given image dimensions
	// Caller must drop the returned pointer
	scene::SMesh *getOrCreate(core::dimension2d<u32> dim);

	std::map<int, scene::SMesh *> m_extrusion_meshes;
};
