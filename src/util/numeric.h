// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "basic_macros.h"
#include "constants.h"
#include "irrlichttypes.h"
#include "irr_v2d.h"
#include "irr_v3d.h"
#include "irr_aabb3d.h"
#include "SColor.h"
#include <matrix4.h>
#include <cmath>
#include <algorithm>
#include <climits>
#if _MSC_VER
#include <intrin.h> // For popcnt
#endif

#define rangelim(d, min, max) ((d) < (min) ? (min) : ((d) > (max) ? (max) : (d)))
#define myfloor(x) ((x) < 0.0 ? (int)(x) - 1 : (int)(x))
// The naive swap performs better than the xor version
#define SWAP(t, x, y) do { \
	t temp = x; \
	x = y; \
	y = temp; \
} while (0)

// Maximum radius of a block.  The magic number is
// sqrt(3.0) / 2.0 in literal form.
static constexpr const f32 BLOCK_MAX_RADIUS = 0.866025403784f * MAP_BLOCKSIZE * BS;

inline s16 getContainerPos(s16 p, s16 d)
{
	return (p >= 0 ? p : p - d + 1) / d;
}

inline v2s16 getContainerPos(v2s16 p, s16 d)
{
	return v2s16(
		getContainerPos(p.X, d),
		getContainerPos(p.Y, d)
	);
}

inline v3s16 getContainerPos(v3s16 p, s16 d)
{
	return v3s16(
		getContainerPos(p.X, d),
		getContainerPos(p.Y, d),
		getContainerPos(p.Z, d)
	);
}

inline v2s16 getContainerPos(v2s16 p, v2s16 d)
{
	return v2s16(
		getContainerPos(p.X, d.X),
		getContainerPos(p.Y, d.Y)
	);
}

inline v3s16 getContainerPos(v3s16 p, v3s16 d)
{
	return v3s16(
		getContainerPos(p.X, d.X),
		getContainerPos(p.Y, d.Y),
		getContainerPos(p.Z, d.Z)
	);
}

inline void getContainerPosWithOffset(s16 p, s16 d, s16 &container, s16 &offset)
{
	container = (p >= 0 ? p : p - d + 1) / d;
	offset = p & (d - 1);
}

inline void getContainerPosWithOffset(const v2s16 &p, s16 d, v2s16 &container, v2s16 &offset)
{
	getContainerPosWithOffset(p.X, d, container.X, offset.X);
	getContainerPosWithOffset(p.Y, d, container.Y, offset.Y);
}

inline void getContainerPosWithOffset(const v3s16 &p, s16 d, v3s16 &container, v3s16 &offset)
{
	getContainerPosWithOffset(p.X, d, container.X, offset.X);
	getContainerPosWithOffset(p.Y, d, container.Y, offset.Y);
	getContainerPosWithOffset(p.Z, d, container.Z, offset.Z);
}


inline bool isInArea(v3s16 p, s16 d)
{
	return (
		p.X >= 0 && p.X < d &&
		p.Y >= 0 && p.Y < d &&
		p.Z >= 0 && p.Z < d
	);
}

inline bool isInArea(v2s16 p, s16 d)
{
	return (
		p.X >= 0 && p.X < d &&
		p.Y >= 0 && p.Y < d
	);
}

inline bool isInArea(v3s16 p, v3s16 d)
{
	return (
		p.X >= 0 && p.X < d.X &&
		p.Y >= 0 && p.Y < d.Y &&
		p.Z >= 0 && p.Z < d.Z
	);
}

inline void sortBoxVerticies(v3s16 &p1, v3s16 &p2) {
	if (p1.X > p2.X)
		SWAP(s16, p1.X, p2.X);
	if (p1.Y > p2.Y)
		SWAP(s16, p1.Y, p2.Y);
	if (p1.Z > p2.Z)
		SWAP(s16, p1.Z, p2.Z);
}

inline v3s16 componentwise_min(const v3s16 &a, const v3s16 &b)
{
	return v3s16(MYMIN(a.X, b.X), MYMIN(a.Y, b.Y), MYMIN(a.Z, b.Z));
}

