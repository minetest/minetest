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

#pragma once

#include <set>
#include "irr_v3d.h"
#include "mapnode.h"
#include "exceptions.h"
#include "constants.h"
#include "staticobject.h"
#include "nodemetadata.h"
#include "nodetimer.h"
#include "modifiedstate.h"
#include "util/numeric.h" // getContainerPos
#include "settings.h"
#include "mapgen/mapgen.h"

class Map;
class NodeMetadataList;
class IGameDef;
class MapBlockMesh;
class VoxelManipulator;

#define BLOCK_TIMESTAMP_UNDEFINED 0xffffffff

////
//// MapBlock modified reason flags
////

#define MOD_REASON_INITIAL                   (1 << 0)
#define MOD_REASON_REALLOCATE                (1 << 1)
#define MOD_REASON_SET_IS_UNDERGROUND        (1 << 2)
#define MOD_REASON_SET_LIGHTING_COMPLETE     (1 << 3)
#define MOD_REASON_SET_GENERATED             (1 << 4)
#define MOD_REASON_SET_NODE                  (1 << 5)
#define MOD_REASON_SET_NODE_NO_CHECK         (1 << 6)
#define MOD_REASON_SET_TIMESTAMP             (1 << 7)
#define MOD_REASON_REPORT_META_CHANGE        (1 << 8)
#define MOD_REASON_CLEAR_ALL_OBJECTS         (1 << 9)
#define MOD_REASON_BLOCK_EXPIRED             (1 << 10)
#define MOD_REASON_ADD_ACTIVE_OBJECT_RAW     (1 << 11)
#define MOD_REASON_REMOVE_OBJECTS_REMOVE     (1 << 12)
#define MOD_REASON_REMOVE_OBJECTS_DEACTIVATE (1 << 13)
#define MOD_REASON_TOO_MANY_OBJECTS          (1 << 14)
#define MOD_REASON_STATIC_DATA_ADDED         (1 << 15)
#define MOD_REASON_STATIC_DATA_REMOVED       (1 << 16)
#define MOD_REASON_STATIC_DATA_CHANGED       (1 << 17)
#define MOD_REASON_EXPIRE_DAYNIGHTDIFF       (1 << 18)
#define MOD_REASON_VMANIP                    (1 << 19)
#define MOD_REASON_UNKNOWN                   (1 << 20)

////
//// MapBlock itself
////

