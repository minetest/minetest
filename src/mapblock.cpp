/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "mapblock.h"
#include "map.h"
// For g_settings
#include "main.h"
#include "light.h"
#include <sstream>

/*
	MapBlock
*/

MapBlock::MapBlock(Map *parent, v3s16 pos, bool dummy):
		m_parent(parent),
		m_pos(pos),
		m_modified(MOD_STATE_WRITE_NEEDED),
		is_underground(false),
		m_lighting_expired(true),
		m_day_night_differs(false),
		m_generated(false),
		m_objects(this),
		m_timestamp(BLOCK_TIMESTAMP_UNDEFINED),
		m_usage_timer(0)
{
	data = NULL;
	if(dummy == false)
		reallocate();
	
	//m_spawn_timer = -10000;

#ifndef SERVER
	m_mesh_expired = false;
	mesh_mutex.Init();
	mesh = NULL;
	m_temp_mods_mutex.Init();
#endif
}

MapBlock::~MapBlock()
{
#ifndef SERVER
	{
		JMutexAutoLock lock(mesh_mutex);
		
		if(mesh)
		{
			mesh->drop();
			mesh = NULL;
		}
	}
#endif

	if(data)
		delete[] data;
}

bool MapBlock::isValidPositionParent(v3s16 p)
{
	if(isValidPosition(p))
	{
		return true;
	}
	else{
		return m_parent->isValidPosition(getPosRelative() + p);
	}
}

MapNode MapBlock::getNodeParent(v3s16 p)
{
	if(isValidPosition(p) == false)
	{
		return m_parent->getNode(getPosRelative() + p);
	}
	else
	{
		if(data == NULL)
			throw InvalidPositionException();
		return data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X];
	}
}

void MapBlock::setNodeParent(v3s16 p, MapNode & n)
{
	if(isValidPosition(p) == false)
	{
		m_parent->setNode(getPosRelative() + p, n);
	}
	else
	{
		if(data == NULL)
			throw InvalidPositionException();
		data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X] = n;
	}
}

MapNode MapBlock::getNodeParentNoEx(v3s16 p)
{
	if(isValidPosition(p) == false)
	{
		try{
			return m_parent->getNode(getPosRelative() + p);
		}
		catch(InvalidPositionException &e)
		{
			return MapNode(CONTENT_IGNORE);
		}
	}
	else
	{
		if(data == NULL)
		{
			return MapNode(CONTENT_IGNORE);
		}
		return data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X];
	}
}

#ifndef SERVER

#if 1
void MapBlock::updateMesh(u32 daynight_ratio)
{
#if 0
	/*
		DEBUG: If mesh has been generated, don't generate it again
	*/
	{
		JMutexAutoLock meshlock(mesh_mutex);
		if(mesh != NULL)
			return;
	}
#endif

	MeshMakeData data;
	data.fill(daynight_ratio, this);
	
	scene::SMesh *mesh_new = makeMapBlockMesh(&data);
	
	/*
		Replace the mesh
	*/

	replaceMesh(mesh_new);

}
#endif

void MapBlock::replaceMesh(scene::SMesh *mesh_new)
{
	mesh_mutex.Lock();

	//scene::SMesh *mesh_old = mesh[daynight_i];
	//mesh[daynight_i] = mesh_new;

	scene::SMesh *mesh_old = mesh;
	mesh = mesh_new;
	setMeshExpired(false);
	
	if(mesh_old != NULL)
	{
		// Remove hardware buffers of meshbuffers of mesh
		// NOTE: No way, this runs in a different thread and everything
		/*u32 c = mesh_old->getMeshBufferCount();
		for(u32 i=0; i<c; i++)
		{
			IMeshBuffer *buf = mesh_old->getMeshBuffer(i);
		}*/
		
		/*dstream<<"mesh_old->getReferenceCount()="
				<<mesh_old->getReferenceCount()<<std::endl;
		u32 c = mesh_old->getMeshBufferCount();
		for(u32 i=0; i<c; i++)
		{
			scene::IMeshBuffer *buf = mesh_old->getMeshBuffer(i);
			dstream<<"buf->getReferenceCount()="
					<<buf->getReferenceCount()<<std::endl;
		}*/

		// Drop the mesh
		mesh_old->drop();

		//delete mesh_old;
	}

	mesh_mutex.Unlock();
}
	
#endif // !SERVER

