/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include <math.h>
#include "noise.h"
#include <iostream>
#include <string.h> // memset
#include "debug.h"
#include "util/numeric.h"

#define NOISE_MAGIC_X    1619
#define NOISE_MAGIC_Y    31337
#define NOISE_MAGIC_Z    52591
#define NOISE_MAGIC_SEED 1013

typedef float (*Interp3dFxn)(
		float v000, float v100, float v010, float v110,
		float v001, float v101, float v011, float v111,
		float x, float y, float z);

float cos_lookup[16] = {
	1.0,  0.9238,  0.7071,  0.3826, 0, -0.3826, -0.7071, -0.9238,
	1.0, -0.9238, -0.7071, -0.3826, 0,  0.3826,  0.7071,  0.9238
};


///////////////////////////////////////////////////////////////////////////////


//noise poly:  p(n) = 60493n^3 + 19990303n + 137612589
float noise2d(int x, int y, int seed)
{
	int n = (NOISE_MAGIC_X * x + NOISE_MAGIC_Y * y
			+ NOISE_MAGIC_SEED * seed) & 0x7fffffff;
	n = (n >> 13) ^ n;
	n = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
	return 1.f - (float)n / 0x40000000;
}


float noise3d(int x, int y, int z, int seed)
{
	int n = (NOISE_MAGIC_X * x + NOISE_MAGIC_Y * y + NOISE_MAGIC_Z * z
			+ NOISE_MAGIC_SEED * seed) & 0x7fffffff;
	n = (n >> 13) ^ n;
	n = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
	return 1.f - (float)n / 0x40000000;
}


float dotProduct(float vx, float vy, float wx, float wy)
{
	return vx * wx + vy * wy;
}


inline float linearInterpolation(float v0, float v1, float t)
{
    return v0 + (v1 - v0) * t;
}


float biLinearInterpolation(
		float v00, float v10,
		float v01, float v11,
		float x, float y)
{
    float tx = easeCurve(x);
    float ty = easeCurve(y);
    float u = linearInterpolation(v00, v10, tx);
    float v = linearInterpolation(v01, v11, tx);
    return linearInterpolation(u, v, ty);
}


float biLinearInterpolationNoEase(
		float x0y0, float x1y0,
		float x0y1, float x1y1,
		float x, float y)
{
    float u = linearInterpolation(x0y0, x1y0, x);
    float v = linearInterpolation(x0y1, x1y1, x);
    return linearInterpolation(u, v, y);
}

/*
float triLinearInterpolation(
		float v000, float v100, float v010, float v110,
		float v001, float v101, float v011, float v111,
		float x, float y, float z)
{
	float u = biLinearInterpolation(v000, v100, v010, v110, x, y);
	float v = biLinearInterpolation(v001, v101, v011, v111, x, y);
	return linearInterpolation(u, v, z);
}


float triLinearInterpolationNoEase(
		float v000, float v100, float v010, float v110,
		float v001, float v101, float v011, float v111,
		float x, float y, float z)
{
	float u = biLinearInterpolationNoEase(v000, v100, v010, v110, x, y);
	float v = biLinearInterpolationNoEase(v001, v101, v011, v111, x, y);
	return linearInterpolation(u, v, z);
}
*/


float triLinearInterpolation(
		float v000, float v100, float v010, float v110,
		float v001, float v101, float v011, float v111,
		float x, float y, float z)
{
	float tx = easeCurve(x);
	float ty = easeCurve(y);
	float tz = easeCurve(z);

	return (
		v000 * (1 - tx) * (1 - ty) * (1 - tz) +
		v100 * tx * (1 - ty) * (1 - tz) +
		v010 * (1 - tx) * ty * (1 - tz) +
		v110 * tx * ty * (1 - tz) +
		v001 * (1 - tx) * (1 - ty) * tz +
		v101 * tx * (1 - ty) * tz +
		v011 * (1 - tx) * ty * tz +
		v111 * tx * ty * tz
	);
}

