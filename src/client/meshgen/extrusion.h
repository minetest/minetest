/*
Minetest
Copyright (C) 2017-2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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
#include <SMesh.h>

/** Extrudes a texture, or copies a cached extruded mesh.
 * @returns a mesh with 1 or 2 buffers.
 * Buffer 0 uses @p texture
 * Buffer 1 uses @p overlay_texture, unless it is NULL, in which case it is not present
 * @note Non-thread-safe!
 */
scene::SMesh *createExtrusionMesh(
		video::ITexture *texture, video::ITexture *overlay_texture = nullptr);
