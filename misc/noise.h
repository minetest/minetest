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

#ifndef NOISE_HEADER
#define NOISE_HEADER

double easeCurve(double t);
 
// Return value: -1 ... 1
double noise2d(int x, int y, int seed);
double noise3d(int x, int y, int z, int seed);

double noise2d_gradient(double x, double y, int seed);
double noise3d_gradient(double x, double y, double z, int seed);

double noise2d_perlin(double x, double y, int seed,
		int octaves, double persistence);

double noise2d_perlin_abs(double x, double y, int seed,
		int octaves, double persistence);

double noise3d_perlin(double x, double y, double z, int seed,
		int octaves, double persistence);

double noise3d_perlin_abs(double x, double y, double z, int seed,
		int octaves, double persistence);

enum NoiseType
{
	NOISE_PERLIN,
	NOISE_PERLIN_ABS,
	NOISE_PERLIN_CONTOUR,
	NOISE_PERLIN_CONTOUR_FLIP_YZ
};

struct NoiseParams
{
	NoiseType type;
	int seed;
	int octaves;
	double persistence;
	double pos_scale;
	double noise_scale; // Useful for contour noises
	
	NoiseParams(NoiseType type_=NOISE_PERLIN, int seed_=0,
			int octaves_=3, double persistence_=0.5,
			double pos_scale_=100.0, double noise_scale_=1.0):
		type(type_),
		seed(seed_),
		octaves(octaves_),
		persistence(persistence_),
		pos_scale(pos_scale_),
		noise_scale(noise_scale_)
	{
	}
};

double noise3d_param(const NoiseParams &param, double x, double y, double z);

class NoiseBuffer
{
public:
	NoiseBuffer();
	~NoiseBuffer();
	
	void clear();
	void create(const NoiseParams &param,
			double first_x, double first_y, double first_z,
			double last_x, double last_y, double last_z,
			double samplelength_x, double samplelength_y, double samplelength_z);
	void multiply(const NoiseParams &param);
	// Deprecated
	void create(int seed, int octaves, double persistence,
			bool abs,
			double first_x, double first_y, double first_z,
			double last_x, double last_y, double last_z,
			double samplelength_x, double samplelength_y, double samplelength_z);

	void intSet(int x, int y, int z, double d);
	void intMultiply(int x, int y, int z, double d);
	double intGet(int x, int y, int z);
	double get(double x, double y, double z);

private:
	double *m_data;
	double m_start_x, m_start_y, m_start_z;
	double m_samplelength_x, m_samplelength_y, m_samplelength_z;
	int m_size_x, m_size_y, m_size_z;
};

#endif