inline v3s16 componentwise_max(const v3s16 &a, const v3s16 &b)
{
	return v3s16(MYMAX(a.X, b.X), MYMAX(a.Y, b.Y), MYMAX(a.Z, b.Z));
}

/// @brief Describes a grid with given step, oirginating at (0,0,0)
struct MeshGrid {
	u16 cell_size;

	u32 getCellVolume() const { return cell_size * cell_size * cell_size; }

	/// @brief returns coordinate of mesh cell given coordinate of a map block
	s16 getCellPos(s16 p) const
	{
		return (p - (p < 0) * (cell_size - 1)) / cell_size;
	}

	/// @brief returns position of mesh cell in the grid given position of a map block
	v3s16 getCellPos(v3s16 block_pos) const
	{
		return v3s16(getCellPos(block_pos.X), getCellPos(block_pos.Y), getCellPos(block_pos.Z));
	}

	/// @brief returns closest step of the grid smaller than p
	s16 getMeshPos(s16 p) const
	{
		return getCellPos(p) * cell_size;
	}

	/// @brief Returns coordinates of the origin of the grid cell containing p
	v3s16 getMeshPos(v3s16 p) const
	{
		return v3s16(getMeshPos(p.X), getMeshPos(p.Y), getMeshPos(p.Z));
	}

	/// @brief Returns true if p is an origin of a cell in the grid.
	bool isMeshPos(v3s16 &p) const
	{
		return p.X % cell_size == 0
				&& p.Y % cell_size == 0
				&& p.Z % cell_size == 0;
	}

	/// @brief Returns index of the given offset in a grid cell
	/// All offset coordinates must be smaller than the size of the cell
	u16 getOffsetIndex(v3s16 offset) const
	{
		return (offset.Z * cell_size + offset.Y) * cell_size + offset.X;
	}
};

/** Returns \p f wrapped to the range [-360, 360]
 *
 *  See test.cpp for example cases.
 *
 *  \note This is also used in cases where degrees wrapped to the range [0, 360]
 *  is innapropriate (e.g. pitch needs negative values)
 */
inline float modulo360f(float f)
{
	return fmodf(f, 360.0f);
}


/** Returns \p f wrapped to the range [0, 360]
  */
inline float wrapDegrees_0_360(float f)
{
	float value = modulo360f(f);
	return value < 0 ? value + 360 : value;
}


/** Returns \p v3f wrapped to the range [0, 360]
  */
inline v3f wrapDegrees_0_360_v3f(v3f v)
{
	v3f value_v3f;
	value_v3f.X = modulo360f(v.X);
	value_v3f.Y = modulo360f(v.Y);
	value_v3f.Z = modulo360f(v.Z);

	// Now that values are wrapped, use to get values for certain ranges
	value_v3f.X = value_v3f.X < 0 ? value_v3f.X + 360 : value_v3f.X;
	value_v3f.Y = value_v3f.Y < 0 ? value_v3f.Y + 360 : value_v3f.Y;
	value_v3f.Z = value_v3f.Z < 0 ? value_v3f.Z + 360 : value_v3f.Z;
	return value_v3f;
}


/** Returns \p f wrapped to the range [-180, 180]
  */
inline float wrapDegrees_180(float f)
{
	float value = modulo360f(f + 180);
	if (value < 0)
		value += 360;
	return value - 180;
}

/*
	Pseudo-random (VC++ rand() sucks)
*/
#define MYRAND_RANGE 0xffffffff
u32 myrand();
void mysrand(unsigned int seed);
void myrand_bytes(void *out, size_t len);
int myrand_range(int min, int max);
float myrand_range(float min, float max);
float myrand_float();

// Implements a C++11 UniformRandomBitGenerator using the above functions
struct MyRandGenerator {
	typedef u32 result_type;
	static constexpr result_type min() { return 0; }
	static constexpr result_type max() { return MYRAND_RANGE; }
	inline result_type operator()() {
		return myrand();
	}
};

/*
	Miscellaneous functions
*/

inline u32 get_bits(u32 x, u32 pos, u32 len)
{
	u32 mask = (1 << len) - 1;
	return (x >> pos) & mask;
}

