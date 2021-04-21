#include "diamond_square.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

DiamondSquareMountain::DiamondSquareMountain(double angle, double base, double center, double rnd, PseudoRandom *prandom)
{
    this->prandom = prandom;
    this->base = base;
	double h = center - base;
	double w = 2*h*tan(angle*3.141/180);
	n = int(log(w)/log(2)+1);

	int half = pow(2, n-1);
	size = pow(2, n);
	height_map = new double*[size+1];
	for (int i = 0; i < size+1; i++)
	{
		height_map[i] = new double[size+1];
		for (int j = 0; j < size+1; j++)
			height_map[i][j] = -1;
	}

	// Init diamond square

    // corners
    double hs = center - (center-base)*1.41;
	height_map[0][0] = hs;
	height_map[0][size] = hs;
	height_map[size][0] = hs;
	height_map[size][size] = hs;

    // center
	height_map[half][half] = center;

    // sides
    height_map[0][half] = base;
	height_map[half][0] = base;
	height_map[size][half] = base;
	height_map[half][size] = base;

    big_rnd = rnd;
	small_rnd = big_rnd*2;
	small_rnd_size = 8;
	process();
}

double DiamondSquareMountain::get_value(int z, int x)
{
	if (x < 0 || z < 0 || x > size || z > size)
	{
        return base;
    }

    return height_map[z][x];
}

void DiamondSquareMountain::set_value(int z, int x, double h)
{
	if (x < 0 || z < 0 || x > size || z > size)
	{
        return;
    }
    height_map[z][x] = h;
}

void DiamondSquareMountain::process()
{
	for (int i = n-1; i >= 1; i--)
	{
		double random_div;
		int step = pow(2, i);
		int step2 = step/2;
		
		int num = pow(2, n-i);

		if (step >= small_rnd_size)
			random_div = big_rnd;
		else
			random_div = small_rnd;

		// Diamond step
		int z = 0;
        for (int p = 1; p <= num; p++)
		{
            int x = 0;
            for (int q = 1; q <= num; q++)
			{
                double val1 = get_value(z, x);
                double val2 = get_value(z+step, x);
                double val3 = get_value(z, x+step);
                double val4 = get_value(z+step, x+step);
                double rv = prandom->range(-step2/random_div, step2/random_div);

                double val = (val1 + val2 + val3 + val4)/4 + rv;
                set_value(z+step2, x+step2, val);
                x += step;
			}
            z += step;
        }

		// Square step
		z = 0;
        for (int p = 1; p <= num+1; p++)
		{
	        int x = 0;
	        for (int q = 1; q <= num+1; q++)
			{
				double val1, val2, val3, val4;
				double val, rv;

				val1 = get_value(z-step2, x+step2);
				val2 = get_value(z+step2, x+step2);
				val3 = get_value(z, x);
				val4 = get_value(z, x+step);
				rv = prandom->range(-step2/random_div, step2/random_div);

				val = (val1 + val2 + val3 + val4)/4 + rv;
				set_value(z, x+step2, val);

				val1 = get_value(z+step2, x-step2);
				val2 = get_value(z+step2, x+step2);
				val3 = get_value(z, x);
				val4 = get_value(z+step, x);
				rv = prandom->range(-step2/random_div, step2/random_div);

				val = (val1 + val2 + val3 + val4)/4 + rv;
				set_value(z+step2, x, val);

				x += step;
			}
            z += step;
        }
    }

    // cut center peak
	int half = size/2;
	double val1 = get_value(half-1, half);
	double val2 = get_value(half+1, half);
    double val3 = get_value(half, half-1);
    double val4 = get_value(half, half+1);

	double val = (val1 + val2 + val3 + val4)/4;
	set_value(half, half, val); 
}

DiamondSquareMountain::~DiamondSquareMountain()
{
	for (int i = 0; i < size; i++)
		delete[] height_map[i];
	delete height_map;
}

void MountainLandscape::AddMountain(double angle, double z, double x, double height, double rnd, double base)
{
    DiamondSquareMountain *mountain = new DiamondSquareMountain(angle, base, height, rnd, prandom);
    mountains.push_back(std::make_tuple(mountain, std::make_pair(z, x)));
}

double MountainLandscape::Height(double z, double x)
{
    double h = 0;
    for (auto item : mountains)
    {
        double posz = std::get<1>(item).first;
        double posx = std::get<1>(item).second;
        auto m = std::get<0>(item);

        int mx = x - posx + m->size/2;
		int mz = z - posz + m->size/2;
		
        double height = m->get_value(mz, mx);
        h = std::max(h, height);
    }

    return std::max(h, 0.0);
}

MountainLandscape::~MountainLandscape()
{
    for (auto item : mountains)
    {
        auto m = std::get<0>(item);
        delete m;
    }
}
