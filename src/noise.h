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
	// Returns 0...32767
	int next()
	{
		m_next = m_next * 1103515245 + 12345;
		return((unsigned)(m_next/65536) % 32768);
	}
	int range(int min, int max)
	{
		if(max-min > 32768/10)
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

struct NoiseParams {
	float offset;
	float scale;
	v3f spread;
	int seed;
	int octaves;
	float persist;

	NoiseParams() {}

	NoiseParams(float offset_, float scale_, v3f spread_,
		int seed_, int octaves_, float persist_)
	{
		offset  = offset_;
		scale   = scale_;
		spread  = spread_;
		seed    = seed_;
		octaves = octaves_;
		persist = persist_;
	}
};


// Convenience macros for getting/setting NoiseParams in Settings

#define NOISEPARAMS_FMT_STR "f,f,v3,s32,s32,f"

#define getNoiseParams(x, y) getStruct((x), NOISEPARAMS_FMT_STR, &(y), sizeof(y))
#define setNoiseParams(x, y) setStruct((x), NOISEPARAMS_FMT_STR, &(y))

class Noise {
public:
	NoiseParams *np;
	int seed;
	int sx;
	int sy;
	int sz;
	float *noisebuf;
	float *buf;
	float *result;

	Noise(NoiseParams *np, int seed, int sx, int sy, int sz=1);
	~Noise();

	void setSize(int sx, int sy, int sz=1);
	void setSpreadFactor(v3f spread);
	void setOctaves(int octaves);
	void resizeNoiseBuf(bool is3d);

	void gradientMap2D(
		float x, float y,
		float step_x, float step_y,
		int seed);
	void gradientMap3D(
		float x, float y, float z,
		float step_x, float step_y, float step_z,
		int seed, bool eased=false);
	float *perlinMap2D(float x, float y);
	float *perlinMap2DModulated(float x, float y, float *persist_map);
	float *perlinMap3D(float x, float y, float z, bool eased=false);
	void transformNoiseMap();
};

// Return value: -1 ... 1
float noise2d(int x, int y, int seed);
float noise3d(int x, int y, int z, int seed);

float noise2d_gradient(float x, float y, int seed);
float noise3d_gradient(float x, float y, float z, int seed);

float noise2d_perlin(float x, float y, int seed,
		int octaves, float persistence);

float noise2d_perlin_abs(float x, float y, int seed,
		int octaves, float persistence);

float noise3d_perlin(float x, float y, float z, int seed,
		int octaves, float persistence);

float noise3d_perlin_abs(float x, float y, float z, int seed,
		int octaves, float persistence);

inline float easeCurve(float t) {
	return t * t * t * (t * (6.f * t - 15.f) + 10.f);
}

#define NoisePerlin2D(np, x, y, s) \
		((np)->offset + (np)->scale * noise2d_perlin( \
		(float)(x) / (np)->spread.X, \
		(float)(y) / (np)->spread.Y, \
		(s) + (np)->seed, (np)->octaves, (np)->persist))

#define NoisePerlin2DNoTxfm(np, x, y, s) \
		(noise2d_perlin( \
		(float)(x) / (np)->spread.X, \
		(float)(y) / (np)->spread.Y, \
		(s) + (np)->seed, (np)->octaves, (np)->persist))

#define NoisePerlin2DPosOffset(np, x, xoff, y, yoff, s) \
		((np)->offset + (np)->scale * noise2d_perlin( \
		(float)(xoff) + (float)(x) / (np)->spread.X, \
		(float)(yoff) + (float)(y) / (np)->spread.Y, \
		(s) + (np)->seed, (np)->octaves, (np)->persist))

#define NoisePerlin2DNoTxfmPosOffset(np, x, xoff, y, yoff, s) \
		(noise2d_perlin( \
		(float)(xoff) + (float)(x) / (np)->spread.X, \
		(float)(yoff) + (float)(y) / (np)->spread.Y, \
		(s) + (np)->seed, (np)->octaves, (np)->persist))

#define NoisePerlin3D(np, x, y, z, s) ((np)->offset + (np)->scale * \
		noise3d_perlin((float)(x) / (np)->spread.X, (float)(y) / (np)->spread.Y, \
		(float)(z) / (np)->spread.Z, (s) + (np)->seed, (np)->octaves, (np)->persist))

#endif

