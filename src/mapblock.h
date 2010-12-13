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

#ifndef MAPBLOCK_HEADER
#define MAPBLOCK_HEADER

#include <jmutex.h>
#include <jmutexautolock.h>
#include <exception>
#include "debug.h"
#include "common_irrlicht.h"
#include "mapnode.h"
#include "exceptions.h"
#include "serialization.h"
#include "constants.h"
#include "mapblockobject.h"
#include "voxel.h"

#define MAP_BLOCKSIZE 16

// Named by looking towards z+
enum{
	FACE_BACK=0,
	FACE_TOP,
	FACE_RIGHT,
	FACE_FRONT,
	FACE_BOTTOM,
	FACE_LEFT
};

struct FastFace
{
	u16 tile;
	video::S3DVertex vertices[4]; // Precalculated vertices
};

enum
{
	NODECONTAINER_ID_MAPBLOCK,
	NODECONTAINER_ID_MAPSECTOR,
	NODECONTAINER_ID_MAP,
	NODECONTAINER_ID_MAPBLOCKCACHE,
	NODECONTAINER_ID_VOXELMANIPULATOR,
};

class NodeContainer
{
public:
	virtual bool isValidPosition(v3s16 p) = 0;
	virtual MapNode getNode(v3s16 p) = 0;
	virtual void setNode(v3s16 p, MapNode & n) = 0;
	virtual u16 nodeContainerId() const = 0;
};

class MapBlock : public NodeContainer
{
public:

	/*
		This used by Server's block creation stuff for not sending
		blocks that are waiting a lighting update.

		If true, the block needs some work by the one who set this
		to true.

		While true, nobody else should touch the block.
	*/
	//bool is_incomplete;
	
	scene::SMesh *mesh;
	JMutex mesh_mutex;

	MapBlock(NodeContainer *parent, v3s16 pos, bool dummy=false):
			m_parent(parent),
			m_pos(pos),
			changed(true),
			is_underground(false),
			m_objects(this)
			//is_incomplete(false)
	{
		data = NULL;
		if(dummy == false)
			reallocate();
		mesh_mutex.Init();
		mesh = NULL;
	}

	~MapBlock()
	{
		{
			JMutexAutoLock lock(mesh_mutex);
			
			if(mesh != NULL)
			{
				mesh->drop();
				mesh = NULL;
			}
		}

		if(data)
			delete[] data;
	}
	
	virtual u16 nodeContainerId() const
	{
		return NODECONTAINER_ID_MAPBLOCK;
	}

	NodeContainer * getParent()
	{
		return m_parent;
	}

	bool isDummy()
	{
		return (data == NULL);
	}

	void unDummify()
	{
		assert(isDummy());
		reallocate();
	}
	
	bool getChangedFlag()
	{
		return changed;
	}

	void resetChangedFlag()
	{
		changed = false;
	}

	void setChangedFlag()
	{
		changed = true;
	}

	v3s16 getPos()
	{
		return m_pos;
	}
		
	v3s16 getPosRelative()
	{
		return m_pos * MAP_BLOCKSIZE;
	}
		
	bool getIsUnderground()
	{
		return is_underground;
	}

	void setIsUnderground(bool a_is_underground)
	{
		is_underground = a_is_underground;
		setChangedFlag();
	}

	core::aabbox3d<s16> getBox()
	{
		return core::aabbox3d<s16>(getPosRelative(),
				getPosRelative()
				+ v3s16(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE)
				- v3s16(1,1,1));
	}
	
	void reallocate()
	{
		if(data != NULL)
			delete[] data;
		u32 l = MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE;
		data = new MapNode[l];
		for(u32 i=0; i<l; i++){
			data[i] = MapNode();
		}
		setChangedFlag();
	}

	bool isValidPosition(v3s16 p)
	{
		if(data == NULL)
			return false;
		return (p.X >= 0 && p.X < MAP_BLOCKSIZE
				&& p.Y >= 0 && p.Y < MAP_BLOCKSIZE
				&& p.Z >= 0 && p.Z < MAP_BLOCKSIZE);
	}

	/*
		Regular MapNode get-setters
	*/
	
	MapNode getNode(s16 x, s16 y, s16 z)
	{
		if(data == NULL)
			throw InvalidPositionException();
		if(x < 0 || x >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(y < 0 || y >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(z < 0 || z >= MAP_BLOCKSIZE) throw InvalidPositionException();
		return data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x];
	}
	
	MapNode getNode(v3s16 p)
	{
		return getNode(p.X, p.Y, p.Z);
	}
	
	void setNode(s16 x, s16 y, s16 z, MapNode & n)
	{
		if(data == NULL)
			throw InvalidPositionException();
		if(x < 0 || x >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(y < 0 || y >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(z < 0 || z >= MAP_BLOCKSIZE) throw InvalidPositionException();
		data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x] = n;
		setChangedFlag();
	}
	
	void setNode(v3s16 p, MapNode & n)
	{
		setNode(p.X, p.Y, p.Z, n);
	}

	/*
		Non-checking variants of the above
	*/

	MapNode getNodeNoCheck(s16 x, s16 y, s16 z)
	{
		if(data == NULL)
			throw InvalidPositionException();
		return data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x];
	}
	
	MapNode getNodeNoCheck(v3s16 p)
	{
		return getNodeNoCheck(p.X, p.Y, p.Z);
	}
	
	void setNodeNoCheck(s16 x, s16 y, s16 z, MapNode & n)
	{
		if(data == NULL)
			throw InvalidPositionException();
		data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x] = n;
		setChangedFlag();
	}
	