inline void set_bits(u32 *x, u32 pos, u32 len, u32 val)
{
	u32 mask = (1 << len) - 1;
	*x &= ~(mask << pos);
	*x |= (val & mask) << pos;
}

inline unsigned popcount(unsigned b)
{
#if _MSC_VER
	return __popcnt(b);
#else
	return __builtin_popcount(b);
#endif
}

template<typename T>
class BitField {
public:
	static_assert(static_cast<T>(-1) > static_cast<T>(0),
			"Bit fields only work with unsigned integer types");

	BitField(): m_mask(0), m_offset(0) {}

	BitField(u8 width, u8 offset)
	{
		// Truncate to the type width to avoid undefined behavior.
		unsigned bit_count = sizeof(T) * CHAR_BIT;
		offset = MYMIN(offset, bit_count);
		width = MYMIN(width, bit_count - offset);
		m_mask = ((width < bit_count ? 1 << width : 0) - 1) << (offset % bit_count);
		m_offset = offset % bit_count;
	}

	inline u8 getWidth() const { return popcount(m_mask); }
	inline u8 getOffset() const { return m_offset; }

	inline T get(T bits) const
	{
		return (bits & m_mask) >> m_offset;
	}

	inline T set(T bits, T value) const
	{
		bits &= ~m_mask;
		bits |= (value << m_offset) & m_mask;
		return bits;
	}

private:
	T m_mask;
	u8 m_offset;
};

inline u32 calc_parity(u32 v)
{
	v ^= v >> 16;
	v ^= v >> 8;
	v ^= v >> 4;
	v &= 0xf;
	return (0x6996 >> v) & 1;
}

u64 murmur_hash_64_ua(const void *key, int len, unsigned int seed);

bool isBlockInSight(v3s16 blockpos_b, v3f camera_pos, v3f camera_dir,
		f32 camera_fov, f32 range, f32 *distance_ptr=NULL);

s16 adjustDist(s16 dist, float zoom_fov);

/*
	Returns nearest 32-bit integer for given floating point number.
	<cmath> and <math.h> in VC++ don't provide round().
*/
inline s32 myround(f32 f)
{
	return (s32)(f < 0.f ? (f - 0.5f) : (f + 0.5f));
}

inline constexpr f32 sqr(f32 f)
{
	return f * f;
}

/*
	Returns integer position of node in given floating point position
*/
inline v3s16 floatToInt(v3f p, f32 d)
{
	return v3s16(
		(p.X + (p.X > 0 ? d / 2 : -d / 2)) / d,
		(p.Y + (p.Y > 0 ? d / 2 : -d / 2)) / d,
		(p.Z + (p.Z > 0 ? d / 2 : -d / 2)) / d);
}

/*
	Returns integer position of node in given double precision position
 */
inline v3s16 doubleToInt(v3d p, double d)
{
	return v3s16(
		(p.X + (p.X > 0 ? d / 2 : -d / 2)) / d,
		(p.Y + (p.Y > 0 ? d / 2 : -d / 2)) / d,
		(p.Z + (p.Z > 0 ? d / 2 : -d / 2)) / d);
}

/*
	Returns floating point position of node in given integer position
*/
inline v3f intToFloat(v3s16 p, f32 d)
{
	return v3f(
		(f32)p.X * d,
		(f32)p.Y * d,
		(f32)p.Z * d
	);
}

// Random helper. Usually d=BS
inline aabb3f getNodeBox(v3s16 p, float d)
{
	return aabb3f(
		(float)p.X * d - 0.5f * d,
		(float)p.Y * d - 0.5f * d,
		(float)p.Z * d - 0.5f * d,
		(float)p.X * d + 0.5f * d,
		(float)p.Y * d + 0.5f * d,
		(float)p.Z * d + 0.5f * d
	);
}


class IntervalLimiter
{
public:
	IntervalLimiter() = default;

	/**
		@param dtime time from last call to this method
		@param wanted_interval interval wanted
		@return true if action should be done
	*/
	bool step(float dtime, float wanted_interval)
	{
		m_accumulator += dtime;
		if (m_accumulator < wanted_interval)
			return false;
		m_accumulator -= wanted_interval;
		return true;
	}

private:
	float m_accumulator = 0.0f;
};


