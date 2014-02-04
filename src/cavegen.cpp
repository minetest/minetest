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
#include "mapgen_v6.h"
#include "mapgen_v7.h"
#include "cavegen.h"

NoiseParams nparams_caveliquids(0, 1, v3f(150.0, 150.0, 150.0), 776, 3, 0.6);


///////////////////////////////////////////////////////////////////////////////


CaveV6::CaveV6(MapgenV6 *mg, PseudoRandom *ps, PseudoRandom *ps2, bool is_large_cave) {
	this->mg   = mg;
	this->vm   = mg->vm;
	this->ndef = mg->ndef;
	this->water_level = mg->water_level;
	this->large_cave = is_large_cave;
	this->ps  = ps;
	this->ps2 = ps2;
	this->c_water_source = mg->c_water_source;
	this->c_lava_source  = mg->c_lava_source;

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
	s16 more = MYMAX(max_spread_amount - max_tunnel_diameter / 2 - insure, 1);
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

	int notifytype = large_cave ? GENNOTIFY_LARGECAVE_BEGIN : GENNOTIFY_CAVE_BEGIN;
	if (mg->gennotify & (1 << notifytype)) {
		std::vector <v3s16> *nvec = mg->gen_notifications[notifytype];
		nvec->push_back(v3s16(of.X + orp.X, of.Y + orp.Y, of.Z + orp.Z));
	}

	// Generate some tunnel starting from orp
	for (u16 j = 0; j < tunnel_routepoints; j++)
		makeTunnel(j % dswitchint == 0);

	notifytype = large_cave ? GENNOTIFY_LARGECAVE_END : GENNOTIFY_CAVE_END;
	if (mg->gennotify & (1 << notifytype)) {
		std::vector <v3s16> *nvec = mg->gen_notifications[notifytype];
		nvec->push_back(v3s16(of.X + orp.X, of.Y + orp.Y, of.Z + orp.Z));
	}
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
	if (veclen < 0.05)
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
				content_t c = vm->m_data[i].getContent();
				if (!ndef->get(c).is_ground_content)
					continue;

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


///////////////////////////////////////// Caves V7

CaveV7::CaveV7(MapgenV7 *mg, PseudoRandom *ps, bool is_large_cave) {
	this->mg   = mg;
	this->vm   = mg->vm;
	this->ndef = mg->ndef;
	this->water_level = mg->water_level;
	this->large_cave  = is_large_cave;
	this->ps = ps;
	this->c_water_source = mg->c_water_source;
	this->c_lava_source  = mg->c_lava_source;
	this->c_ice          = mg->c_ice;
	this->np_caveliquids = &nparams_caveliquids;

	dswitchint = ps->range(1, 14);
	flooded    = ps->range(1, 2) == 2;
	
	if (large_cave) {
		part_max_length_rs = ps->range(2, 4);
		tunnel_routepoints = ps->range(5, ps->range(15, 30));
		min_tunnel_diameter = 5;
		max_tunnel_diameter = ps->range(7, ps->range(8, 24));
	} else {
		part_max_length_rs = ps->range(2, 9);
		tunnel_routepoints = ps->range(10, ps->range(15, 30));
		min_tunnel_diameter = 2;
		max_tunnel_diameter = ps->range(2, 6);
	}
	
	large_cave_is_flat = (ps->range(0, 1) == 0);
}


void CaveV7::makeCave(v3s16 nmin, v3s16 nmax, int max_stone_height) {
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
	s16 insure = 10;
	s16 more = MYMAX(MAP_BLOCKSIZE - max_tunnel_diameter / 2 - insure, 1);
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

	route_start_y_min = rangelim(route_start_y_min, 0, ar.Y - 1);
	route_start_y_max = rangelim(route_start_y_max, route_start_y_min, ar.Y - 1);

	// Randomize starting position
	orp = v3f(
		(float)(ps->next() % ar.X) + 0.5,
		(float)(ps->range(route_start_y_min, route_start_y_max)) + 0.5,
		(float)(ps->next() % ar.Z) + 0.5
	);

	int notifytype = large_cave ? GENNOTIFY_LARGECAVE_BEGIN : GENNOTIFY_CAVE_BEGIN;
	if (mg->gennotify & (1 << notifytype)) {
		std::vector <v3s16> *nvec = mg->gen_notifications[notifytype];
		nvec->push_back(v3s16(of.X + orp.X, of.Y + orp.Y, of.Z + orp.Z));
	}

	// Generate some tunnel starting from orp
	for (u16 j = 0; j < tunnel_routepoints; j++)
		makeTunnel(j % dswitchint == 0);

	notifytype = large_cave ? GENNOTIFY_LARGECAVE_END : GENNOTIFY_CAVE_END;
	if (mg->gennotify & (1 << notifytype)) {
		std::vector <v3s16> *nvec = mg->gen_notifications[notifytype];
		nvec->push_back(v3s16(of.X + orp.X, of.Y + orp.Y, of.Z + orp.Z));
	}
}


void CaveV7::makeTunnel(bool dirswitch) {
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

	v3f vec;
	// Jump downward sometimes
	if (!large_cave && ps->range(0, 12) == 0) {
		vec = v3f(
			(float)(ps->next() % (maxlen.X * 1)) - (float)maxlen.X / 2,
			(float)(ps->next() % (maxlen.Y * 2)) - (float)maxlen.Y,
			(float)(ps->next() % (maxlen.Z * 1)) - (float)maxlen.Z / 2
		);
	} else {
		vec = v3f(
			(float)(ps->next() % (maxlen.X * 1)) - (float)maxlen.X / 2,
			(float)(ps->next() % (maxlen.Y * 1)) - (float)maxlen.Y / 2,
			(float)(ps->next() % (maxlen.Z * 1)) - (float)maxlen.Z / 2
		);
	}

	// Do not make large caves that are above ground.
	// It is only necessary to check the startpoint and endpoint.
	if (large_cave) {
		v3s16 orpi(orp.X, orp.Y, orp.Z);
		v3s16 veci(vec.X, vec.Y, vec.Z);
		v3s16 p;
		
		p = orpi + veci + of + rs / 2;
		if (p.Z >= node_min.Z && p.Z <= node_max.Z &&
			p.X >= node_min.X && p.X <= node_max.X) {
			u32 index = (p.Z - node_min.Z) * mg->ystride + (p.X - node_min.X);
			s16 h = mg->ridge_heightmap[index];
			if (h < p.Y)
				return;
		} else if (p.Y > water_level) {
			return; // If it's not in our heightmap, use a simple heuristic
		}
		
		p = orpi + of + rs / 2;
		if (p.Z >= node_min.Z && p.Z <= node_max.Z &&
			p.X >= node_min.X && p.X <= node_max.X) {
			u32 index = (p.Z - node_min.Z) * mg->ystride + (p.X - node_min.X);
			s16 h = mg->ridge_heightmap[index];
			if (h < p.Y)
				return;
		} else if (p.Y > water_level) { 
			return;
		}
	}

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
	if (veclen < 0.05)
		veclen = 1.0;
		
	// Every second section is rough
	bool randomize_xz = (ps->range(1, 2) == 1);

	// Make a ravine every once in a while if it's long enough
	//float xylen = vec.X * vec.X + vec.Z * vec.Z;
	//disable ravines for now
	bool is_ravine = false; //(xylen > 500.0) && !large_cave && (ps->range(1, 8) == 1);

	// Carve routes
	for (float f = 0; f < 1.0; f += 1.0 / veclen)
		carveRoute(vec, f, randomize_xz, is_ravine);
	
	orp = rp;
}


void CaveV7::carveRoute(v3f vec, float f, bool randomize_xz, bool is_ravine) {
	MapNode airnode(CONTENT_AIR);
	MapNode waternode(c_water_source);
	MapNode lavanode(c_lava_source);
	
	v3s16 startp(orp.X, orp.Y, orp.Z);
	startp += of;
	
	float nval = NoisePerlin3D(np_caveliquids, startp.X,
							startp.Y, startp.Z, mg->seed);
	MapNode liquidnode = nval < 0.40 ? lavanode : waternode;
	
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
	
	bool flat_cave_floor = !large_cave && ps->range(0, 2) == 2;
	bool should_make_cave_hole = ps->range(1, 10) == 1;
	
	for (s16 z0 = d0; z0 <= d1; z0++) {
		s16 si = rs / 2 - MYMAX(0, abs(z0) - rs / 7 - 1);
		for (s16 x0 = -si - ps->range(0,1); x0 <= si - 1 + ps->range(0,1); x0++) {
			s16 maxabsxz = MYMAX(abs(x0), abs(z0));
	
			s16 si2 = is_ravine ? MYMIN(ps->range(25, 26), ar.Y) :
								 rs / 2 - MYMAX(0, maxabsxz - rs / 7 - 1);
			
			for (s16 y0 = -si2; y0 <= si2; y0++) {
				// Make better floors in small caves
				if(flat_cave_floor && y0 <= -rs/2 && rs<=7)
					continue;
				
				if (large_cave_is_flat) {
					// Make large caves not so tall
					if (rs > 7 && abs(y0) >= rs / 3)
						continue;
				}

				v3s16 p(cp.X + x0, cp.Y + y0, cp.Z + z0);
				p += of;
				
				if (!is_ravine && mg->heightmap && should_make_cave_hole &&
					p.X <= node_max.X && p.Z <= node_max.Z) {
					int maplen = node_max.X - node_min.X + 1;
					int idx = (p.Z - node_min.Z) * maplen + (p.X - node_min.X);
					if (p.Y >= mg->heightmap[idx] - 2)
						continue;
				}

				if (vm->m_area.contains(p) == false)
					continue;

				u32 i = vm->m_area.index(p);
				
				// Don't replace air, water, lava, or ice
				content_t c = vm->m_data[i].getContent();
				if (!ndef->get(c).is_ground_content || c == CONTENT_AIR ||
					c == c_water_source || c == c_lava_source || c == c_ice)
					continue;
					
				if (large_cave) {
					int full_ymin = node_min.Y - MAP_BLOCKSIZE;
					int full_ymax = node_max.Y + MAP_BLOCKSIZE;

					if (flooded && full_ymin < water_level && full_ymax > water_level)
						vm->m_data[i] = (p.Y <= water_level) ? waternode : airnode;
					else if (flooded && full_ymax < water_level)
						vm->m_data[i] = (p.Y < startp.Y - 4) ? liquidnode : airnode;
					else
						vm->m_data[i] = airnode;
				} else {
					if (c == CONTENT_IGNORE)
						continue;
					
					vm->m_data[i] = airnode;
					vm->m_flags[i] |= VMANIP_FLAG_CAVE;
				}
			}
		}
	}
}
