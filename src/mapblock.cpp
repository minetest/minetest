/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "mapblock.h"

#include <sstream>
#include "map.h"
#include "light.h"
#include "nodedef.h"
#include "nodemetadata.h"
#include "gamedef.h"
#include "log.h"
#include "nameidmapping.h"
#include "content_mapnode.h"  // For legacy name-id mapping
#include "content_nodemeta.h" // For legacy deserialization
#include "serialization.h"
#ifndef SERVER
#include "client/mapblock_mesh.h"
#endif
#include "porting.h"
#include "util/string.h"
#include "util/serialize.h"
#include "util/basic_macros.h"

static const char *modified_reason_strings[] = {
	"initial",
	"reallocate",
	"setIsUnderground",
	"setLightingExpired",
	"setGenerated",
	"setNode",
	"setNodeNoCheck",
	"setTimestamp",
	"NodeMetaRef::reportMetadataChange",
	"clearAllObjects",
	"Timestamp expired (step)",
	"addActiveObjectRaw",
	"removeRemovedObjects/remove",
	"removeRemovedObjects/deactivate",
	"Stored list cleared in activateObjects due to overflow",
	"deactivateFarObjects: Static data moved in",
	"deactivateFarObjects: Static data moved out",
	"deactivateFarObjects: Static data changed considerably",
	"finishBlockMake: expireDayNightDiff",
	"unknown",
};

/*
	MapBlock
*/

MapBlock::MapBlock(Map *parent, v3s16 pos, IGameDef *gamedef):
		m_parent(parent),
		m_pos(pos),
		m_pos_relative(pos * MAP_BLOCKSIZE),
		m_gamedef(gamedef)
{
	reallocate();
}

MapBlock::~MapBlock()
{
#ifndef SERVER
	{
		delete mesh;
		mesh = nullptr;
	}
#endif
}

bool MapBlock::onObjectsActivation()
{
	// Ignore if no stored objects (to not set changed flag)
	if (m_static_objects.getAllStored().empty())
		return false;

	verbosestream << "MapBlock::onObjectsActivation(): "
			<< "activating objects of block " << PP(getPos()) << " ("
			<< m_static_objects.getStoredSize() << " objects)" << std::endl;

	if (m_static_objects.getStoredSize() > g_settings->getU16("max_objects_per_block")) {
		errorstream << "suspiciously large amount of objects detected: "
			<< m_static_objects.getStoredSize() << " in "
			<< PP(getPos()) << "; removing all of them." << std::endl;
		// Clear stored list
		m_static_objects.clearStored();
		raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_TOO_MANY_OBJECTS);
		return false;
	}

	return true;
}

bool MapBlock::saveStaticObject(u16 id, const StaticObject &obj, u32 reason)
{
	if (m_static_objects.getStoredSize() >= g_settings->getU16("max_objects_per_block")) {
		warningstream << "MapBlock::saveStaticObject(): Trying to store id = " << id
				<< " statically but block " << PP(getPos()) << " already contains "
				<< m_static_objects.getStoredSize() << " objects."
				<< std::endl;
		return false;
	}

	m_static_objects.insert(id, obj);
	if (reason != MOD_REASON_UNKNOWN) // Do not mark as modified if requested
		raiseModified(MOD_STATE_WRITE_NEEDED, reason);

	return true;
}

// This method is only for Server, don't call it on client
void MapBlock::step(float dtime, const std::function<bool(v3s16, MapNode, f32)> &on_timer_cb)
{
	// Run script callbacks for elapsed node_timers
	std::vector<NodeTimer> elapsed_timers = m_node_timers.step(dtime);
	if (!elapsed_timers.empty()) {
		MapNode n;
		v3s16 p;
		for (const NodeTimer &elapsed_timer : elapsed_timers) {
			n = getNodeNoEx(elapsed_timer.position);
			p = elapsed_timer.position + getPosRelative();
			if (on_timer_cb(p, n, elapsed_timer.elapsed))
				setNodeTimer(NodeTimer(elapsed_timer.timeout, 0, elapsed_timer.position));
		}
	}
}

