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
#include "irrlichttypes.h"
#include <ITexture.h>
#include <SMesh.h>

class IItemMeshSource
{
public:
	virtual ~IItemMeshSource() = default;
	virtual scene::SMesh *createExtrusionMesh(
			video::ITexture *texture, video::ITexture *overlay_texture) = 0;
	virtual scene::SMesh *createFlatMesh(
			video::ITexture *texture, video::ITexture *overlay_texture) = 0;
};