float triLinearInterpolationNoEase(
		float v000, float v100, float v010, float v110,
		float v001, float v101, float v011, float v111,
		float x, float y, float z)
{
	float tx = x;
	float ty = y;
	float tz = z;
	return (
		v000 * (1 - tx) * (1 - ty) * (1 - tz) +
		v100 * tx * (1 - ty) * (1 - tz) +
		v010 * (1 - tx) * ty * (1 - tz) +
		v110 * tx * ty * (1 - tz) +
		v001 * (1 - tx) * (1 - ty) * tz +
		v101 * tx * (1 - ty) * tz +
		v011 * (1 - tx) * ty * tz +
		v111 * tx * ty * tz
	);
}


#if 0
float noise2d_gradient(float x, float y, int seed)
{
	// Calculate the integer coordinates
	int x0 = (x > 0.0 ? (int)x : (int)x - 1);
	int y0 = (y > 0.0 ? (int)y : (int)y - 1);
	// Calculate the remaining part of the coordinates
	float xl = x - (float)x0;
	float yl = y - (float)y0;
	// Calculate random cosine lookup table indices for the integer corners.
	// They are looked up as unit vector gradients from the lookup table.
	int n00 = (int)((noise2d(x0, y0, seed)+1)*8);
	int n10 = (int)((noise2d(x0+1, y0, seed)+1)*8);
	int n01 = (int)((noise2d(x0, y0+1, seed)+1)*8);
	int n11 = (int)((noise2d(x0+1, y0+1, seed)+1)*8);
	// Make a dot product for the gradients and the positions, to get the values
	float s = dotProduct(cos_lookup[n00], cos_lookup[(n00+12)%16], xl, yl);
	float u = dotProduct(-cos_lookup[n10], cos_lookup[(n10+12)%16], 1.-xl, yl);
	float v = dotProduct(cos_lookup[n01], -cos_lookup[(n01+12)%16], xl, 1.-yl);
	float w = dotProduct(-cos_lookup[n11], -cos_lookup[(n11+12)%16], 1.-xl, 1.-yl);
	// Interpolate between the values
	return biLinearInterpolation(s,u,v,w,xl,yl);
}
#endif


float noise2d_gradient(float x, float y, int seed)
{
	// Calculate the integer coordinates
	int x0 = myfloor(x);
	int y0 = myfloor(y);
	// Calculate the remaining part of the coordinates
	float xl = x - (float)x0;
	float yl = y - (float)y0;
	// Get values for corners of square
	float v00 = noise2d(x0, y0, seed);
	float v10 = noise2d(x0+1, y0, seed);
	float v01 = noise2d(x0, y0+1, seed);
	float v11 = noise2d(x0+1, y0+1, seed);
	// Interpolate
	return biLinearInterpolation(v00, v10, v01, v11, xl, yl);
}


float noise3d_gradient(float x, float y, float z, int seed)
{
	// Calculate the integer coordinates
	int x0 = myfloor(x);
	int y0 = myfloor(y);
	int z0 = myfloor(z);
	// Calculate the remaining part of the coordinates
	float xl = x - (float)x0;
	float yl = y - (float)y0;
	float zl = z - (float)z0;
	// Get values for corners of cube
	float v000 = noise3d(x0,     y0,     z0,     seed);
	float v100 = noise3d(x0 + 1, y0,     z0,     seed);
	float v010 = noise3d(x0,     y0 + 1, z0,     seed);
	float v110 = noise3d(x0 + 1, y0 + 1, z0,     seed);
	float v001 = noise3d(x0,     y0,     z0 + 1, seed);
	float v101 = noise3d(x0 + 1, y0,     z0 + 1, seed);
	float v011 = noise3d(x0,     y0 + 1, z0 + 1, seed);
	float v111 = noise3d(x0 + 1, y0 + 1, z0 + 1, seed);
	// Interpolate
	return triLinearInterpolationNoEase(
		v000, v100, v010, v110,
		v001, v101, v011, v111,
		xl, yl, zl);
}