bool MapBlock::isValidPositionParent(v3s16 p)
{
	if (isValidPosition(p)) {
		return true;
	}

	return m_parent->isValidPosition(getPosRelative() + p);
}

MapNode MapBlock::getNodeParent(v3s16 p, bool *is_valid_position)
{
	if (!isValidPosition(p))
		return m_parent->getNode(getPosRelative() + p, is_valid_position);

	if (is_valid_position)
		*is_valid_position = true;
	return data[p.Z * zstride + p.Y * ystride + p.X];
}

std::string MapBlock::getModifiedReasonString()
{
	std::string reason;

	const u32 ubound = MYMIN(sizeof(m_modified_reason) * CHAR_BIT,
		ARRLEN(modified_reason_strings));

	for (u32 i = 0; i != ubound; i++) {
		if ((m_modified_reason & (1 << i)) == 0)
			continue;

		reason += modified_reason_strings[i];
		reason += ", ";
	}

	if (reason.length() > 2)
		reason.resize(reason.length() - 2);

	return reason;
}


void MapBlock::copyTo(VoxelManipulator &dst)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));

	// Copy from data to VoxelManipulator
	dst.copyFrom(data, data_area, v3s16(0,0,0),
			getPosRelative(), data_size);
}

void MapBlock::copyFrom(VoxelManipulator &dst)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));

	// Copy from VoxelManipulator to data
	dst.copyTo(data, data_area, v3s16(0,0,0),
			getPosRelative(), data_size);
}

void MapBlock::actuallyUpdateDayNightDiff()
{
	const NodeDefManager *nodemgr = m_gamedef->ndef();

	// Running this function un-expires m_day_night_differs
	m_day_night_differs_expired = false;

	bool differs = false;

	/*
		Check if any lighting value differs
	*/

	MapNode previous_n(CONTENT_IGNORE);
	for (u32 i = 0; i < nodecount; i++) {
		MapNode n = data[i];

		// If node is identical to previous node, don't verify if it differs
		if (n == previous_n)
			continue;

		differs = !n.isLightDayNightEq(nodemgr->getLightingFlags(n));
		if (differs)
			break;
		previous_n = n;
	}

	/*
		If some lighting values differ, check if the whole thing is
		just air. If it is just air, differs = false
	*/
	if (differs) {
		bool only_air = true;
		for (u32 i = 0; i < nodecount; i++) {
			MapNode &n = data[i];
			if (n.getContent() != CONTENT_AIR) {
				only_air = false;
				break;
			}
		}
		if (only_air)
			differs = false;
	}

	// Set member variable
	m_day_night_differs = differs;
}

void MapBlock::expireDayNightDiff()
{
	m_day_night_differs_expired = true;
}

/*
	Serialization
*/

// List relevant id-name pairs for ids in the block using nodedef
// Renumbers the content IDs (starting at 0 and incrementing)
static void getBlockNodeIdMapping(NameIdMapping *nimap, MapNode *nodes,
	const NodeDefManager *nodedef)
{
	// The static memory requires about 65535 * sizeof(int) RAM in order to be
	// sure we can handle all content ids. But it's absolutely worth it as it's
	// a speedup of 4 for one of the major time consuming functions on storing
	// mapblocks.
	thread_local std::unique_ptr<content_t[]> mapping;
	static_assert(sizeof(content_t) == 2, "content_t must be 16-bit");
	if (!mapping)
		mapping = std::make_unique<content_t[]>(USHRT_MAX + 1);

	memset(mapping.get(), 0xFF, (USHRT_MAX + 1) * sizeof(content_t));

	std::unordered_set<content_t> unknown_contents;
	content_t id_counter = 0;
	for (u32 i = 0; i < MapBlock::nodecount; i++) {
		content_t global_id = nodes[i].getContent();
		content_t id = CONTENT_IGNORE;

		// Try to find an existing mapping
		if (mapping[global_id] != 0xFFFF) {
			id = mapping[global_id];
		} else {
			// We have to assign a new mapping
			id = id_counter++;
			mapping[global_id] = id;

			const ContentFeatures &f = nodedef->get(global_id);
			const std::string &name = f.name;
			if (name.empty())
				unknown_contents.insert(global_id);
			else
				nimap->set(id, name);
		}

		// Update the MapNode
		nodes[i].setContent(id);
	}
	for (u16 unknown_content : unknown_contents) {
		errorstream << "getBlockNodeIdMapping(): IGNORING ERROR: "
				<< "Name for node id " << unknown_content << " not known" << std::endl;
	}
}

