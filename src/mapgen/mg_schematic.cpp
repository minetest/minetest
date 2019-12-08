/*
Minetest
Copyright (C) 2014-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2015-2018 paramat

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
#include "server.h"
#include "mapgen.h"
#include "emerge.h"
#include "map.h"
#include "mapblock.h"
#include "log.h"
#include "util/numeric.h"
#include "util/serialize.h"
#include "serialization.h"
#include "filesys.h"
#include "voxelalgorithms.h"

///////////////////////////////////////////////////////////////////////////////


SchematicManager::SchematicManager(Server *server) :
	ObjDefManager(server, OBJDEF_SCHEMATIC),
	m_server(server)
{
}


void SchematicManager::clear()
{
	EmergeManager *emerge = m_server->getEmergeManager();

	// Remove all dangling references in Decorations
	DecorationManager *decomgr = emerge->decomgr;
	for (size_t i = 0; i != decomgr->getNumObjects(); i++) {
		Decoration *deco = (Decoration *)decomgr->getRaw(i);

		try {
			DecoSchematic *dschem = dynamic_cast<DecoSchematic *>(deco);
			if (dschem)
				dschem->schematic = NULL;
		} catch (const std::bad_cast &) {
		}
	}

	ObjDefManager::clear();
}


///////////////////////////////////////////////////////////////////////////////


Schematic::Schematic()
= default;


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


void Schematic::blitToVManip(MMVManip *vm, v3s16 p, Rotation rot, bool force_place)
{
	sanity_check(m_ndef != NULL);

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
		if ((slice_probs[y] != MTSCHEM_PROB_ALWAYS) &&
			(slice_probs[y] <= myrand_range(1, MTSCHEM_PROB_ALWAYS)))
			continue;

		for (s16 z = 0; z != sz; z++) {
			u32 i = z * i_step_z + y * ystride + i_start;
			for (s16 x = 0; x != sx; x++, i += i_step_x) {
				v3s16 pos(p.X + x, y_map, p.Z + z);
				if (!vm->m_area.contains(pos))
					continue;

				if (schemdata[i].getContent() == CONTENT_IGNORE)
					continue;

				u8 placement_prob     = schemdata[i].param1 & MTSCHEM_PROB_MASK;
				bool force_place_node = schemdata[i].param1 & MTSCHEM_FORCE_PLACE;

				if (placement_prob == MTSCHEM_PROB_NEVER)
					continue;

				u32 vi = vm->m_area.index(pos);
				if (!force_place && !force_place_node) {
					content_t c = vm->m_data[vi].getContent();
					if (c != CONTENT_AIR && c != CONTENT_IGNORE)
						continue;
				}

				if ((placement_prob != MTSCHEM_PROB_ALWAYS) &&
					(placement_prob <= myrand_range(1, MTSCHEM_PROB_ALWAYS)))
					continue;

				vm->m_data[vi] = schemdata[i];
				vm->m_data[vi].param1 = 0;

				if (rot)
					vm->m_data[vi].rotateAlongYAxis(m_ndef, rot);
			}
		}
		y_map++;
	}
}


bool Schematic::placeOnVManip(MMVManip *vm, v3s16 p, u32 flags,
	Rotation rot, bool force_place)
{
	assert(vm != NULL);
	assert(schemdata != NULL);
	sanity_check(m_ndef != NULL);

	//// Determine effective rotation and effective schematic dimensions
	if (rot == ROTATE_RAND)
		rot = (Rotation)myrand_range(ROTATE_0, ROTATE_270);

	v3s16 s = (rot == ROTATE_90 || rot == ROTATE_270) ?
		v3s16(size.Z, size.Y, size.X) : size;

	//// Adjust placement position if necessary
	if (flags & DECO_PLACE_CENTER_X)
		p.X -= (s.X - 1) / 2;
	if (flags & DECO_PLACE_CENTER_Y)
		p.Y -= (s.Y - 1) / 2;
	if (flags & DECO_PLACE_CENTER_Z)
		p.Z -= (s.Z - 1) / 2;

	blitToVManip(vm, p, rot, force_place);

	return vm->m_area.contains(VoxelArea(p, p + s - v3s16(1, 1, 1)));
}

void Schematic::placeOnMap(ServerMap *map, v3s16 p, u32 flags,
	Rotation rot, bool force_place)
{
	std::map<v3s16, MapBlock *> lighting_modified_blocks;
	std::map<v3s16, MapBlock *> modified_blocks;
	std::map<v3s16, MapBlock *>::iterator it;

	assert(map != NULL);
	assert(schemdata != NULL);
	sanity_check(m_ndef != NULL);

	//// Determine effective rotation and effective schematic dimensions
	if (rot == ROTATE_RAND)
		rot = (Rotation)myrand_range(ROTATE_0, ROTATE_270);

	v3s16 s = (rot == ROTATE_90 || rot == ROTATE_270) ?
			v3s16(size.Z, size.Y, size.X) : size;

	//// Adjust placement position if necessary
	if (flags & DECO_PLACE_CENTER_X)
		p.X -= (s.X - 1) / 2;
	if (flags & DECO_PLACE_CENTER_Y)
		p.Y -= (s.Y - 1) / 2;
	if (flags & DECO_PLACE_CENTER_Z)
		p.Z -= (s.Z - 1) / 2;

	//// Create VManip for effected area, emerge our area, modify area
	//// inside VManip, then blit back.
	v3s16 bp1 = getNodeBlockPos(p);
	v3s16 bp2 = getNodeBlockPos(p + s - v3s16(1, 1, 1));

	MMVManip vm(map);
	vm.initialEmerge(bp1, bp2);

	blitToVManip(&vm, p, rot, force_place);

	voxalgo::blit_back_with_light(map, &vm, &modified_blocks);

	//// Carry out post-map-modification actions

	//// Create & dispatch map modification events to observers
	MapEditEvent event;
	event.type = MEET_OTHER;
	for (it = modified_blocks.begin(); it != modified_blocks.end(); ++it)
		event.modified_blocks.insert(it->first);

	map->dispatchEvent(event);
}


bool Schematic::deserializeFromMts(std::istream *is,
	std::vector<std::string> *names)
{
	std::istream &ss = *is;
	content_t cignore = CONTENT_IGNORE;
	bool have_cignore = false;

	//// Read signature
	u32 signature = readU32(ss);
	if (signature != MTSCHEM_FILE_SIGNATURE) {
		errorstream << __FUNCTION__ << ": invalid schematic "
			"file" << std::endl;
		return false;
	}

	//// Read version
	u16 version = readU16(ss);
	if (version > MTSCHEM_FILE_VER_HIGHEST_READ) {
		errorstream << __FUNCTION__ << ": unsupported schematic "
			"file version" << std::endl;
		return false;
	}

	//// Read size
	size = readV3S16(ss);

	//// Read Y-slice probability values
	delete []slice_probs;
	slice_probs = new u8[size.Y];
	for (int y = 0; y != size.Y; y++)
		slice_probs[y] = (version >= 3) ? readU8(ss) : MTSCHEM_PROB_ALWAYS_OLD;

	//// Read node names
	u16 nidmapcount = readU16(ss);
	for (int i = 0; i != nidmapcount; i++) {
		std::string name = deSerializeString(ss);

		// Instances of "ignore" from v1 are converted to air (and instances
		// are fixed to have MTSCHEM_PROB_NEVER later on).
		if (name == "ignore") {
			name = "air";
			cignore = i;
			have_cignore = true;
		}

		names->push_back(name);
	}

	//// Read node data
	size_t nodecount = size.X * size.Y * size.Z;

	delete []schemdata;
	schemdata = new MapNode[nodecount];

	MapNode::deSerializeBulk(ss, SER_FMT_VER_HIGHEST_READ, schemdata,
		nodecount, 2, 2, true);

	// Fix probability values for nodes that were ignore; removed in v2
	if (version < 2) {
		for (size_t i = 0; i != nodecount; i++) {
			if (schemdata[i].param1 == 0)
				schemdata[i].param1 = MTSCHEM_PROB_ALWAYS_OLD;
			if (have_cignore && schemdata[i].getContent() == cignore)
				schemdata[i].param1 = MTSCHEM_PROB_NEVER;
		}
	}

	// Fix probability values for probability range truncation introduced in v4
	if (version < 4) {
		for (s16 y = 0; y != size.Y; y++)
			slice_probs[y] >>= 1;
		for (size_t i = 0; i != nodecount; i++)
			schemdata[i].param1 >>= 1;
	}

	return true;
}


bool Schematic::serializeToMts(std::ostream *os,
	const std::vector<std::string> &names)
{
	std::ostream &ss = *os;

	writeU32(ss, MTSCHEM_FILE_SIGNATURE);         // signature
	writeU16(ss, MTSCHEM_FILE_VER_HIGHEST_WRITE); // version
	writeV3S16(ss, size);                         // schematic size

	for (int y = 0; y != size.Y; y++)             // Y slice probabilities
		writeU8(ss, slice_probs[y]);

	writeU16(ss, names.size()); // name count
	for (size_t i = 0; i != names.size(); i++)
		ss << serializeString(names[i]); // node names

	// compressed bulk node data
	MapNode::serializeBulk(ss, SER_FMT_VER_HIGHEST_WRITE,
		schemdata, size.X * size.Y * size.Z, 2, 2, true);

	return true;
}


bool Schematic::serializeToLua(std::ostream *os,
	const std::vector<std::string> &names, bool use_comments, u32 indent_spaces)
{
	std::ostream &ss = *os;

	std::string indent("\t");
	if (indent_spaces > 0)
		indent.assign(indent_spaces, ' ');

	//// Write header
	{
		ss << "schematic = {" << std::endl;
		ss << indent << "size = "
			<< "{x=" << size.X
			<< ", y=" << size.Y
			<< ", z=" << size.Z
			<< "}," << std::endl;
	}

	//// Write y-slice probabilities
	{
		ss << indent << "yslice_prob = {" << std::endl;

		for (u16 y = 0; y != size.Y; y++) {
			u8 probability = slice_probs[y] & MTSCHEM_PROB_MASK;

			ss << indent << indent << "{"
				<< "ypos=" << y
				<< ", prob=" << (u16)probability * 2
				<< "}," << std::endl;
		}

		ss << indent << "}," << std::endl;
	}

	//// Write node data
	{
		ss << indent << "data = {" << std::endl;

		u32 i = 0;
		for (u16 z = 0; z != size.Z; z++)
		for (u16 y = 0; y != size.Y; y++) {
			if (use_comments) {
				ss << std::endl
					<< indent << indent
					<< "-- z=" << z
					<< ", y=" << y << std::endl;
			}

			for (u16 x = 0; x != size.X; x++, i++) {
				u8 probability   = schemdata[i].param1 & MTSCHEM_PROB_MASK;
				bool force_place = schemdata[i].param1 & MTSCHEM_FORCE_PLACE;

				ss << indent << indent << "{"
					<< "name=\"" << names[schemdata[i].getContent()]
					<< "\", prob=" << (u16)probability * 2
					<< ", param2=" << (u16)schemdata[i].param2;

				if (force_place)
					ss << ", force_place=true";

				ss << "}," << std::endl;
			}
		}

		ss << indent << "}," << std::endl;
	}

	ss << "}" << std::endl;

	return true;
}


bool Schematic::loadSchematicFromFile(const std::string &filename,
	const NodeDefManager *ndef, StringMap *replace_names)
{
	std::ifstream is(filename.c_str(), std::ios_base::binary);
	if (!is.good()) {
		errorstream << __FUNCTION__ << ": unable to open file '"
			<< filename << "'" << std::endl;
		return false;
	}

	size_t origsize = m_nodenames.size();
	if (!deserializeFromMts(&is, &m_nodenames))
		return false;

	m_nnlistsizes.push_back(m_nodenames.size() - origsize);

	name = filename;

	if (replace_names) {
		for (size_t i = origsize; i < m_nodenames.size(); i++) {
			std::string &node_name = m_nodenames[i];
			StringMap::iterator it = replace_names->find(node_name);
			if (it != replace_names->end())
				node_name = it->second;
		}
	}

	if (ndef)
		ndef->pendNodeResolve(this);

	return true;
}


bool Schematic::saveSchematicToFile(const std::string &filename,
	const NodeDefManager *ndef)
{
	MapNode *orig_schemdata = schemdata;
	std::vector<std::string> ndef_nodenames;
	std::vector<std::string> *names;

	if (m_resolve_done && ndef == NULL)
		ndef = m_ndef;

	if (ndef) {
		names = &ndef_nodenames;

		u32 volume = size.X * size.Y * size.Z;
		schemdata = new MapNode[volume];
		for (u32 i = 0; i != volume; i++)
			schemdata[i] = orig_schemdata[i];

		generate_nodelist_and_update_ids(schemdata, volume, names, ndef);
	} else { // otherwise, use the names we have on hand in the list
		names = &m_nodenames;
	}

	std::ostringstream os(std::ios_base::binary);
	bool status = serializeToMts(&os, *names);

	if (ndef) {
		delete []schemdata;
		schemdata = orig_schemdata;
	}

	if (!status)
		return false;

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


void generate_nodelist_and_update_ids(MapNode *nodes, size_t nodecount,
	std::vector<std::string> *usednodes, const NodeDefManager *ndef)
{
	std::unordered_map<content_t, content_t> nodeidmap;
	content_t numids = 0;

	for (size_t i = 0; i != nodecount; i++) {
		content_t id;
		content_t c = nodes[i].getContent();

		std::unordered_map<content_t, content_t>::const_iterator it = nodeidmap.find(c);
		if (it == nodeidmap.end()) {
			id = numids;
			numids++;

			usednodes->push_back(ndef->get(c).name);
			nodeidmap.insert(std::make_pair(c, id));
		} else {
			id = it->second;
		}
		nodes[i].setContent(id);
	}
}
