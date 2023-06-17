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

#include "irrlichttypes_bloated.h"
#include "shootline.h"

#include <cmath>

Shootline::Shootline(const core::line3d<f32> &shootline)
{
	m_dir = shootline.getVector().normalize();
	v3f li2dir = m_dir + (std::fabs(m_dir.X) < 0.5f ? v3f(1, 0, 0) : v3f(0, 1, 0));
	m_dir_ortho1 = m_dir.crossProduct(li2dir).normalize();
	m_dir_ortho2 = m_dir.crossProduct(m_dir_ortho1);
}

bool Shootline::mayHitSelectionBox(
		const v3f &pos_diff, f32 dist, f32 selection_box_radius) const
{
	// backward- and far-plane
	f32 max_d = m_line.getLength();
	if (dist + selection_box_radius < 0.0f || dist - selection_box_radius > max_d)
		return false;

	// side-planes
	if (std::fabs(m_dir_ortho1.dotProduct(pos_diff)) > selection_box_radius ||
			std::fabs(m_dir_ortho2.dotProduct(pos_diff)) > selection_box_radius)
		return false;
	return true;
}
