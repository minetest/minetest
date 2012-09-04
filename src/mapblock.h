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

#ifndef MAPBLOCK_HEADER
#define MAPBLOCK_HEADER

#include <jmutex.h>
#include <jmutexautolock.h>
#include <exception>
#include "debug.h"
#include "irrlichttypes.h"
#include "irr_v3d.h"
#include "irr_aabb3d.h"
#include "mapnode.h"
#include "exceptions.h"
#include "serialization.h"
#include "constants.h"
#include "voxel.h"
#include "staticobject.h"
#include "nodemetadata.h"
#include "nodetimer.h"
#include "modifiedstate.h"
#include "util/numeric.h" // getContainerPos

class Map;
class NodeMetadataList;
class IGameDef;
class MapBlockMesh;

#define BLOCK_TIMESTAMP_UNDEFINED 0xffffffff

/*// Named by looking towards z+
enum{
	FACE_BACK=0,
	FACE_TOP,
	FACE_RIGHT,
	FACE_FRONT,
	FACE_BOTTOM,
	FACE_LEFT
};*/

// NOTE: If this is enabled, set MapBlock to be initialized with
//       CONTENT_IGNORE.
/*enum BlockGenerationStatus
{
	// Completely non-generated (filled with CONTENT_IGNORE).
	BLOCKGEN_UNTOUCHED=0,
	// Trees or similar might have been blitted from other blocks to here.
	// Otherwise, the block contains CONTENT_IGNORE
	BLOCKGEN_FROM_NEIGHBORS=2,
	// Has been generated, but some neighbors might put some stuff in here
	// when they are generated.
	// Does not contain any CONTENT_IGNORE
	BLOCKGEN_SELF_GENERATED=4,
	// The block and all its neighbors have been generated
	BLOCKGEN_FULLY_GENERATED=6
};*/

#if 0
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

	MapNode getNodeNoEx(v3s16 p)
	{
		try{
			return getNode(p);
		}
		catch(InvalidPositionException &e){
			return MapNode(CONTENT_IGNORE);
		}
	}
};
#endif

/*
	MapBlock itself
*/

class MapBlock /*: public NodeContainer*/
{
public:
	MapBlock(Map *parent, v3s16 pos, IGameDef *gamedef, bool dummy=false);
	~MapBlock();
	
	/*virtual u16 nodeContainerId() const
	{
		return NODECONTAINER_ID_MAPBLOCK;
	}*/
	
	Map * getParent()
	{
		return m_parent;
	}

	void reallocate()
	{
		if(data != NULL)
			delete[] data;
		u32 l = MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE;
		data = new MapNode[l];
		for(u32 i=0; i<l; i++){
			//data[i] = MapNode();
			data[i] = MapNode(CONTENT_IGNORE);
		}
		raiseModified(MOD_STATE_WRITE_NEEDED, "reallocate");
	}

	/*
		Flags
	*/

	bool isDummy()
	{
		return (data == NULL);
	}
	void unDummify()
	{
		assert(isDummy());
		reallocate();
	}
	
	// m_modified methods
	void raiseModified(u32 mod, const std::string &reason="unknown")
	{
		if(mod > m_modified){
			m_modified = mod;
			m_modified_reason = reason;
			m_modified_reason_too_long = false;

			if(m_modified >= MOD_STATE_WRITE_AT_UNLOAD){
				m_disk_timestamp = m_timestamp;
			}
		} else if(mod == m_modified){
			if(!m_modified_reason_too_long){
				if(m_modified_reason.size() < 40)
					m_modified_reason += ", " + reason;
				else{
					m_modified_reason += "...";
					m_modified_reason_too_long = true;
				}
			}
		}
	}
	u32 getModified()
	{
		return m_modified;
	}
	std::string getModifiedReason()
	{
		return m_modified_reason;
	}
	void resetModified()
	{
		m_modified = MOD_STATE_CLEAN;
		m_modified_reason = "none";
		m_modified_reason_too_long = false;
	}
	
	// is_underground getter/setter
	bool getIsUnderground()
	{
		return is_underground;
	}
	void setIsUnderground(bool a_is_underground)
	{
		is_underground = a_is_underground;
		raiseModified(MOD_STATE_WRITE_NEEDED, "setIsUnderground");
	}

