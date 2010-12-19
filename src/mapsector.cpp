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

#include "mapsector.h"
#include "jmutexautolock.h"
#include "client.h"
#include "exceptions.h"

MapSector::MapSector(NodeContainer *parent, v2s16 pos):
		differs_from_disk(true),
		usage_timer(0.0),
		m_parent(parent),
		m_pos(pos),
		m_block_cache(NULL)
{
	m_mutex.Init();
	assert(m_mutex.IsInitialized());
}

MapSector::~MapSector()
{
	deleteBlocks();
}

void MapSector::deleteBlocks()
{
	JMutexAutoLock lock(m_mutex);

	// Clear cache
	m_block_cache = NULL;

	// Delete all
	core::map<s16, MapBlock*>::Iterator i = m_blocks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		delete i.getNode()->getValue();
	}

	// Clear container
	m_blocks.clear();
}

MapBlock * MapSector::getBlockBuffered(s16 y)
{
	MapBlock *block;

	if(m_block_cache != NULL && y == m_block_cache_y){
		return m_block_cache;
	}
	
	// If block doesn't exist, return NULL
	core::map<s16, MapBlock*>::Node *n = m_blocks.find(y);
	if(n == NULL)
	{
		block = NULL;
	}
	// If block exists, return it
	else{
		block = n->getValue();
	}
	
	// Cache the last result
	m_block_cache_y = y;
	m_block_cache = block;
	
	return block;
}

MapBlock * MapSector::getBlockNoCreate(s16 y)
{
	JMutexAutoLock lock(m_mutex);
	
	MapBlock *block = getBlockBuffered(y);

	if(block == NULL)
		throw InvalidPositionException();
	
	return block;
}

MapBlock * MapSector::createBlankBlockNoInsert(s16 y)
{
	// There should not be a block at this position
	if(getBlockBuffered(y) != NULL)
		throw AlreadyExistsException("Block already exists");

	v3s16 blockpos_map(m_pos.X, y, m_pos.Y);
	
	MapBlock *block = new MapBlock(m_parent, blockpos_map);
	
	return block;
}

MapBlock * MapSector::createBlankBlock(s16 y)
{
	JMutexAutoLock lock(m_mutex);
	
	MapBlock *block = createBlankBlockNoInsert(y);
	
	m_blocks.insert(y, block);

	return block;
}

void MapSector::insertBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	{
		JMutexAutoLock lock(m_mutex);

		MapBlock *block2 = getBlockBuffered(block_y);
		if(block2 != NULL){
			throw AlreadyExistsException("Block already exists");
		}

		v2s16 p2d(block->getPos().X, block->getPos().Z);
		assert(p2d == m_pos);
		
		// Insert into container
		m_blocks.insert(block_y, block);
	}
}

void MapSector::removeBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	JMutexAutoLock lock(m_mutex);
	
	// Clear from cache
	m_block_cache = NULL;
	
	// Remove from container
	m_blocks.remove(block_y);
}

void MapSector::getBlocks(core::list<MapBlock*> &dest)
{
	JMutexAutoLock lock(m_mutex);

	core::list<MapBlock*> ref_list;

	core::map<s16, MapBlock*>::Iterator bi;

	bi = m_blocks.getIterator();
	for(; bi.atEnd() == false; bi++)
	{
		MapBlock *b = bi.getNode()->getValue();
		dest.push_back(b);
	}
}

/*
	ServerMapSector
*/

ServerMapSector::ServerMapSector(NodeContainer *parent, v2s16 pos, u16 hm_split):
		MapSector(parent, pos),
		m_hm_split(hm_split),
		m_objects(NULL)
{
	// hm_split has to be 1 or 2^x
	assert(hm_split == 0 || hm_split == 1 || (hm_split & (hm_split-1)) == 0);
	assert(hm_split * hm_split <= MAPSECTOR_FIXEDHEIGHTMAPS_MAXCOUNT);
	
	for(u16 i=0; i<hm_split*hm_split; i++)
		m_heightmaps[i] = NULL;
}

ServerMapSector::~ServerMapSector()
{
	u16 hm_count = m_hm_split * m_hm_split;

	// Write heightmaps
	for(u16 i=0; i<hm_count; i++)
	{
		if(m_heightmaps[i])
			delete m_heightmaps[i];
	}

	if(m_objects)
		delete m_objects;
}