// Correct ids in the block to match nodedef based on names.
// Unknown ones are added to nodedef.
// Will not update itself to match id-name pairs in nodedef.
static void correctBlockNodeIds(const NameIdMapping *nimap, MapNode *nodes,
		IGameDef *gamedef)
{
	const NodeDefManager *nodedef = gamedef->ndef();
	// This means the block contains incorrect ids, and we contain
	// the information to convert those to names.
	// nodedef contains information to convert our names to globally
	// correct ids.
	std::unordered_set<content_t> unnamed_contents;
	std::unordered_set<std::string> unallocatable_contents;

	bool previous_exists = false;
	content_t previous_local_id = CONTENT_IGNORE;
	content_t previous_global_id = CONTENT_IGNORE;

	for (u32 i = 0; i < MapBlock::nodecount; i++) {
		content_t local_id = nodes[i].getContent();
		// If previous node local_id was found and same than before, don't lookup maps
		// apply directly previous resolved id
		// This permits to massively improve loading performance when nodes are similar
		// example: default:air, default:stone are massively present
		if (previous_exists && local_id == previous_local_id) {
			nodes[i].setContent(previous_global_id);
			continue;
		}

		std::string name;
		if (!nimap->getName(local_id, name)) {
			unnamed_contents.insert(local_id);
			previous_exists = false;
			continue;
		}

		content_t global_id;
		if (!nodedef->getId(name, global_id)) {
			global_id = gamedef->allocateUnknownNodeId(name);
			if (global_id == CONTENT_IGNORE) {
				unallocatable_contents.insert(name);
				previous_exists = false;
				continue;
			}
		}
		nodes[i].setContent(global_id);

		// Save previous node local_id & global_id result
		previous_local_id = local_id;
		previous_global_id = global_id;
		previous_exists = true;
	}

	for (const content_t c: unnamed_contents) {
		errorstream << "correctBlockNodeIds(): IGNORING ERROR: "
				<< "Block contains id " << c
				<< " with no name mapping" << std::endl;
	}
	for (const std::string &node_name: unallocatable_contents) {
		errorstream << "correctBlockNodeIds(): IGNORING ERROR: "
				<< "Could not allocate global id for node name \""
				<< node_name << "\"" << std::endl;
	}
}