	void setLightingExpired(bool expired)
	{
		if(expired != m_lighting_expired){
			m_lighting_expired = expired;
			raiseModified(MOD_STATE_WRITE_NEEDED, "setLightingExpired");
		}
	}
	bool getLightingExpired()
	{
		return m_lighting_expired;
	}

	bool isGenerated()
	{
		return m_generated;
	}
	void setGenerated(bool b)
	{
		if(b != m_generated){
			raiseModified(MOD_STATE_WRITE_NEEDED, "setGenerated");
			m_generated = b;
		}
	}

	bool isValid()
	{
		if(m_lighting_expired)
			return false;
		if(data == NULL)
			return false;
		return true;
	}

	/*
		Position stuff
	*/

	v3s16 getPos()
	{
		return m_pos;
	}
		
	v3s16 getPosRelative()
	{
		return m_pos * MAP_BLOCKSIZE;
	}
		
	core::aabbox3d<s16> getBox()
	{
		return core::aabbox3d<s16>(getPosRelative(),
				getPosRelative()
				+ v3s16(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE)
				- v3s16(1,1,1));
	}

	/*
		Regular MapNode get-setters
	*/
	
	bool isValidPosition(v3s16 p)
	{
		if(data == NULL)
			return false;
		return (p.X >= 0 && p.X < MAP_BLOCKSIZE
				&& p.Y >= 0 && p.Y < MAP_BLOCKSIZE
				&& p.Z >= 0 && p.Z < MAP_BLOCKSIZE);
	}

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
	
	MapNode getNodeNoEx(v3s16 p)
	{
		try{
			return getNode(p.X, p.Y, p.Z);
		}catch(InvalidPositionException &e){
			return MapNode(CONTENT_IGNORE);
		}
	}
	
