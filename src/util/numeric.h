/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef UTIL_NUMERIC_HEADER
#define UTIL_NUMERIC_HEADER

#include "../irrlichttypes.h"
#include "../irr_v2d.h"
#include "../irr_v3d.h"
#include "../irr_aabb3d.h"
#include <irrList.h>

// Calculate the borders of a "d-radius" cube
void getFacePositions(core::list<v3s16> &list, u16 d);

class IndentationRaiser
{
public:
	IndentationRaiser(u16 *indentation)
	{
		m_indentation = indentation;
		(*m_indentation)++;
	}
	~IndentationRaiser()
	{
		(*m_indentation)--;
	}
private:
	u16 *m_indentation;
};

inline s16 getContainerPos(s16 p, s16 d)
{
	return (p>=0 ? p : p-d+1) / d;
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

inline s16 rangelim(s16 i, s16 max)
{
	if(i < 0)
		return 0;
	if(i > max)
		return max;
	return i;
}

#define rangelim(d, min, max) ((d) < (min) ? (min) : ((d)>(max)?(max):(d)))

inline v3s16 arealim(v3s16 p, s16 d)
{
	if(p.X < 0)
		p.X = 0;
	if(p.Y < 0)
		p.Y = 0;
	if(p.Z < 0)
		p.Z = 0;
	if(p.X > d-1)
		p.X = d-1;
	if(p.Y > d-1)
		p.Y = d-1;
	if(p.Z > d-1)
		p.Z = d-1;
	return p;
}


/*
	See test.cpp for example cases.
	wraps degrees to the range of -360...360
	NOTE: Wrapping to 0...360 is not used because pitch needs negative values.
*/
inline float wrapDegrees(float f)
{
	// Take examples of f=10, f=720.5, f=-0.5, f=-360.5
	// This results in
	// 10, 720, -1, -361
	int i = floor(f);
	// 0, 2, 0, -1
	int l = i / 360;
	// NOTE: This would be used for wrapping to 0...360
	// 0, 2, -1, -2
	/*if(i < 0)
		l -= 1;*/
	// 0, 720, 0, -360
	int k = l * 360;
	// 10, 0.5, -0.5, -0.5
	f -= float(k);
	return f;
}

/* Wrap to 0...360 */
inline float wrapDegrees_0_360(float f)
{
	// Take examples of f=10, f=720.5, f=-0.5, f=-360.5
	// This results in
	// 10, 720, -1, -361
	int i = floor(f);
	// 0, 2, 0, -1
	int l = i / 360;
	// Wrap to 0...360
	// 0, 2, -1, -2
	if(i < 0)
		l -= 1;
	// 0, 720, 0, -360
	int k = l * 360;
	// 10, 0.5, -0.5, -0.5
	f -= float(k);
	return f;
}

/* Wrap to -180...180 */
inline float wrapDegrees_180(float f)
{
	f += 180;
	f = wrapDegrees_0_360(f);
	f -= 180;
	return f;
}

/*
	Pseudo-random (VC++ rand() sucks)
*/
int myrand(void);
void mysrand(unsigned seed);
#define MYRAND_MAX 32767

int myrand_range(int min, int max);

/*
	Miscellaneous functions
*/

bool isBlockInSight(v3s16 blockpos_b, v3f camera_pos, v3f camera_dir,
		f32 camera_fov, f32 range, f32 *distance_ptr=NULL);

/*
	Some helper stuff
*/
#define MYMIN(a,b) ((a)<(b)?(a):(b))
#define MYMAX(a,b) ((a)>(b)?(a):(b))

/*
	Returns nearest 32-bit integer for given floating point number.
	<cmath> and <math.h> in VC++ don't provide round().
*/
inline s32 myround(f32 f)
{
	return floor(f + 0.5);
}

/*
	Returns integer position of node in given floating point position
*/
inline v3s16 floatToInt(v3f p, f32 d)
{
	v3s16 p2(
		(p.X + (p.X>0 ? d/2 : -d/2))/d,
		(p.Y + (p.Y>0 ? d/2 : -d/2))/d,
		(p.Z + (p.Z>0 ? d/2 : -d/2))/d);
	return p2;
}

/*
	Returns floating point position of node in given integer position
*/
inline v3f intToFloat(v3s16 p, f32 d)
{
	v3f p2(
		(f32)p.X * d,
		(f32)p.Y * d,
		(f32)p.Z * d
	);
	return p2;
}

// Random helper. Usually d=BS
inline core::aabbox3d<f32> getNodeBox(v3s16 p, float d)
{
	return core::aabbox3d<f32>(
		(float)p.X * d - 0.5*d,
		(float)p.Y * d - 0.5*d,
		(float)p.Z * d - 0.5*d,
		(float)p.X * d + 0.5*d,
		(float)p.Y * d + 0.5*d,
		(float)p.Z * d + 0.5*d
	);
}

class IntervalLimiter
{
public:
	IntervalLimiter():
		m_accumulator(0)
	{
	}
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
		if(m_accumulator < wanted_interval)
			return false;
		m_accumulator -= wanted_interval;
		return true;
	}
protected:
	float m_accumulator;
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
	if(length < 1 || pagecount < 1 || page < 1 || page > pagecount)
	{
		// Special cases or invalid parameters
		minindex = maxindex = 0;
	}
	else if(pagecount <= length)
	{
		// Less pages than entries in the list:
		// Each page contains at least one entry
		minindex = (length * (page-1) + (pagecount-1)) / pagecount;
		maxindex = (length * page + (pagecount-1)) / pagecount;
	}
	else
	{
		// More pages than entries in the list:
		// Make sure the empty pages are at the end
		if(page < length)
		{
			minindex = page-1;
			maxindex = page;
		}
		else
		{
			minindex = 0;
			maxindex = 0;
		}
	}
}

#endif

