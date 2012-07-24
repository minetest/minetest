/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "irrlichttypes_extrabloated.h"
#include "mapnode.h"
#include "porting.h"
#include "main.h" // For g_settings
#include "nodedef.h"
#include "content_mapnode.h" // For mapnode_translate_*_internal
#include "serialization.h" // For ser_ver_supported
#include "util/serialize.h"
#include <string>
#include <sstream>

/*
	MapNode
*/

// Create directly from a nodename
// If name is unknown, sets CONTENT_IGNORE
MapNode::MapNode(INodeDefManager *ndef, const std::string &name,
		u8 a_param1, u8 a_param2)
{
	content_t id = CONTENT_IGNORE;
	ndef->getId(name, id);
	param0 = id;
	param1 = a_param1;
	param2 = a_param2;
}

void MapNode::setLight(enum LightBank bank, u8 a_light, INodeDefManager *nodemgr)
{
	// If node doesn't contain light data, ignore this
	if(nodemgr->get(*this).param_type != CPT_LIGHT)
		return;
	if(bank == LIGHTBANK_DAY)
	{
		param1 &= 0xf0;
		param1 |= a_light & 0x0f;
	}
	else if(bank == LIGHTBANK_NIGHT)
	{
		param1 &= 0x0f;
		param1 |= (a_light & 0x0f)<<4;
	}
	else
		assert(0);
}

u8 MapNode::getLight(enum LightBank bank, INodeDefManager *nodemgr) const
{
	// Select the brightest of [light source, propagated light]
	const ContentFeatures &f = nodemgr->get(*this);
	u8 light = 0;
	if(f.param_type == CPT_LIGHT)
	{
		if(bank == LIGHTBANK_DAY)
			light = param1 & 0x0f;
		else if(bank == LIGHTBANK_NIGHT)
			light = (param1>>4)&0x0f;
		else
			assert(0);
	}
	if(f.light_source > light)
		light = f.light_source;
	return light;
}

bool MapNode::getLightBanks(u8 &lightday, u8 &lightnight, INodeDefManager *nodemgr) const
{
	// Select the brightest of [light source, propagated light]
	const ContentFeatures &f = nodemgr->get(*this);
	if(f.param_type == CPT_LIGHT)
	{
		lightday = param1 & 0x0f;
		lightnight = (param1>>4)&0x0f;
	}
	else
	{
		lightday = 0;
		lightnight = 0;
	}
	if(f.light_source > lightday)
		lightday = f.light_source;
	if(f.light_source > lightnight)
		lightnight = f.light_source;
	return f.param_type == CPT_LIGHT || f.light_source != 0;
}

u8 MapNode::getFaceDir(INodeDefManager *nodemgr) const
{
	const ContentFeatures &f = nodemgr->get(*this);
	if(f.param_type_2 == CPT2_FACEDIR)
		return getParam2() & 0x03;
	return 0;
}

u8 MapNode::getWallMounted(INodeDefManager *nodemgr) const
{
	const ContentFeatures &f = nodemgr->get(*this);
	if(f.param_type_2 == CPT2_WALLMOUNTED)
		return getParam2() & 0x07;
	return 0;
}

v3s16 MapNode::getWallMountedDir(INodeDefManager *nodemgr) const
{
	switch(getWallMounted(nodemgr))
	{
	case 0: default: return v3s16(0,1,0);
	case 1: return v3s16(0,-1,0);
	case 2: return v3s16(1,0,0);
	case 3: return v3s16(-1,0,0);
	case 4: return v3s16(0,0,1);
	case 5: return v3s16(0,0,-1);
	}
}

