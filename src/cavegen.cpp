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

#include "util/numeric.h"
#include "map.h"
#include "mapgen.h"
#include "cavegen.h"


CaveV6::CaveV6(Mapgen *mg, PseudoRandom *ps, PseudoRandom *ps2,
			bool is_large_cave, content_t c_water, content_t c_lava) {
	this->vm = mg->vm;
	this->water_level = mg->water_level;
	this->large_cave = is_large_cave;
	this->ps  = ps;
	this->ps2 = ps2;
	this->c_water_source = c_water;
	this->c_lava_source  = c_lava;

	min_tunnel_diameter = 2;
	max_tunnel_diameter = ps->range(2, 6);
	dswitchint = ps->range(1, 14);
	flooded = true;
	
	if (large_cave) {
		part_max_length_rs = ps->range(2,4);
		tunnel_routepoints = ps->range(5, ps->range(15,30));
		min_tunnel_diameter = 5;
		max_tunnel_diameter = ps->range(7, ps->range(8,24));
	} else {
		part_max_length_rs = ps->range(2,9);
		tunnel_routepoints = ps->range(10, ps->range(15,30));
	}
	
	large_cave_is_flat = (ps->range(0,1) == 0);
}


void CaveV6::makeCave(v3s16 nmin, v3s16 nmax, int max_stone_height) {
	node_min = nmin;
	node_max = nmax;
	max_stone_y = max_stone_height;
	main_direction = v3f(0, 0, 0);

	// Allowed route area size in nodes
	ar = node_max - node_min + v3s16(1, 1, 1);
	// Area starting point in nodes
	of = node_min;

	// Allow a bit more
	//(this should be more than the maximum radius of the tunnel)
	const s16 max_spread_amount = MAP_BLOCKSIZE;
	s16 insure = 10;
	s16 more = max_spread_amount - max_tunnel_diameter / 2 - insure;
	ar += v3s16(1,0,1) * more * 2;
	of -= v3s16(1,0,1) * more;

	route_y_min = 0;
	// Allow half a diameter + 7 over stone surface
	route_y_max = -of.Y + max_stone_y + max_tunnel_diameter / 2 + 7;

	// Limit maximum to area
	route_y_max = rangelim(route_y_max, 0, ar.Y - 1);

	if (large_cave) {
		s16 min = 0;
		if (node_min.Y < water_level && node_max.Y > water_level) {
			min = water_level - max_tunnel_diameter/3 - of.Y;
			route_y_max = water_level + max_tunnel_diameter/3 - of.Y;
		}
		route_y_min = ps->range(min, min + max_tunnel_diameter);
		route_y_min = rangelim(route_y_min, 0, route_y_max);
	}

	s16 route_start_y_min = route_y_min;
	s16 route_start_y_max = route_y_max;

	route_start_y_min = rangelim(route_start_y_min, 0, ar.Y-1);
	route_start_y_max = rangelim(route_start_y_max, route_start_y_min, ar.Y-1);

	// Randomize starting position
	orp = v3f(
		(float)(ps->next() % ar.X) + 0.5,
		(float)(ps->range(route_start_y_min, route_start_y_max)) + 0.5,
		(float)(ps->next() % ar.Z) + 0.5
	);

	// Generate some tunnel starting from orp
	for (u16 j = 0; j < tunnel_routepoints; j++)
		makeTunnel(j % dswitchint == 0);
}


