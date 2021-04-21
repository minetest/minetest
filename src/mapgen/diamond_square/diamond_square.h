#pragma once

#include "noise.h"
#include <tuple>
#include <list>

class DiamondSquareMountain
{
	PseudoRandom *prandom;
public:
	DiamondSquareMountain(double angle, double base, double center, double rnd, PseudoRandom *prandom);
	~DiamondSquareMountain();

	void process();
	double get_value(int x, int z);
	void   set_value(int z, int x, double h);

	double **height_map;
	int size;
	int n;

	// random divisor
	double big_rnd;

	// random divisor for small cells
	double small_rnd;
	int small_rnd_size;

	double base;
};

class MountainLandscape
{
    PseudoRandom *prandom;
    public:
        MountainLandscape(PseudoRandom *prandom) {this->prandom = prandom;}
        std::list<std::tuple<DiamondSquareMountain *, std::pair<double, double>>> mountains;

        void AddMountain(double angle, double z, double x, double height, double rnd, double base);
        double Height(double z, double x);

        ~MountainLandscape();
};