static std::vector<aabb3f> transformNodeBox(const MapNode &n,
		const NodeBox &nodebox, INodeDefManager *nodemgr)
{
	std::vector<aabb3f> boxes;
	if(nodebox.type == NODEBOX_FIXED)
	{
		const std::vector<aabb3f> &fixed = nodebox.fixed;
		int facedir = n.getFaceDir(nodemgr);
		for(std::vector<aabb3f>::const_iterator
				i = fixed.begin();
				i != fixed.end(); i++)
		{
			aabb3f box = *i;
			if(facedir == 1)
			{
				box.MinEdge.rotateXZBy(-90);
				box.MaxEdge.rotateXZBy(-90);
				box.repair();
			}
			else if(facedir == 2)
			{
				box.MinEdge.rotateXZBy(180);
				box.MaxEdge.rotateXZBy(180);
				box.repair();
			}
			else if(facedir == 3)
			{
				box.MinEdge.rotateXZBy(90);
				box.MaxEdge.rotateXZBy(90);
				box.repair();
			}
			boxes.push_back(box);
		}
	}
	else if(nodebox.type == NODEBOX_WALLMOUNTED)
	{
		v3s16 dir = n.getWallMountedDir(nodemgr);

		// top
		if(dir == v3s16(0,1,0))
		{
			boxes.push_back(nodebox.wall_top);
		}
		// bottom
		else if(dir == v3s16(0,-1,0))
		{
			boxes.push_back(nodebox.wall_bottom);
		}
		// side
		else
		{
			v3f vertices[2] =
			{
				nodebox.wall_side.MinEdge,
				nodebox.wall_side.MaxEdge
			};

			for(s32 i=0; i<2; i++)
			{
				if(dir == v3s16(-1,0,0))
					vertices[i].rotateXZBy(0);
				if(dir == v3s16(1,0,0))
					vertices[i].rotateXZBy(180);
				if(dir == v3s16(0,0,-1))
					vertices[i].rotateXZBy(90);
				if(dir == v3s16(0,0,1))
					vertices[i].rotateXZBy(-90);
			}

			aabb3f box = aabb3f(vertices[0]);
			box.addInternalPoint(vertices[1]);
			boxes.push_back(box);
		}
	}
	else // NODEBOX_REGULAR
	{
		boxes.push_back(aabb3f(-BS/2,-BS/2,-BS/2,BS/2,BS/2,BS/2));
	}
	return boxes;
}

std::vector<aabb3f> MapNode::getNodeBoxes(INodeDefManager *nodemgr) const
{
	const ContentFeatures &f = nodemgr->get(*this);
	return transformNodeBox(*this, f.node_box, nodemgr);
}

std::vector<aabb3f> MapNode::getSelectionBoxes(INodeDefManager *nodemgr) const
{
	const ContentFeatures &f = nodemgr->get(*this);
	return transformNodeBox(*this, f.selection_box, nodemgr);
}

u32 MapNode::serializedLength(u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");
		
	if(version == 0)
		return 1;
	else if(version <= 9)
		return 2;
	else if(version <= 23)
		return 3;
	else
		return 4;
}
void MapNode::serialize(u8 *dest, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");
	
	// Can't do this anymore; we have 16-bit dynamically allocated node IDs
	// in memory; conversion just won't work in this direction.
	if(version < 24)
		throw SerializationError("MapNode::serialize: serialization to "
				"version < 24 not possible");
		
	writeU16(dest+0, param0);
	writeU8(dest+2, param1);
	writeU8(dest+3, param2);
}
void MapNode::deSerialize(u8 *source, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");
		
	if(version <= 21)
	{
		deSerialize_pre22(source, version);
		return;
	}

	if(version >= 24){
		param0 = readU16(source+0);
		param1 = readU8(source+2);
		param2 = readU8(source+3);
	}
	else{
		param0 = readU8(source+0);
		param1 = readU8(source+1);
		param2 = readU8(source+2);
		if(param0 > 0x7F){
			param0 |= ((param2&0xF0)<<4);
			param2 &= 0x0F;
		}
	}
}
void MapNode::serializeBulk(std::ostream &os, int version,
		const MapNode *nodes, u32 nodecount,
		u8 content_width, u8 params_width, bool compressed)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");

	assert(content_width == 2);
	assert(params_width == 2);

	// Can't do this anymore; we have 16-bit dynamically allocated node IDs
	// in memory; conversion just won't work in this direction.
	if(version < 24)
		throw SerializationError("MapNode::serializeBulk: serialization to "
				"version < 24 not possible");

	SharedBuffer<u8> databuf(nodecount * (content_width + params_width));

	// Serialize content
	for(u32 i=0; i<nodecount; i++)
		writeU16(&databuf[i*2], nodes[i].param0);

	// Serialize param1
	u32 start1 = content_width * nodecount;
	for(u32 i=0; i<nodecount; i++)
		writeU8(&databuf[start1 + i], nodes[i].param1);

	// Serialize param2
	u32 start2 = (content_width + 1) * nodecount;
	for(u32 i=0; i<nodecount; i++)
		writeU8(&databuf[start2 + i], nodes[i].param2);

	/*
		Compress data to output stream
	*/

	if(compressed)
	{
		compressZlib(databuf, os);
	}
	else
	{
		os.write((const char*) &databuf[0], databuf.getSize());
	}
}