void MapBlock::serialize(std::ostream &os_compressed, u8 version, bool disk, int compression_level)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");

	FATAL_ERROR_IF(version < SER_FMT_VER_LOWEST_WRITE, "Serialization version error");

	std::ostringstream os_raw(std::ios_base::binary);
	std::ostream &os = version >= 29 ? os_raw : os_compressed;

	// First byte
	u8 flags = 0;
	if(is_underground)
		flags |= 0x01;
	if(getDayNightDiff())
		flags |= 0x02;
	if (!m_generated)
		flags |= 0x08;
	writeU8(os, flags);
	if (version >= 27) {
		writeU16(os, m_lighting_complete);
	}

	/*
		Bulk node data
	*/
	NameIdMapping nimap;
	SharedBuffer<u8> buf;
	const u8 content_width = 2;
	const u8 params_width = 2;
 	if(disk)
	{
		MapNode *tmp_nodes = new MapNode[nodecount];
		memcpy(tmp_nodes, data, nodecount * sizeof(MapNode));
		getBlockNodeIdMapping(&nimap, tmp_nodes, m_gamedef->ndef());

		buf = MapNode::serializeBulk(version, tmp_nodes, nodecount,
				content_width, params_width);
		delete[] tmp_nodes;

		// write timestamp and node/id mapping first
		if (version >= 29) {
			writeU32(os, getTimestamp());

			nimap.serialize(os);
		}
	}
	else
	{
		buf = MapNode::serializeBulk(version, data, nodecount,
				content_width, params_width);
	}

	writeU8(os, content_width);
	writeU8(os, params_width);
	if (version >= 29) {
		os.write(reinterpret_cast<char*>(*buf), buf.getSize());
	} else {
		// prior to 29 node data was compressed individually
		compress(buf, os, version, compression_level);
	}

	/*
		Node metadata
	*/
	if (version >= 29) {
		m_node_metadata.serialize(os, version, disk);
	} else {
		// use os_raw from above to avoid allocating another stream object
		m_node_metadata.serialize(os_raw, version, disk);
		// prior to 29 node data was compressed individually
		compress(os_raw.str(), os, version, compression_level);
	}

	/*
		Data that goes to disk, but not the network
	*/
	if (disk) {
		if (version <= 24) {
			// Node timers
			m_node_timers.serialize(os, version);
		}

		// Static objects
		m_static_objects.serialize(os);

		if (version < 29) {
			// Timestamp
			writeU32(os, getTimestamp());

			// Write block-specific node definition id mapping
			nimap.serialize(os);
		}

		if (version >= 25) {
			// Node timers
			m_node_timers.serialize(os, version);
		}
	}

	if (version >= 29) {
		// now compress the whole thing
		compress(os_raw.str(), os_compressed, version, compression_level);
	}
}

void MapBlock::serializeNetworkSpecific(std::ostream &os)
{
	writeU8(os, 2); // version
}

