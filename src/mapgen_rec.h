/*
Minetest
Copyright (C) 2016-2017 MillersMan <millersman@users.noreply.github.com>

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

#ifndef RECURSIVEMAPGEN_H
#define RECURSIVEMAPGEN_H

#include "mapgen.h"

#include <cassert>
#include <string>

enum RecursionBufferType
{
	RBT_Int, RBT_Float, RBT_Double, RBT_Vector2D
};

typedef u16 RecursionBufferId;
enum { InvalidRecursionBuffer = 0xFFFF };

class RecursionBuffer;

struct RecursionBuffer
{
	// Data is the pointer to the origin-point which isn't the first element.
	// Negative offsets are therefore allowed if they remain in the proclaimed range.
	void *data;
	int offset_x;
	int offset_z;
	int delta_z;
	int size;
};

class BufferIface
{
	friend class RecursionPass;
	friend class RecursiveMapgen;
public:
	BufferIface(RecursionBufferType type, bool input, bool output, bool split,
	            int scan_nx = 0, int scan_px = 0, int scan_nz = 0, int scan_pz = 0)
	    : m_binding(InvalidRecursionBuffer)
	    , m_type(type)
	    , m_input(input)
	    , m_output(output)
	    , m_splitted(split)
	    , m_scan_nx(scan_nx)
	    , m_scan_px(scan_px)
	    , m_scan_nz(scan_nz)
	    , m_scan_pz(scan_pz)
	{
	}
	virtual ~BufferIface() { }
	virtual void bind(const RecursionBuffer &buffer) = 0;
	RecursionBufferType type() const
	{
		return m_type;
	}
	bool isInput() const {
		return m_input;
	}
	bool isOutput() const {
		return m_output;
	}
	bool isSplitted() const {
		return m_splitted;
	}
	v2s16 scanRangeNeg() const {
		return v2s16(m_scan_nx, m_scan_nz);
	}
	v2s16 scanRangePos() const {
		return v2s16(m_scan_px, m_scan_pz);
	}
	RecursionBufferId binding() const
	{
		return m_binding;
	}
	void setBinding(RecursionBufferId id)
	{
		m_binding = id;
	}
protected:
	// Helper function
	template <typename T>
	inline static T flipped(const T &t)
	{
		return t;
	}
	template <typename T>
	inline static RecursionBufferType typeId();
protected:
	RecursionBufferId m_binding;
	RecursionBufferType m_type;
	bool m_input;
	bool m_output;
	bool m_splitted;
	// Those values are only used for preparation
	u8 m_scan_nx, m_scan_px, m_scan_nz, m_scan_pz;
	// Those values are only used for interactive use
	int offset; // Offset between origin and first fragment item
	int delta; // Offset between two rows
	int size;
};

template <>
inline core::vector2d<float> BufferIface::flipped(const core::vector2d<float> &v)
{
	return core::vector2d<float>(v.Y, v.X);
}

template <> inline RecursionBufferType BufferIface::typeId<int>() { return RBT_Int; }
template <> inline RecursionBufferType BufferIface::typeId<float>() { return RBT_Float; }
template <> inline RecursionBufferType BufferIface::typeId<double>() { return RBT_Double; }
template <> inline RecursionBufferType BufferIface::typeId<core::vector2d<float> >() { return RBT_Vector2D; }

template <typename T>
struct InputBuffer : public BufferIface
{
public:
	InputBuffer(int scan_nx = 0, int scan_px = 0, int scan_ny = 0, int scan_py = 0)
	    : BufferIface(typeId<T>(), true, false, false, scan_nx, scan_px, scan_ny, scan_py)
	    , data(NULL)
	{
	}
	void bind(const RecursionBuffer &buffer)
	{
		offset = buffer.offset_z * buffer.delta_z + buffer.offset_x;
		delta = buffer.delta_z;
		data = static_cast<T*>(buffer.data);
		size = buffer.size;
	}
	T get(int x, int z) const
	{
		int index = offset + z * delta + x;
		assert(index >= 0 && index < size);
		return *(data + index);
	}
private:
	const T * data;
};

template <typename T>
struct SplitInputBuffer : public BufferIface
{
public:
	SplitInputBuffer(int scan_nx, int scan_px, int scan_ny, int scan_py)
	    : BufferIface(typeId<T>(), true, false, true, scan_nx, scan_px, scan_ny, scan_py)
	    , left(NULL)
		, right(NULL)
	{
	}
	void bind(const RecursionBuffer &buffer)
	{
		offset = buffer.offset_x * buffer.delta_z + buffer.offset_z;
		delta = buffer.delta_z;
		left = static_cast<T*>(buffer.data);
		right = left + buffer.delta_z;
		size = buffer.size;
	}
	inline T getLeft(int x, int z) const
	{
		int index = offset + x * delta + z;
		assert(index >= 0 && index < size);
		return flipped(*(left + index));
	}
	inline T getRight(int x, int z) const
	{
		int index = offset + x * delta + z;
		assert(index >= 0 && index < size);
		return flipped(*(right + index));
	}
private:
	const T * left, * right;
};

template <typename T>
struct OutputBuffer : public BufferIface
{
public:
	OutputBuffer()
	    : BufferIface(typeId<T>(), false, true, false)
	    , data(NULL)
	{
	}
	void bind(const RecursionBuffer &buffer)
	{
		offset = buffer.offset_z * buffer.delta_z + buffer.offset_x;
		delta = buffer.delta_z;
		data = static_cast<T*>(buffer.data);
		size = buffer.size;
	}
	void set(int x, int z, const T & value)
	{
		int index = offset + z * delta + x;
		assert(index >= 0 && index < size);
		*(data + index) = value;
	}
private:
	T * data;
};

template <typename T>
struct SplitOutputBuffer : public BufferIface
{
public:
	SplitOutputBuffer()
	    : BufferIface(typeId<T>(), false, true, true)
	    , left(NULL)
		, right(NULL)
	{
	}
	void bind(const RecursionBuffer &buffer)
	{
		offset = buffer.offset_z * buffer.delta_z + buffer.offset_x;
		delta = buffer.delta_z * 2;
		left = static_cast<T*>(buffer.data);
		right = left + delta / 2;
		size = buffer.size;
	}
	void setLeft(int x, int z, const T & value) {
		int index = offset + x * delta + z;
		assert(index >= 0 && index < size);
		*(left + index) = flipped(value);
	}
	void setRight(int x, int z, const T & value) {
		int index = offset + x * delta + z;
		assert(index >= 0 && index < size);
		*(right + index) = flipped(value);
	}
private:
	T * left, * right;
};

template <typename T>
struct InOutBuffer : public BufferIface
{
public:
	InOutBuffer()
	    : BufferIface(typeId<T>(), true, true, false)
	    , data(NULL)
	{
	}
	void bind(const RecursionBuffer &buffer)
	{
		offset = buffer.offset_z * buffer.delta_z + buffer.offset_x;
		delta = buffer.delta_z;
		data = static_cast<T*>(buffer.data);
		size = buffer.size;
	}
	T get(int x, int z) const
	{
		int index = offset + z * delta + x;
		assert(index >= 0 && index < size);
		return data[index];
	}
	void set(int x, int z, const T & value)
	{
		int index = offset + z * delta + x;
		assert(index >= 0 && index < size);
		*(data + index) = value;
	}
private:
	T * data;
};

template <typename T>
struct SplitInOutBuffer : public BufferIface
{
public:
	SplitInOutBuffer()
	    : BufferIface(typeId<T>(), true, true, true)
	    , left(NULL)
		, right(NULL)
	{
	}
	void bind(const RecursionBuffer &buffer)
	{
		offset = buffer.offset_x * buffer.delta_z + buffer.offset_z;
		delta = buffer.delta_z;
		left = static_cast<T*>(buffer.data);
		right = left + delta / 2;
		size = buffer.size;
	}
	T getLeft(int x, int z) const
	{
		int index = offset + x * delta + z;
		assert(index >= 0 && index < size);
		return flipped(*(left + index));
	}
	void setLeft(int x, int z, const T & value)
	{
		int index = offset + x * delta + z;
		assert(index >= 0 && index < size);
		*(left + index) = flipped(value);
	}
	T getRight(int x, int z) const
	{
		int index = offset + x * delta + z;
		assert(index >= 0 && index < size);
		return flipped(*(right + index));
	}
	void setRight(int x, int z, const T & value)
	{
		int index = offset + x * delta + z;
		assert(index >= 0 && index < size);
		*(right + index) = flipped(value);
	}
private:
	T * left, * right;
};

class RecursionPass
{
	std::vector<BufferIface*> m_buffers;
public:
	virtual ~RecursionPass() { }
	const std::vector<BufferIface*>& buffers() const
	{
		return m_buffers;
	}
	virtual void doSplit(int min_x, int max_x, int min_z, int max_z, int level) = 0;
protected:
	void addBuffer(BufferIface *iface) {
		m_buffers.push_back(iface);
	}

	// Only for the mapgen itself
	// TODO Store this in the mapgen
	friend class RecursiveMapgen;
	int border_nx, border_px, border_nz, border_pz;
};

struct RecursionFragment
{
	int level, split, pos_x, pos_z;
	std::vector<RecursionBuffer> buffers;
};

class RecursiveMapgen : public Mapgen
{
public:
	RecursiveMapgen(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	void generateAndBindPosition(s16 x, s16 z);
	void generateTerrain();
	virtual void generateTerrainPatch() = 0;
	virtual void generateRootFragment() = 0;
protected:
	struct BufferInfo {
		u32 id;
		RecursionBufferType type;
		bool initialized; // Is there already a pass generating this buffer
		bool consumed; // Is there already a pass consuming this buffer
		bool splitted; // If temporary buffer should it be used as split or unsplit buffer
		u8 border_nx, border_px, border_nz, border_pz;
		u16 full_width, full_height;
		int buffer_size;
	};

	RecursionBufferId addBuffer(BufferIface *iface);
	RecursionBufferId addTemporaryBuffer(RecursionBufferType type, bool split);
	void addPass(RecursionPass *pass, std::vector<RecursionBufferId> binding);
	void finalize();
private:
	RecursionFragment * getFragment(s32 x, s32 z, int level = 0, int split = 0);
	RecursionFragment * createFragment(s32 x, s32 z, int level, int split);
	RecursionBuffer createBuffer(const BufferInfo &info);
	void positionBuffer(s32 x, s32 z, RecursionBuffer *buffer, const BufferInfo &info);
protected:
	int root_level;
	s16 frag_size;
	v2s32 grid_origin;
	v3s16 node_min;
	v3s16 node_max;
	v3s16 patch_min;
	v3s16 patch_max;
private:
	enum {
		BufferCacheFlag = 0x8000,
		BufferIndexMask = 0x7FFF
	};
	typedef std::vector<BufferInfo> BufferInfoList;
	BufferInfoList cache_buffer_infos;
	BufferInfoList temp_buffer_infos;
	typedef std::map<std::string, int> BufferLookup;
	BufferLookup buffer_lookup;
	typedef std::vector<BufferIface*> BufferIfaceList;
	BufferIfaceList buffer_ifaces;
	typedef std::vector<RecursionPass*> PassList;
	PassList passes;
	typedef std::vector<RecursionBuffer> BufferList;
	BufferList temp_buffers;
	typedef std::map<v2s32, RecursionFragment*> FragmentLevelCache;
	typedef std::vector<FragmentLevelCache> FragmentCache;
	FragmentCache cache;
	u8 border_nx, border_nz, border_px, border_pz;
};

/* ===== DEMO AREA ============ */

class NoisePass : public RecursionPass
{
public:
	NoisePass(int seed);
	void doSplit(int min_x, int max_x, int min_z, int max_z, int level) override;
private:
	int seed;
	InputBuffer<float> input;
	SplitOutputBuffer<float> output;
};


class Mapgen_Test : public RecursiveMapgen
{
	InOutBuffer<float> noise_buffer;
public:
	Mapgen_Test(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	MapgenType getType() const;
	void makeChunk(BlockMakeData * data);
	int getGroundLevelAtPoint(v2s16 p);
	int getSpawnLevelAtPoint(v2s16 p);
	void generateTerrainPatch();
	void generateRootFragment();
protected:
	NoisePass noise_pass;
	content_t c_stone;
};

#endif // RECURSIVEMAPGEN_H