/*
	Propagates sunlight down through the block.
	Doesn't modify nodes that are not affected by sunlight.
	
	Returns false if sunlight at bottom block is invalid.
	Returns true if sunlight at bottom block is valid.
	Returns true if bottom block doesn't exist.

	If there is a block above, continues from it.
	If there is no block above, assumes there is sunlight, unless
	is_underground is set or highest node is water.

	All sunlighted nodes are added to light_sources.

	if remove_light==true, sets non-sunlighted nodes black.

	if black_air_left!=NULL, it is set to true if non-sunlighted
	air is left in block.
*/
bool MapBlock::propagateSunlight(core::map<v3s16, bool> & light_sources,
		bool remove_light, bool *black_air_left)
{
	// Whether the sunlight at the top of the bottom block is valid
	bool block_below_is_valid = true;
	
	v3s16 pos_relative = getPosRelative();
	
	for(s16 x=0; x<MAP_BLOCKSIZE; x++)
	{
		for(s16 z=0; z<MAP_BLOCKSIZE; z++)
		{
#if 1
			bool no_sunlight = false;
			bool no_top_block = false;
			// Check if node above block has sunlight
			try{
				MapNode n = getNodeParent(v3s16(x, MAP_BLOCKSIZE, z));
				if(n.getContent() == CONTENT_IGNORE)
				{
					// Trust heuristics
					no_sunlight = is_underground;
				}
				else if(n.getLight(LIGHTBANK_DAY) != LIGHT_SUN)
				{
					no_sunlight = true;
				}
			}
			catch(InvalidPositionException &e)
			{
				no_top_block = true;
				
				// NOTE: This makes over-ground roofed places sunlighted
				// Assume sunlight, unless is_underground==true
				if(is_underground)
				{
					no_sunlight = true;
				}
				else
				{
					MapNode n = getNode(v3s16(x, MAP_BLOCKSIZE-1, z));
					//if(n.getContent() == CONTENT_WATER || n.getContent() == CONTENT_WATERSOURCE)
					if(content_features(n).sunlight_propagates == false)
					{
						no_sunlight = true;
					}
				}
				// NOTE: As of now, this just would make everything dark.
				// No sunlight here
				//no_sunlight = true;
			}
#endif
#if 0 // Doesn't work; nothing gets light.
			bool no_sunlight = true;
			bool no_top_block = false;
			// Check if node above block has sunlight
			try{
				MapNode n = getNodeParent(v3s16(x, MAP_BLOCKSIZE, z));
				if(n.getLight(LIGHTBANK_DAY) == LIGHT_SUN)
				{
					no_sunlight = false;
				}
			}
			catch(InvalidPositionException &e)
			{
				no_top_block = true;
			}
#endif

			/*std::cout<<"("<<x<<","<<z<<"): "
					<<"no_top_block="<<no_top_block
					<<", is_underground="<<is_underground
					<<", no_sunlight="<<no_sunlight
					<<std::endl;*/
		
			s16 y = MAP_BLOCKSIZE-1;
			
			// This makes difference to diminishing in water.
			bool stopped_to_solid_object = false;
			
			u8 current_light = no_sunlight ? 0 : LIGHT_SUN;

			for(; y >= 0; y--)
			{
				v3s16 pos(x, y, z);
				MapNode &n = getNodeRef(pos);
				
				if(current_light == 0)
				{
					// Do nothing
				}
				else if(current_light == LIGHT_SUN && n.sunlight_propagates())
				{
					// Do nothing: Sunlight is continued
				}
				else if(n.light_propagates() == false)
				{
					/*// DEPRECATED TODO: REMOVE
					if(grow_grass)
					{
						bool upper_is_air = false;
						try
						{
							if(getNodeParent(pos+v3s16(0,1,0)).getContent() == CONTENT_AIR)
								upper_is_air = true;
						}
						catch(InvalidPositionException &e)
						{
						}
						// Turn mud into grass
						if(upper_is_air && n.getContent() == CONTENT_MUD
								&& current_light == LIGHT_SUN)
						{
							n.d = CONTENT_GRASS;
						}
					}*/

					// A solid object is on the way.
					stopped_to_solid_object = true;
					
					// Light stops.
					current_light = 0;
				}
				else
				{
					// Diminish light
					current_light = diminish_light(current_light);
				}

				u8 old_light = n.getLight(LIGHTBANK_DAY);

				if(current_light > old_light || remove_light)
				{
					n.setLight(LIGHTBANK_DAY, current_light);
				}
				
				if(diminish_light(current_light) != 0)
				{
					light_sources.insert(pos_relative + pos, true);
				}

				if(current_light == 0 && stopped_to_solid_object)
				{
					if(black_air_left)
					{
						*black_air_left = true;
					}
				}
			}

			// Whether or not the block below should see LIGHT_SUN
			bool sunlight_should_go_down = (current_light == LIGHT_SUN);

			/*
				If the block below hasn't already been marked invalid:

				Check if the node below the block has proper sunlight at top.
				If not, the block below is invalid.
				
				Ignore non-transparent nodes as they always have no light
			*/
			try
			{
			if(block_below_is_valid)
			{
				MapNode n = getNodeParent(v3s16(x, -1, z));
				if(n.light_propagates())
				{
					if(n.getLight(LIGHTBANK_DAY) == LIGHT_SUN
							&& sunlight_should_go_down == false)
						block_below_is_valid = false;
					else if(n.getLight(LIGHTBANK_DAY) != LIGHT_SUN
							&& sunlight_should_go_down == true)
						block_below_is_valid = false;
				}
			}//if
			}//try
			catch(InvalidPositionException &e)
			{
				/*std::cout<<"InvalidBlockException for bottom block node"
						<<std::endl;*/
				// Just no block below, no need to panic.
			}
		}
	}

	return block_below_is_valid;
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

void MapBlock::stepObjects(float dtime, bool server, u32 daynight_ratio)
{
	/*
		Step objects
	*/
	m_objects.step(dtime, server, daynight_ratio);

	setChangedFlag();
}


void MapBlock::updateDayNightDiff()
{
	if(data == NULL)
	{
		m_day_night_differs = false;
		return;
	}

	bool differs = false;

	/*
		Check if any lighting value differs
	*/
	for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
	{
		MapNode &n = data[i];
		if(n.getLight(LIGHTBANK_DAY) != n.getLight(LIGHTBANK_NIGHT))
		{
			differs = true;
			break;
		}
	}

	/*
		If some lighting values differ, check if the whole thing is
		just air. If it is, differ = false
	*/
	if(differs)
	{
		bool only_air = true;
		for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
		{
			MapNode &n = data[i];
			if(n.getContent() != CONTENT_AIR)
			{
				only_air = false;
				break;
			}
		}
		if(only_air)
			differs = false;
	}

	// Set member variable
	m_day_night_differs = differs;
}

s16 MapBlock::getGroundLevel(v2s16 p2d)
{
	if(isDummy())
		return -3;
	try
	{
		s16 y = MAP_BLOCKSIZE-1;
		for(; y>=0; y--)
		{
			MapNode n = getNodeRef(p2d.X, y, p2d.Y);
			if(content_features(n).walkable)
			{
				if(y == MAP_BLOCKSIZE-1)
					return -2;
				else
					return y;
			}
		}
		return -1;
	}
	catch(InvalidPositionException &e)
	{
		return -3;
	}
}

/*
	Serialization
*/

void MapBlock::serialize(std::ostream &os, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");
	
	if(data == NULL)
	{
		throw SerializationError("ERROR: Not writing dummy block.");
	}
	
	// These have no compression
	if(version <= 3 || version == 5 || version == 6)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;
		
		u32 buflen = 1 + nodecount * MapNode::serializedLength(version);
		SharedBuffer<u8> dest(buflen);

		dest[0] = is_underground;
		for(u32 i=0; i<nodecount; i++)
		{
			u32 s = 1 + i * MapNode::serializedLength(version);
			data[i].serialize(&dest[s], version);
		}
		
		os.write((char*)*dest, dest.getSize());
	}
	else if(version <= 10)
	{
		/*
			With compression.
			Compress the materials and the params separately.
		*/
		
		// First byte
		os.write((char*)&is_underground, 1);

		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		// Get and compress materials
		SharedBuffer<u8> materialdata(nodecount);
		for(u32 i=0; i<nodecount; i++)
		{
			materialdata[i] = data[i].param0;
		}
		compress(materialdata, os, version);

		// Get and compress lights
		SharedBuffer<u8> lightdata(nodecount);
		for(u32 i=0; i<nodecount; i++)
		{
			lightdata[i] = data[i].param1;
		}
		compress(lightdata, os, version);
		
		if(version >= 10)
		{
			// Get and compress param2
			SharedBuffer<u8> param2data(nodecount);
			for(u32 i=0; i<nodecount; i++)
			{
				param2data[i] = data[i].param2;
			}
			compress(param2data, os, version);
		}
	}
	// All other versions (newest)
	else
	{
		// First byte
		u8 flags = 0;
		if(is_underground)
			flags |= 0x01;
		if(m_day_night_differs)
			flags |= 0x02;
		if(m_lighting_expired)
			flags |= 0x04;
		if(version >= 18)
		{
			if(m_generated == false)
				flags |= 0x08;
		}
		os.write((char*)&flags, 1);

		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		/*
			Get data
		*/

		// Serialize nodes
		SharedBuffer<u8> databuf_nodelist(nodecount*3);
		for(u32 i=0; i<nodecount; i++)
		{
			data[i].serialize(&databuf_nodelist[i*3], version);
		}
		
		// Create buffer with different parameters sorted
		SharedBuffer<u8> databuf(nodecount*3);
		for(u32 i=0; i<nodecount; i++)
		{
			databuf[i] = databuf_nodelist[i*3];
			databuf[i+nodecount] = databuf_nodelist[i*3+1];
			databuf[i+nodecount*2] = databuf_nodelist[i*3+2];
		}

		/*
			Compress data to output stream
		*/

		compress(databuf, os, version);
		
		/*
			NodeMetadata
		*/
		if(version >= 14)
		{
			if(version <= 15)
			{
				try{
					std::ostringstream oss(std::ios_base::binary);
					m_node_metadata.serialize(oss);
					os<<serializeString(oss.str());
				}
				// This will happen if the string is longer than 65535
				catch(SerializationError &e)
				{
					// Use an empty string
					os<<serializeString("");
				}
			}
			else
			{
				std::ostringstream oss(std::ios_base::binary);
				m_node_metadata.serialize(oss);
				compressZlib(oss.str(), os);
				//os<<serializeLongString(oss.str());
			}
		}
	}
}