void ServerMapSector::setHeightmap(v2s16 hm_p, FixedHeightmap *hm)
{
	assert(isInArea(hm_p, m_hm_split));

	s16 i = hm_p.Y * m_hm_split + hm_p.X;
	
	// Don't allow setting already set heightmaps as of now
	assert(m_heightmaps[i] == NULL);
	
	/*std::cout<<"MapSector::setHeightmap for sector "
			<<"("<<m_pos.X<<","<<m_pos.Y<<"): "
			<<"Setting heightmap "
			<<"("<<hm_p.X<<","<<hm_p.Y<<")"
			<<" which is i="<<i
			<<" to pointer "<<(long long)hm
			<<std::endl;*/

	m_heightmaps[i] = hm;
	
	differs_from_disk = true;
}

FixedHeightmap * ServerMapSector::getHeightmap(v2s16 hm_p)
{
	assert(isInArea(hm_p, m_hm_split));

	s16 i = hm_p.Y * m_hm_split + hm_p.X;
	
	return m_heightmaps[i];
}

f32 ServerMapSector::getGroundHeight(v2s16 p, bool generate)
{
	// If no heightmaps
	if(m_hm_split == 0)
	{
		/*std::cout<<"Sector has no heightmap"
				<<" while trying to get height at ("<<p.X<<","<<p.Y<<")"
				<<" for sector ("<<m_pos.X<<","<<m_pos.Y<<")"
				<<std::endl;*/
		return GROUNDHEIGHT_NOTFOUND_SETVALUE;
	}
	
	// Side length of heightmap
	s16 hm_d = MAP_BLOCKSIZE / m_hm_split;

	// Position of selected heightmap
	v2s16 hm_p = getContainerPos(p, hm_d);
	if(isInArea(hm_p, m_hm_split) == false)
	{
		/*std::cout<<"Sector has no heightmap ("<<hm_p.X<<","<<hm_p.Y<<")"
				<<" while trying to get height at ("<<p.X<<","<<p.Y<<")"
				<<" for sector ("<<m_pos.X<<","<<m_pos.Y<<")"
				<<std::endl;*/
		return GROUNDHEIGHT_NOTFOUND_SETVALUE;
	}

	// Selected heightmap
	FixedHeightmap *hm = m_heightmaps[hm_p.Y * m_hm_split + hm_p.X];

	if(hm == NULL)
	{
		/*std::cout<<"Sector heightmap ("<<hm_p.X<<","<<hm_p.Y<<")"
				" is NULL"
				<<" while trying to get height at ("<<p.X<<","<<p.Y<<")"
				<<" for sector ("<<m_pos.X<<","<<m_pos.Y<<")"
				<<std::endl;*/
		return GROUNDHEIGHT_NOTFOUND_SETVALUE;
	}
	
	// Position in selected heighmap
	v2s16 p_in_hm = p - hm_p * hm_d;
	if(isInArea(p_in_hm, hm_d+1) == false)
	{
		/*std::cout<<"Position ("<<p_in_hm.X<<","<<p_in_hm.Y<<")"
				" not in sector heightmap area"
				<<" while trying to get height at ("<<p.X<<","<<p.Y<<")"
				<<" for sector ("<<m_pos.X<<","<<m_pos.Y<<")"
				<<std::endl;*/
		return GROUNDHEIGHT_NOTFOUND_SETVALUE;
	}
	
	f32 h = hm->getGroundHeight(p_in_hm);
	
	/*if(h < GROUNDHEIGHT_VALID_MINVALUE)
	{
		std::cout<<"Sector heightmap ("<<hm_p.X<<","<<hm_p.Y<<")"
				" returned invalid value"
				<<" while trying to get height at ("<<p.X<<","<<p.Y<<")"
				<<" which is ("<<p_in_hm.X<<","<<p_in_hm.Y<<") in heightmap"
				<<" for sector ("<<m_pos.X<<","<<m_pos.Y<<")"
				<<std::endl;
	}*/

	return h;
}