void CaveV6::makeTunnel(bool dirswitch) {
	if (dirswitch && !large_cave) {
		main_direction = v3f(
			((float)(ps->next() % 20) - (float)10) / 10,
			((float)(ps->next() % 20) - (float)10) / 30,
			((float)(ps->next() % 20) - (float)10) / 10
		);
		main_direction *= (float)ps->range(0, 10) / 10;
	}

	// Randomize size
	s16 min_d = min_tunnel_diameter;
	s16 max_d = max_tunnel_diameter;
	rs = ps->range(min_d, max_d);

	v3s16 maxlen;
	if (large_cave) {
		maxlen = v3s16(
			rs * part_max_length_rs,
			rs * part_max_length_rs / 2,
			rs * part_max_length_rs
		);
	} else {
		maxlen = v3s16(
			rs * part_max_length_rs,
			ps->range(1, rs * part_max_length_rs),
			rs * part_max_length_rs
		);
	}

	v3f vec(
		(float)(ps->next() % (maxlen.X * 1)) - (float)maxlen.X / 2,
		(float)(ps->next() % (maxlen.Y * 1)) - (float)maxlen.Y / 2,
		(float)(ps->next() % (maxlen.Z * 1)) - (float)maxlen.Z / 2
	);

	// Jump downward sometimes
	if (!large_cave && ps->range(0, 12) == 0) {
		vec = v3f(
			(float)(ps->next() % (maxlen.X * 1)) - (float)maxlen.X / 2,
			(float)(ps->next() % (maxlen.Y * 2)) - (float)maxlen.Y,
			(float)(ps->next() % (maxlen.Z * 1)) - (float)maxlen.Z / 2
		);
	}

	/*if(large_cave){
		v3f p = orp + vec;
		s16 h = find_ground_level_clever(vmanip, v2s16(p.X, p.Z), ndef);
		route_y_min = h - rs/3;
		route_y_max = h + rs;
	}*/

	vec += main_direction;

	v3f rp = orp + vec;
	if (rp.X < 0)
		rp.X = 0;
	else if (rp.X >= ar.X)
		rp.X = ar.X - 1;
	
	if (rp.Y < route_y_min)
		rp.Y = route_y_min;
	else if (rp.Y >= route_y_max)
		rp.Y = route_y_max - 1;
	
	if (rp.Z < 0)
		rp.Z = 0;
	else if (rp.Z >= ar.Z)
		rp.Z = ar.Z - 1;
	
	vec = rp - orp;

	float veclen = vec.getLength();
	// As odd as it sounds, veclen is *exactly* 0.0 sometimes, causing a FPE
	if (veclen == 0.0)
		veclen = 1.0;
		
	// Every second section is rough
	bool randomize_xz = (ps2->range(1, 2) == 1);

	// Carve routes
	for (float f = 0; f < 1.0; f += 1.0 / veclen)
		carveRoute(vec, f, randomize_xz);
	
	orp = rp;
}


void CaveV6::carveRoute(v3f vec, float f, bool randomize_xz) {
	MapNode airnode(CONTENT_AIR);
	MapNode waternode(c_water_source);
	MapNode lavanode(c_lava_source);
	
	v3s16 startp(orp.X, orp.Y, orp.Z);
	startp += of;
	
	v3f fp = orp + vec * f;
	fp.X += 0.1 * ps->range(-10, 10);
	fp.Z += 0.1 * ps->range(-10, 10);
	v3s16 cp(fp.X, fp.Y, fp.Z);

	s16 d0 = -rs/2;
	s16 d1 = d0 + rs;
	if (randomize_xz) {
		d0 += ps->range(-1, 1);
		d1 += ps->range(-1, 1);
	}
	
	for (s16 z0 = d0; z0 <= d1; z0++) {
		s16 si = rs / 2 - MYMAX(0, abs(z0) - rs / 7 - 1);
		for (s16 x0 = -si - ps->range(0,1); x0 <= si - 1 + ps->range(0,1); x0++) {
			s16 maxabsxz = MYMAX(abs(x0), abs(z0));
			s16 si2 = rs / 2 - MYMAX(0, maxabsxz - rs / 7 - 1);
			for (s16 y0 = -si2; y0 <= si2; y0++) {
				// Make better floors in small caves
				//if(y0 <= -rs/2 && rs<=7)
				//	continue;
				
				if (large_cave_is_flat) {
					// Make large caves not so tall
					if (rs > 7 && abs(y0) >= rs / 3)
						continue;
				}

				v3s16 p(cp.X + x0, cp.Y + y0, cp.Z + z0);
				p += of;

				if (vm->m_area.contains(p) == false)
					continue;

				u32 i = vm->m_area.index(p);

				if (large_cave) {
					int full_ymin = node_min.Y - MAP_BLOCKSIZE;
					int full_ymax = node_max.Y + MAP_BLOCKSIZE;
					
					if (flooded && full_ymin < water_level && full_ymax > water_level) {
						vm->m_data[i] = (p.Y <= water_level) ? waternode : airnode;
					} else if (flooded && full_ymax < water_level) {
						vm->m_data[i] = (p.Y < startp.Y - 2) ? lavanode : airnode;
					} else {
						vm->m_data[i] = airnode;
					}
				} else {
					// Don't replace air or water or lava or ignore
					content_t c = vm->m_data[i].getContent();
					if (c == CONTENT_IGNORE || c == CONTENT_AIR ||
						c == c_water_source || c == c_lava_source)
						continue;

					vm->m_data[i] = airnode;
					vm->m_flags[i] |= VMANIP_FLAG_CAVE;
				}
			}
		}
	}
}