void MapBlock::deSerialize(std::istream &is, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");

	// These have no lighting info
	if(version <= 1)
	{
		setLightingExpired(true);
	}

	// These have no "generated" field
	if(version < 18)
	{
		m_generated = true;
	}

	// These have no compression
	if(version <= 3 || version == 5 || version == 6)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;
		char tmp;
		is.read(&tmp, 1);
		if(is.gcount() != 1)
			throw SerializationError
					("MapBlock::deSerialize: no enough input data");
		is_underground = tmp;
		for(u32 i=0; i<nodecount; i++)
		{
			s32 len = MapNode::serializedLength(version);
			SharedBuffer<u8> d(len);
			is.read((char*)*d, len);
			if(is.gcount() != len)
				throw SerializationError
						("MapBlock::deSerialize: no enough input data");
			data[i].deSerialize(*d, version);
		}
	}
	else if(version <= 10)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		u8 t8;
		is.read((char*)&t8, 1);
		is_underground = t8;

		{
			// Uncompress and set material data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].param0 = s[i];
			}
		}
		{
			// Uncompress and set param data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].param1 = s[i];
			}
		}
	
		if(version >= 10)
		{
			// Uncompress and set param2 data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].param2 = s[i];
			}
		}
	}
	// All other versions (newest)
	else
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		u8 flags;
		is.read((char*)&flags, 1);
		is_underground = (flags & 0x01) ? true : false;
		m_day_night_differs = (flags & 0x02) ? true : false;
		m_lighting_expired = (flags & 0x04) ? true : false;
		if(version >= 18)
			m_generated = (flags & 0x08) ? false : true;

		// Uncompress data
		std::ostringstream os(std::ios_base::binary);
		decompress(is, os, version);
		std::string s = os.str();
		if(s.size() != nodecount*3)
			throw SerializationError
					("MapBlock::deSerialize: decompress resulted in size"
					" other than nodecount*3");

		// deserialize nodes from buffer
		for(u32 i=0; i<nodecount; i++)
		{
			u8 buf[3];
			buf[0] = s[i];
			buf[1] = s[i+nodecount];
			buf[2] = s[i+nodecount*2];
			data[i].deSerialize(buf, version);
		}
		
		/*
			NodeMetadata
		*/
		if(version >= 14)
		{
			// Ignore errors
			try{
				if(version <= 15)
				{
					std::string data = deSerializeString(is);
					std::istringstream iss(data, std::ios_base::binary);
					m_node_metadata.deSerialize(iss);
				}
				else
				{
					//std::string data = deSerializeLongString(is);
					std::ostringstream oss(std::ios_base::binary);
					decompressZlib(is, oss);
					std::istringstream iss(oss.str(), std::ios_base::binary);
					m_node_metadata.deSerialize(iss);
				}
			}
			catch(SerializationError &e)
			{
				dstream<<"WARNING: MapBlock::deSerialize(): Ignoring an error"
						<<" while deserializing node metadata"<<std::endl;
			}
		}
	}
}

void MapBlock::serializeDiskExtra(std::ostream &os, u8 version)
{
	// Versions up from 9 have block objects.
	if(version >= 9)
	{
		//serializeObjects(os, version); // DEPRECATED
		// count=0
		writeU16(os, 0);
	}
	
	// Versions up from 15 have static objects.
	if(version >= 15)
	{
		m_static_objects.serialize(os);
	}

	// Timestamp
	if(version >= 17)
	{
		writeU32(os, getTimestamp());
	}
}

void MapBlock::deSerializeDiskExtra(std::istream &is, u8 version)
{
	/*
		Versions up from 9 have block objects.
	*/
	if(version >= 9)
	{
		updateObjects(is, version, NULL, 0);
	}

	/*
		Versions up from 15 have static objects.
	*/
	if(version >= 15)
	{
		m_static_objects.deSerialize(is);
	}
		
	// Timestamp
	if(version >= 17)
	{
		setTimestamp(readU32(is));
	}
	else
	{
		setTimestamp(BLOCK_TIMESTAMP_UNDEFINED);
	}
}


//END