void ServerMapSector::setGroundHeight(v2s16 p, f32 y, bool generate)
{
	/*
		NOTE:
		This causes glitches because the sector cannot be actually
		modified according to heightmap changes.

		This is useful when generating continued sub-heightmaps
		inside the sector.
	*/

	// If no heightmaps
	if(m_hm_split == 0)
		return;
	
	// Side length of heightmap
	s16 hm_d = MAP_BLOCKSIZE / m_hm_split;

	// Position of selected heightmap
	v2s16 hm_p = getContainerPos(p, hm_d);
	if(isInArea(hm_p, m_hm_split) == false)
		return;

	// Selected heightmap
	FixedHeightmap *hm = m_heightmaps[hm_p.Y * m_hm_split + hm_p.X];

	if(hm == NULL)
		return;
	
	// Position in selected heighmap
	v2s16 p_in_hm = p - hm_p * hm_d;
	if(isInArea(p_in_hm, hm_d) == false)
		return;
	
	hm->setGroundHeight(p_in_hm, y);
	
	differs_from_disk = true;
}

void ServerMapSector::serialize(std::ostream &os, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapSector format not supported");
	
	/*
		[0] u8 serialization version
		+ heightmap data
	*/
	
	// Server has both of these, no need to support not having them.
	assert(m_objects != NULL);

	// Write version
	os.write((char*)&version, 1);
	
	/*
		Serialize heightmap(s)
	*/

	// Version with single heightmap
	if(version <= 7)
	{
		u32 heightmap_size =
				FixedHeightmap::serializedLength(version, MAP_BLOCKSIZE);
		
		SharedBuffer<u8> data(heightmap_size);
		m_heightmaps[0]->serialize(*data, version);

		os.write((const char*)*data, heightmap_size);
		
		if(version >= 5)
		{
			/*
				Write objects
			*/
			
			u16 object_count;
			if(m_objects->size() > 65535)
				object_count = 65535;
			else
				object_count = m_objects->size();

			u8 b[2];
			writeU16(b, object_count);
			os.write((char*)b, 2);
			
			core::map<v3s16, u8>::Iterator i;
			i = m_objects->getIterator();
			for(; i.atEnd() == false; i++)
			{
				v3s16 p = i.getNode()->getKey();
				u8 d = i.getNode()->getValue();
				u8 b[7];
				writeV3S16(&b[0], p);
				b[6] = d;
				os.write((char*)b, 7);
			}
		}
	}
	// Version with multiple heightmaps
	else
	{
		u8 buf[2];
		
		if(m_hm_split > 255)
			throw SerializationError("Sector has too many heightmaps");
		
		// Write heightmap split ratio
		writeU8(buf, m_hm_split);
		os.write((char*)buf, 1);

		// If there are heightmaps, write them
		if(m_hm_split != 0)
		{
			u16 hm_d = MAP_BLOCKSIZE / m_hm_split;

			u32 hm_size = FixedHeightmap::serializedLength(version, hm_d);
			SharedBuffer<u8> data(hm_size);
			
			u16 hm_count = m_hm_split * m_hm_split;

			// Write heightmaps
			for(u16 i=0; i<hm_count; i++)
			{
				m_heightmaps[i]->serialize(*data, version);
				os.write((const char*)*data, hm_size);
			}
		}
	
		/*
			Write objects
		*/
		
		u16 object_count;
		if(m_objects->size() > 65535)
			object_count = 65535;
		else
			object_count = m_objects->size();

		u8 b[2];
		writeU16(b, object_count);
		os.write((char*)b, 2);
		
		core::map<v3s16, u8>::Iterator i;
		i = m_objects->getIterator();
		for(; i.atEnd() == false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			u8 d = i.getNode()->getValue();
			u8 b[7];
			writeV3S16(&b[0], p);
			b[6] = d;
			os.write((char*)b, 7);
		}
	}
}