class MapBlock
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
		delete[] data;
		data = new MapNode[nodecount];
		for (u32 i = 0; i < nodecount; i++)
			data[i] = MapNode(CONTENT_IGNORE);

		raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_REALLOCATE);
	}

	MapNode* getData()
	{
		return data;
	}

	////
	//// Modification tracking methods
	////
	void raiseModified(u32 mod, u32 reason=MOD_REASON_UNKNOWN)
	{
		if (mod > m_modified) {
			m_modified = mod;
			m_modified_reason = reason;
			if (m_modified >= MOD_STATE_WRITE_AT_UNLOAD)
				m_disk_timestamp = m_timestamp;
		} else if (mod == m_modified) {
			m_modified_reason |= reason;
		}
		if (mod == MOD_STATE_WRITE_NEEDED)
			contents_cached = false;
	}

	inline u32 getModified()
	{
		return m_modified;
	}

	inline u32 getModifiedReason()
	{
		return m_modified_reason;
	}

	std::string getModifiedReasonString();

	inline void resetModified()
	{
		m_modified = MOD_STATE_CLEAN;
		m_modified_reason = 0;
	}

	////
	//// Flags
	////

	inline bool isDummy() const
	{
		return !data;
	}

	inline void unDummify()
	{
		assert(isDummy()); // Pre-condition
		reallocate();
	}

	// is_underground getter/setter
	inline bool getIsUnderground()
	{
		return is_underground;
	}

	inline void setIsUnderground(bool a_is_underground)
	{
		is_underground = a_is_underground;
		raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_SET_IS_UNDERGROUND);
	}

	inline void setLightingComplete(u16 newflags)
	{
		if (newflags != m_lighting_complete) {
			m_lighting_complete = newflags;
			raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_SET_LIGHTING_COMPLETE);
		}
	}

	inline u16 getLightingComplete()
	{
		return m_lighting_complete;
	}

	inline void setLightingComplete(LightBank bank, u8 direction,
		bool is_complete)
	{
		assert(direction >= 0 && direction <= 5);
		if (bank == LIGHTBANK_NIGHT) {
			direction += 6;
		}
		u16 newflags = m_lighting_complete;
		if (is_complete) {
			newflags |= 1 << direction;
		} else {
			newflags &= ~(1 << direction);
		}
		setLightingComplete(newflags);
	}

	inline bool isLightingComplete(LightBank bank, u8 direction)
	{
		assert(direction >= 0 && direction <= 5);
		if (bank == LIGHTBANK_NIGHT) {
			direction += 6;
		}
		return (m_lighting_complete & (1 << direction)) != 0;
	}

	inline bool isGenerated()
	{
		return m_generated;
	}

	inline void setGenerated(bool b)
	{
		if (b != m_generated) {
			raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_SET_GENERATED);
			m_generated = b;
		}
	}

	////
	//// Position stuff
	////

	inline v3s16 getPos()
	{
		return m_pos;
	}

	inline v3s16 getPosRelative()
	{
		return m_pos_relative;
	}

	inline core::aabbox3d<s16> getBox()
	{
		return core::aabbox3d<s16>(getPosRelative(),
				getPosRelative()
				+ v3s16(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE)
				- v3s16(1,1,1));
	}

	////
	//// Regular MapNode get-setters
	////

	inline bool isValidPosition(s16 x, s16 y, s16 z)
	{
		return data
			&& x >= 0 && x < MAP_BLOCKSIZE
			&& y >= 0 && y < MAP_BLOCKSIZE
			&& z >= 0 && z < MAP_BLOCKSIZE;
	}

	inline bool isValidPosition(v3s16 p)
	{
		return isValidPosition(p.X, p.Y, p.Z);
	}

	inline MapNode getNode(s16 x, s16 y, s16 z, bool *valid_position)
	{
		*valid_position = isValidPosition(x, y, z);

		if (!*valid_position)
			return {CONTENT_IGNORE};

		return data[z * zstride + y * ystride + x];
	}

	inline MapNode getNode(v3s16 p, bool *valid_position)
	{
		return getNode(p.X, p.Y, p.Z, valid_position);
	}

	inline MapNode getNodeNoEx(v3s16 p)
	{
		bool is_valid;
		return getNode(p.X, p.Y, p.Z, &is_valid);
	}

	inline void setNode(s16 x, s16 y, s16 z, MapNode & n)
	{
		if (!isValidPosition(x, y, z))
			throw InvalidPositionException();

		data[z * zstride + y * ystride + x] = n;
		raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_SET_NODE);
	}

	inline void setNode(v3s16 p, MapNode & n)
	{
		setNode(p.X, p.Y, p.Z, n);
	}

	////
	//// Non-checking variants of the above
	////

	inline MapNode getNodeNoCheck(s16 x, s16 y, s16 z, bool *valid_position)
	{
		*valid_position = data != nullptr;
		if (!*valid_position)
			return {CONTENT_IGNORE};

		return data[z * zstride + y * ystride + x];
	}

	inline MapNode getNodeNoCheck(v3s16 p, bool *valid_position)
	{
		return getNodeNoCheck(p.X, p.Y, p.Z, valid_position);
	}

	////
	//// Non-checking, unsafe variants of the above
	//// MapBlock must be loaded by another function in the same scope/function
	//// Caller must ensure that this is not a dummy block (by calling isDummy())
	////

	inline const MapNode &getNodeUnsafe(s16 x, s16 y, s16 z)
	{
		return data[z * zstride + y * ystride + x];
	}

	inline const MapNode &getNodeUnsafe(v3s16 &p)
	{
		return getNodeUnsafe(p.X, p.Y, p.Z);
	}

	inline void setNodeNoCheck(s16 x, s16 y, s16 z, MapNode & n)
	{
		if (!data)
			throw InvalidPositionException();

		data[z * zstride + y * ystride + x] = n;
		raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_SET_NODE_NO_CHECK);
	}

	inline void setNodeNoCheck(v3s16 p, MapNode & n)
	{
		setNodeNoCheck(p.X, p.Y, p.Z, n);
	}

	// These functions consult the parent container if the position
	// is not valid on this MapBlock.
	bool isValidPositionParent(v3s16 p);
	MapNode getNodeParent(v3s16 p, bool *is_valid_position = NULL);

	// Copies data to VoxelManipulator to getPosRelative()
	void copyTo(VoxelManipulator &dst);

	// Copies data from VoxelManipulator getPosRelative()
	void copyFrom(VoxelManipulator &dst);

	// Update day-night lighting difference flag.
	// Sets m_day_night_differs to appropriate value.
	// These methods don't care about neighboring blocks.
	void actuallyUpdateDayNightDiff();

	// Call this to schedule what the previous function does to be done
	// when the value is actually needed.
	void expireDayNightDiff();

	inline bool getDayNightDiff()
	{
		if (m_day_night_differs_expired)
			actuallyUpdateDayNightDiff();
		return m_day_night_differs;
	}

	////
	//// Timestamp (see m_timestamp)
	////

	// NOTE: BLOCK_TIMESTAMP_UNDEFINED=0xffffffff means there is no timestamp.

	inline void setTimestamp(u32 time)
	{
		m_timestamp = time;
		raiseModified(MOD_STATE_WRITE_AT_UNLOAD, MOD_REASON_SET_TIMESTAMP);
	}

	inline void setTimestampNoChangedFlag(u32 time)
	{
		m_timestamp = time;
	}

	inline u32 getTimestamp()
	{
		return m_timestamp;
	}

	inline u32 getDiskTimestamp()
	{
		return m_disk_timestamp;
	}

	////
	//// Usage timer (see m_usage_timer)
	////

	inline void resetUsageTimer()
	{
		m_usage_timer = 0;
	}

	inline void incrementUsageTimer(float dtime)
	{
		m_usage_timer += dtime;
	}

	inline float getUsageTimer()
	{
		return m_usage_timer;
	}

	////
	//// Reference counting (see m_refcount)
	////

	inline void refGrab()
	{
		m_refcount++;
	}

	inline void refDrop()
	{
		m_refcount--;
	}

	inline int refGet()
	{
		return m_refcount;
	}

	////
	//// Node Timers
	////

	inline NodeTimer getNodeTimer(const v3s16 &p)
	{
		return m_node_timers.get(p);
	}

	inline void removeNodeTimer(const v3s16 &p)
	{
		m_node_timers.remove(p);
	}

	inline void setNodeTimer(const NodeTimer &t)
	{
		m_node_timers.set(t);
	}

	inline void clearNodeTimers()
	{
		m_node_timers.clear();
	}

	////
	//// Serialization
	///

	// These don't write or read version by itself
	// Set disk to true for on-disk format, false for over-the-network format
	// Precondition: version >= SER_FMT_VER_LOWEST_WRITE
	void serialize(std::ostream &result, u8 version, bool disk, int compression_level);
	// If disk == true: In addition to doing other things, will add
	// unknown blocks from id-name mapping to wndef
	void deSerialize(std::istream &is, u8 version, bool disk);

	void serializeNetworkSpecific(std::ostream &os);
	void deSerializeNetworkSpecific(std::istream &is);
