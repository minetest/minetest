/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <math.h>
#include "noise.h"
#include <iostream>
#include "debug.h"

#define NOISE_MAGIC_X 1619
#define NOISE_MAGIC_Y 31337
#define NOISE_MAGIC_Z 52591
#define NOISE_MAGIC_SEED 1013

double cos_lookup[16] = {
	1.0,0.9238,0.7071,0.3826,0,-0.3826,-0.7071,-0.9238,
	1.0,-0.9238,-0.7071,-0.3826,0,0.3826,0.7071,0.9238
};

double dotProduct(double vx, double vy, double wx, double wy){
    return vx*wx+vy*wy;
}
 
double easeCurve(double t){
    return 6*pow(t,5)-15*pow(t,4)+10*pow(t,3);
}
 
double linearInterpolation(double x0, double x1, double t){
    return x0+(x1-x0)*t;
}
 
double biLinearInterpolation(double x0y0, double x1y0, double x0y1, double x1y1, double x, double y){
    double tx = easeCurve(x);
    double ty = easeCurve(y);
	/*double tx = x;
	double ty = y;*/
    double u = linearInterpolation(x0y0,x1y0,tx);
    double v = linearInterpolation(x0y1,x1y1,tx);
    return linearInterpolation(u,v,ty);
}

double triLinearInterpolation(
		double v000, double v100, double v010, double v110,
		double v001, double v101, double v011, double v111,
		double x, double y, double z)
{
    /*double tx = easeCurve(x);
    double ty = easeCurve(y);
    double tz = easeCurve(z);*/
    double tx = x;
    double ty = y;
    double tz = z;
	return(
		v000*(1-tx)*(1-ty)*(1-tz) +
		v100*tx*(1-ty)*(1-tz) +
		v010*(1-tx)*ty*(1-tz) +
		v110*tx*ty*(1-tz) +
		v001*(1-tx)*(1-ty)*tz +
		v101*tx*(1-ty)*tz +
		v011*(1-tx)*ty*tz +
		v111*tx*ty*tz
	);
}

double noise2d(int x, int y, int seed)
{
	int n = (NOISE_MAGIC_X * x + NOISE_MAGIC_Y * y
			+ NOISE_MAGIC_SEED * seed) & 0x7fffffff;
	n = (n>>13)^n;
	n = (n * (n*n*60493+19990303) + 1376312589) & 0x7fffffff;
	return 1.0 - (double)n/1073741824;
}

double noise3d(int x, int y, int z, int seed)
{
	int n = (NOISE_MAGIC_X * x + NOISE_MAGIC_Y * y + NOISE_MAGIC_Z * z
			+ NOISE_MAGIC_SEED * seed) & 0x7fffffff;
	n = (n>>13)^n;
	n = (n * (n*n*60493+19990303) + 1376312589) & 0x7fffffff;
	return 1.0 - (double)n/1073741824;
}

#if 0
double noise2d_gradient(double x, double y, int seed)
{
	// Calculate the integer coordinates
	int x0 = (x > 0.0 ? (int)x : (int)x - 1);
	int y0 = (y > 0.0 ? (int)y : (int)y - 1);
	// Calculate the remaining part of the coordinates
	double xl = x - (double)x0;
	double yl = y - (double)y0;
	// Calculate random cosine lookup table indices for the integer corners.
	// They are looked up as unit vector gradients from the lookup table.
	int n00 = (int)((noise2d(x0, y0, seed)+1)*8);
	int n10 = (int)((noise2d(x0+1, y0, seed)+1)*8);
	int n01 = (int)((noise2d(x0, y0+1, seed)+1)*8);
	int n11 = (int)((noise2d(x0+1, y0+1, seed)+1)*8);
	// Make a dot product for the gradients and the positions, to get the values
	double s = dotProduct(cos_lookup[n00], cos_lookup[(n00+12)%16], xl, yl);
	double u = dotProduct(-cos_lookup[n10], cos_lookup[(n10+12)%16], 1.-xl, yl);
	double v = dotProduct(cos_lookup[n01], -cos_lookup[(n01+12)%16], xl, 1.-yl);
	double w = dotProduct(-cos_lookup[n11], -cos_lookup[(n11+12)%16], 1.-xl, 1.-yl);
	// Interpolate between the values
	return biLinearInterpolation(s,u,v,w,xl,yl);
}
#endif