	void setNodeNoCheck(v3s16 p, MapNode & n)
	{
		setNodeNoCheck(p.X, p.Y, p.Z, n);
	}

	/*
		These functions consult the parent container if the position
		is not valid on this MapBlock.
	*/
	bool isValidPositionParent(v3s16 p);
	MapNode getNodeParent(v3s16 p);
	void setNodeParent(v3s16 p, MapNode & n);

	void drawbox(s16 x0, s16 y0, s16 z0, s16 w, s16 h, s16 d, MapNode node)
	{
		for(u16 z=0; z<d; z++)
			for(u16 y=0; y<h; y++)
				for(u16 x=0; x<w; x++)
					setNode(x0+x, y0+y, z0+z, node);
	}

	static FastFace * makeFastFace(u16 tile, u8 light, v3f p,
			v3f dir, v3f scale, v3f posRelative_f);
	
	u8 getFaceLight(v3s16 p, v3s16 face_dir);
	
	u8 getNodeTile(v3s16 p);

	/*
		startpos:
		translate_dir: unit vector with only one of x, y or z
		face_dir: unit vector with only one of x, y or z
	*/
	void updateFastFaceRow(v3s16 startpos,
			u16 length,
			v3s16 translate_dir,
			v3s16 face_dir,
			core::list<FastFace*> &dest);

	void updateMesh();

	bool propagateSunlight(core::map<v3s16, bool> & light_sources);
	
	// Copies data to VoxelManipulator to getPosRelative()
	void copyTo(VoxelManipulator &dst);

	/*
		Object stuff
	*/
	
	void serializeObjects(std::ostream &os, u8 version)
	{
		m_objects.serialize(os, version);
	}
	// If smgr!=NULL, new objects are added to the scene
	void updateObjects(std::istream &is, u8 version,
			scene::ISceneManager *smgr)
	{
		m_objects.update(is, version, smgr);

		setChangedFlag();
	}
	void clearObjects()
	{
		m_objects.clear();

		setChangedFlag();
	}
	void addObject(MapBlockObject *object)
			throw(ContainerFullException, AlreadyExistsException)
	{
		m_objects.add(object);

		setChangedFlag();
	}
	void removeObject(s16 id)
	{
		m_objects.remove(id);

		setChangedFlag();
	}
	MapBlockObject * getObject(s16 id)
	{
		return m_objects.get(id);
	}
	JMutexAutoLock * getObjectLock()
	{
		return m_objects.getLock();
	}
	void stepObjects(float dtime, bool server)
	{
		m_objects.step(dtime, server);

		setChangedFlag();
	}

	/*void wrapObject(MapBlockObject *object)
	{
		m_objects.wrapObject(object);

		setChangedFlag();
	}*/

	// origin is relative to block
	void getObjects(v3f origin, f32 max_d,
			core::array<DistanceSortedObject> &dest)
	{
		m_objects.getObjects(origin, max_d, dest);
	}

	/*void getPseudoObjects(v3f origin, f32 max_d,
			core::array<DistanceSortedObject> &dest);*/

	s32 getObjectCount()
	{
		return m_objects.getCount();
	}

	/*
		Serialization
	*/
	
	// Doesn't write version by itself
	void serialize(std::ostream &os, u8 version);

	void deSerialize(std::istream &is, u8 version);

private:

	/*
		Used only internally, because changes can't be tracked
	*/

	MapNode & getNodeRef(s16 x, s16 y, s16 z)
	{
		if(x < 0 || x >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(y < 0 || y >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(z < 0 || z >= MAP_BLOCKSIZE) throw InvalidPositionException();
		return data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x];
	}
	MapNode & getNodeRef(v3s16 &p)
	{
		return getNodeRef(p.X, p.Y, p.Z);
	}

	
	NodeContainer *m_parent;
	// Position in blocks on parent
	v3s16 m_pos;
	/*
		If NULL, block is a dummy block.
		Dummy blocks are used for caching not-found-on-disk blocks.
	*/
	MapNode * data;
	/*
		- On the client, this is used for checking whether to
		  recalculate the face cache. (Is it anymore?)
		- On the server, this is used for telling whether the
		  block has been changed from the one on disk.
	*/
	bool changed;
	/*
		Used for some initial lighting stuff.
		At least /has been/ used. 8)
	*/
	bool is_underground;
	
	MapBlockObjectList m_objects;
	
};

inline bool blockpos_over_limit(v3s16 p)
{
	return
	  (p.X < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.X >  MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Y < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Y >  MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Z < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Z >  MAP_GENERATION_LIMIT / MAP_BLOCKSIZE);
}

/*
	Returns the position of the block where the node is located
*/
inline v3s16 getNodeBlockPos(v3s16 p)
{
	return getContainerPos(p, MAP_BLOCKSIZE);
}

inline v2s16 getNodeSectorPos(v2s16 p)
{
	return getContainerPos(p, MAP_BLOCKSIZE);
}

inline s16 getNodeBlockY(s16 y)
{
	return getContainerPos(y, MAP_BLOCKSIZE);
}

#endif

