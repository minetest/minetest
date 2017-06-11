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

#include "irr_v3d.h"
#include "exceptions.h"
#include "util/string.h"

extern FlagDesc flagdesc_noiseparams[];

// Note: this class is not polymorphic so that its high level of
// optimizability may be preserved in the common use case
class PseudoRandom {
public:
	const static u32 RANDOM_RANGE = 32767;

	inline PseudoRandom(int seed=0):
		m_next(seed)
	{
	}

	inline void seed(int seed)
	{
		m_next = seed;
	}

	inline int next()
	{
		m_next = m_next * 1103515245 + 12345;
		return (unsigned)(m_next / 65536) % (RANDOM_RANGE + 1);
	}

	inline int range(int min, int max)
	{
		if (max < min)
			throw PrngException("Invalid range (max < min)");
		/*
		Here, we ensure the range is not too large relative to RANDOM_MAX,
		as otherwise the effects of bias would become noticable.  Unlike
		PcgRandom, we cannot modify this RNG's range as it would change the
		output of this RNG for reverse compatibility.
		*/
		if ((u32)(max - min) > (RANDOM_RANGE + 1) / 10)
			throw PrngException("Range too large");

		return (next() % (max - min + 1)) + min;
	}

private:
	int m_next;
};

class PcgRandom {
public:
	const static s32 RANDOM_MIN   = -0x7fffffff - 1;
	const static s32 RANDOM_MAX   = 0x7fffffff;
	const static u32 RANDOM_RANGE = 0xffffffff;

	PcgRandom(u64 state=0x853c49e6748fea9bULL, u64 seq=0xda3e39cb94b95bdbULL);
	void seed(u64 state, u64 seq=0xda3e39cb94b95bdbULL);
	u32 next();
	u32 range(u32 bound);
	s32 range(s32 min, s32 max);
	s32 randNormalDist(s32 min, s32 max, int num_trials=6);

private:
	u64 m_state;
	u64 m_inc;
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

	NoiseParams()
	{
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
	s32 seed;
	u32 sx;
	u32 sy;
	u32 sz;
	float *noise_buf;
	float *gradient_buf;
	float *persist_buf;
	float *result;

	Noise(NoiseParams *np, s32 seed, u32 sx, u32 sy, u32 sz=1);
	~Noise();

	void setSize(u32 sx, u32 sy, u32 sz=1);
	void setSpreadFactor(v3f spread);
	void setOctaves(int octaves);

	void gradientMap2D(
		float x, float y,
		float step_x, float step_y,
		s32 seed);
	void gradientMap3D(
		float x, float y, float z,
		float step_x, float step_y, float step_z,
		s32 seed);

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

float NoisePerlin2D(NoiseParams *np, float x, float y, s32 seed);
float NoisePerlin3D(NoiseParams *np, float x, float y, float z, s32 seed);

inline float NoisePerlin2D_PO(NoiseParams *np, float x, float xoff,
	float y, float yoff, s32 seed)
{
	return NoisePerlin2D(np,
		x + xoff * np->spread.X,
		y + yoff * np->spread.Y,
		seed);
}

inline float NoisePerlin3D_PO(NoiseParams *np, float x, float xoff,
	float y, float yoff, float z, float zoff, s32 seed)
{
	return NoisePerlin3D(np,
		x + xoff * np->spread.X,
		y + yoff * np->spread.Y,
		z + zoff * np->spread.Z,
		seed);
}

// Return value: -1 ... 1
float noise2d(int x, int y, s32 seed);
float noise3d(int x, int y, int z, s32 seed);

float noise2d_gradient(float x, float y, s32 seed, bool eased=true);
float noise3d_gradient(float x, float y, float z, s32 seed, bool eased=false);

float noise2d_perlin(float x, float y, s32 seed,
		int octaves, float persistence, bool eased=true);

float noise2d_perlin_abs(float x, float y, s32 seed,
		int octaves, float persistence, bool eased=true);

float noise3d_perlin(float x, float y, float z, s32 seed,
		int octaves, float persistence, bool eased=false);

float noise3d_perlin_abs(float x, float y, float z, s32 seed,
		int octaves, float persistence, bool eased=false);

inline float easeCurve(float t)
{
	return t * t * t * (t * (6.f * t - 15.f) + 10.f);
}

float contour(float v);

#endif

