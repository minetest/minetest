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

#include "mapgen.h"
#include "voxel.h"
#include "noise.h"
#include "biome.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "content_sao.h"
#include "nodedef.h"
#include "content_mapnode.h" // For content_mapnode_get_new_name
#include "voxelalgorithms.h"
#include "profiler.h"
#include "settings.h" // For g_settings
#include "main.h" // For g_profiler
#include "treegen.h"
#include "mapgen_v6.h"
#include "mapgen_v7.h"
#include "serialization.h"
#include "util/serialize.h"
#include "filesys.h"
#include "log.h"


FlagDesc flagdesc_mapgen[] = {
	{"trees",    MG_TREES},
	{"caves",    MG_CAVES},
	{"dungeons", MG_DUNGEONS},
	{"flat",     MG_FLAT},
	{"light",    MG_LIGHT},
	{NULL,       0}
};

FlagDesc flagdesc_ore[] = {
	{"absheight",            OREFLAG_ABSHEIGHT},
	{"scatter_noisedensity", OREFLAG_DENSITY},
	{"claylike_nodeisnt",    OREFLAG_NODEISNT},
	{NULL,                   0}
};

FlagDesc flagdesc_deco_schematic[] = {
	{"place_center_x", DECO_PLACE_CENTER_X},
	{"place_center_y", DECO_PLACE_CENTER_Y},
	{"place_center_z", DECO_PLACE_CENTER_Z},
	{NULL,             0}
};

FlagDesc flagdesc_gennotify[] = {
	{"dungeon",          1 << GENNOTIFY_DUNGEON},
	{"temple",           1 << GENNOTIFY_TEMPLE},
	{"cave_begin",       1 << GENNOTIFY_CAVE_BEGIN},
	{"cave_end",         1 << GENNOTIFY_CAVE_END},
	{"large_cave_begin", 1 << GENNOTIFY_LARGECAVE_BEGIN},
	{"large_cave_end",   1 << GENNOTIFY_LARGECAVE_END},
	{NULL,               0}
};

///////////////////////////////////////////////////////////////////////////////


Ore *createOre(OreType type) {
	switch (type) {
		case ORE_SCATTER:
			return new OreScatter;
		case ORE_SHEET:
			return new OreSheet;
		//case ORE_CLAYLIKE: //TODO: implement this!
		//	return new OreClaylike;
		default:
			return NULL;
	}
}


Ore::~Ore() {
	delete np;
	delete noise;
}


void Ore::placeOre(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax) {
	int in_range = 0;

	in_range |= (nmin.Y <= height_max && nmax.Y >= height_min);
	if (flags & OREFLAG_ABSHEIGHT)
		in_range |= (nmin.Y >= -height_max && nmax.Y <= -height_min) << 1;
	if (!in_range)
		return;

	int ymin, ymax;
	if (in_range & ORE_RANGE_MIRROR) {
		ymin = MYMAX(nmin.Y, -height_max);
		ymax = MYMIN(nmax.Y, -height_min);
	} else {
		ymin = MYMAX(nmin.Y, height_min);
		ymax = MYMIN(nmax.Y, height_max);
	}
	if (clust_size >= ymax - ymin + 1)
		return;

	nmin.Y = ymin;
	nmax.Y = ymax;
	generate(mg->vm, mg->seed, blockseed, nmin, nmax);
}


void OreScatter::generate(ManualMapVoxelManipulator *vm, int seed,
						  u32 blockseed, v3s16 nmin, v3s16 nmax) {
	PseudoRandom pr(blockseed);
	MapNode n_ore(c_ore, 0, ore_param2);

	int volume = (nmax.X - nmin.X + 1) *
				 (nmax.Y - nmin.Y + 1) *
				 (nmax.Z - nmin.Z + 1);
	int csize     = clust_size;
	int orechance = (csize * csize * csize) / clust_num_ores;
	int nclusters = volume / clust_scarcity;

	for (int i = 0; i != nclusters; i++) {
		int x0 = pr.range(nmin.X, nmax.X - csize + 1);
		int y0 = pr.range(nmin.Y, nmax.Y - csize + 1);
		int z0 = pr.range(nmin.Z, nmax.Z - csize + 1);

		if (np && (NoisePerlin3D(np, x0, y0, z0, seed) < nthresh))
			continue;

		for (int z1 = 0; z1 != csize; z1++)
		for (int y1 = 0; y1 != csize; y1++)
		for (int x1 = 0; x1 != csize; x1++) {
			if (pr.range(1, orechance) != 1)
				continue;

			u32 i = vm->m_area.index(x0 + x1, y0 + y1, z0 + z1);
			for (size_t ii = 0; ii < c_wherein.size(); ii++)
				if (vm->m_data[i].getContent() == c_wherein[ii])
					vm->m_data[i] = n_ore;
		}
	}
}