void MapBlock::deSerialize(std::istream &in_compressed, u8 version, bool disk)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");

	TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())<<std::endl);

	m_day_night_differs_expired = false;

	if(version <= 21)
	{
		deSerialize_pre22(in_compressed, version, disk);
		return;
	}

	// Decompress the whole block (version >= 29)
	std::stringstream in_raw(std::ios_base::binary | std::ios_base::in | std::ios_base::out);
	if (version >= 29)
		decompress(in_compressed, in_raw, version);
	std::istream &is = version >= 29 ? in_raw : in_compressed;

	u8 flags = readU8(is);
	is_underground = (flags & 0x01) != 0;
	m_day_night_differs = (flags & 0x02) != 0;
	if (version < 27)
		m_lighting_complete = 0xFFFF;
	else
		m_lighting_complete = readU16(is);
	m_generated = (flags & 0x08) == 0;

	NameIdMapping nimap;
	if (disk && version >= 29) {
		// Timestamp
		TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
				<<": Timestamp"<<std::endl);
		setTimestampNoChangedFlag(readU32(is));
		m_disk_timestamp = m_timestamp;

		// Node/id mapping
		TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
				<<": NameIdMapping"<<std::endl);
		nimap.deSerialize(is);
	}

	TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
			<<": Bulk node data"<<std::endl);
	u8 content_width = readU8(is);
	u8 params_width = readU8(is);
	if(content_width != 1 && content_width != 2)
		throw SerializationError("MapBlock::deSerialize(): invalid content_width");
	if(params_width != 2)
		throw SerializationError("MapBlock::deSerialize(): invalid params_width");

	/*
		Bulk node data
	*/
	if (version >= 29) {
		MapNode::deSerializeBulk(is, version, data, nodecount,
			content_width, params_width);
	} else {
		// use in_raw from above to avoid allocating another stream object
		decompress(is, in_raw, version);
		MapNode::deSerializeBulk(in_raw, version, data, nodecount,
			content_width, params_width);
	}

	/*
		NodeMetadata
	*/
	TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
			<<": Node metadata"<<std::endl);
	if (version >= 29) {
		m_node_metadata.deSerialize(is, m_gamedef->idef());
	} else {
		try {
			// reuse in_raw
			in_raw.str("");
			in_raw.clear();
			decompress(is, in_raw, version);
			if (version >= 23)
				m_node_metadata.deSerialize(in_raw, m_gamedef->idef());
			else
				content_nodemeta_deserialize_legacy(in_raw,
					&m_node_metadata, &m_node_timers,
					m_gamedef->idef());
		} catch(SerializationError &e) {
			warningstream<<"MapBlock::deSerialize(): Ignoring an error"
					<<" while deserializing node metadata at ("
					<<PP(getPos())<<": "<<e.what()<<std::endl;
		}
	}

	/*
		Data that is only on disk
	*/
	if (disk) {
		// Node timers
		if (version == 23) {
			// Read unused zero
			readU8(is);
		}
		if (version == 24) {
			TRACESTREAM(<< "MapBlock::deSerialize " << PP(getPos())
						<< ": Node timers (ver==24)" << std::endl);
			m_node_timers.deSerialize(is, version);
		}

		// Static objects
		TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
				<<": Static objects"<<std::endl);
		m_static_objects.deSerialize(is);

		if (version < 29) {
			// Timestamp
			TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
				    <<": Timestamp"<<std::endl);
			setTimestampNoChangedFlag(readU32(is));
			m_disk_timestamp = m_timestamp;

			// Node/id mapping
			TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
				    <<": NameIdMapping"<<std::endl);
			nimap.deSerialize(is);
		}

		// Dynamically re-set ids based on node names
		correctBlockNodeIds(&nimap, data, m_gamedef);

		if(version >= 25){
			TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
					<<": Node timers (ver>=25)"<<std::endl);
			m_node_timers.deSerialize(is, version);
		}
	}

	TRACESTREAM(<<"MapBlock::deSerialize "<<PP(getPos())
			<<": Done."<<std::endl);
}

void MapBlock::deSerializeNetworkSpecific(std::istream &is)
{
	try {
		readU8(is);
		//const u8 version = readU8(is);
		//if (version != 1)
			//throw SerializationError("unsupported MapBlock version");

	} catch(SerializationError &e) {
		warningstream<<"MapBlock::deSerializeNetworkSpecific(): Ignoring an error"
				<<": "<<e.what()<<std::endl;
	}
}

bool MapBlock::storeActiveObject(u16 id)
{
	if (m_static_objects.storeActiveObject(id)) {
		raiseModified(MOD_STATE_WRITE_NEEDED,
			MOD_REASON_REMOVE_OBJECTS_DEACTIVATE);
		return true;
	}

	return false;
}

u32 MapBlock::clearObjects()
{
	u32 size = m_static_objects.size();
	if (size > 0) {
		m_static_objects.clear();
		raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_CLEAR_ALL_OBJECTS);
	}
	return size;
}
/*
	Legacy serialization
*/

