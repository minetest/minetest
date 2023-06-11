/*
Minetest
Copyright (C) 2023 Josiah VanderZee <josiah_vanderzee@mediacombb.net>

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

class Shootline
{
public:
	Shootline(const core::line3d<f32> &shootline);

	bool hitsSelectionBox(const v3f &pos_diff, f32 dist, f32 selection_box_radius) const;

	const v3f &getDir() const { return m_dir; }
	const core::line3d<f32> &getLine() const { return m_line; }

private:
	v3f m_dir{};
	v3f m_dir_ortho1{};
	v3f m_dir_ortho2{};
	core::line3d<f32> m_line{};
};
