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

#include "common_irrlicht.h"
#include "mapnode.h"
#include "porting.h"
#include <string>
#include "main.h" // For g_settings
#include "nodedef.h"
#include "content_mapnode.h" // For mapnode_translate_*_internal
#include "serialization.h" // For ser_ver_supported

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
	param1 = a_param1;
	param2 = a_param2;
	// Set content (param0 and (param2&0xf0)) after other params
	// because this needs to override part of param2
	setContent(id);
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



u32 MapNode::serializedLength(u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");
		
	if(version == 0)
		return 1;
	else if(version <= 9)
		return 2;
	else
		return 3;
}
void MapNode::serialize(u8 *dest, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");

	if(version <= 21)
	{
		serialize_pre22(dest, version);
		return;
	}

	writeU8(dest+0, param0);
	writeU8(dest+1, param1);
	writeU8(dest+2, param2);
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

	param0 = readU8(source+0);
	param1 = readU8(source+1);
	param2 = readU8(source+2);
}
void MapNode::serializeBulk(std::ostream &os, int version,
		const MapNode *nodes, u32 nodecount,
		u8 content_width, u8 params_width, bool compressed)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");

	assert(version >= 22);
	assert(content_width == 1);
	assert(params_width == 2);

	SharedBuffer<u8> databuf(nodecount * (content_width + params_width));

	// Serialize content
	if(content_width == 1)
	{
		for(u32 i=0; i<nodecount; i++)
			writeU8(&databuf[i], nodes[i].param0);
	}
	/* If param0 is extended to two bytes, use something like this: */
	/*else if(content_width == 2)
	{
		for(u32 i=0; i<nodecount; i++)
			writeU16(&databuf[i*2], nodes[i].param0);
	}*/

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
	assert(content_width == 1);
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
	/* If param0 is extended to two bytes, use something like this: */
	/*else if(content_width == 2)
	{
		for(u32 i=0; i<nodecount; i++)
			nodes[i].param0 = readU16(&databuf[i*2]);
	}*/

	// Deserialize param1
	u32 start1 = content_width * nodecount;
	for(u32 i=0; i<nodecount; i++)
		nodes[i].param1 = readU8(&databuf[start1 + i]);

	// Deserialize param2
	u32 start2 = (content_width + 1) * nodecount;
	for(u32 i=0; i<nodecount; i++)
		nodes[i].param2 = readU8(&databuf[start2 + i]);
}

/*
	Legacy serialization
*/
void MapNode::serialize_pre22(u8 *dest, u8 version)
{
	// Translate to wanted version
	MapNode n_foreign = mapnode_translate_from_internal(*this, version);

	u8 actual_param0 = n_foreign.param0;

	// Convert special values from new version to old
	if(version <= 18)
	{
		// In these versions, CONTENT_IGNORE and CONTENT_AIR
		// are 255 and 254
		if(actual_param0 == CONTENT_IGNORE)
			actual_param0 = 255;
		else if(actual_param0 == CONTENT_AIR)
			actual_param0 = 254;
	}

	if(version == 0)
	{
		dest[0] = actual_param0;
	}
	else if(version <= 9)
	{
		dest[0] = actual_param0;
		dest[1] = n_foreign.param1;
	}
	else
	{
		dest[0] = actual_param0;
		dest[1] = n_foreign.param1;
		dest[2] = n_foreign.param2;
	}
}
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
