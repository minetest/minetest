/*
Minetest
Copyright (C) 2010-2014 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include <fstream>
#include "mg_schematic.h"
#include "mapgen.h"
#include "map.h"
#include "mapblock.h"
#include "log.h"
#include "util/numeric.h"
#include "util/serialize.h"
#include "serialization.h"
#include "filesys.h"

const char *SchematicManager::ELEMENT_TITLE = "schematic";

///////////////////////////////////////////////////////////////////////////////


SchematicManager::SchematicManager(IGameDef *gamedef) :
	GenElementManager(gamedef)
{
}


///////////////////////////////////////////////////////////////////////////////


Schematic::Schematic()
{
	schemdata   = NULL;
	slice_probs = NULL;
	flags       = 0;
	size        = v3s16(0, 0, 0);
}


Schematic::~Schematic()
{
	delete []schemdata;
	delete []slice_probs;
}


void Schematic::resolveNodeNames(NodeResolveInfo *nri)
{
	m_ndef->getIdsFromResolveInfo(nri, c_nodes);
}


void Schematic::updateContentIds()
{
	if (flags & SCHEM_CIDS_UPDATED)
		return;

	flags |= SCHEM_CIDS_UPDATED;

	size_t bufsize = size.X * size.Y * size.Z;
	for (size_t i = 0; i != bufsize; i++)
		schemdata[i].setContent(c_nodes[schemdata[i].getContent()]);
}


void Schematic::blitToVManip(v3s16 p, MMVManip *vm, Rotation rot,
	bool force_placement, INodeDefManager *ndef)
{
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

				if (schemdata[i].getContent() == CONTENT_IGNORE)
					continue;

				if (schemdata[i].param1 == MTSCHEM_PROB_NEVER)
					continue;

				if (!force_placement) {
					content_t c = vm->m_data[vi].getContent();
					if (c != CONTENT_AIR && c != CONTENT_IGNORE)
						continue;
				}

				if (schemdata[i].param1 != MTSCHEM_PROB_ALWAYS &&
					myrand_range(1, 255) > schemdata[i].param1)
					continue;

				vm->m_data[vi] = schemdata[i];
				vm->m_data[vi].param1 = 0;

				if (rot)
					vm->m_data[vi].rotateAlongYAxis(ndef, rot);
			}
		}
		y_map++;
	}
}


void Schematic::placeStructure(Map *map, v3s16 p, u32 flags, Rotation rot,
	bool force_placement, INodeDefManager *ndef)
{
	assert(schemdata != NULL);
	MMVManip *vm = new MMVManip(map);

	if (rot == ROTATE_RAND)
		rot = (Rotation)myrand_range(ROTATE_0, ROTATE_270);

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

	blitToVManip(p, vm, rot, force_placement, ndef);

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


bool Schematic::loadSchematicFromFile(const char *filename, INodeDefManager *ndef,
	std::map<std::string, std::string> &replace_names)
{
	content_t cignore = CONTENT_IGNORE;
	bool have_cignore = false;

	std::ifstream is(filename, std::ios_base::binary);

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
	for (int y = 0; y != size.Y; y++)
		slice_probs[y] = (version >= 3) ? readU8(is) : MTSCHEM_PROB_ALWAYS;

	NodeResolveInfo *nri = new NodeResolveInfo(this);

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

		nri->nodenames.push_back(name);
	}

	nri->nodelistinfo.push_back(NodeListInfo(nidmapcount, CONTENT_AIR));
	ndef->pendNodeResolve(nri);

	size_t nodecount = size.X * size.Y * size.Z;

	delete []schemdata;
	schemdata = new MapNode[nodecount];

	MapNode::deSerializeBulk(is, SER_FMT_VER_HIGHEST_READ, schemdata,
		nodecount, 2, 2, true);

	if (version == 1) { // fix up the probability values
		for (size_t i = 0; i != nodecount; i++) {
			if (schemdata[i].param1 == 0)
				schemdata[i].param1 = MTSCHEM_PROB_ALWAYS;
			if (have_cignore && schemdata[i].getContent() == cignore)
				schemdata[i].param1 = MTSCHEM_PROB_NEVER;
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
void Schematic::saveSchematicToFile(const char *filename, INodeDefManager *ndef)
{
	std::ostringstream ss(std::ios_base::binary);

	writeU32(ss, MTSCHEM_FILE_SIGNATURE);         // signature
	writeU16(ss, MTSCHEM_FILE_VER_HIGHEST_WRITE); // version
	writeV3S16(ss, size);                         // schematic size

	for (int y = 0; y != size.Y; y++)             // Y slice probabilities
		writeU8(ss, slice_probs[y]);

	std::vector<content_t> usednodes;
	int nodecount = size.X * size.Y * size.Z;
	build_nnlist_and_update_ids(schemdata, nodecount, &usednodes);

	u16 numids = usednodes.size();
	writeU16(ss, numids); // name count
	for (int i = 0; i != numids; i++)
		ss << serializeString(ndef->get(usednodes[i]).name); // node names

	// compressed bulk node data
	MapNode::serializeBulk(ss, SER_FMT_VER_HIGHEST_WRITE, schemdata,
				nodecount, 2, 2, true);

	fs::safeWriteToFile(filename, ss.str());
}


void build_nnlist_and_update_ids(MapNode *nodes, u32 nodecount,
	std::vector<content_t> *usednodes)
{
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


bool Schematic::getSchematicFromMap(Map *map, v3s16 p1, v3s16 p2)
{
	MMVManip *vm = new MMVManip(map);

	v3s16 bp1 = getNodeBlockPos(p1);
	v3s16 bp2 = getNodeBlockPos(p2);
	vm->initialEmerge(bp1, bp2);

	size = p2 - p1 + 1;

	slice_probs = new u8[size.Y];
	for (s16 y = 0; y != size.Y; y++)
		slice_probs[y] = MTSCHEM_PROB_ALWAYS;

	schemdata = new MapNode[size.X * size.Y * size.Z];

	u32 i = 0;
	for (s16 z = p1.Z; z <= p2.Z; z++)
	for (s16 y = p1.Y; y <= p2.Y; y++) {
		u32 vi = vm->m_area.index(p1.X, y, z);
		for (s16 x = p1.X; x <= p2.X; x++, i++, vi++) {
			schemdata[i] = vm->m_data[vi];
			schemdata[i].param1 = MTSCHEM_PROB_ALWAYS;
		}
	}

	delete vm;
	return true;
}


void Schematic::applyProbabilities(v3s16 p0,
	std::vector<std::pair<v3s16, u8> > *plist,
	std::vector<std::pair<s16, u8> > *splist)
{
	for (size_t i = 0; i != plist->size(); i++) {
		v3s16 p = (*plist)[i].first - p0;
		int index = p.Z * (size.Y * size.X) + p.Y * size.X + p.X;
		if (index < size.Z * size.Y * size.X) {
			u8 prob = (*plist)[i].second;
			schemdata[index].param1 = prob;

			// trim unnecessary node names from schematic
			if (prob == MTSCHEM_PROB_NEVER)
				schemdata[index].setContent(CONTENT_AIR);
		}
	}

	for (size_t i = 0; i != splist->size(); i++) {
		s16 y = (*splist)[i].first - p0.Y;
		slice_probs[y] = (*splist)[i].second;
	}
}