/*
	Splits a list into "pages". For example, the list [1,2,3,4,5] split
	into two pages would be [1,2,3],[4,5]. This function computes the
	minimum and maximum indices of a single page.

	length: Length of the list that should be split
	page: Page number, 1 <= page <= pagecount
	pagecount: The number of pages, >= 1
	minindex: Receives the minimum index (inclusive).
	maxindex: Receives the maximum index (exclusive).

	Ensures 0 <= minindex <= maxindex <= length.
*/
inline void paging(u32 length, u32 page, u32 pagecount, u32 &minindex, u32 &maxindex)
{
	if (length < 1 || pagecount < 1 || page < 1 || page > pagecount) {
		// Special cases or invalid parameters
		minindex = maxindex = 0;
	} else if(pagecount <= length) {
		// Less pages than entries in the list:
		// Each page contains at least one entry
		minindex = (length * (page-1) + (pagecount-1)) / pagecount;
		maxindex = (length * page + (pagecount-1)) / pagecount;
	} else {
		// More pages than entries in the list:
		// Make sure the empty pages are at the end
		if (page < length) {
			minindex = page-1;
			maxindex = page;
		} else {
			minindex = 0;
			maxindex = 0;
		}
	}
}

inline float cycle_shift(float value, float by = 0, float max = 1)
{
	if (value + by < 0)   return value + by + max;
	if (value + by > max) return value + by - max;
	return value + by;
}

inline bool is_power_of_two(u32 n)
{
	return n != 0 && (n & (n - 1)) == 0;
}

// Compute next-higher power of 2 efficiently, e.g. for power-of-2 texture sizes.
// Public Domain: https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline u32 npot2(u32 orig) {
	orig--;
	orig |= orig >> 1;
	orig |= orig >> 2;
	orig |= orig >> 4;
	orig |= orig >> 8;
	orig |= orig >> 16;
	return orig + 1;
}

// Distance between two values in a wrapped (circular) system
template<typename T>
inline unsigned wrappedDifference(T a, T b, const T maximum)
{
	if (a > b)
		std::swap(a, b);
	// now b >= a
	unsigned s = b - a, l = static_cast<unsigned>(maximum - b) + a + 1;
	return std::min(s, l);
}

// Gradual steps towards the target value in a wrapped (circular) system
// using the shorter of both ways
template<typename T>
inline void wrappedApproachShortest(T &current, const T target, const T stepsize,
	const T maximum)
{
	T delta = target - current;
	if (delta < 0)
		delta += maximum;

	if (delta > stepsize && maximum - delta > stepsize) {
		current += (delta < maximum / 2) ? stepsize : -stepsize;
		if (current >= maximum)
			current -= maximum;
	} else {
		current = target;
	}
}

void setPitchYawRollRad(core::matrix4 &m, v3f rot);

inline void setPitchYawRoll(core::matrix4 &m, v3f rot)
{
	setPitchYawRollRad(m, rot * core::DEGTORAD);
}

v3f getPitchYawRollRad(const core::matrix4 &m);

inline v3f getPitchYawRoll(const core::matrix4 &m)
{
	return getPitchYawRollRad(m) * core::RADTODEG;
}

// Muliply the RGB value of a color linearly, and clamp to black/white
inline irr::video::SColor multiplyColorValue(const irr::video::SColor &color, float mod)
{
	return irr::video::SColor(color.getAlpha(),
			core::clamp<u32>(color.getRed() * mod, 0, 255),
			core::clamp<u32>(color.getGreen() * mod, 0, 255),
			core::clamp<u32>(color.getBlue() * mod, 0, 255));
}

template <typename T> inline T numericAbsolute(T v) { return v < 0 ? T(-v) : v;                }
template <typename T> inline T numericSign(T v)     { return T(v < 0 ? -1 : (v == 0 ? 0 : 1)); }

inline v3f vecAbsolute(v3f v)
{
	return v3f(
		numericAbsolute(v.X),
		numericAbsolute(v.Y),
		numericAbsolute(v.Z)
	);
}

inline v3f vecSign(v3f v)
{
	return v3f(
		numericSign(v.X),
		numericSign(v.Y),
		numericSign(v.Z)
	);
}