ServerMapSector* ServerMapSector::deSerialize(
		std::istream &is,
		NodeContainer *parent,
		v2s16 p2d,
		Heightmap *master_hm,
		core::map<v2s16, MapSector*> & sectors
	)
{
	/*
		[0] u8 serialization version
		+ heightmap data
	*/

	/*
		Read stuff
	*/
	
	// Read version
	u8 version = SER_FMT_VER_INVALID;
	is.read((char*)&version, 1);
	
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapSector format not supported");
	
	/*
		Read heightmap(s)
	*/

	FixedHeightmap *hms[MAPSECTOR_FIXEDHEIGHTMAPS_MAXCOUNT];
	u16 hm_split = 0;
	
	// Version with a single heightmap
	if(version <= 7)
	{
		hm_split = 1;

		u32 hm_size =
				FixedHeightmap::serializedLength(version, MAP_BLOCKSIZE);
		
		SharedBuffer<u8> data(hm_size);
		is.read((char*)*data, hm_size);

		hms[0] = new FixedHeightmap(master_hm, p2d, MAP_BLOCKSIZE);
		hms[0]->deSerialize(*data, version);
	}
	// Version with multiple heightmaps
	else
	{
		u8 buf[2];

		// Read split ratio
		is.read((char*)buf, 1);
		hm_split = readU8(buf);

		// If there are heightmaps, read them
		if(hm_split != 0)
		{
			u16 hm_count = hm_split * hm_split;

			if(hm_count > MAPSECTOR_FIXEDHEIGHTMAPS_MAXCOUNT)
				throw SerializationError("Sector has too many heightmaps");
			
			u16 hm_d = MAP_BLOCKSIZE / hm_split;

			u32 hm_size = FixedHeightmap::serializedLength(version, hm_d);
			
			u16 i=0;
			for(s16 y=0; y<hm_split; y++)
			for(s16 x=0; x<hm_split; x++)
			{
				SharedBuffer<u8> data(hm_size);
				is.read((char*)*data, hm_size);

				hms[i] = new FixedHeightmap(master_hm, p2d+v2s16(x,y), hm_d);
				hms[i]->deSerialize(*data, version);
				i++;
			}
		}
	}

	/*
		Read objects
	*/

	core::map<v3s16, u8> *objects = new core::map<v3s16, u8>;

	if(version >= 5)
	{
		u8 b[2];
		is.read((char*)b, 2);
		u16 object_count = readU16(b);

		for(u16 i=0; i<object_count; i++)
		{
			u8 b[7];
			is.read((char*)b, 7);
			v3s16 p = readV3S16(&b[0]);
			u8 d = b[6];
			objects->insert(p, d);
		}
	}
	
	/*
		Get or create sector
	*/

	ServerMapSector *sector = NULL;

	core::map<v2s16, MapSector*>::Node *n = sectors.find(p2d);

	if(n != NULL)
	{
		dstream<<"deSerializing existent sectors not supported "
				"at the moment, because code hasn't been tested."
				<<std::endl;
		assert(0);
		// NOTE: At least hm_split mismatch would have to be checked
		
		//sector = n->getValue();
	}
	else
	{
		sector = new ServerMapSector(parent, p2d, hm_split);
		sectors.insert(p2d, sector);
	}

	/*
		Set stuff in sector
	*/

	// Set heightmaps
	
	sector->m_hm_split = hm_split;

	u16 hm_count = hm_split * hm_split;

	for(u16 i=0; i<hm_count; i++)
	{
		// Set (or change) heightmap
		FixedHeightmap *oldhm = sector->m_heightmaps[i];
		sector->m_heightmaps[i] = hms[i];
		if(oldhm != NULL)
			delete oldhm;
	}
	
	// Set (or change) objects
	core::map<v3s16, u8> *oldfo = sector->m_objects;
	sector->m_objects = objects;
	if(oldfo)
		delete oldfo;

	return sector;
}

#ifndef SERVER
/*
	ClientMapSector
*/

ClientMapSector::ClientMapSector(NodeContainer *parent, v2s16 pos):
		MapSector(parent, pos)
{
}

ClientMapSector::~ClientMapSector()
{
}

void ClientMapSector::deSerialize(std::istream &is)
{
	/*
		[0] u8 serialization version
		[1] s16 corners[0]
		[3] s16 corners[1]
		[5] s16 corners[2]
		[7] s16 corners[3]
		size = 9
		
		In which corners are in these positions
		v2s16(0,0),
		v2s16(1,0),
		v2s16(1,1),
		v2s16(0,1),
	*/
	
	// Read version
	u8 version = SER_FMT_VER_INVALID;
	is.read((char*)&version, 1);
	
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapSector format not supported");
	if(version <= 7)
		throw VersionMismatchException("ERROR: MapSector format not supported");
	
	u8 buf[2];
	
	// Read corners
	is.read((char*)buf, 2);
	s16 c0 = readU16(buf);
	is.read((char*)buf, 2);
	s16 c1 = readU16(buf);
	is.read((char*)buf, 2);
	s16 c2 = readU16(buf);
	is.read((char*)buf, 2);
	s16 c3 = readU16(buf);
	
	/*
		Set stuff in sector
	*/
	
	m_corners[0] = c0;
	m_corners[1] = c1;
	m_corners[2] = c2;
	m_corners[3] = c3;
}
#endif // !SERVER

//END
