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

double noise3d_gradient(double x, double y, double z, int seed)
{
	// Calculate the integer coordinates
	int x0 = (x > 0.0 ? (int)x : (int)x - 1);
	int y0 = (y > 0.0 ? (int)y : (int)y - 1);
	int z0 = (z > 0.0 ? (int)z : (int)z - 1);
	// Calculate the remaining part of the coordinates
	double xl = x - (double)x0;
	double yl = y - (double)y0;
	double zl = y - (double)z0;
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

