/*
Minetest
Copyright (C) 2024 DS

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

#include "convex_polygon.h"
#include <algorithm>

#ifndef SERVER

#include <IVideoDriver.h>

#include "log.h"
#include "irrlicht_changes/printing.h"

void draw_convex_polygon(video::IVideoDriver *driver, const std::vector<v2f> &polygon,
		const video::SColor &color)
{
	if (polygon.size() < 3)
		return;

	v2u32 ss = driver->getScreenSize();

	// errorstream << "ss: "<< ss <<std::endl;

	auto new_2d_vertex = [&color, &ss](const v2f &pos) -> video::S3DVertex {
		if (false) {
			return video::S3DVertex(v3f(pos.X * ss.X, pos.Y * ss.Y, 0.0f), v3f(),
					color, v2f());
		} else {
			return video::S3DVertex(v3f(pos.X * 2.0f - 1.0f, -(pos.Y * 2.0f - 1.0f), 0.0f), v3f(),
					color, v2f());
		}
	};

	std::vector<video::S3DVertex> vertices;
	vertices.reserve(polygon.size());
	for (const v2f &v : polygon)
		vertices.push_back(new_2d_vertex(v));

	std::vector<u16> index_list;
	index_list.reserve(polygon.size());
	for (size_t i = 0; i < polygon.size(); ++i)
		index_list.push_back(i);

	video::SMaterial material;
	material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
	material.ZWriteEnable = video::EZW_OFF;

	driver->setMaterial(material);

	driver->draw2DVertexPrimitiveList((void *)&vertices[0], (u32)vertices.size(),
			(void *)&index_list[0], (u32)index_list.size() - 2,
			video::EVT_STANDARD, // S3DVertex vertices
			scene::EPT_TRIANGLE_FAN,
			video::EIT_16BIT); // 16 bit indices
}

#endif // !def(SERVER)

f32 get_convex_polygon_area(const std::vector<v2f> &polygon)
{
	if (polygon.size() < 3)
		return 0.0f;
	// sum up the areas of all triangles
	f32 area = 0.0f;
	const v2f &v1 = polygon[0];
	for (size_t i = 2; i < polygon.size(); ++i) {
		const v2f &v2 = polygon[i-1];
		const v2f &v3 = polygon[i];
		// area of the triangle v1 v2 v3: 0.5 * det(d1 d2)
		// (winding order matters, for sign)
		v2f d1 = v2 - v1;
		v2f d2 = v3 - v1;
		area += 0.5f * (d1.X * d2.Y - d1.Y * d2.X);
	}
	return area;
}

std::vector<v2f> clip_convex_polygon(const std::vector<v2f> &polygon, v3f clip_line)
{
	using polygon_iterator = std::vector<v2f>::const_iterator;

	// the return value
	std::vector<v2f> polygon_clipped;

	// returns whether pos is in the not clipped half-space
	auto is_in = [&](const v2f &pos) {
		return pos.X * clip_line.X + pos.Y * clip_line.Y + clip_line.Z >= 0.0f;
	};

	auto is_out = [&](const v2f &pos) {
		return !is_in(pos);
	};

	// There are 3 cases:
	// * all vertices are clipped away
	// * no vertices are clipped away
	// * clip_line intersects the polygon at two points.
	//   There is hence one (possibly over the list end) contiguous sub-list of
	//   vertices that are all in (not clipped)
	//
	// is_in applied on the polygon vertices looks like this:
	// {in, in, in, out, out, in, in}
	//  last_in ^             ^ first_in
	//    first_out ^    ^ last_out
	//
	// We need to cut the polygon between the out-in and in-out vertex pairs.

	// find the first vertex that is in/out of the clip_line, and also the vertex
	// before
	polygon_iterator first_in, first_out;
	polygon_iterator last_out, last_in;
	if (is_in(polygon[0])) {
		first_out = std::find_if(polygon.begin() + 1, polygon.end(), is_out);
		if (first_out == polygon.end()) {
			// all are in
			polygon_clipped = polygon;
			return polygon_clipped;
		}
		last_in = first_out - 1;
		first_in = std::find_if(first_out + 1, polygon.end(), is_in);
		last_out = first_in - 1;
		if (first_in == polygon.end())
			first_in = polygon.begin(); // we already checked that the 0th is in
	} else {
		first_in = std::find_if(polygon.begin() + 1, polygon.end(), is_in);
		if (first_in == polygon.end()) {
			// all are out
			return polygon_clipped; // empty
		}
		last_out = first_in - 1;
		first_out = std::find_if(first_in + 1, polygon.end(), is_out);
		last_in = first_out - 1;
		if (first_out == polygon.end())
			first_out = polygon.begin(); // we already checked that the 0th is out
	}

	// copy all vertices that are in
	if (first_in <= last_in) {
		polygon_clipped.reserve((last_in - first_in) + 1 + 2);
		polygon_clipped.insert(polygon_clipped.end(), first_in, last_in + 1);
	} else {
		polygon_clipped.reserve((polygon.end() - first_in) + (last_in - polygon.begin()) + 1 + 2);
		polygon_clipped.insert(polygon_clipped.end(), first_in, polygon.end());
		polygon_clipped.insert(polygon_clipped.end(), polygon.begin(), last_in + 1);
	}

	auto split_edge = [&](const v2f &p1, const v2f &p2) {
		auto homogenize = [](const v2f &p) { return v3f(p.X, p.Y, 1.0f); };
		// find a pos that is on clip_line and between p1 and p2
		// compute: meet(clip_line, join(p1, p2))
		v3f pos_homo = clip_line.crossProduct(homogenize(p1).crossProduct(homogenize(p2)));
		// dehomogenize
		return v2f(pos_homo.X / pos_homo.Z, pos_homo.Y / pos_homo.Z);
	};

	// split in-out pair
	polygon_clipped.push_back(split_edge(*last_in, *first_out));

	// split out-in pair
	polygon_clipped.push_back(split_edge(*last_out, *first_in));

	return polygon_clipped;
}
