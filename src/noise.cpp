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

double noise2d(int x, int y, int seed)
{
	int n = (NOISE_MAGIC_X * x + NOISE_MAGIC_Y * y
			+ NOISE_MAGIC_SEED * seed) & 0x7fffffff;
	n = (n>>13)^n;
	n = (n * (n*n*60493+19990303) + 1376312589) & 0x7fffffff;
	return 1.0 - (double)n/1073741824;
}

double noise2d_gradient(double x, double y, int seed)
{
	int x0 = (x > 0.0 ? (int)x : (int)x - 1);
	int y0 = (y > 0.0 ? (int)y : (int)y - 1);
	double xl = x - (double)x0;
	double yl = y - (double)y0;
	int n00 = (int)((noise2d(x0, y0, seed)+1)*8);
	int n10 = (int)((noise2d(x0+1, y0, seed)+1)*8);
	int n01 = (int)((noise2d(x0, y0+1, seed)+1)*8);
	int n11 = (int)((noise2d(x0+1, y0+1, seed)+1)*8);
	/* In this format, these fail to work on MSVC8 if n00 < 4
	double s = dotProduct(cos_lookup[n00], cos_lookup[(n00-4)%16], xl, yl);
	double u = dotProduct(-cos_lookup[n10], cos_lookup[(n10-4)%16], 1.-xl, yl);
	double v = dotProduct(cos_lookup[n01], -cos_lookup[(n01-4)%16], xl, 1.-yl);
	double w = dotProduct(-cos_lookup[n11], -cos_lookup[(n11-4)%16], 1.-xl, 1.-yl);*/
	double s = dotProduct(cos_lookup[n00], cos_lookup[(n00+12)%16], xl, yl);
	double u = dotProduct(-cos_lookup[n10], cos_lookup[(n10+12)%16], 1.-xl, yl);
	double v = dotProduct(cos_lookup[n01], -cos_lookup[(n01+12)%16], xl, 1.-yl);
	double w = dotProduct(-cos_lookup[n11], -cos_lookup[(n11+12)%16], 1.-xl, 1.-yl);
	/*std::cout<<"x="<<x<<" y="<<y<<" x0="<<x0<<" y0="<<y0<<" xl="<<xl<<" yl="<<yl<<" n00="<<n00<<" n10="<<n01<<" s="<<s<<std::endl;
	std::cout<<"cos_lookup[n00]="<<(cos_lookup[n00])<<" cos_lookup[(n00-4)%16]="<<(cos_lookup[(n00-4)%16])<<std::endl;*/
	return biLinearInterpolation(s,u,v,w,xl,yl);
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

