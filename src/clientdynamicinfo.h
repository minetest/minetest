/*
Minetest
Copyright (C) 2022-3 rubenwardy <rw@rubenwardy.com>

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

#include "irrlichttypes_bloated.h"


struct ClientDynamicInfo
{
public:
	v2u32 render_target_size;
	f32 real_gui_scaling;
	f32 real_hud_scaling;
	v2f32 max_fs_size;
	bool touch_controls;

	bool equal(const ClientDynamicInfo &other) const {
		return render_target_size == other.render_target_size &&
				std::abs(real_gui_scaling - other.real_gui_scaling) < 0.001f &&
				std::abs(real_hud_scaling - other.real_hud_scaling) < 0.001f &&
				touch_controls == other.touch_controls;
	}

#ifndef SERVER
	static ClientDynamicInfo getCurrent();

private:
	static v2f32 calculateMaxFSSize(v2u32 render_target_size, f32 gui_scaling);
#endif
};
