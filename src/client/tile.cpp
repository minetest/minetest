/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "tile.h"

// Sets everything else except the texture in the material
void TileLayer::applyMaterialOptions(video::SMaterial &material) const
{
	switch (material_type) {
	case TILE_MATERIAL_OPAQUE:
	case TILE_MATERIAL_LIQUID_OPAQUE:
	case TILE_MATERIAL_WAVING_LIQUID_OPAQUE:
		material.MaterialType = video::EMT_SOLID;
		break;
	case TILE_MATERIAL_BASIC:
	case TILE_MATERIAL_WAVING_LEAVES:
	case TILE_MATERIAL_WAVING_PLANTS:
	case TILE_MATERIAL_WAVING_LIQUID_BASIC:
		material.MaterialTypeParam = 0.5;
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		break;
	case TILE_MATERIAL_ALPHA:
	case TILE_MATERIAL_LIQUID_TRANSPARENT:
	case TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT:
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		break;
	default:
		break;
	}
	material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING) != 0;
	if (!(material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)) {
		material.TextureLayers[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	}
	if (!(material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL)) {
		material.TextureLayers[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

void TileLayer::applyMaterialOptionsWithShaders(video::SMaterial &material) const
{
	material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING) != 0;
	if (!(material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)) {
		material.TextureLayers[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		material.TextureLayers[1].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	}
	if (!(material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL)) {
		material.TextureLayers[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
		material.TextureLayers[1].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}
