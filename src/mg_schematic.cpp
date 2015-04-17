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
#include <typeinfo>
#include "mg_schematic.h"
#include "gamedef.h"
#include "mapgen.h"
#include "emerge.h"
#include "map.h"
#include "mapblock.h"
#include "log.h"
#include "util/numeric.h"
#include "util/serialize.h"
#include "serialization.h"
#include "filesys.h"

///////////////////////////////////////////////////////////////////////////////


SchematicManager::SchematicManager(IGameDef *gamedef) :
	ObjDefManager(gamedef, OBJDEF_SCHEMATIC)
{
	m_gamedef = gamedef;
}


void SchematicManager::clear()
{
	EmergeManager *emerge = m_gamedef->getEmergeManager();

	// Remove all dangling references in Decorations
	DecorationManager *decomgr = emerge->decomgr;
	for (size_t i = 0; i != decomgr->getNumObjects(); i++) {
		Decoration *deco = (Decoration *)decomgr->getRaw(i);

		try {
			DecoSchematic *dschem = dynamic_cast<DecoSchematic *>(deco);
			if (dschem)
				dschem->schematic = NULL;
		} catch (std::bad_cast) {
		}
	}

	ObjDefManager::clear();
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


void Schematic::resolveNodeNames()
{
	getIdsFromNrBacklog(&c_nodes, true, CONTENT_AIR);

	size_t bufsize = size.X * size.Y * size.Z;
	for (size_t i = 0; i != bufsize; i++) {
		content_t c_original = schemdata[i].getContent();
		content_t c_new = c_nodes[c_original];
		schemdata[i].setContent(c_new);
	}
}


void Schematic::blitToVManip(v3s16 p, MMVManip *vm, Rotation rot,
	bool force_placement, INodeDefManager *ndef)
{
	int xstride = 1;
	int ystride = size.X;
	int zstride = size.X * size.Y;

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
	assert(schemdata != NULL); // Pre-condition
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


bool Schematic::deserializeFromMts(std::istream *is, std::vector<std::string> *names)
{
	std::istream &ss = *is;
	content_t cignore = CONTENT_IGNORE;
	bool have_cignore = false;

	u32 signature = readU32(ss);
	if (signature != MTSCHEM_FILE_SIGNATURE) {
		errorstream << "Schematic::deserializeFromMts: invalid schematic "
			"file" << std::endl;
		return false;
	}

	u16 version = readU16(ss);
	if (version > MTSCHEM_FILE_VER_HIGHEST_READ) {
		errorstream << "Schematic::deserializeFromMts: unsupported schematic "
			"file version" << std::endl;
		return false;
	}

	size = readV3S16(ss);

	delete []slice_probs;
	slice_probs = new u8[size.Y];
	for (int y = 0; y != size.Y; y++)
		slice_probs[y] = (version >= 3) ? readU8(ss) : MTSCHEM_PROB_ALWAYS;

	u16 nidmapcount = readU16(ss);
	for (int i = 0; i != nidmapcount; i++) {
		std::string name = deSerializeString(ss);

		// Instances of "ignore" from ver 1 are converted to air (and instances
		// are fixed to have MTSCHEM_PROB_NEVER later on).
		if (name == "ignore") {
			name = "air";
			cignore = i;
			have_cignore = true;
		}

		names->push_back(name);
	}

	size_t nodecount = size.X * size.Y * size.Z;

	delete []schemdata;
	schemdata = new MapNode[nodecount];

	MapNode::deSerializeBulk(ss, SER_FMT_VER_HIGHEST_READ, schemdata,
		nodecount, 2, 2, true);

	// fix any probability values for nodes that were ignore
	if (version == 1) {
		for (size_t i = 0; i != nodecount; i++) {
			if (schemdata[i].param1 == 0)
				schemdata[i].param1 = MTSCHEM_PROB_ALWAYS;
			if (have_cignore && schemdata[i].getContent() == cignore)
				schemdata[i].param1 = MTSCHEM_PROB_NEVER;
		}
	}

	return true;
}


bool Schematic::serializeToMts(std::ostream *os)
{
	std::ostream &ss = *os;

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
		ss << serializeString(getNodeName(usednodes[i])); // node names

	// compressed bulk node data
	MapNode::serializeBulk(ss, SER_FMT_VER_HIGHEST_WRITE,
		schemdata, nodecount, 2, 2, true);

	return true;
}


bool Schematic::serializeToLua(std::ostream *os, bool use_comments)
{
	std::ostream &ss = *os;

	//// Write header
	{
		ss << "schematic = {" << std::endl;
		ss << "\tsize = "
			<< "{x=" << size.X
			<< ", y=" << size.Y
			<< ", z=" << size.Z
			<< "}," << std::endl;
	}

	//// Write y-slice probabilities
	{
		ss << "\tyslice_prob = {" << std::endl;

		for (u16 y = 0; y != size.Y; y++) {
			ss << "\t\t{"
				<< "ypos=" << y
				<< ", prob=" << (u16)slice_probs[y]
				<< "}," << std::endl;
		}

		ss << "\t}," << std::endl;
	}

	//// Write node data
	{
		ss << "\tdata = {" << std::endl;

		u32 i = 0;
		for (u16 z = 0; z != size.Z; z++)
		for (u16 y = 0; y != size.Y; y++) {
			if (use_comments) {
				ss << std::endl
					<< "\t\t-- z=" << z
					<< ", y=" << y << std::endl;
			}

			for (u16 x = 0; x != size.X; x++, i++) {
				ss << "\t\t{"
					<< "name=\"" << getNodeName(schemdata[i].getContent())
					<< "\", param1=" << (u16)schemdata[i].param1
					<< ", param2=" << (u16)schemdata[i].param2
					<< "}," << std::endl;
			}
		}

		ss << "\t}," << std::endl;
	}

	ss << "}" << std::endl;

	return true;
}


bool Schematic::loadSchematicFromFile(const std::string &filename,
	INodeDefManager *ndef, StringMap *replace_names,
	NodeResolveMethod resolve_method)
{
	std::ifstream is(filename.c_str(), std::ios_base::binary);
	if (!is.good()) {
		errorstream << "Schematic::loadSchematicFile: unable to open file '"
			<< filename << "'" << std::endl;
		return false;
	}

	size_t origsize = m_nodenames.size();
	if (!deserializeFromMts(&is, &m_nodenames))
		return false;

	if (replace_names) {
		for (size_t i = origsize; i != m_nodenames.size(); i++) {
			std::string &name = m_nodenames[i];
			StringMap::iterator it = replace_names->find(name);
			if (it != replace_names->end())
				name = it->second;
		}
	}

	m_nnlistsizes.push_back(m_nodenames.size() - origsize);

	ndef->pendNodeResolve(this, resolve_method);

	return true;
}


bool Schematic::saveSchematicToFile(const std::string &filename)
{
	std::ostringstream os(std::ios_base::binary);
	serializeToMts(&os);
	return fs::safeWriteToFile(filename, os.str());
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