// Deserialize bulk node data
void MapNode::deSerializeBulk(std::istream &is, int version,
		MapNode *nodes, u32 nodecount,
		u8 content_width, u8 params_width, bool compressed)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");

	assert(version >= 22);
	assert(content_width == 1 || content_width == 2);
	assert(params_width == 2);

	// Uncompress or read data
	u32 len = nodecount * (content_width + params_width);
	SharedBuffer<u8> databuf(len);
	if(compressed)
	{
		std::ostringstream os(std::ios_base::binary);
		decompressZlib(is, os);
		std::string s = os.str();
		if(s.size() != len)
			throw SerializationError("deSerializeBulkNodes: "
					"decompress resulted in invalid size");
		memcpy(&databuf[0], s.c_str(), len);
	}
	else
	{
		is.read((char*) &databuf[0], len);
		if(is.eof() || is.fail())
			throw SerializationError("deSerializeBulkNodes: "
					"failed to read bulk node data");
	}

	// Deserialize content
	if(content_width == 1)
	{
		for(u32 i=0; i<nodecount; i++)
			nodes[i].param0 = readU8(&databuf[i]);
	}
	else if(content_width == 2)
	{
		for(u32 i=0; i<nodecount; i++)
			nodes[i].param0 = readU16(&databuf[i*2]);
	}

	// Deserialize param1
	u32 start1 = content_width * nodecount;
	for(u32 i=0; i<nodecount; i++)
		nodes[i].param1 = readU8(&databuf[start1 + i]);

	// Deserialize param2
	u32 start2 = (content_width + 1) * nodecount;
	if(content_width == 1)
	{
		for(u32 i=0; i<nodecount; i++) {
			nodes[i].param2 = readU8(&databuf[start2 + i]);
			if(nodes[i].param0 > 0x7F){
				nodes[i].param0 <<= 4;
				nodes[i].param0 |= (nodes[i].param2&0xF0)>>4;
				nodes[i].param2 &= 0x0F;
			}
		}
	}
	else if(content_width == 2)
	{
		for(u32 i=0; i<nodecount; i++)
			nodes[i].param2 = readU8(&databuf[start2 + i]);
	}
}

/*
	Legacy serialization
*/
void MapNode::deSerialize_pre22(u8 *source, u8 version)
{
	if(version <= 1)
	{
		param0 = source[0];
	}
	else if(version <= 9)
	{
		param0 = source[0];
		param1 = source[1];
	}
	else
	{
		param0 = source[0];
		param1 = source[1];
		param2 = source[2];
		if(param0 > 0x7f){
			param0 <<= 4;
			param0 |= (param2&0xf0)>>4;
			param2 &= 0x0f;
		}
	}
	
	// Convert special values from old version to new
	if(version <= 19)
	{
		// In these versions, CONTENT_IGNORE and CONTENT_AIR
		// are 255 and 254
		// Version 19 is fucked up with sometimes the old values and sometimes not
		if(param0 == 255)
			param0 = CONTENT_IGNORE;
		else if(param0 == 254)
			param0 = CONTENT_AIR;
	}

	// Translate to our known version
	*this = mapnode_translate_to_internal(*this, version);
}