float noise2d_perlin(float x, float y, int seed,
		int octaves, float persistence)
{
	float a = 0;
	float f = 1.0;
	float g = 1.0;
	for (int i = 0; i < octaves; i++)
	{
		a += g * noise2d_gradient(x * f, y * f, seed + i);
		f *= 2.0;
		g *= persistence;
	}
	return a;
}


float noise2d_perlin_abs(float x, float y, int seed,
		int octaves, float persistence)
{
	float a = 0;
	float f = 1.0;
	float g = 1.0;
	for (int i = 0; i < octaves; i++)
	{
		a += g * fabs(noise2d_gradient(x * f, y * f, seed + i));
		f *= 2.0;
		g *= persistence;
	}
	return a;
}


float noise3d_perlin(float x, float y, float z, int seed,
		int octaves, float persistence)
{
	float a = 0;
	float f = 1.0;
	float g = 1.0;
	for (int i = 0; i < octaves; i++)
	{
		a += g * noise3d_gradient(x * f, y * f, z * f, seed + i);
		f *= 2.0;
		g *= persistence;
	}
	return a;
}


float noise3d_perlin_abs(float x, float y, float z, int seed,
		int octaves, float persistence)
{
	float a = 0;
	float f = 1.0;
	float g = 1.0;
	for (int i = 0; i < octaves; i++)
	{
		a += g * fabs(noise3d_gradient(x * f, y * f, z * f, seed + i));
		f *= 2.0;
		g *= persistence;
	}
	return a;
}


// -1->0, 0->1, 1->0
float contour(float v)
{
	v = fabs(v);
	if(v >= 1.0)
		return 0.0;
	return (1.0 - v);
}


///////////////////////// [ New perlin stuff ] ////////////////////////////


Noise::Noise(NoiseParams *np, int seed, int sx, int sy, int sz)
{
	this->np   = np;
	this->seed = seed;
	this->sx   = sx;
	this->sy   = sy;
	this->sz   = sz;

	this->noisebuf = NULL;
	resizeNoiseBuf(sz > 1);

	this->buf    = new float[sx * sy * sz];
	this->result = new float[sx * sy * sz];
}


Noise::~Noise()
{
	delete[] buf;
	delete[] result;
	delete[] noisebuf;
}


void Noise::setSize(int sx, int sy, int sz)
{
	this->sx = sx;
	this->sy = sy;
	this->sz = sz;

	this->noisebuf = NULL;
	resizeNoiseBuf(sz > 1);

	delete[] buf;
	delete[] result;
	this->buf    = new float[sx * sy * sz];
	this->result = new float[sx * sy * sz];
}


void Noise::setSpreadFactor(v3f spread)
{
	this->np->spread = spread;

	resizeNoiseBuf(sz > 1);
}


void Noise::setOctaves(int octaves)
{
	this->np->octaves = octaves;

	resizeNoiseBuf(sz > 1);
}


void Noise::resizeNoiseBuf(bool is3d)
{
	int nlx, nly, nlz;
	float ofactor;

	//maximum possible spread value factor
	ofactor = (float)(1 << (np->octaves - 1));

	//noise lattice point count
	//(int)(sz * spread * ofactor) is # of lattice points crossed due to length
	// + 2 for the two initial endpoints
	// + 1 for potentially crossing a boundary due to offset
	nlx = (int)(sx * ofactor / np->spread.X) + 3;
	nly = (int)(sy * ofactor / np->spread.Y) + 3;
	nlz = is3d ? (int)(sz * ofactor / np->spread.Z) + 3 : 1;

	if (noisebuf)
		delete[] noisebuf;
	noisebuf = new float[nlx * nly * nlz];
}