void MapBlock::deSerialize_pre22(std::istream &is, u8 version, bool disk)
{
	// Initialize default flags
	is_underground = false;
	m_day_night_differs = false;
	m_lighting_complete = 0xFFFF;
	m_generated = true;

	// Make a temporary buffer
	u32 ser_length = MapNode::serializedLength(version);
	SharedBuffer<u8> databuf_nodelist(nodecount * ser_length);

	// These have no compression
	if (version <= 3 || version == 5 || version == 6) {
		char tmp;
		is.read(&tmp, 1);
		if (is.gcount() != 1)
			throw SerializationError(std::string(FUNCTION_NAME)
				+ ": not enough input data");
		is_underground = tmp;
		is.read((char *)*databuf_nodelist, nodecount * ser_length);
		if ((u32)is.gcount() != nodecount * ser_length)
			throw SerializationError(std::string(FUNCTION_NAME)
				+ ": not enough input data");
	} else if (version <= 10) {
		u8 t8;
		is.read((char *)&t8, 1);
		is_underground = t8;

		{
			// Uncompress and set material data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if (s.size() != nodecount)
				throw SerializationError(std::string(FUNCTION_NAME)
					+ ": not enough input data");
			for (u32 i = 0; i < s.size(); i++) {
				databuf_nodelist[i*ser_length] = s[i];
			}
		}
		{
			// Uncompress and set param data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if (s.size() != nodecount)
				throw SerializationError(std::string(FUNCTION_NAME)
					+ ": not enough input data");
			for (u32 i = 0; i < s.size(); i++) {
				databuf_nodelist[i*ser_length + 1] = s[i];
			}
		}

		if (version >= 10) {
			// Uncompress and set param2 data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if (s.size() != nodecount)
				throw SerializationError(std::string(FUNCTION_NAME)
					+ ": not enough input data");
			for (u32 i = 0; i < s.size(); i++) {
				databuf_nodelist[i*ser_length + 2] = s[i];
			}
		}
	} else { // All other versions (10 to 21)
		u8 flags;
		is.read((char*)&flags, 1);
		is_underground = (flags & 0x01) != 0;
		m_day_night_differs = (flags & 0x02) != 0;
		if (version >= 18)
			m_generated = (flags & 0x08) == 0;

		// Uncompress data
		std::ostringstream os(std::ios_base::binary);
		decompress(is, os, version);
		std::string s = os.str();
		if (s.size() != nodecount * 3)
			throw SerializationError(std::string(FUNCTION_NAME)
				+ ": decompress resulted in size other than nodecount*3");

		// deserialize nodes from buffer
		for (u32 i = 0; i < nodecount; i++) {
			databuf_nodelist[i*ser_length] = s[i];
			databuf_nodelist[i*ser_length + 1] = s[i+nodecount];
			databuf_nodelist[i*ser_length + 2] = s[i+nodecount*2];
		}

		/*
			NodeMetadata
		*/
		if (version >= 14) {
			// Ignore errors
			try {
				if (version <= 15) {
					std::string data = deSerializeString16(is);
					std::istringstream iss(data, std::ios_base::binary);
					content_nodemeta_deserialize_legacy(iss,
						&m_node_metadata, &m_node_timers,
						m_gamedef->idef());
				} else {
					//std::string data = deSerializeString32(is);
					std::ostringstream oss(std::ios_base::binary);
					decompressZlib(is, oss);
					std::istringstream iss(oss.str(), std::ios_base::binary);
					content_nodemeta_deserialize_legacy(iss,
						&m_node_metadata, &m_node_timers,
						m_gamedef->idef());
				}
			} catch(SerializationError &e) {
				warningstream<<"MapBlock::deSerialize(): Ignoring an error"
						<<" while deserializing node metadata"<<std::endl;
			}
		}
	}

	// Deserialize node data
	for (u32 i = 0; i < nodecount; i++) {
		data[i].deSerialize(&databuf_nodelist[i * ser_length], version);
	}

	if (disk) {
		/*
			Versions up from 9 have block objects. (DEPRECATED)
		*/
		if (version >= 9) {
			u16 count = readU16(is);
			// Not supported and length not known if count is not 0
			if(count != 0){
				warningstream<<"MapBlock::deSerialize_pre22(): "
						<<"Ignoring stuff coming at and after MBOs"<<std::endl;
				return;
			}
		}

		/*
			Versions up from 15 have static objects.
		*/
		if (version >= 15)
			m_static_objects.deSerialize(is);

		// Timestamp
		if (version >= 17) {
			setTimestampNoChangedFlag(readU32(is));
			m_disk_timestamp = m_timestamp;
		} else {
			setTimestampNoChangedFlag(BLOCK_TIMESTAMP_UNDEFINED);
		}

		// Dynamically re-set ids based on node names
		NameIdMapping nimap;
		// If supported, read node definition id mapping
		if (version >= 21) {
			nimap.deSerialize(is);
		// Else set the legacy mapping
		} else {
			content_mapnode_get_name_id_mapping(&nimap);
		}
		correctBlockNodeIds(&nimap, data, m_gamedef);
	}

	// Legacy data changes
	// This code has to convert from pre-22 to post-22 format.
	const NodeDefManager *nodedef = m_gamedef->ndef();
	for (u32 i = 0; i < nodecount; i++) {
		const ContentFeatures &f = nodedef->get(data[i].getContent());
		// Mineral
		if(nodedef->getId("default:stone") == data[i].getContent()
				&& data[i].getParam1() == 1)
		{
			data[i].setContent(nodedef->getId("default:stone_with_coal"));
			data[i].setParam1(0);
		}
		else if(nodedef->getId("default:stone") == data[i].getContent()
				&& data[i].getParam1() == 2)
		{
			data[i].setContent(nodedef->getId("default:stone_with_iron"));
			data[i].setParam1(0);
		}
		// facedir_simple
		if (f.legacy_facedir_simple) {
			data[i].setParam2(data[i].getParam1());
			data[i].setParam1(0);
		}
		// wall_mounted
		if (f.legacy_wallmounted) {
			u8 wallmounted_new_to_old[8] = {0x04, 0x08, 0x01, 0x02, 0x10, 0x20, 0, 0};
			u8 dir_old_format = data[i].getParam2();
			u8 dir_new_format = 0;
			for (u8 j = 0; j < 8; j++) {
				if ((dir_old_format & wallmounted_new_to_old[j]) != 0) {
					dir_new_format = j;
					break;
				}
			}
			data[i].setParam2(dir_new_format);
		}
	}
}

