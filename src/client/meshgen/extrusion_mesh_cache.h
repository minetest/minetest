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
class ExtrusionMeshCache: public IItemMeshSource
{
public:
	ExtrusionMeshCache();
	~ExtrusionMeshCache() override;
	scene::SMesh *createExtrusionMesh(video::ITexture *texture, video::ITexture *overlay_texture) override;
	scene::SMesh *createFlatMesh(video::ITexture *texture, video::ITexture *overlay_texture) override;

private:
	// Get closest extrusion mesh for given image dimensions
	// Caller must drop the returned pointer
	scene::SMesh *create(core::dimension2d<u32> dim);

	std::map<int, scene::SMesh *> m_extrusion_meshes;
};