/*
 * NB:  This algorithm is not optimal in terms of space complexity.  The entire
 * integer lattice of noise points could be done as 2 lines instead, and for 3D,
 * 2 lines + 2 planes.
 * However, this would require the noise calls to be interposed with the
 * interpolation loops, which may trash the icache, leading to lower overall
 * performance.
 * Another optimization that could save half as many noise calls is to carry over
 * values from the previous noise lattice as midpoints in the new lattice for the
 * next octave.
 */
#define idx(x, y) ((y) * nlx + (x))
void Noise::gradientMap2D(
		float x, float y,
		float step_x, float step_y,
		int seed)
{
	float v00, v01, v10, v11, u, v, orig_u;
	int index, i, j, x0, y0, noisex, noisey;
	int nlx, nly;

	x0 = floor(x);
	y0 = floor(y);
	u = x - (float)x0;
	v = y - (float)y0;
	orig_u = u;

	//calculate noise point lattice
	nlx = (int)(u + sx * step_x) + 2;
	nly = (int)(v + sy * step_y) + 2;
	index = 0;
	for (j = 0; j != nly; j++)
		for (i = 0; i != nlx; i++)
			noisebuf[index++] = noise2d(x0 + i, y0 + j, seed);

	//calculate interpolations
	index  = 0;
	noisey = 0;
	for (j = 0; j != sy; j++) {
		v00 = noisebuf[idx(0, noisey)];
		v10 = noisebuf[idx(1, noisey)];
		v01 = noisebuf[idx(0, noisey + 1)];
		v11 = noisebuf[idx(1, noisey + 1)];

		u = orig_u;
		noisex = 0;
		for (i = 0; i != sx; i++) {
			buf[index++] = biLinearInterpolation(v00, v10, v01, v11, u, v);
			u += step_x;
			if (u >= 1.0) {
				u -= 1.0;
				noisex++;
				v00 = v10;
				v01 = v11;
				v10 = noisebuf[idx(noisex + 1, noisey)];
				v11 = noisebuf[idx(noisex + 1, noisey + 1)];
			}
		}

		v += step_y;
		if (v >= 1.0) {
			v -= 1.0;
			noisey++;
		}
	}
}
#undef idx


#define idx(x, y, z) ((z) * nly * nlx + (y) * nlx + (x))
void Noise::gradientMap3D(
		float x, float y, float z,
		float step_x, float step_y, float step_z,
		int seed, bool eased)
{
	float v000, v010, v100, v110;
	float v001, v011, v101, v111;
	float u, v, w, orig_u, orig_v;
	int index, i, j, k, x0, y0, z0, noisex, noisey, noisez;
	int nlx, nly, nlz;

	Interp3dFxn interpolate = eased ?
		triLinearInterpolation : triLinearInterpolationNoEase;

	x0 = floor(x);
	y0 = floor(y);
	z0 = floor(z);
	u = x - (float)x0;
	v = y - (float)y0;
	w = z - (float)z0;
	orig_u = u;
	orig_v = v;

	//calculate noise point lattice
	nlx = (int)(u + sx * step_x) + 2;
	nly = (int)(v + sy * step_y) + 2;
	nlz = (int)(w + sz * step_z) + 2;
	index = 0;
	for (k = 0; k != nlz; k++)
		for (j = 0; j != nly; j++)
			for (i = 0; i != nlx; i++)
				noisebuf[index++] = noise3d(x0 + i, y0 + j, z0 + k, seed);

	//calculate interpolations
	index  = 0;
	noisey = 0;
	noisez = 0;
	for (k = 0; k != sz; k++) {
		v = orig_v;
		noisey = 0;
		for (j = 0; j != sy; j++) {
			v000 = noisebuf[idx(0, noisey,     noisez)];
			v100 = noisebuf[idx(1, noisey,     noisez)];
			v010 = noisebuf[idx(0, noisey + 1, noisez)];
			v110 = noisebuf[idx(1, noisey + 1, noisez)];
			v001 = noisebuf[idx(0, noisey,     noisez + 1)];
			v101 = noisebuf[idx(1, noisey,     noisez + 1)];
			v011 = noisebuf[idx(0, noisey + 1, noisez + 1)];
			v111 = noisebuf[idx(1, noisey + 1, noisez + 1)];

			u = orig_u;
			noisex = 0;
			for (i = 0; i != sx; i++) {
				buf[index++] = interpolate(
									v000, v100, v010, v110,
									v001, v101, v011, v111,
									u, v, w);

				u += step_x;
				if (u >= 1.0) {
					u -= 1.0;
					noisex++;
					v000 = v100;
					v010 = v110;
					v100 = noisebuf[idx(noisex + 1, noisey,     noisez)];
					v110 = noisebuf[idx(noisex + 1, noisey + 1, noisez)];
					v001 = v101;
					v011 = v111;
					v101 = noisebuf[idx(noisex + 1, noisey,     noisez + 1)];
					v111 = noisebuf[idx(noisex + 1, noisey + 1, noisez + 1)];
				}
			}

			v += step_y;
			if (v >= 1.0) {
				v -= 1.0;
				noisey++;
			}
		}

		w += step_z;
		if (w >= 1.0) {
			w -= 1.0;
			noisez++;
		}
	}
}
#undef idx