/*
	Get a quick string to describe what a block actually contains
*/
std::string analyze_block(MapBlock *block)
{
	if (block == NULL)
		return "NULL";

	std::ostringstream desc;

	v3s16 p = block->getPos();
	char spos[25];
	porting::mt_snprintf(spos, sizeof(spos), "(%2d,%2d,%2d), ", p.X, p.Y, p.Z);
	desc<<spos;

	switch(block->getModified())
	{
	case MOD_STATE_CLEAN:
		desc<<"CLEAN,           ";
		break;
	case MOD_STATE_WRITE_AT_UNLOAD:
		desc<<"WRITE_AT_UNLOAD, ";
		break;
	case MOD_STATE_WRITE_NEEDED:
		desc<<"WRITE_NEEDED,    ";
		break;
	default:
		desc<<"unknown getModified()="+itos(block->getModified())+", ";
	}

	if(block->isGenerated())
		desc<<"is_gen [X], ";
	else
		desc<<"is_gen [ ], ";

	if(block->getIsUnderground())
		desc<<"is_ug [X], ";
	else
		desc<<"is_ug [ ], ";

	desc<<"lighting_complete: "<<block->getLightingComplete()<<", ";

	bool full_ignore = true;
	bool some_ignore = false;
	bool full_air = true;
	bool some_air = false;
	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		v3s16 p(x0,y0,z0);
		MapNode n = block->getNodeNoEx(p);
		content_t c = n.getContent();
		if(c == CONTENT_IGNORE)
			some_ignore = true;
		else
			full_ignore = false;
		if(c == CONTENT_AIR)
			some_air = true;
		else
			full_air = false;
	}

	desc<<"content {";

	std::ostringstream ss;

	if(full_ignore)
		ss<<"IGNORE (full), ";
	else if(some_ignore)
		ss<<"IGNORE, ";

	if(full_air)
		ss<<"AIR (full), ";
	else if(some_air)
		ss<<"AIR, ";

	if(ss.str().size()>=2)
		desc<<ss.str().substr(0, ss.str().size()-2);

	desc<<"}, ";

	return desc.str().substr(0, desc.str().size()-2);
}


//END
