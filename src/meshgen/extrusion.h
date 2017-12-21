#pragma once
#include "irrlichttypes.h"
#include <IMesh.h>
#include "irr_ptr.h"

irr_ptr<scene::IMesh> createExtrusionMesh(video::ITexture *texture,
		video::ITexture *overlay_texture = nullptr);
void purgeExtrusionMeshCache();