float *Noise::perlinMap2D(float x, float y)
{
	float f = 1.0, g = 1.0;
	size_t bufsize = sx * sy;

	x /= np->spread.X;
	y /= np->spread.Y;

	memset(result, 0, sizeof(float) * bufsize);

	for (int oct = 0; oct < np->octaves; oct++) {
		gradientMap2D(x * f, y * f,
			f / np->spread.X, f / np->spread.Y,
			seed + np->seed + oct);

		for (size_t i = 0; i != bufsize; i++)
			result[i] += g * buf[i];

		f *= 2.0;
		g *= np->persist;
	}

	return result;
}


float *Noise::perlinMap2DModulated(float x, float y, float *persist_map)
{
	float f = 1.0;
	size_t bufsize = sx * sy;

	x /= np->spread.X;
	y /= np->spread.Y;

	memset(result, 0, sizeof(float) * bufsize);

	float *g = new float[bufsize];
	for (size_t i = 0; i != bufsize; i++)
		g[i] = 1.0;

	for (int oct = 0; oct < np->octaves; oct++) {
		gradientMap2D(x * f, y * f,
			f / np->spread.X, f / np->spread.Y,
			seed + np->seed + oct);

		for (size_t i = 0; i != bufsize; i++) {
			result[i] += g[i] * buf[i];
			g[i] *= persist_map[i];
		}

		f *= 2.0;
	}
	
	delete[] g;
	return result;
}


float *Noise::perlinMap3D(float x, float y, float z, bool eased)
{
	float f = 1.0, g = 1.0;
	size_t bufsize = sx * sy * sz;

	x /= np->spread.X;
	y /= np->spread.Y;
	z /= np->spread.Z;

	memset(result, 0, sizeof(float) * bufsize);

	for (int oct = 0; oct < np->octaves; oct++) {
		gradientMap3D(x * f, y * f, z * f,
			f / np->spread.X, f / np->spread.Y, f / np->spread.Z,
			seed + np->seed + oct, eased);

		for (size_t i = 0; i != bufsize; i++)
			result[i] += g * buf[i];

		f *= 2.0;
		g *= np->persist;
	}

	return result;
}


void Noise::transformNoiseMap()
{
	size_t i = 0;

	for (int z = 0; z != sz; z++)
	for (int y = 0; y != sy; y++)
	for (int x = 0; x != sx; x++) {
		result[i] = result[i] * np->scale + np->offset;
		i++;
	}
}