	void setNode(s16 x, s16 y, s16 z, MapNode & n)
	{
		if(data == NULL)
			throw InvalidPositionException();
		if(x < 0 || x >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(y < 0 || y >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(z < 0 || z >= MAP_BLOCKSIZE) throw InvalidPositionException();
		data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x] = n;
		raiseModified(MOD_STATE_WRITE_NEEDED, "setNode");
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
		raiseModified(MOD_STATE_WRITE_NEEDED, "setNodeNoCheck");
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
	MapNode getNodeParentNoEx(v3s16 p);

	void drawbox(s16 x0, s16 y0, s16 z0, s16 w, s16 h, s16 d, MapNode node)
	{
		for(u16 z=0; z<d; z++)
			for(u16 y=0; y<h; y++)
				for(u16 x=0; x<w; x++)
					setNode(x0+x, y0+y, z0+z, node);
	}

	// See comments in mapblock.cpp
	bool propagateSunlight(core::map<v3s16, bool> & light_sources,
			bool remove_light=false, bool *black_air_left=NULL);
	
	// Copies data to VoxelManipulator to getPosRelative()
	void copyTo(VoxelManipulator &dst);
	// Copies data from VoxelManipulator getPosRelative()
	void copyFrom(VoxelManipulator &dst);

	/*
		Update day-night lighting difference flag.
		Sets m_day_night_differs to appropriate value.
		These methods don't care about neighboring blocks.
	*/
	void actuallyUpdateDayNightDiff();
	/*
		Call this to schedule what the previous function does to be done
		when the value is actually needed.
	*/
	void expireDayNightDiff();

	bool getDayNightDiff()
	{
		if(m_day_night_differs_expired)
			actuallyUpdateDayNightDiff();
		return m_day_night_differs;
	}

	/*
		Miscellaneous stuff
	*/
	
	/*
		Tries to measure ground level.
		Return value:
			-1 = only air
			-2 = only ground
			-3 = random fail
			0...MAP_BLOCKSIZE-1 = ground level
	*/
	s16 getGroundLevel(v2s16 p2d);

	/*
		Timestamp (see m_timestamp)
		NOTE: BLOCK_TIMESTAMP_UNDEFINED=0xffffffff means there is no timestamp.
	*/
	void setTimestamp(u32 time)
	{
		m_timestamp = time;
		raiseModified(MOD_STATE_WRITE_AT_UNLOAD, "setTimestamp");
	}
	void setTimestampNoChangedFlag(u32 time)
	{
		m_timestamp = time;
	}
	u32 getTimestamp()
	{
		return m_timestamp;
	}
	u32 getDiskTimestamp()
	{
		return m_disk_timestamp;
	}
	
	/*
		See m_usage_timer
	*/
	void resetUsageTimer()
	{
		m_usage_timer = 0;
	}
	void incrementUsageTimer(float dtime)
	{
		m_usage_timer += dtime;
	}
	u32 getUsageTimer()
	{
		return m_usage_timer;
	}

	/*
		See m_refcount
	*/
	void refGrab()
	{
		m_refcount++;
	}
	void refDrop()
	{
		m_refcount--;
	}
	int refGet()
	{
		return m_refcount;
	}
	
	/*
		Node Timers
	*/
	// Get timer
	NodeTimer getNodeTimer(v3s16 p){ 
		return m_node_timers.get(p);
	}
	// Deletes timer
	void removeNodeTimer(v3s16 p){
		m_node_timers.remove(p);
	}
	// Deletes old timer and sets a new one
	void setNodeTimer(v3s16 p, NodeTimer t){
		m_node_timers.set(p,t);
	}
	// Deletes all timers
	void clearNodeTimers(){
		m_node_timers.clear();
	}

	/*
		Serialization
	*/
	
	// These don't write or read version by itself
	// Set disk to true for on-disk format, false for over-the-network format
	void serialize(std::ostream &os, u8 version, bool disk);
	// If disk == true: In addition to doing other things, will add
	// unknown blocks from id-name mapping to wndef
	void deSerialize(std::istream &is, u8 version, bool disk);

private:
	/*
		Private methods
	*/

	void deSerialize_pre22(std::istream &is, u8 version, bool disk);

	/*
		Used only internally, because changes can't be tracked
	*/

	MapNode & getNodeRef(s16 x, s16 y, s16 z)
	{
		if(data == NULL)
			throw InvalidPositionException();
		if(x < 0 || x >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(y < 0 || y >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(z < 0 || z >= MAP_BLOCKSIZE) throw InvalidPositionException();
		return data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x];
	}
	MapNode & getNodeRef(v3s16 &p)
	{
		return getNodeRef(p.X, p.Y, p.Z);
	}

public:
	/*
		Public member variables
	*/

#ifndef SERVER // Only on client
	MapBlockMesh *mesh;
	//JMutex mesh_mutex;
#endif
	
	NodeMetadataList m_node_metadata;
	NodeTimerList m_node_timers;
	StaticObjectList m_static_objects;
	
private:
	/*
		Private member variables
	*/

	// NOTE: Lots of things rely on this being the Map
	Map *m_parent;
	// Position in blocks on parent
	v3s16 m_pos;

	IGameDef *m_gamedef;
	
	/*
		If NULL, block is a dummy block.
		Dummy blocks are used for caching not-found-on-disk blocks.
	*/
	MapNode * data;

	/*
		- On the server, this is used for telling whether the
		  block has been modified from the one on disk.
		- On the client, this is used for nothing.
	*/
	u32 m_modified;
	std::string m_modified_reason;
	bool m_modified_reason_too_long;

	/*
		When propagating sunlight and the above block doesn't exist,
		sunlight is assumed if this is false.

		In practice this is set to true if the block is completely
		undeground with nothing visible above the ground except
		caves.
	*/
	bool is_underground;

	/*
		Set to true if changes has been made that make the old lighting
		values wrong but the lighting hasn't been actually updated.

		If this is false, lighting is exactly right.
		If this is true, lighting might be wrong or right.
	*/
	bool m_lighting_expired;
	
	// Whether day and night lighting differs
	bool m_day_night_differs;
	bool m_day_night_differs_expired;

	bool m_generated;
	
	/*
		When block is removed from active blocks, this is set to gametime.
		Value BLOCK_TIMESTAMP_UNDEFINED=0xffffffff means there is no timestamp.
	*/
	u32 m_timestamp;
	// The on-disk (or to-be on-disk) timestamp value
	u32 m_disk_timestamp;

	/*
		When the block is accessed, this is set to 0.
		Map will unload the block when this reaches a timeout.
	*/
	float m_usage_timer;

	/*
		Reference count; currently used for determining if this block is in
		the list of blocks to be drawn.
	*/
	int m_refcount;
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

/*
	Get a quick string to describe what a block actually contains
*/
std::string analyze_block(MapBlock *block);

#endif

