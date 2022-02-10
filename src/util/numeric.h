/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "basic_macros.h"
#include "constants.h"
#include "irrlichttypes.h"
#include "irr_v2d.h"
#include "irr_v3d.h"
#include "irr_aabb3d.h"
#include "SColor.h"
#include <matrix4.h>

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

inline bpos_t getContainerPos(pos_t p, pos_t d)
{
	return (p >= 0 ? p : p - d + 1) / d;
}

inline v2bpos_t getContainerPos(v2pos_t p, pos_t d)
{
	return v2bpos_t(
		getContainerPos(p.X, d),
		getContainerPos(p.Y, d)
	);
}

inline v3bpos_t getContainerPos(v3pos_t p, pos_t d)
{
	return v3bpos_t(
		getContainerPos(p.X, d),
		getContainerPos(p.Y, d),
		getContainerPos(p.Z, d)
	);
}

inline v2bpos_t getContainerPos(v2pos_t p, v2pos_t d)
{
	return v2bpos_t(
		getContainerPos(p.X, d.X),
		getContainerPos(p.Y, d.Y)
	);
}

inline v3bpos_t getContainerPos(v3pos_t p, v3pos_t d)
{
	return v3bpos_t(
		getContainerPos(p.X, d.X),
		getContainerPos(p.Y, d.Y),
		getContainerPos(p.Z, d.Z)
	);
}

inline void getContainerPosWithOffset(pos_t p, pos_t d, bpos_t &container, pos_t &offset)
{
	container = (p >= 0 ? p : p - d + 1) / d;
	offset = p & (d - 1);
}

inline void getContainerPosWithOffset(const v2pos_t &p, pos_t d, v2bpos_t &container, v2pos_t &offset)
{
	getContainerPosWithOffset(p.X, d, container.X, offset.X);
	getContainerPosWithOffset(p.Y, d, container.Y, offset.Y);
}

inline void getContainerPosWithOffset(const v3pos_t &p, pos_t d, v3bpos_t &container, v3pos_t &offset)
{
	getContainerPosWithOffset(p.X, d, container.X, offset.X);
	getContainerPosWithOffset(p.Y, d, container.Y, offset.Y);
	getContainerPosWithOffset(p.Z, d, container.Z, offset.Z);
}


inline bool isInArea(v3pos_t p, pos_t d)
{
	return (
		p.X >= 0 && p.X < d &&
		p.Y >= 0 && p.Y < d &&
		p.Z >= 0 && p.Z < d
	);
}

inline bool isInArea(v2pos_t p, pos_t d)
{
	return (
		p.X >= 0 && p.X < d &&
		p.Y >= 0 && p.Y < d
	);
}

inline bool isInArea(v3pos_t p, v3pos_t d)
{
	return (
		p.X >= 0 && p.X < d.X &&
		p.Y >= 0 && p.Y < d.Y &&
		p.Z >= 0 && p.Z < d.Z
	);
}

inline void sortBoxVerticies(v3pos_t &p1, v3pos_t &p2) {
	if (p1.X > p2.X)
		SWAP(pos_t, p1.X, p2.X);
	if (p1.Y > p2.Y)
		SWAP(pos_t, p1.Y, p2.Y);
	if (p1.Z > p2.Z)
		SWAP(pos_t, p1.Z, p2.Z);
}

 /*
inline void sortBoxVerticies(v3bpos_t &p1, v3bpos_t &p2) {
	if (p1.X > p2.X)
		SWAP(bpos_t, p1.X, p2.X);
	if (p1.Y > p2.Y)
		SWAP(bpos_t, p1.Y, p2.Y);
	if (p1.Z > p2.Z)
		SWAP(bpos_t, p1.Z, p2.Z);
}
 */

inline v3pos_t componentwise_min(const v3pos_t &a, const v3pos_t &b)
{
	return v3pos_t(MYMIN(a.X, b.X), MYMIN(a.Y, b.Y), MYMIN(a.Z, b.Z));
}

inline v3pos_t componentwise_max(const v3pos_t &a, const v3pos_t &b)
{
	return v3pos_t(MYMAX(a.X, b.X), MYMAX(a.Y, b.Y), MYMAX(a.Z, b.Z));
}


/** Returns \p f wrapped to the range [-360, 360]
 *
 *  See test.cpp for example cases.
 *
 *  \note This is also used in cases where degrees wrapped to the range [0, 360]
 *  is innapropriate (e.g. pitch needs negative values)
 *
 *  \internal functionally equivalent -- although precision may vary slightly --
 *  to fmodf((f), 360.0f) however empirical tests indicate that this approach is
 *  faster.
 */