#if 1
double noise2d_gradient(double x, double y, int seed)
{
	// Calculate the integer coordinates
	int x0 = (x > 0.0 ? (int)x : (int)x - 1);
	int y0 = (y > 0.0 ? (int)y : (int)y - 1);
	// Calculate the remaining part of the coordinates
	double xl = x - (double)x0;
	double yl = y - (double)y0;
	// Get values for corners of cube
	double v00 = noise2d(x0, y0, seed);
	double v10 = noise2d(x0+1, y0, seed);
	double v01 = noise2d(x0, y0+1, seed);
	double v11 = noise2d(x0+1, y0+1, seed);
	// Interpolate
	return biLinearInterpolation(v00,v10,v01,v11,xl,yl);
}
#endif

double noise3d_gradient(double x, double y, double z, int seed)
{
	// Calculate the integer coordinates
	int x0 = (x > 0.0 ? (int)x : (int)x - 1);
	int y0 = (y > 0.0 ? (int)y : (int)y - 1);
	int z0 = (z > 0.0 ? (int)z : (int)z - 1);
	// Calculate the remaining part of the coordinates
	double xl = x - (double)x0;
	double yl = y - (double)y0;
	double zl = z - (double)z0;
	// Get values for corners of cube
	double v000 = noise3d(x0, y0, z0, seed);
	double v100 = noise3d(x0+1, y0, z0, seed);
	double v010 = noise3d(x0, y0+1, z0, seed);
	double v110 = noise3d(x0+1, y0+1, z0, seed);
	double v001 = noise3d(x0, y0, z0+1, seed);
	double v101 = noise3d(x0+1, y0, z0+1, seed);
	double v011 = noise3d(x0, y0+1, z0+1, seed);
	double v111 = noise3d(x0+1, y0+1, z0+1, seed);
	// Interpolate
	return triLinearInterpolation(v000,v100,v010,v110,v001,v101,v011,v111,xl,yl,zl);
}

double noise2d_perlin(double x, double y, int seed,
		int octaves, double persistence)
{
	double a = 0;
	double f = 1.0;
	double g = 1.0;
	for(int i=0; i<octaves; i++)
	{
		a += g * noise2d_gradient(x*f, y*f, seed+i);
		f *= 2.0;
		g *= persistence;
	}
	return a;
}

double noise2d_perlin_abs(double x, double y, int seed,
		int octaves, double persistence)
{
	double a = 0;
	double f = 1.0;
	double g = 1.0;
	for(int i=0; i<octaves; i++)
	{
		a += g * fabs(noise2d_gradient(x*f, y*f, seed+i));
		f *= 2.0;
		g *= persistence;
	}
	return a;
}

double noise3d_perlin(double x, double y, double z, int seed,
		int octaves, double persistence)
{
	double a = 0;
	double f = 1.0;
	double g = 1.0;
	for(int i=0; i<octaves; i++)
	{
		a += g * noise3d_gradient(x*f, y*f, z*f, seed+i);
		f *= 2.0;
		g *= persistence;
	}
	return a;
}

double noise3d_perlin_abs(double x, double y, double z, int seed,
		int octaves, double persistence)
{
	double a = 0;
	double f = 1.0;
	double g = 1.0;
	for(int i=0; i<octaves; i++)
	{
		a += g * fabs(noise3d_gradient(x*f, y*f, z*f, seed+i));
		f *= 2.0;
		g *= persistence;
	}
	return a;
}

// -1->0, 0->1, 1->0
double contour(double v)
{
	v = fabs(v);
	if(v >= 1.0)
		return 0.0;
	return (1.0-v);
}

double noise3d_param(const NoiseParams &param, double x, double y, double z)
{
	double s = param.pos_scale;
	x /= s;
	y /= s;
	z /= s;

	if(param.type == NOISE_PERLIN)
	{
		return param.noise_scale*noise3d_perlin(x,y,z, param.seed,
				param.octaves,
				param.persistence);
	}
	else if(param.type == NOISE_PERLIN_ABS)
	{
		return param.noise_scale*noise3d_perlin_abs(x,y,z, param.seed,
				param.octaves,
				param.persistence);
	}
	else if(param.type == NOISE_PERLIN_CONTOUR)
	{
		return contour(param.noise_scale*noise3d_perlin(x,y,z,
				param.seed, param.octaves,
				param.persistence));
	}
	else if(param.type == NOISE_PERLIN_CONTOUR_FLIP_YZ)
	{
		return contour(param.noise_scale*noise3d_perlin(x,z,y,
				param.seed, param.octaves,
				param.persistence));
	}
	else assert(0);
}

/*
	NoiseBuffer
*/

NoiseBuffer::NoiseBuffer():
	m_data(NULL)
{
}

NoiseBuffer::~NoiseBuffer()
{
	clear();
}

void NoiseBuffer::clear()
{
	if(m_data)
		delete[] m_data;
	m_data = NULL;
	m_size_x = 0;
	m_size_y = 0;
	m_size_z = 0;
}

