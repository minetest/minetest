// SPDX-FileCopyrightText: 2024 Minetest
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irr_v2d.h"
#include "irr_v3d.h"
#include <SColor.h>
#include <vector>

#ifndef SERVER

namespace irr::video { class IVideoDriver; }

/** Draws a convex polygon over the whole screen.
 * @param polygon Ordered list of corner vertices.
 *                X coordinates go from 0 (left) to 1 (right), Y from 0 (up) to
 *                1 (down).
 */
void draw_convex_polygon(video::IVideoDriver *driver, const std::vector<v2f> &polygon,
		const video::SColor &color);

#endif // !def(SERVER)

/** Calculates the area of a convex polygon, given by its corners
 * @param polygon Ordered list of corner vertices.
 * @return The area.
 */
f32 get_convex_polygon_area(const std::vector<v2f> &polygon);

/** Clips away the parts of a convex polygon that are on the wrong side of the
 * given clip line.
 * @param polygon Nonempty list of vertices that form a convex polygon if you connect
 *                them in order.
 * @param clip_line The homogeneous coordinates of an oriented 2D line. Clips
 *                  away all points p for which dot(clip_line, p) < 0 holds.
 * @return The clipped polygon.
 */
std::vector<v2f> clip_convex_polygon(const std::vector<v2f> &polygon, v3f clip_line);