private:
	/*
		Private methods
	*/

	void deSerialize_pre22(std::istream &is, u8 version, bool disk);

	/*
		Used only internally, because changes can't be tracked
	*/

	inline MapNode &getNodeRef(s16 x, s16 y, s16 z)
	{
		if (!isValidPosition(x, y, z))
			throw InvalidPositionException();

		return data[z * zstride + y * ystride + x];
	}

	inline MapNode &getNodeRef(v3s16 &p)
	{
		return getNodeRef(p.X, p.Y, p.Z);
	}

public:
	/*
		Public member variables
	*/

#ifndef SERVER // Only on client
	MapBlockMesh *mesh = nullptr;
#endif

	NodeMetadataList m_node_metadata;
	NodeTimerList m_node_timers;
	StaticObjectList m_static_objects;

	static const u32 ystride = MAP_BLOCKSIZE;
	static const u32 zstride = MAP_BLOCKSIZE * MAP_BLOCKSIZE;

	static const u32 nodecount = MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE;

	//// ABM optimizations ////
	// Cache of content types
	std::unordered_set<content_t> contents;
	// True if content types are cached
	bool contents_cached = false;
	// True if we never want to cache content types for this block
	bool do_not_cache_contents = false;

private:
	/*
		Private member variables
	*/

	// NOTE: Lots of things rely on this being the Map
	Map *m_parent;
	// Position in blocks on parent
	v3s16 m_pos;

	/* This is the precalculated m_pos_relative value
	* This caches the value, improving performance by removing 3 s16 multiplications
	* at runtime on each getPosRelative call
	* For a 5 minutes runtime with valgrind this removes 3 * 19M s16 multiplications
	* The gain can be estimated in Release Build to 3 * 100M multiply operations for 5 mins
	*/
	v3s16 m_pos_relative;

	IGameDef *m_gamedef;

	/*
		If NULL, block is a dummy block.
		Dummy blocks are used for caching not-found-on-disk blocks.
	*/
	MapNode *data = nullptr;

	/*
		- On the server, this is used for telling whether the
		  block has been modified from the one on disk.
		- On the client, this is used for nothing.
	*/
	u32 m_modified = MOD_STATE_WRITE_NEEDED;
	u32 m_modified_reason = MOD_REASON_INITIAL;

	/*
		When propagating sunlight and the above block doesn't exist,
		sunlight is assumed if this is false.

		In practice this is set to true if the block is completely
		undeground with nothing visible above the ground except
		caves.
	*/
	bool is_underground = false;

	/*!
	 * Each bit indicates if light spreading was finished
	 * in a direction. (Because the neighbor could also be unloaded.)
	 * Bits (most significant first):
	 * nothing,  nothing,  nothing,  nothing,
	 * night X-, night Y-, night Z-, night Z+, night Y+, night X+,
	 * day X-,   day Y-,   day Z-,   day Z+,   day Y+,   day X+.
	*/
	u16 m_lighting_complete = 0xFFFF;

	// Whether day and night lighting differs
	bool m_day_night_differs = false;
	bool m_day_night_differs_expired = true;

	bool m_generated = false;

	/*
		When block is removed from active blocks, this is set to gametime.
		Value BLOCK_TIMESTAMP_UNDEFINED=0xffffffff means there is no timestamp.
	*/
	u32 m_timestamp = BLOCK_TIMESTAMP_UNDEFINED;
	// The on-disk (or to-be on-disk) timestamp value
	u32 m_disk_timestamp = BLOCK_TIMESTAMP_UNDEFINED;

	/*
		When the block is accessed, this is set to 0.
		Map will unload the block when this reaches a timeout.
	*/
	float m_usage_timer = 0;

	/*
		Reference count; currently used for determining if this block is in
		the list of blocks to be drawn.
	*/
	int m_refcount = 0;
};