void NoiseBuffer::create(const NoiseParams &param,
		double first_x, double first_y, double first_z,
		double last_x, double last_y, double last_z,
		double samplelength_x, double samplelength_y, double samplelength_z)
{
	clear();
	
	m_start_x = first_x - samplelength_x;
	m_start_y = first_y - samplelength_y;
	m_start_z = first_z - samplelength_z;
	m_samplelength_x = samplelength_x;
	m_samplelength_y = samplelength_y;
	m_samplelength_z = samplelength_z;

	m_size_x = (last_x - m_start_x)/samplelength_x + 2;
	m_size_y = (last_y - m_start_y)/samplelength_y + 2;
	m_size_z = (last_z - m_start_z)/samplelength_z + 2;

	m_data = new double[m_size_x*m_size_y*m_size_z];

	for(int x=0; x<m_size_x; x++)
	for(int y=0; y<m_size_y; y++)
	for(int z=0; z<m_size_z; z++)
	{
		double xd = (m_start_x + (double)x*m_samplelength_x);
		double yd = (m_start_y + (double)y*m_samplelength_y);
		double zd = (m_start_z + (double)z*m_samplelength_z);
		double a = noise3d_param(param, xd,yd,zd);
		intSet(x,y,z, a);
	}
}

void NoiseBuffer::multiply(const NoiseParams &param)
{
	assert(m_data != NULL);

	for(int x=0; x<m_size_x; x++)
	for(int y=0; y<m_size_y; y++)
	for(int z=0; z<m_size_z; z++)
	{
		double xd = (m_start_x + (double)x*m_samplelength_x);
		double yd = (m_start_y + (double)y*m_samplelength_y);
		double zd = (m_start_z + (double)z*m_samplelength_z);
		double a = noise3d_param(param, xd,yd,zd);
		intMultiply(x,y,z, a);
	}
}

// Deprecated
void NoiseBuffer::create(int seed, int octaves, double persistence,
		bool abs,
		double first_x, double first_y, double first_z,
		double last_x, double last_y, double last_z,
		double samplelength_x, double samplelength_y, double samplelength_z)
{
	NoiseParams param;
	param.type = abs ? NOISE_PERLIN_ABS : NOISE_PERLIN;
	param.seed = seed;
	param.octaves = octaves;
	param.persistence = persistence;

	create(param, first_x, first_y, first_z,
			last_x, last_y, last_z,
			samplelength_x, samplelength_y, samplelength_z);
}

void NoiseBuffer::intSet(int x, int y, int z, double d)
{
	int i = m_size_x*m_size_y*z + m_size_x*y + x;
	assert(i >= 0);
	assert(i < m_size_x*m_size_y*m_size_z);
	m_data[i] = d;
}

void NoiseBuffer::intMultiply(int x, int y, int z, double d)
{
	int i = m_size_x*m_size_y*z + m_size_x*y + x;
	assert(i >= 0);
	assert(i < m_size_x*m_size_y*m_size_z);
	m_data[i] = m_data[i] * d;
}

double NoiseBuffer::intGet(int x, int y, int z)
{
	int i = m_size_x*m_size_y*z + m_size_x*y + x;
	assert(i >= 0);
	assert(i < m_size_x*m_size_y*m_size_z);
	return m_data[i];
}

double NoiseBuffer::get(double x, double y, double z)
{
	x -= m_start_x;
	y -= m_start_y;
	z -= m_start_z;
	x /= m_samplelength_x;
	y /= m_samplelength_y;
	z /= m_samplelength_z;
	// Calculate the integer coordinates
	int x0 = (x > 0.0 ? (int)x : (int)x - 1);
	int y0 = (y > 0.0 ? (int)y : (int)y - 1);
	int z0 = (z > 0.0 ? (int)z : (int)z - 1);
	// Calculate the remaining part of the coordinates
	double xl = x - (double)x0;
	double yl = y - (double)y0;
	double zl = z - (double)z0;
	// Get values for corners of cube
	double v000 = intGet(x0,   y0,   z0);
	double v100 = intGet(x0+1, y0,   z0);
	double v010 = intGet(x0,   y0+1, z0);
	double v110 = intGet(x0+1, y0+1, z0);
	double v001 = intGet(x0,   y0,   z0+1);
	double v101 = intGet(x0+1, y0,   z0+1);
	double v011 = intGet(x0,   y0+1, z0+1);
	double v111 = intGet(x0+1, y0+1, z0+1);
	// Interpolate
	return triLinearInterpolation(v000,v100,v010,v110,v001,v101,v011,v111,xl,yl,zl);
}