inline float modulo360f(float f)
{
	int sign;
	int whole;
	float fraction;

	if (f < 0) {
		f = -f;
		sign = -1;
	} else {
		sign = 1;
	}

	whole = f;

	fraction = f - whole;
	whole %= 360;

	return sign * (whole + fraction);
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

inline u32 calc_parity(u32 v)
{
	v ^= v >> 16;
	v ^= v >> 8;
	v ^= v >> 4;
	v &= 0xf;
	return (0x6996 >> v) & 1;
}

u64 murmur_hash_64_ua(const void *key, int len, unsigned int seed);

bool isBlockInSight(v3bpos_t blockpos_b, v3opos_t camera_pos, v3f camera_dir,
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
inline v3pos_t floatToInt(v3f p, f32 d)
{
	return v3pos_t(
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

inline v3pos_t doubleToPos(v3d p, double d)
{
	return v3pos_t(
		(p.X + (p.X > 0 ? d / 2 : -d / 2)) / d,
		(p.Y + (p.Y > 0 ? d / 2 : -d / 2)) / d,
		(p.Z + (p.Z > 0 ? d / 2 : -d / 2)) / d);
}

inline v3pos_t floatToInt(v3d p, f32 d)
{
	return v3pos_t(
		(p.X + (p.X > 0 ? d / 2 : -d / 2)) / d,
		(p.Y + (p.Y > 0 ? d / 2 : -d / 2)) / d,
		(p.Z + (p.Z > 0 ? d / 2 : -d / 2)) / d);
}


inline v3pos_t oposToPos(v3opos_t p, double d)
{
	return v3pos_t(
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

#if USE_OPOS64 || USE_POS32
inline v3opos_t intToFloat(const v3pos_t & p, const opos_t d)
{
	return v3opos_t(
		(opos_t)p.X * d,
		(opos_t)p.Y * d,
		(opos_t)p.Z * d
	);
}
#endif

inline v3f posToFloat(const v3pos_t & p, f32 d)
{
	return v3f(
		(f32)p.X * d,
		(f32)p.Y * d,
		(f32)p.Z * d
	);
}

inline v3opos_t posToOpos(const v3pos_t & p, const opos_t d)
{
	return v3opos_t(
		(opos_t)p.X * d,
		(opos_t)p.Y * d,
		(opos_t)p.Z * d
	);
}

inline v3opos_t v3fToOpos(const v3f & p)
{
	return v3opos_t(p.X, p.Y, p.Z);
}

inline v3f oposToV3f(const v3opos_t & p)
{
	return v3f(p.X, p.Y, p.Z);
}

 /*
inline v3f intToFloat(v3bpos_t p, f32 d)
{
	return v3f(
		(f32)p.X * d,
		(f32)p.Y * d,
		(f32)p.Z * d
	);
}
 */

inline v3s16 posToS16(const v3pos_t & p)
{
#if USE_POS32
	return v3s16(p.X, p.Y, p.Z);
#else
	return p;
#endif
}

inline v3pos_t s16ToPos(const v3s16 & p)
{
#if USE_POS32
	return v3pos_t(p.X, p.Y, p.Z);
#else
	return p;
#endif
}


// Random helper. Usually d=BS
inline aabb3f getNodeBox(v3pos_t p, float d)
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

#if USE_OPOS64
inline aabb3o getNodeBox(v3pos_t p, opos_t d)
{
	return aabb3o(
		(opos_t)p.X * d - 0.5f * d,
		(opos_t)p.Y * d - 0.5f * d,
		(opos_t)p.Z * d - 0.5f * d,
		(opos_t)p.X * d + 0.5f * d,
		(opos_t)p.Y * d + 0.5f * d,
		(opos_t)p.Z * d + 0.5f * d
	);
}
#endif

class IntervalLimiter
{
public:
	IntervalLimiter() = default;

	/*
		dtime: time from last call to this method
		wanted_interval: interval wanted
		return value:
			true: action should be skipped
			false: action should be done
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

void setPitchYawRollRad(core::matrix4 &m, const v3f &rot);

inline void setPitchYawRoll(core::matrix4 &m, const v3f &rot)
{
	setPitchYawRollRad(m, rot * core::DEGTORAD64);
}

v3f getPitchYawRollRad(const core::matrix4 &m);

inline v3f getPitchYawRoll(const core::matrix4 &m)
{
	return getPitchYawRollRad(m) * core::RADTODEG64;
}

// Muliply the RGB value of a color linearly, and clamp to black/white
inline irr::video::SColor multiplyColorValue(const irr::video::SColor &color, float mod)
{
	return irr::video::SColor(color.getAlpha(),
			core::clamp<u32>(color.getRed() * mod, 0, 255),
			core::clamp<u32>(color.getGreen() * mod, 0, 255),
			core::clamp<u32>(color.getBlue() * mod, 0, 255));
}
