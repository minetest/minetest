#pragma once
#include "irrlichttypes.h"
#include <SMesh.h>

/** Extrudes a texture, or copies a cached extruded mesh.
 * @returns a mesh with 1 or 2 buffers.
 * Buffer 0 uses @p texture
 * Buffer 1 uses @p overlay_texture, unless it is NULL, in which case it is not present
 */
scene::SMesh *createExtrusionMesh(video::ITexture *texture,
		video::ITexture *overlay_texture = nullptr);

void purgeExtrusionMeshCache();
