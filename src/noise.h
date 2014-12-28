/*
 * Minetest
 * Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>
 * Copyright (C) 2010-2014 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *     conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *     of conditions and the following disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NOISE_HEADER
#define NOISE_HEADER

#include "debug.h"
#include "irr_v3d.h"
#include "util/string.h"

#define PSEUDORANDOM_MAX 32767

extern FlagDesc flagdesc_noiseparams[];

class PseudoRandom
{
public:
	PseudoRandom(): m_next(0)
	{
	}
	PseudoRandom(int seed): m_next(seed)
	{
	}
	void seed(int seed)
	{
		m_next = seed;
	}
	// Returns 0...PSEUDORANDOM_MAX
	int next()
	{
		m_next = m_next * 1103515245 + 12345;
		return((unsigned)(m_next/65536) % (PSEUDORANDOM_MAX + 1));
	}
	int range(int min, int max)
	{
		if (max-min > (PSEUDORANDOM_MAX + 1) / 10)
		{
			//dstream<<"WARNING: PseudoRandom::range: max > 32767"<<std::endl;
			assert(0);
		}
		if(min > max)
		{
			assert(0);
			return max;
		}
		return (next()%(max-min+1))+min;
	}
private:
	int m_next;
};

#define NOISE_FLAG_DEFAULTS    0x01
#define NOISE_FLAG_EASED       0x02
#define NOISE_FLAG_ABSVALUE    0x04

//// TODO(hmmmm): implement these!
#define NOISE_FLAG_POINTBUFFER 0x08
#define NOISE_FLAG_SIMPLEX     0x10

struct NoiseParams {
	float offset;
	float scale;
	v3f spread;
	s32 seed;
	u16 octaves;
	float persist;
	float lacunarity;
	u32 flags;

	NoiseParams() {
		offset     = 0.0f;
		scale      = 1.0f;
		spread     = v3f(250, 250, 250);
		seed       = 12345;
		octaves    = 3;
		persist    = 0.6f;
		lacunarity = 2.0f;
		flags      = NOISE_FLAG_DEFAULTS;
	}

	NoiseParams(float offset_, float scale_, v3f spread_, s32 seed_,
		u16 octaves_, float persist_, float lacunarity_,
		u32 flags_=NOISE_FLAG_DEFAULTS)
	{
		offset     = offset_;
		scale      = scale_;
		spread     = spread_;
		seed       = seed_;
		octaves    = octaves_;
		persist    = persist_;
		lacunarity = lacunarity_;
		flags      = flags_;
	}
};


// Convenience macros for getting/setting NoiseParams in Settings as a string
// WARNING:  Deprecated, use Settings::getNoiseParamsFromValue() instead
#define NOISEPARAMS_FMT_STR "f,f,v3,s32,u16,f"
//#define getNoiseParams(x, y) getStruct((x), NOISEPARAMS_FMT_STR, &(y), sizeof(y))
//#define setNoiseParams(x, y) setStruct((x), NOISEPARAMS_FMT_STR, &(y))

class Noise {
public:
	NoiseParams np;
	int seed;
	int sx;
	int sy;
	int sz;
	float *noise_buf;
	float *gradient_buf;
	float *persist_buf;
	float *result;

	Noise(NoiseParams *np, int seed, int sx, int sy, int sz=1);
	~Noise();

	void setSize(int sx, int sy, int sz=1);
	void setSpreadFactor(v3f spread);
	void setOctaves(int octaves);

	void gradientMap2D(
		float x, float y,
		float step_x, float step_y,
		int seed);
	void gradientMap3D(
		float x, float y, float z,
		float step_x, float step_y, float step_z,
		int seed);

	float *perlinMap2D(float x, float y, float *persistence_map=NULL);
	float *perlinMap3D(float x, float y, float z, float *persistence_map=NULL);

	inline float *perlinMap2D_PO(float x, float xoff, float y, float yoff,
		float *persistence_map=NULL)
	{
		return perlinMap2D(
			x + xoff * np.spread.X,
			y + yoff * np.spread.Y,
			persistence_map);
	}

	inline float *perlinMap3D_PO(float x, float xoff, float y, float yoff,
		float z, float zoff, float *persistence_map=NULL)
	{
		return perlinMap3D(
			x + xoff * np.spread.X,
			y + yoff * np.spread.Y,
			z + zoff * np.spread.Z,
			persistence_map);
	}

private:
	void allocBuffers();
	void resizeNoiseBuf(bool is3d);
	void updateResults(float g, float *gmap, float *persistence_map, size_t bufsize);

};

float NoisePerlin2D(NoiseParams *np, float x, float y, int seed);
float NoisePerlin3D(NoiseParams *np, float x, float y, float z, int seed);

inline float NoisePerlin2D_PO(NoiseParams *np, float x, float xoff,
	float y, float yoff, int seed)
{
	return NoisePerlin2D(np,
		x + xoff * np->spread.X,
		y + yoff * np->spread.Y,
		seed);
}

inline float NoisePerlin3D_PO(NoiseParams *np, float x, float xoff,
	float y, float yoff, float z, float zoff, int seed)
{
	return NoisePerlin3D(np,
		x + xoff * np->spread.X,
		y + yoff * np->spread.Y,
		z + zoff * np->spread.Z,
		seed);
}

// Return value: -1 ... 1
float noise2d(int x, int y, int seed);
float noise3d(int x, int y, int z, int seed);

float noise2d_gradient(float x, float y, int seed, bool eased=true);
float noise3d_gradient(float x, float y, float z, int seed, bool eased=false);

float noise2d_perlin(float x, float y, int seed,
		int octaves, float persistence, bool eased=true);

float noise2d_perlin_abs(float x, float y, int seed,
		int octaves, float persistence, bool eased=true);

float noise3d_perlin(float x, float y, float z, int seed,
		int octaves, float persistence, bool eased=false);

float noise3d_perlin_abs(float x, float y, float z, int seed,
		int octaves, float persistence, bool eased=false);

inline float easeCurve(float t)
{
	return t * t * t * (t * (6.f * t - 15.f) + 10.f);
}

float contour(float v);

#endif

