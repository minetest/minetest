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

#pragma once

#include "irrlichttypes.h"
#include "irr_v3d.h"

extern const v3s16 g_6dirs[6];

extern const v3s16 g_7dirs[7];

extern const v3s16 g_26dirs[26];

// 26th is (0,0,0)
extern const v3s16 g_27dirs[27];

extern const u8 wallmounted_to_facedir[6];

extern const v3s16 wallmounted_dirs[8];

extern const v3s16 facedir_dirs[32];

extern const v3s16 fourdir_dirs[4];

/// Direction in the 6D format. g_27dirs contains corresponding vectors.
/// Here P means Positive, N stands for Negative.
enum Direction6D {
// 0
	D6D_ZP,
	D6D_YP,
	D6D_XP,
	D6D_ZN,
	D6D_YN,
	D6D_XN,
// 6
	D6D_XN_YP,
	D6D_XP_YP,
	D6D_YP_ZP,
	D6D_YP_ZN,
	D6D_XN_ZP,
	D6D_XP_ZP,
	D6D_XN_ZN,
	D6D_XP_ZN,
	D6D_XN_YN,
	D6D_XP_YN,
	D6D_YN_ZP,
	D6D_YN_ZN,
// 18
	D6D_XN_YP_ZP,
	D6D_XP_YP_ZP,
	D6D_XN_YP_ZN,
	D6D_XP_YP_ZN,
	D6D_XN_YN_ZP,
	D6D_XP_YN_ZP,
	D6D_XN_YN_ZN,
	D6D_XP_YN_ZN,
// 26
	D6D,

// aliases
	D6D_BACK   = D6D_ZP,
	D6D_TOP    = D6D_YP,
	D6D_RIGHT  = D6D_XP,
	D6D_FRONT  = D6D_ZN,
	D6D_BOTTOM = D6D_YN,
	D6D_LEFT   = D6D_XN,
};

/// Direction in the wallmounted format.
/// P is Positive, N is Negative.
enum DirectionWallmounted {
	DWM_YP,
	DWM_YN,
	DWM_XP,
	DWM_XN,
	DWM_ZP,
	DWM_ZN,
};