void OreSheet::generate(ManualMapVoxelManipulator *vm, int seed,
						u32 blockseed, v3s16 nmin, v3s16 nmax) {
	PseudoRandom pr(blockseed + 4234);
	MapNode n_ore(c_ore, 0, ore_param2);

	int max_height = clust_size;
	int y_start = pr.range(nmin.Y, nmax.Y - max_height);

	if (!noise) {
		int sx = nmax.X - nmin.X + 1;
		int sz = nmax.Z - nmin.Z + 1;
		noise = new Noise(np, 0, sx, sz);
	}
	noise->seed = seed + y_start;
	noise->perlinMap2D(nmin.X, nmin.Z);

	int index = 0;
	for (int z = nmin.Z; z <= nmax.Z; z++)
	for (int x = nmin.X; x <= nmax.X; x++) {
		float noiseval = noise->result[index++];
		if (noiseval < nthresh)
			continue;

		int height = max_height * (1. / pr.range(1, 3));
		int y0 = y_start + np->scale * noiseval; //pr.range(1, 3) - 1;
		int y1 = y0 + height;
		for (int y = y0; y != y1; y++) {
			u32 i = vm->m_area.index(x, y, z);
			if (!vm->m_area.contains(i))
				continue;

			for (size_t ii = 0; ii < c_wherein.size(); ii++) {
				if (vm->m_data[i].getContent() == c_wherein[ii]) {
					vm->m_data[i] = n_ore;
					break;
				}
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////


Decoration *createDecoration(DecorationType type) {
	switch (type) {
		case DECO_SIMPLE:
			return new DecoSimple;
		case DECO_SCHEMATIC:
			return new DecoSchematic;
		//case DECO_LSYSTEM:
		//	return new DecoLSystem;
		default:
			return NULL;
	}
}


Decoration::Decoration() {
	mapseed    = 0;
	np         = NULL;
	fill_ratio = 0;
	sidelen    = 1;
}


Decoration::~Decoration() {
	delete np;
}


void Decoration::placeDeco(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax) {
	PseudoRandom ps(blockseed + 53);
	int carea_size = nmax.X - nmin.X + 1;

	// Divide area into parts
	if (carea_size % sidelen) {
		errorstream << "Decoration::placeDeco: chunk size is not divisible by "
			"sidelen; setting sidelen to " << carea_size << std::endl;
		sidelen = carea_size;
	}

	s16 divlen = carea_size / sidelen;
	int area = sidelen * sidelen;

	for (s16 z0 = 0; z0 < divlen; z0++)
	for (s16 x0 = 0; x0 < divlen; x0++) {
		v2s16 p2d_center( // Center position of part of division
			nmin.X + sidelen / 2 + sidelen * x0,
			nmin.Z + sidelen / 2 + sidelen * z0
		);
		v2s16 p2d_min( // Minimum edge of part of division
			nmin.X + sidelen * x0,
			nmin.Z + sidelen * z0
		);
		v2s16 p2d_max( // Maximum edge of part of division
			nmin.X + sidelen + sidelen * x0 - 1,
			nmin.Z + sidelen + sidelen * z0 - 1
		);

		// Amount of decorations
		float nval = np ?
			NoisePerlin2D(np, p2d_center.X, p2d_center.Y, mapseed) :
			fill_ratio;
		u32 deco_count = area * MYMAX(nval, 0.f);

		for (u32 i = 0; i < deco_count; i++) {
			s16 x = ps.range(p2d_min.X, p2d_max.X);
			s16 z = ps.range(p2d_min.Y, p2d_max.Y);

			int mapindex = carea_size * (z - nmin.Z) + (x - nmin.X);

			s16 y = mg->heightmap ?
					mg->heightmap[mapindex] :
					mg->findGroundLevel(v2s16(x, z), nmin.Y, nmax.Y);

			if (y < nmin.Y || y > nmax.Y)
				continue;

			int height = getHeight();
			int max_y = nmax.Y;// + MAP_BLOCKSIZE - 1;
			if (y + 1 + height > max_y) {
				continue;
#if 0
				printf("Decoration at (%d %d %d) cut off\n", x, y, z);
				//add to queue
				JMutexAutoLock cutofflock(cutoff_mutex);
				cutoffs.push_back(CutoffData(x, y, z, height));
#endif
			}

			if (mg->biomemap) {
				std::set<u8>::iterator iter;

				if (biomes.size()) {
					iter = biomes.find(mg->biomemap[mapindex]);
					if (iter == biomes.end())
						continue;
				}
			}

			generate(mg, &ps, max_y, v3s16(x, y, z));
		}
	}
}


#if 0
void Decoration::placeCutoffs(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax) {
	PseudoRandom pr(blockseed + 53);
	std::vector<CutoffData> handled_cutoffs;

	// Copy over the cutoffs we're interested in so we don't needlessly hold a lock
	{
		JMutexAutoLock cutofflock(cutoff_mutex);
		for (std::list<CutoffData>::iterator i = cutoffs.begin();
			i != cutoffs.end(); ++i) {
			CutoffData cutoff = *i;
			v3s16 p    = cutoff.p;
			s16 height = cutoff.height;
			if (p.X < nmin.X || p.X > nmax.X ||
				p.Z < nmin.Z || p.Z > nmax.Z)
				continue;
			if (p.Y + height < nmin.Y || p.Y > nmax.Y)
				continue;

			handled_cutoffs.push_back(cutoff);
		}
	}

	// Generate the cutoffs
	for (size_t i = 0; i != handled_cutoffs.size(); i++) {
		v3s16 p    = handled_cutoffs[i].p;
		s16 height = handled_cutoffs[i].height;

		if (p.Y + height > nmax.Y) {
			//printf("Decoration at (%d %d %d) cut off again!\n", p.X, p.Y, p.Z);
			cuttoffs.push_back(v3s16(p.X, p.Y, p.Z));
		}

		generate(mg, &pr, nmax.Y, nmin.Y - p.Y, v3s16(p.X, nmin.Y, p.Z));
	}

	// Remove cutoffs that were handled from the cutoff list
	{
		JMutexAutoLock cutofflock(cutoff_mutex);
		for (std::list<CutoffData>::iterator i = cutoffs.begin();
			i != cutoffs.end(); ++i) {

			for (size_t j = 0; j != handled_cutoffs.size(); j++) {
				CutoffData coff = *i;
				if (coff.p == handled_cutoffs[j].p)
					i = cutoffs.erase(i);
			}
		}
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////


void DecoSimple::generate(Mapgen *mg, PseudoRandom *pr, s16 max_y, v3s16 p) {
	ManualMapVoxelManipulator *vm = mg->vm;

	u32 vi = vm->m_area.index(p);
	content_t c = vm->m_data[vi].getContent();
	size_t idx;
	for (idx = 0; idx != c_place_on.size(); idx++) {
		if (c == c_place_on[idx])
			break;
	}
	if ((idx != 0) && (idx == c_place_on.size()))
		return;

	if (nspawnby != -1) {
		int nneighs = 0;
		v3s16 dirs[8] = { // a Moore neighborhood
			v3s16( 0, 0,  1),
			v3s16( 0, 0, -1),
			v3s16( 1, 0,  0),
			v3s16(-1, 0,  0),
			v3s16( 1, 0,  1),
			v3s16(-1, 0,  1),
			v3s16(-1, 0, -1),
			v3s16( 1, 0, -1)
		};

		for (int i = 0; i != 8; i++) {
			u32 index = vm->m_area.index(p + dirs[i]);
			if (!vm->m_area.contains(index))
				continue;

			content_t c = vm->m_data[index].getContent();
			for (size_t j = 0; j != c_spawnby.size(); j++) {
				if (c == c_spawnby[j]) {
					nneighs++;
					break;
				}
			}
		}

		if (nneighs < nspawnby)
			return;
	}

	if (c_decos.size() == 0)
		return;
	content_t c_place = c_decos[pr->range(0, c_decos.size() - 1)];

	s16 height = (deco_height_max > 0) ?
		pr->range(deco_height, deco_height_max) : deco_height;

	height = MYMIN(height, max_y - p.Y);

	v3s16 em = vm->m_area.getExtent();
	for (int i = 0; i < height; i++) {
		vm->m_area.add_y(em, vi, 1);

		content_t c = vm->m_data[vi].getContent();
		if (c != CONTENT_AIR && c != CONTENT_IGNORE)
			break;

		vm->m_data[vi] = MapNode(c_place);
	}
}


int DecoSimple::getHeight() {
	return (deco_height_max > 0) ? deco_height_max : deco_height;
}


std::string DecoSimple::getName() {
	return "";
}


///////////////////////////////////////////////////////////////////////////////


DecoSchematic::DecoSchematic() {
	schematic   = NULL;
	slice_probs = NULL;
	flags       = 0;
	size        = v3s16(0, 0, 0);
}


DecoSchematic::~DecoSchematic() {
	delete []schematic;
	delete []slice_probs;
}


void DecoSchematic::updateContentIds() {
	if (flags & DECO_SCHEM_CIDS_UPDATED)
		return;

	flags |= DECO_SCHEM_CIDS_UPDATED;

	for (int i = 0; i != size.X * size.Y * size.Z; i++)
		schematic[i].setContent(c_nodes[schematic[i].getContent()]);
}


void DecoSchematic::generate(Mapgen *mg, PseudoRandom *pr, s16 max_y, v3s16 p) {
	ManualMapVoxelManipulator *vm = mg->vm;

	if (flags & DECO_PLACE_CENTER_X)
		p.X -= (size.X + 1) / 2;
	if (flags & DECO_PLACE_CENTER_Y)
		p.Y -= (size.Y + 1) / 2;
	if (flags & DECO_PLACE_CENTER_Z)
		p.Z -= (size.Z + 1) / 2;

	u32 vi = vm->m_area.index(p);
	content_t c = vm->m_data[vi].getContent();
	size_t idx;
	for (idx = 0; idx != c_place_on.size(); idx++) {
		if (c == c_place_on[idx])
			break;
	}
	if ((idx != 0) && (idx == c_place_on.size()))
		return;

	Rotation rot = (rotation == ROTATE_RAND) ?
		(Rotation)pr->range(ROTATE_0, ROTATE_270) : rotation;

	blitToVManip(p, vm, rot, false);
}


int DecoSchematic::getHeight() {
	return size.Y;
}


std::string DecoSchematic::getName() {
	return filename;
}


void DecoSchematic::blitToVManip(v3s16 p, ManualMapVoxelManipulator *vm,
								Rotation rot, bool force_placement) {
	int xstride = 1;
	int ystride = size.X;
	int zstride = size.X * size.Y;

	updateContentIds();

	s16 sx = size.X;
	s16 sy = size.Y;
	s16 sz = size.Z;

	int i_start, i_step_x, i_step_z;
	switch (rot) {
		case ROTATE_90:
			i_start  = sx - 1;
			i_step_x = zstride;
			i_step_z = -xstride;
			SWAP(s16, sx, sz);
			break;
		case ROTATE_180:
			i_start  = zstride * (sz - 1) + sx - 1;
			i_step_x = -xstride;
			i_step_z = -zstride;
			break;
		case ROTATE_270:
			i_start  = zstride * (sz - 1);
			i_step_x = -zstride;
			i_step_z = xstride;
			SWAP(s16, sx, sz);
			break;
		default:
			i_start  = 0;
			i_step_x = xstride;
			i_step_z = zstride;
	}

	s16 y_map = p.Y;
	for (s16 y = 0; y != sy; y++) {
		if (slice_probs[y] != MTSCHEM_PROB_ALWAYS &&
			myrand_range(1, 255) > slice_probs[y])
			continue;

		for (s16 z = 0; z != sz; z++) {
			u32 i = z * i_step_z + y * ystride + i_start;
			for (s16 x = 0; x != sx; x++, i += i_step_x) {
				u32 vi = vm->m_area.index(p.X + x, y_map, p.Z + z);
				if (!vm->m_area.contains(vi))
					continue;

				if (schematic[i].getContent() == CONTENT_IGNORE)
					continue;

				if (schematic[i].param1 == MTSCHEM_PROB_NEVER)
					continue;

				if (!force_placement) {
					content_t c = vm->m_data[vi].getContent();
					if (c != CONTENT_AIR && c != CONTENT_IGNORE)
						continue;
				}

				if (schematic[i].param1 != MTSCHEM_PROB_ALWAYS &&
					myrand_range(1, 255) > schematic[i].param1)
					continue;

				vm->m_data[vi] = schematic[i];
				vm->m_data[vi].param1 = 0;

				if (rot)
					vm->m_data[vi].rotateAlongYAxis(ndef, rot);
			}
		}
		y_map++;
	}
}


void DecoSchematic::placeStructure(Map *map, v3s16 p, bool force_placement) {
	assert(schematic != NULL);
	ManualMapVoxelManipulator *vm = new ManualMapVoxelManipulator(map);

	Rotation rot = (rotation == ROTATE_RAND) ?
		(Rotation)myrand_range(ROTATE_0, ROTATE_270) : rotation;

	v3s16 s = (rot == ROTATE_90 || rot == ROTATE_270) ?
				v3s16(size.Z, size.Y, size.X) : size;

	if (flags & DECO_PLACE_CENTER_X)
		p.X -= (s.X + 1) / 2;
	if (flags & DECO_PLACE_CENTER_Y)
		p.Y -= (s.Y + 1) / 2;
	if (flags & DECO_PLACE_CENTER_Z)
		p.Z -= (s.Z + 1) / 2;

	v3s16 bp1 = getNodeBlockPos(p);
	v3s16 bp2 = getNodeBlockPos(p + s - v3s16(1,1,1));
	vm->initialEmerge(bp1, bp2);

	blitToVManip(p, vm, rot, force_placement);

	std::map<v3s16, MapBlock *> lighting_modified_blocks;
	std::map<v3s16, MapBlock *> modified_blocks;
	vm->blitBackAll(&modified_blocks);

	// TODO: Optimize this by using Mapgen::calcLighting() instead
	lighting_modified_blocks.insert(modified_blocks.begin(), modified_blocks.end());
	map->updateLighting(lighting_modified_blocks, modified_blocks);

	MapEditEvent event;
	event.type = MEET_OTHER;
	for (std::map<v3s16, MapBlock *>::iterator
		it = modified_blocks.begin();
		it != modified_blocks.end(); ++it)
		event.modified_blocks.insert(it->first);

	map->dispatchEvent(&event);
}


bool DecoSchematic::loadSchematicFile(NodeResolver *resolver,
	std::map<std::string, std::string> &replace_names)
{
	content_t cignore = CONTENT_IGNORE;
	bool have_cignore = false;

	std::ifstream is(filename.c_str(), std::ios_base::binary);

	u32 signature = readU32(is);
	if (signature != MTSCHEM_FILE_SIGNATURE) {
		errorstream << "loadSchematicFile: invalid schematic "
			"file" << std::endl;
		return false;
	}

	u16 version = readU16(is);
	if (version > MTSCHEM_FILE_VER_HIGHEST_READ) {
		errorstream << "loadSchematicFile: unsupported schematic "
			"file version" << std::endl;
		return false;
	}

	size = readV3S16(is);

	delete []slice_probs;
	slice_probs = new u8[size.Y];
	if (version >= 3) {
		for (int y = 0; y != size.Y; y++)
			slice_probs[y] = readU8(is);
	} else {
		for (int y = 0; y != size.Y; y++)
			slice_probs[y] = MTSCHEM_PROB_ALWAYS;
	}

	int nodecount = size.X * size.Y * size.Z;

	u16 nidmapcount = readU16(is);

	for (int i = 0; i != nidmapcount; i++) {
		std::string name = deSerializeString(is);
		if (name == "ignore") {
			name = "air";
			cignore = i;
			have_cignore = true;
		}

		std::map<std::string, std::string>::iterator it;

		it = replace_names.find(name);
		if (it != replace_names.end())
			name = it->second;

		resolver->addNodeList(name.c_str(), &c_nodes);
	}

	delete []schematic;
	schematic = new MapNode[nodecount];
	MapNode::deSerializeBulk(is, SER_FMT_VER_HIGHEST_READ, schematic,
				nodecount, 2, 2, true);

	if (version == 1) { // fix up the probability values
		for (int i = 0; i != nodecount; i++) {
			if (schematic[i].param1 == 0)
				schematic[i].param1 = MTSCHEM_PROB_ALWAYS;
			if (have_cignore && schematic[i].getContent() == cignore)
				schematic[i].param1 = MTSCHEM_PROB_NEVER;
		}
	}

	return true;
}


/*
	Minetest Schematic File Format

	All values are stored in big-endian byte order.
	[u32] signature: 'MTSM'
	[u16] version: 3
	[u16] size X
	[u16] size Y
	[u16] size Z
	For each Y:
		[u8] slice probability value
	[Name-ID table] Name ID Mapping Table
		[u16] name-id count
		For each name-id mapping:
			[u16] name length
			[u8[]] name
	ZLib deflated {
	For each node in schematic:  (for z, y, x)
		[u16] content
	For each node in schematic:
		[u8] probability of occurance (param1)
	For each node in schematic:
		[u8] param2
	}

	Version changes:
	1 - Initial version
	2 - Fixed messy never/always place; 0 probability is now never, 0xFF is always
	3 - Added y-slice probabilities; this allows for variable height structures
*/
void DecoSchematic::saveSchematicFile(INodeDefManager *ndef) {
	std::ostringstream ss(std::ios_base::binary);

	writeU32(ss, MTSCHEM_FILE_SIGNATURE);         // signature
	writeU16(ss, MTSCHEM_FILE_VER_HIGHEST_WRITE); // version
	writeV3S16(ss, size);                         // schematic size

	for (int y = 0; y != size.Y; y++)             // Y slice probabilities
		writeU8(ss, slice_probs[y]);

	std::vector<content_t> usednodes;
	int nodecount = size.X * size.Y * size.Z;
	build_nnlist_and_update_ids(schematic, nodecount, &usednodes);

	u16 numids = usednodes.size();
	writeU16(ss, numids); // name count
	for (int i = 0; i != numids; i++)
		ss << serializeString(ndef->get(usednodes[i]).name); // node names

	// compressed bulk node data
	MapNode::serializeBulk(ss, SER_FMT_VER_HIGHEST_WRITE, schematic,
				nodecount, 2, 2, true);

	fs::safeWriteToFile(filename, ss.str());
}


void build_nnlist_and_update_ids(MapNode *nodes, u32 nodecount,
						std::vector<content_t> *usednodes) {
	std::map<content_t, content_t> nodeidmap;
	content_t numids = 0;

	for (u32 i = 0; i != nodecount; i++) {
		content_t id;
		content_t c = nodes[i].getContent();

		std::map<content_t, content_t>::const_iterator it = nodeidmap.find(c);
		if (it == nodeidmap.end()) {
			id = numids;
			numids++;

			usednodes->push_back(c);
			nodeidmap.insert(std::make_pair(c, id));
		} else {
			id = it->second;
		}
		nodes[i].setContent(id);
	}
}


bool DecoSchematic::getSchematicFromMap(Map *map, v3s16 p1, v3s16 p2) {
	ManualMapVoxelManipulator *vm = new ManualMapVoxelManipulator(map);

	v3s16 bp1 = getNodeBlockPos(p1);
	v3s16 bp2 = getNodeBlockPos(p2);
	vm->initialEmerge(bp1, bp2);

	size = p2 - p1 + 1;

	slice_probs = new u8[size.Y];
	for (s16 y = 0; y != size.Y; y++)
		slice_probs[y] = MTSCHEM_PROB_ALWAYS;

	schematic = new MapNode[size.X * size.Y * size.Z];

	u32 i = 0;
	for (s16 z = p1.Z; z <= p2.Z; z++)
	for (s16 y = p1.Y; y <= p2.Y; y++) {
		u32 vi = vm->m_area.index(p1.X, y, z);
		for (s16 x = p1.X; x <= p2.X; x++, i++, vi++) {
			schematic[i] = vm->m_data[vi];
			schematic[i].param1 = MTSCHEM_PROB_ALWAYS;
		}
	}

	delete vm;
	return true;
}


void DecoSchematic::applyProbabilities(v3s16 p0,
	std::vector<std::pair<v3s16, u8> > *plist,
	std::vector<std::pair<s16, u8> > *splist) {

	for (size_t i = 0; i != plist->size(); i++) {
		v3s16 p = (*plist)[i].first - p0;
		int index = p.Z * (size.Y * size.X) + p.Y * size.X + p.X;
		if (index < size.Z * size.Y * size.X) {
			u8 prob = (*plist)[i].second;
			schematic[index].param1 = prob;

			// trim unnecessary node names from schematic
			if (prob == MTSCHEM_PROB_NEVER)
				schematic[index].setContent(CONTENT_AIR);
		}
	}

	for (size_t i = 0; i != splist->size(); i++) {
		s16 y = (*splist)[i].first - p0.Y;
		slice_probs[y] = (*splist)[i].second;
	}
}


///////////////////////////////////////////////////////////////////////////////


Mapgen::Mapgen() {
	seed        = 0;
	water_level = 0;
	generating  = false;
	id          = -1;
	vm          = NULL;
	ndef        = NULL;
	heightmap   = NULL;
	biomemap    = NULL;

	for (unsigned int i = 0; i != NUM_GEN_NOTIFY; i++)
		gen_notifications[i] = new std::vector<v3s16>;
}


Mapgen::~Mapgen() {
	for (unsigned int i = 0; i != NUM_GEN_NOTIFY; i++)
		delete gen_notifications[i];
}


// Returns Y one under area minimum if not found
s16 Mapgen::findGroundLevelFull(v2s16 p2d) {
	v3s16 em = vm->m_area.getExtent();
	s16 y_nodes_max = vm->m_area.MaxEdge.Y;
	s16 y_nodes_min = vm->m_area.MinEdge.Y;
	u32 i = vm->m_area.index(p2d.X, y_nodes_max, p2d.Y);
	s16 y;

	for (y = y_nodes_max; y >= y_nodes_min; y--) {
		MapNode &n = vm->m_data[i];
		if (ndef->get(n).walkable)
			break;

		vm->m_area.add_y(em, i, -1);
	}
	return (y >= y_nodes_min) ? y : y_nodes_min - 1;
}


s16 Mapgen::findGroundLevel(v2s16 p2d, s16 ymin, s16 ymax) {
	v3s16 em = vm->m_area.getExtent();
	u32 i = vm->m_area.index(p2d.X, ymax, p2d.Y);
	s16 y;

	for (y = ymax; y >= ymin; y--) {
		MapNode &n = vm->m_data[i];
		if (ndef->get(n).walkable)
			break;

		vm->m_area.add_y(em, i, -1);
	}
	return y;
}


void Mapgen::updateHeightmap(v3s16 nmin, v3s16 nmax) {
	if (!heightmap)
		return;

	//TimeTaker t("Mapgen::updateHeightmap", NULL, PRECISION_MICRO);
	int index = 0;
	for (s16 z = nmin.Z; z <= nmax.Z; z++) {
		for (s16 x = nmin.X; x <= nmax.X; x++, index++) {
			s16 y = findGroundLevel(v2s16(x, z), nmin.Y, nmax.Y);

			// if the values found are out of range, trust the old heightmap
			if (y == nmax.Y && heightmap[index] > nmax.Y)
				continue;
			if (y == nmin.Y - 1 && heightmap[index] < nmin.Y)
				continue;

			heightmap[index] = y;
		}
	}
	//printf("updateHeightmap: %dus\n", t.stop());
}


void Mapgen::updateLiquid(UniqueQueue<v3s16> *trans_liquid, v3s16 nmin, v3s16 nmax) {
	bool isliquid, wasliquid;
	v3s16 em  = vm->m_area.getExtent();

	for (s16 z = nmin.Z; z <= nmax.Z; z++) {
		for (s16 x = nmin.X; x <= nmax.X; x++) {
			wasliquid = true;

			u32 i = vm->m_area.index(x, nmax.Y, z);
			for (s16 y = nmax.Y; y >= nmin.Y; y--) {
				isliquid = ndef->get(vm->m_data[i]).isLiquid();

				// there was a change between liquid and nonliquid, add to queue.
				if (isliquid != wasliquid)
					trans_liquid->push_back(v3s16(x, y, z));

				wasliquid = isliquid;
				vm->m_area.add_y(em, i, -1);
			}
		}
	}
}


void Mapgen::setLighting(v3s16 nmin, v3s16 nmax, u8 light) {
	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update", SPT_AVG);
	VoxelArea a(nmin, nmax);

	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int y = a.MinEdge.Y; y <= a.MaxEdge.Y; y++) {
			u32 i = vm->m_area.index(a.MinEdge.X, y, z);
			for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++, i++)
				vm->m_data[i].param1 = light;
		}
	}
}


void Mapgen::lightSpread(VoxelArea &a, v3s16 p, u8 light) {
	if (light <= 1 || !a.contains(p))
		return;

	u32 vi = vm->m_area.index(p);
	MapNode &nn = vm->m_data[vi];

	light--;
	// should probably compare masked, but doesn't seem to make a difference
	if (light <= nn.param1 || !ndef->get(nn).light_propagates)
		return;

	nn.param1 = light;

	lightSpread(a, p + v3s16(0, 0, 1), light);
	lightSpread(a, p + v3s16(0, 1, 0), light);
	lightSpread(a, p + v3s16(1, 0, 0), light);
	lightSpread(a, p - v3s16(0, 0, 1), light);
	lightSpread(a, p - v3s16(0, 1, 0), light);
	lightSpread(a, p - v3s16(1, 0, 0), light);
}


void Mapgen::calcLighting(v3s16 nmin, v3s16 nmax) {
	VoxelArea a(nmin, nmax);
	bool block_is_underground = (water_level >= nmax.Y);

	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update", SPT_AVG);
	//TimeTaker t("updateLighting");

	// first, send vertical rays of sunshine downward
	v3s16 em = vm->m_area.getExtent();
	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++) {
			// see if we can get a light value from the overtop
			u32 i = vm->m_area.index(x, a.MaxEdge.Y + 1, z);
			if (vm->m_data[i].getContent() == CONTENT_IGNORE) {
				if (block_is_underground)
					continue;
			} else if ((vm->m_data[i].param1 & 0x0F) != LIGHT_SUN) {
				continue;
			}
			vm->m_area.add_y(em, i, -1);

			for (int y = a.MaxEdge.Y; y >= a.MinEdge.Y; y--) {
				MapNode &n = vm->m_data[i];
				if (!ndef->get(n).sunlight_propagates)
					break;
				n.param1 = LIGHT_SUN;
				vm->m_area.add_y(em, i, -1);
			}
		}
	}

	// now spread the sunlight and light up any sources
	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int y = a.MinEdge.Y; y <= a.MaxEdge.Y; y++) {
			u32 i = vm->m_area.index(a.MinEdge.X, y, z);
			for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++, i++) {
				MapNode &n = vm->m_data[i];
				if (n.getContent() == CONTENT_IGNORE ||
					!ndef->get(n).light_propagates)
					continue;

				u8 light_produced = ndef->get(n).light_source & 0x0F;
				if (light_produced)
					n.param1 = light_produced;

				u8 light = n.param1 & 0x0F;
				if (light) {
					lightSpread(a, v3s16(x,     y,     z + 1), light - 1);
					lightSpread(a, v3s16(x,     y + 1, z    ), light - 1);
					lightSpread(a, v3s16(x + 1, y,     z    ), light - 1);
					lightSpread(a, v3s16(x,     y,     z - 1), light - 1);
					lightSpread(a, v3s16(x,     y - 1, z    ), light - 1);
					lightSpread(a, v3s16(x - 1, y,     z    ), light - 1);
				}
			}
		}
	}

	//printf("updateLighting: %dms\n", t.stop());
}


void Mapgen::calcLightingOld(v3s16 nmin, v3s16 nmax) {
	enum LightBank banks[2] = {LIGHTBANK_DAY, LIGHTBANK_NIGHT};
	VoxelArea a(nmin, nmax);
	bool block_is_underground = (water_level > nmax.Y);
	bool sunlight = !block_is_underground;

	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update", SPT_AVG);

	for (int i = 0; i < 2; i++) {
		enum LightBank bank = banks[i];
		std::set<v3s16> light_sources;
		std::map<v3s16, u8> unlight_from;

		voxalgo::clearLightAndCollectSources(*vm, a, bank, ndef,
											 light_sources, unlight_from);
		voxalgo::propagateSunlight(*vm, a, sunlight, light_sources, ndef);

		vm->unspreadLight(bank, unlight_from, light_sources, ndef);
		vm->spreadLight(bank, light_sources, ndef);
	}
}