typedef std::vector<MapBlock*> MapBlockVect;

inline bool objectpos_over_limit(v3f p)
{
	const float max_limit_bs = (MAX_MAP_GENERATION_LIMIT + 0.5f) * BS;
	return p.X < -max_limit_bs ||
		p.X >  max_limit_bs ||
		p.Y < -max_limit_bs ||
		p.Y >  max_limit_bs ||
		p.Z < -max_limit_bs ||
		p.Z >  max_limit_bs;
}

inline bool blockpos_over_max_limit(v3s16 p)
{
	const s16 max_limit_bp = MAX_MAP_GENERATION_LIMIT / MAP_BLOCKSIZE;
	return p.X < -max_limit_bp ||
		p.X >  max_limit_bp ||
		p.Y < -max_limit_bp ||
		p.Y >  max_limit_bp ||
		p.Z < -max_limit_bp ||
		p.Z >  max_limit_bp;
}

/*
	Returns the position of the block where the node is located
*/
inline v3s16 getNodeBlockPos(const v3s16 &p)
{
	return getContainerPos(p, MAP_BLOCKSIZE);
}

inline void getNodeBlockPosWithOffset(const v3s16 &p, v3s16 &block, v3s16 &offset)
{
	getContainerPosWithOffset(p, MAP_BLOCKSIZE, block, offset);
}

/*
	Get a quick string to describe what a block actually contains
*/
std::string analyze_block(MapBlock *block);
