#pragma once
#include "irrlichttypes.h"
#include <ITexture.h>
#include <SMesh.h>

class IItemMeshSource {
public:
	virtual ~IItemMeshSource() = default; // not =0 as Clang reports undefined reference otherwise
	virtual scene::SMesh *createExtrusionMesh(video::ITexture *texture, video::ITexture *overlay_texture) = 0;
	virtual scene::SMesh *createFlatMesh(video::ITexture *texture, video::ITexture *overlay_texture) = 0;
};
