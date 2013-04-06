#include "noise.h"

#include <cmath>

const int OLD_NOISE_MAGIC_SEED = 1013;
const int OLD_NOISE_MAGIC_X = 1619;
const int OLD_NOISE_MAGIC_Y = 31337;
const int OLD_NOISE_MAGIC_Z = 52591;

const int OLD_NOISE_PSEUDO_MAGIC_1 = 60493;
const int OLD_NOISE_PSEUDO_MAGIC_2 = 19990303;
const int OLD_NOISE_PSEUDO_MAGIC_3 = 1376312589;

// All prime
const int NOISE_MAGIC_SEED = 10427;
const int NOISE_MAGIC_X = 13693;
const int NOISE_MAGIC_Y = 24029;
const int NOISE_MAGIC_Z = 44699;

const float SIMPLEX_SKEW_FACTOR_2D   = 0.366025f; // (sqrt(3) - 1)/2
const float SIMPLEX_UNSKEW_FACTOR_2D = 0.211325f; // (3 - sqrt(3))/6

const float SIMPLEX_SKEW_FACTOR_3D   = 0.333333f; // 1/3
const float SIMPLEX_UNSKEW_FACTOR_3D = 0.166667f; // 1/6

// Scales so that value range is within [-1, 1].  Determined numerically.

const float PERLIN_NORMALIZATION_FACTOR_2D = 1.2f;
const float PERLIN_NORMALIZATION_FACTOR_3D = 1.05f;

const float SIMPLEX_NORMALIZATION_FACTOR_2D = 7.4f;
const float SIMPLEX_NORMALIZATION_FACTOR_3D = 9.0f;


//// Local Types

enum CoordOrder3D
{
	COORD_ORDER_3D_XYZ,
	COORD_ORDER_3D_XZY,
	COORD_ORDER_3D_YXZ,
	COORD_ORDER_3D_YZX,
	COORD_ORDER_3D_ZXY,
	COORD_ORDER_3D_ZYX
};


//// Local Functions

/* The "fast floor" implemented in most reference noise implementations
 * "rounds" negative integers to the next lower int, so they return -2 for
 * floor(-1), which is not correct.  This implementation is still faster than
 * calling a library function, but sacrifices the speed of an extra conversion
 * and test for negative numbers in order to be mathematically correct.
 */
static inline int fastFloor(float x)
{
	int xi = (int)x;
	return (x >= 0.0)
	          ? xi
	          : ((x == xi) ? xi : xi-1);
}

static inline float lerp(float t, float a, float b)
{
	return a + t*(b - a);
}

static inline float simplexFade(float v, float rSq)
{
	if (rSq >= 0.5f) { return 0.0f; }
	else             { float t =  0.5f - rSq; return t*t*v; }
}

static inline float simplexFade(float v, float xf, float yf)
{
	float rSq = xf*xf + yf*yf;
	if (rSq >= 0.5f) { return 0.0f; }
	else             { float t =  0.5f - rSq; return t*t*v; }
}

static inline float simplexFade(float v, float xf, float yf, float zf)
{
	float rSq = xf*xf + yf*yf + zf*zf;
	if (rSq >= 0.5f) { return 0.0f; }
	else             { float t =  0.5f - rSq; return t*t*v; }
}

inline static int oldCoordHash(int seed, int x, int y)
{
	int n = (OLD_NOISE_MAGIC_SEED*seed +
	         OLD_NOISE_MAGIC_X*x +
	         OLD_NOISE_MAGIC_Y*y) & 0x7fffffff;
	n = (n >> 13) ^ n;
	n = (n*(n*n*OLD_NOISE_PSEUDO_MAGIC_1 + OLD_NOISE_PSEUDO_MAGIC_2) +
	     OLD_NOISE_PSEUDO_MAGIC_3) &
	    0x7fffffff;
	return n;
}

inline static int oldCoordHash(int seed, int x, int y, int z)
{
	int n = (OLD_NOISE_MAGIC_SEED*seed +
	         OLD_NOISE_MAGIC_X*x +
	         OLD_NOISE_MAGIC_Y*y +
	         OLD_NOISE_MAGIC_Z*z) & 0x7fffffff;
	n = (n >> 13) ^ n;
	n = (n*(n*n*OLD_NOISE_PSEUDO_MAGIC_1 + OLD_NOISE_PSEUDO_MAGIC_2) +
	     OLD_NOISE_PSEUDO_MAGIC_3) &
	    0x7fffffff;
	return n;
}

// Good enough for a pseudo-random hash for calculating gradients
inline static int coordHash(int seed, int x, int y)
{
	return ((NOISE_MAGIC_SEED*seed) ^
	        (NOISE_MAGIC_X*x) ^
	        (NOISE_MAGIC_Y*y)) & 0x7fffffff;
}

// Good enough for a pseudo-random hash for calculating gradients
inline static int coordHash(int seed, int x, int y, int z)
{
	return ((NOISE_MAGIC_SEED*seed) ^
	        (NOISE_MAGIC_X*x) ^
	        (NOISE_MAGIC_Y*y) ^
	        (NOISE_MAGIC_Z*z)) & 0x7fffffff;
}

inline static CoordOrder3D orderCoords(float x, float y, float z)
{
	if (x > y)
	{
		if (y > z)
		{
			return COORD_ORDER_3D_XYZ;
		} else if (x > z)
		{
			return COORD_ORDER_3D_XZY;
		} else
		{
			return COORD_ORDER_3D_ZXY;
		}
	} else if (x > z)
	{
		return COORD_ORDER_3D_YXZ;
	} else if (y > z)
	{
		return COORD_ORDER_3D_YZX;
	} else
	{
		return COORD_ORDER_3D_ZYX;
	}
}

// Inverse of geometric sum
inline static float fractalNormalizationFactor(int octaves, float persistence)
{
	if (persistence == 1.0f)
	{
		return octaves;
	} else
	{
		return (1.0f - persistence)/(1.0f - std::pow(persistence, octaves + 1));
	}
}


// Functions used by concrete noise class methods.  Defined as static functions
// for performance; can be called many times without overhead of virtual method
// call.

inline static float valueLatticeNoise(int seed, int x, int y)
{
	return 1.0f - (float)oldCoordHash(seed, x, y) / 0x40000000;
}

inline static float valueLatticeNoise(int seed, int x, int y, int z)
{
	return 1.0f - (float)oldCoordHash(seed, x, y, z) / 0x40000000;
}

inline static float perlinNoiseContrib(
							  int seed, int xl, float xf, int yl, float yf)
{
	int h = coordHash(seed, xl, yl);
	int comps = h%3;
	int signs = h/3;

	float u, v;
	switch (comps)
	{
		case 0: u = xf; v = yf;   break;
		case 1: u = xf; v = 0.0f; break;
		case 2: u = yf; v = 0.0f; break;
	}

	if (signs & 0x1) { u = -u; }
	if (signs & 0x2) { v = -v; }

	return u + v;
}

inline static float perlinNoiseContrib(
	int seed, int xl, float xf, int yl, float yf, int zl, float zf)
{
	int h = coordHash(seed, xl, yl, zl);
	int comps = h%3;
	int signs = h/3;

	float u, v;
	switch (comps)
	{
		case 0: u = xf; v = yf; break;
		case 1: u = xf; v = zf; break;
		case 2: u = yf; v = zf; break;
	}

	if (signs & 0x1) { u = -u; }
	if (signs & 0x2) { v = -v; }

	return u + v;
}

inline static float simplexNoiseContrib(
                       int seed, int xl, float xf, int yl, float yf)
{
	float nv = perlinNoiseContrib(seed, xl, xf, yl, yf);
	return simplexFade(nv, xf, yf);
}

inline static float simplexNoiseContrib(
   int seed, int xl, float xf, int yl, float yf, int zl, float zf)
{
	float nv = perlinNoiseContrib(seed, xl, xf, yl, yf, zl, zf);
	return simplexFade(nv, xf, yf, zf);
}

inline static float simplexNoiseValue(int seed, float x, float y)
{
	// skew coordinates (rectangular grid)
	float skewDist = SIMPLEX_SKEW_FACTOR_2D*(x + y);
	int sxl = fastFloor(x + skewDist);
	int syl = fastFloor(y + skewDist);

	float unskewDist = SIMPLEX_UNSKEW_FACTOR_2D*(sxl + syl);
	float sxf = x - (sxl - unskewDist);
	float syf = y - (syl - unskewDist);

	float noiseVal = simplexNoiseContrib(seed, sxl, sxf, syl, syf);

	if (sxf > syf)
	{
		++sxl;
		sxf += SIMPLEX_UNSKEW_FACTOR_2D - 1.0f;
		syf += SIMPLEX_UNSKEW_FACTOR_2D;
		noiseVal += simplexNoiseContrib(seed, sxl, sxf, syl, syf);

		++syl;
		sxf += SIMPLEX_UNSKEW_FACTOR_2D;
		syf += SIMPLEX_UNSKEW_FACTOR_2D - 1.0f;
		noiseVal += simplexNoiseContrib(seed, sxl, sxf, syl, syf);
	} else
	{
		++syl;
		sxf += SIMPLEX_UNSKEW_FACTOR_2D;
		syf += SIMPLEX_UNSKEW_FACTOR_2D - 1.0f;
		noiseVal += simplexNoiseContrib(seed, sxl, sxf, syl, syf);

		++sxl;
		sxf += SIMPLEX_UNSKEW_FACTOR_2D - 1.0f;
		syf += SIMPLEX_UNSKEW_FACTOR_2D;
		noiseVal += simplexNoiseContrib(seed, sxl, sxf, syl, syf);
	}

	return SIMPLEX_NORMALIZATION_FACTOR_2D*noiseVal;
}

inline static float simplexNoiseValue(int seed, float x, float y, float z)
{
	// skew coordinates (rectangular grid)
	float skewDist = SIMPLEX_SKEW_FACTOR_3D*(x + y + z);
	int sxl = fastFloor(x + skewDist);
	int syl = fastFloor(y + skewDist);
	int szl = fastFloor(z + skewDist);

	float unskewDist = SIMPLEX_UNSKEW_FACTOR_3D*(sxl + syl + szl);
	float sxf = x - (sxl - unskewDist);
	float syf = y - (syl - unskewDist);
	float szf = z - (szl - unskewDist);

	float noiseVal = simplexNoiseContrib(seed, sxl, sxf, syl, syf, szl, szf);

	CoordOrder3D co = orderCoords(sxf, syf, szf);

	// first coordinate
	switch (co)
	{
		case COORD_ORDER_3D_XYZ:
		case COORD_ORDER_3D_XZY:
			++sxl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			syf += SIMPLEX_UNSKEW_FACTOR_3D;
			szf += SIMPLEX_UNSKEW_FACTOR_3D;
			break;

		case COORD_ORDER_3D_YXZ:
		case COORD_ORDER_3D_YZX:
			++syl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D;
			syf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			szf += SIMPLEX_UNSKEW_FACTOR_3D;
			break;

		case COORD_ORDER_3D_ZXY:
		case COORD_ORDER_3D_ZYX:
			++szl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D;
			syf += SIMPLEX_UNSKEW_FACTOR_3D;
			szf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			break;
	}
	noiseVal += simplexNoiseContrib(seed, sxl, sxf, syl, syf, szl, szf);

	// second coordinate
	switch (co)
	{
		case COORD_ORDER_3D_YXZ:
		case COORD_ORDER_3D_ZXY:
			++sxl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			syf += SIMPLEX_UNSKEW_FACTOR_3D;
			szf += SIMPLEX_UNSKEW_FACTOR_3D;
			break;

		case COORD_ORDER_3D_XYZ:
		case COORD_ORDER_3D_ZYX:
			++syl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D;
			syf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			szf += SIMPLEX_UNSKEW_FACTOR_3D;
			break;

		case COORD_ORDER_3D_XZY:
		case COORD_ORDER_3D_YZX:
			++szl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D;
			syf += SIMPLEX_UNSKEW_FACTOR_3D;
			szf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			break;
	}
	noiseVal += simplexNoiseContrib(seed, sxl, sxf, syl, syf, szl, szf);

	// third coordinate
	switch (co)
	{
		case COORD_ORDER_3D_YZX:
		case COORD_ORDER_3D_ZYX:
			++sxl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			syf += SIMPLEX_UNSKEW_FACTOR_3D;
			szf += SIMPLEX_UNSKEW_FACTOR_3D;
			break;

		case COORD_ORDER_3D_XZY:
		case COORD_ORDER_3D_ZXY:
			++syl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D;
			syf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			szf += SIMPLEX_UNSKEW_FACTOR_3D;
			break;

		case COORD_ORDER_3D_XYZ:
		case COORD_ORDER_3D_YXZ:
			++szl;
			sxf += SIMPLEX_UNSKEW_FACTOR_3D;
			syf += SIMPLEX_UNSKEW_FACTOR_3D;
			szf += SIMPLEX_UNSKEW_FACTOR_3D - 1.0f;
			break;
	}
	noiseVal += simplexNoiseContrib(seed, sxl, sxf, syl, syf, szl, szf);

	return SIMPLEX_NORMALIZATION_FACTOR_3D*noiseVal;
}


//// Global Functions

Noise* createDefaultBaseNoise()
{
	return new ValueNoise();
}


//// ValueNoise

float ValueNoise::noise(int seed, float x, float y)
{
	int xl = fastFloor(x);
	int yl = fastFloor(y);
	float xf = x - xl;
	float yf = y - yl;

	float v00 = valueLatticeNoise(seed, xl,   yl);
	float v10 = valueLatticeNoise(seed, xl+1, yl);
	float v01 = valueLatticeNoise(seed, xl,   yl+1);
	float v11 = valueLatticeNoise(seed, xl+1, yl+1);

	float tx = easeCurve(xf);
	float ty = easeCurve(yf);

	return lerp(ty, lerp(tx, v00, v10), lerp(tx, v01, v11));
}

float ValueNoise::noise(int seed, float x, float y, float z)
{
	int xl = fastFloor(x);
	int yl = fastFloor(y);
	int zl = fastFloor(z);
	float xf = x - xl;
	float yf = y - yl;
	float zf = z - zl;

	float v000 = valueLatticeNoise(seed, xl,   yl,   zl);
	float v100 = valueLatticeNoise(seed, xl+1, yl,   zl);
	float v010 = valueLatticeNoise(seed, xl,   yl+1, zl);
	float v110 = valueLatticeNoise(seed, xl+1, yl+1, zl);
	float v001 = valueLatticeNoise(seed, xl,   yl,   zl+1);
	float v101 = valueLatticeNoise(seed, xl+1, yl,   zl+1);
	float v011 = valueLatticeNoise(seed, xl,   yl+1, zl+1);
	float v111 = valueLatticeNoise(seed, xl+1, yl+1, zl+1);

	return lerp(zf, lerp(yf, lerp(xf, v000, v100), lerp(xf, v010, v110)),
	                lerp(yf, lerp(xf, v001, v101), lerp(xf, v011, v111)));
}

void ValueNoise::noiseBlock(int seed,
                            int sx, float x0, float dx,
                            int sy, float y0, float dy,
                            float* results)
{
	int xl0 = fastFloor(x0);
	int yl0 = fastFloor(y0);
	int xlN = fastFloor(x0 + (sx-1)*dx);
	int nxl = xlN - xl0 + 2;

	// first slice is [0, nxl); second slice is [nxl, 2*nxl)
	float* ySlices = new float[2*nxl];

	for (int xi = 0; xi < nxl; ++xi)
	{
		int xl = xl0 + xi;
		ySlices[xi]       = valueLatticeNoise(seed, xl, yl0);
		ySlices[xi + nxl] = valueLatticeNoise(seed, xl, yl0+1);
	}

	int ri = 0;

	int yl = yl0;
	float yf = y0 - yl0;
	for (int yi = 0; yi < sy; ++yi)
	{
		float ty = easeCurve(yf);

		int xl = xl0;
		float xf = x0 - xl0;
		for (int xi = 0; xi < sx; ++xi)
		{
			float tx = easeCurve(xf);

			int si = (xl - xl0);
			float v00 = ySlices[si];
			float v10 = ySlices[si + 1];
			float v01 = ySlices[si +     nxl];
			float v11 = ySlices[si + 1 + nxl];

			results[ri++] = lerp(ty, lerp(tx, v00, v10), lerp(tx, v01, v11));

			xf += dx;
			if (xf >= 1.0f)
			{
				int inc = (int)xf;
				xl += inc;
				xf -= inc;
			}
		}

		yf += dy;
		if (yf >= 1.0f)
		{
			int inc = (int)yf;
			yl += inc;
			yf -= inc;
			if (inc == 1)
			{
				for (int xi = 0; xi < nxl; ++xi)
				{
					int xl = xl0 + xi;
					ySlices[xi] = ySlices[xi + nxl];
					ySlices[xi + nxl] = valueLatticeNoise(seed, xl, yl+1);
				}
			} else
			{
				for (int xi = 0; xi < nxl; ++xi)
				{
					int xl = xl0 + xi;
					ySlices[xi]       = valueLatticeNoise(seed, xl, yl);
					ySlices[xi + nxl] = valueLatticeNoise(seed, xl, yl+1);
				}
			}
		}
	}

	delete[] ySlices;
}

void ValueNoise::noiseBlock(int seed,
                            int sx, float x0, float dx,
                            int sy, float y0, float dy,
                            int sz, float z0, float dz,
                            float* results)
{
	int xl0 = fastFloor(x0);
	int yl0 = fastFloor(y0);
	int zl0 = fastFloor(z0);
	int xlN = fastFloor(x0 + (sx-1)*dx);
	int ylN = fastFloor(y0 + (sy-1)*dy);
	int nxl = xlN - xl0 + 2;
	int nyl = ylN - yl0 + 2;
	int nxly = nxl*nyl;

	// first slice is [0, nxly); second slice is [nxly, 2*nxly)
	float* zSlices = new float[2*nxly];

	int si = 0;
	for (int yi = 0; yi < nyl; ++yi)
	{
		int yl = yl0 + yi;
		for (int xi = 0; xi < nxl; ++xi)
		{
			int xl = xl0 + xi;
			zSlices[si] = valueLatticeNoise(seed, xl, yl, zl0);
			zSlices[si + nxly] = valueLatticeNoise(seed, xl, yl, zl0+1);
			++si;
		}
	}

	int ri = 0;

	int zl = zl0;
	float zf = z0 - zl0;
	for (int zi = 0; zi < sz; ++zi)
	{
		int yl = yl0;
		float yf = y0 - yl0;
		for (int yi = 0; yi < sy; ++yi)
		{
			int xl = xl0;
			float xf = x0 - xl0;
			for (int xi = 0; xi < sx; ++xi)
			{
				int si = (xl - xl0) + nxl*(yl - yl0);
				float v000 = zSlices[si];
				float v100 = zSlices[si + 1];
				float v010 = zSlices[si +     nxl];
				float v110 = zSlices[si + 1 + nxl];
				float v001 = zSlices[si +           nxly];
				float v101 = zSlices[si + 1 +       nxly];
				float v011 = zSlices[si +     nxl + nxly];
				float v111 = zSlices[si + 1 + nxl + nxly];
				++si;

				results[ri++] =
				   lerp(zf, lerp(yf, lerp(xf, v000, v100), lerp(xf, v010, v110)),
				   lerp(yf, lerp(xf, v001, v101), lerp(xf, v011, v111)));

				xf += dx;
				if (xf >= 1.0f)
				{
					int inc = (int)xf;
					xl += inc;
					xf -= inc;
				}
			}

			yf += dy;
			if (yf >= 1.0f)
			{
				int inc = (int)yf;
				yl += inc;
				yf -= inc;
			}
		}

		zf += dz;
		if (zf >= 1.0f)
		{
			int inc = (int)zf;
			zl += inc;
			zf -= inc;
			if (inc == 1)
			{
				si = 0;
				for (int yi = 0; yi < nyl; ++yi)
				{
					int yl = yl0 + yi;
					for (int xi = 0; xi < nxl; ++xi)
					{
						int xl = xl0 + xi;
						zSlices[si] = zSlices[si + nxly];
						zSlices[si + nxly] = valueLatticeNoise(seed, xl, yl, zl + 1);
						++si;
					}
				}
			} else
			{
				si = 0;
				for (int yi = 0; yi < nyl; ++yi)
				{
					int yl = yl0 + yi;
					for (int xi = 0; xi < nxl; ++xi)
					{
						int xl = xl0 + xi;
						zSlices[si] = valueLatticeNoise(seed, xl, yl, zl);
						zSlices[si + nxly] = valueLatticeNoise(seed, xl, yl, zl + 1);
						++si;
					}
				}
			}
		}
	}

	delete[] zSlices;
}


//// PerlinNoise

float PerlinNoise::noise(int seed, float x, float y)
{
	int xl = fastFloor(x);
	int yl = fastFloor(y);
	float xf = x - xl;
	float yf = y - yl;

	float v00 = perlinNoiseContrib(seed, xl,   xf,       yl,   yf);
	float v10 = perlinNoiseContrib(seed, xl+1, xf-1.0f,  yl,   yf);
	float v01 = perlinNoiseContrib(seed, xl,   xf,       yl+1, yf-1.0f);
	float v11 = perlinNoiseContrib(seed, xl+1, xf-1.0f,  yl+1, yf-1.0f);

	float tx = easeCurve(xf);
	float ty = easeCurve(yf);

	return PERLIN_NORMALIZATION_FACTOR_2D*
	          lerp(ty, lerp(tx, v00, v10), lerp(tx, v01, v11));
}

float PerlinNoise::noise(int seed, float x, float y, float z)
{
	int xl = fastFloor(x);
	int yl = fastFloor(y);
	int zl = fastFloor(z);
	float xf = x - xl;
	float yf = y - yl;
	float zf = z - zl;

	float v000 =
	   perlinNoiseContrib(seed, xl,   xf,      yl,   yf,      zl,   zf);
	float v100 =
	   perlinNoiseContrib(seed, xl+1, xf-1.0f, yl,   yf,      zl,   zf);
	float v010 =
	   perlinNoiseContrib(seed, xl,   xf,      yl+1, yf-1.0f, zl,   zf);
	float v110 =
	   perlinNoiseContrib(seed, xl+1, xf-1.0f, yl+1, yf-1.0f, zl,   zf);
	float v001 =
	   perlinNoiseContrib(seed, xl,   xf,      yl,   yf,      zl+1, zf-1.0f);
	float v101 =
	   perlinNoiseContrib(seed, xl+1, xf-1.0f, yl,   yf,      zl+1, zf-1.0f);
	float v011 =
	   perlinNoiseContrib(seed, xl,   xf,      yl+1, yf-1.0f, zl+1, zf-1.0f);
	float v111 =
	   perlinNoiseContrib(seed, xl+1, xf-1.0f, yl+1, yf-1.0f, zl+1, zf-1.0f);

	float tx = easeCurve(xf);
	float ty = easeCurve(yf);
	float tz = easeCurve(zf);

	return PERLIN_NORMALIZATION_FACTOR_3D*
	          lerp(tz, lerp(ty, lerp(tx, v000, v100), lerp(tx, v010, v110)),
	                   lerp(ty, lerp(tx, v001, v101), lerp(tx, v011, v111)));
}

void PerlinNoise::noiseBlock(int seed,
                             int sx, float x0, float dx,
                             int sy, float y0, float dy,
                             float* results)
{
	int xl0 = fastFloor(x0);
	int yl0 = fastFloor(y0);

	int ri = 0;
	int yl = yl0;
	float yf = y0 - yl0;
	for (int yi = 0; yi < sy; ++yi)
	{
		float ty = easeCurve(yf);

		int xl = xl0;
		float xf = x0 - xl0;
		for (int xi = 0; xi < sx; ++xi)
		{
			float tx = easeCurve(xf);

			float v00 = perlinNoiseContrib(seed, xl,   xf,      yl,   yf);
			float v10 = perlinNoiseContrib(seed, xl+1, xf-1.0f, yl,   yf);
			float v01 = perlinNoiseContrib(seed, xl,   xf,      yl+1, yf-1.0f);
			float v11 = perlinNoiseContrib(seed, xl+1, xf-1.0f, yl+1, yf-1.0f);

			results[ri++] = PERLIN_NORMALIZATION_FACTOR_2D*
			                   lerp(ty, lerp(tx, v00, v10), lerp(tx, v01, v11));

			xf += dx;
			if (xf >= 1.0f)
			{
				int inc = (int)xf;
				xl += inc;
				xf -= inc;
			}
		}

		yf += dy;
		if (yf >= 1.0f)
		{
			int inc = (int)yf;
			yl += inc;
			yf -= inc;
		}
	}
}

void PerlinNoise::noiseBlock(int seed,
                             int sx, float x0, float dx,
                             int sy, float y0, float dy,
                             int sz, float z0, float dz,
                             float* results)
{
	int xl0 = fastFloor(x0);
	int yl0 = fastFloor(y0);
	int zl0 = fastFloor(z0);

	int ri = 0;
	int zl = zl0;
	float zf = z0 - zl0;
	for (int zi = 0; zi < sz; ++zi)
	{
		float tz = easeCurve(zf);

		int yl = yl0;
		float yf = y0 - yl0;
		for (int yi = 0; yi < sy; ++yi)
		{
			float ty = easeCurve(yf);

			int xl = xl0;
			float xf = x0 - xl0;
			for (int xi = 0; xi < sx; ++xi)
			{
				float tx = easeCurve(xf);

				float v000 = perlinNoiseContrib(
				                seed, xl,   xf,      yl,   yf,      zl,   zf);
				float v100 = perlinNoiseContrib(
				                seed, xl+1, xf-1.0f, yl,   yf,      zl,   zf);
				float v010 = perlinNoiseContrib(
				                seed, xl,   xf,      yl+1, yf-1.0f, zl,   zf);
				float v110 = perlinNoiseContrib(
				                seed, xl+1, xf-1.0f, yl+1, yf-1.0f, zl,   zf);
				float v001 = perlinNoiseContrib(
				                seed, xl,   xf,      yl,   yf,      zl+1, zf-1.0f);
				float v101 = perlinNoiseContrib(
				                seed, xl+1, xf-1.0f, yl,   yf,      zl+1, zf-1.0f);
				float v011 = perlinNoiseContrib(
				                seed, xl,   xf,      yl+1, yf-1.0f, zl+1, zf-1.0f);
				float v111 = perlinNoiseContrib(
				                seed, xl+1, xf-1.0f, yl+1, yf-1.0f, zl+1, zf-1.0f);

				results[ri++] =
				   PERLIN_NORMALIZATION_FACTOR_3D*
				      lerp(tz, lerp(ty, lerp(tx, v000, v100), lerp(tx, v010, v110)),
				      lerp(ty, lerp(tx, v001, v101), lerp(tx, v011, v111)));

				xf += dx;
				if (xf >= 1.0f)
				{
					int inc = (int)xf;
					xl += inc;
					xf -= inc;
				}
			}

			yf += dy;
			if (yf >= 1.0f)
			{
				int inc = (int)yf;
				yl += inc;
				yf -= inc;
			}
		}

		zf += dz;
		if (zf >= 1.0f)
		{
			int inc = (int)zf;
			zl += inc;
			zf -= inc;
		}
	}
}


//// SimplexNoise

float SimplexNoise::noise(int seed, float x, float y)
{
	return simplexNoiseValue(seed, x, y);
}

float SimplexNoise::noise(int seed, float x, float y, float z)
{
	return simplexNoiseValue(seed, x, y, z);
}

void SimplexNoise::noiseBlock(int seed,
                              int sx, float x0, float dx,
                              int sy, float y0, float dy,
                              float* results)
{
	int ri = 0;

	float y = y0;
	for (int yi = 0; yi < sy; ++yi)
	{
		float x = x0;
		for (int xi = 0; xi < sx; ++xi)
		{
			results[ri++] = simplexNoiseValue(seed, x, y);

			x += dx;
		}

		y += dy;
	}
}

void SimplexNoise::noiseBlock(int seed,
                              int sx, float x0, float dx,
                              int sy, float y0, float dy,
                              int sz, float z0, float dz,
                              float* results)
{
	int ri = 0;

	float z = z0;
	for (int zi = 0; zi < sz; ++zi)
	{
		float y = y0;

		for (int yi = 0; yi < sy; ++yi)
		{
			float x = x0;

			for (int xi = 0; xi < sx; ++xi)
			{
				results[ri++] = simplexNoiseValue(seed, x, y, z);

				x += dx;
			}

			y += dy;
		}

		z += dz;
	}
}


//// FractalNoise

float FractalNoise::noise(int seed, float x, float y)
{
	float v = 0.0f;
	float f = 1.0f;
	float g = (m_normalized)
	             ? fractalNormalizationFactor(m_octaves, m_persistence)
	             : 1.0f;

	for (int octave = 0; octave < m_octaves; ++octave)
	{
		float octaveVal = m_noise->noise(seed + octave, f*x, f*y);
		if (m_sharp && octaveVal < 0.0f)
		{
			octaveVal = -octaveVal;
		}
		v += g*octaveVal;
		f *= 2.0f;
		g *= m_persistence;
	}

	return v;
}

float FractalNoise::noise(int seed, float x, float y, float z)
{
	float v = 0.0f;
	float f = 1.0f;
	float g = (m_normalized)
	             ? fractalNormalizationFactor(m_octaves, m_persistence)
	             : 1.0f;

	for (int octave = 0; octave < m_octaves; ++octave)
	{
		float octaveVal = m_noise->noise(seed + octave, f*x, f*y, f*z);
		if (m_sharp && octaveVal < 0.0f)
		{
			octaveVal = -octaveVal;
		}
		v += g*octaveVal;
		f *= 2.0f;
		g *= m_persistence;
	}

	return v;
}

void FractalNoise::noiseBlock(int seed,
                              int sx, float x0, float dx,
                              int sy, float y0, float dy,
                              float* results)
{
	int octaveSize = sx*sy;

	for (int i = 0; i < octaveSize; ++i)
	{
		results[i] = 0.0f;
	}

	float* octaveBuff = new float[octaveSize];

	float f = 1.0f;
	float g = (m_normalized)
	             ? fractalNormalizationFactor(m_octaves, m_persistence)
	             : 1.0f;

	for (int octave = 0; octave < m_octaves; ++octave)
	{
		m_noise->noiseBlock(seed + octave,
		                    sx, f*x0, f*dx,
		                    sy, f*y0, f*dy,
		                    octaveBuff);
		for (int i = 0; i < octaveSize; ++i)
		{
			float octaveVal = octaveBuff[i];
			if (m_sharp && octaveVal < 0.0f)
			{
				octaveVal = -octaveVal;
			}
			results[i] += g*octaveVal;
		}

		f *= 2.0f;
		g *= m_persistence;
	}

	delete[] octaveBuff;
}

void FractalNoise::noiseBlock(int seed,
                              int sx, float x0, float dx,
                              int sy, float y0, float dy,
                              int sz, float z0, float dz,
                              float* results)
{
	int octaveSize = sx*sy*sz;

	for (int i = 0; i < octaveSize; ++i)
	{
		results[i] = 0.0f;
	}

	float* octaveBuff = new float[octaveSize];

	float f = 1.0f;
	float g = (m_normalized)
	             ? fractalNormalizationFactor(m_octaves, m_persistence)
	             : 1.0f;

	for (int octave = 0; octave < m_octaves; ++octave)
	{
		m_noise->noiseBlock(seed + octave,
		                    sx, f*x0, f*dx,
		                    sy, f*y0, f*dy,
		                    sz, f*z0, f*dz,
		                    octaveBuff);
		for (int i = 0; i < octaveSize; ++i)
		{
			float octaveVal = octaveBuff[i];
			if (m_sharp && octaveVal < 0.0f)
			{
				octaveVal = -octaveVal;
			}
			results[i] += g*octaveVal;
		}

		f *= 2.0f;
		g *= m_persistence;
	}

	delete[] octaveBuff;
}


//// DynamicFractalNoise

float DynamicFractalNoise::noise(int seed, float x, float y)
{
	float v = 0.0f;
	float f = 1.0f;
	float g = 1.0f;

	float persistence = m_persistenceNoise->noise(seed, x, y);
	if (persistence < 0.0f) { persistence = -persistence; }

	for (int octave = 0; octave < m_octaves; ++octave)
	{
		float octaveVal = m_octaveNoise->noise(seed + octave, f*x, f*y);
		if (m_sharp && octaveVal < 0.0f)
		{
			octaveVal = -octaveVal;
		}
		v += g*octaveVal;
		f *= 2.0f;
		g *= persistence;
	}

	return v;
}

float DynamicFractalNoise::noise(int seed, float x, float y, float z)
{
	float v = 0.0f;
	float f = 1.0f;
	float g = 1.0f;

	float persistence = m_persistenceNoise->noise(seed, x, y);
	if (persistence < 0.0f) { persistence = -persistence; }

	for (int octave = 0; octave < m_octaves; ++octave)
	{
		float octaveVal = m_octaveNoise->noise(seed + octave, f*x, f*y, f*z);
		if (m_sharp && octaveVal < 0.0f)
		{
			octaveVal = -octaveVal;
		}
		v += g*octaveVal;
		f *= 2.0f;
		g *= persistence;
	}

	return v;
}

void DynamicFractalNoise::noiseBlock(int seed,
                                     int sx, float x0, float dx,
                                     int sy, float y0, float dy,
                                     float* results)
{
	int octaveSize = sx*sy;

	for (int i = 0; i < octaveSize; ++i)
	{
		results[i] = 0.0f;
	}

	float* octaveBuff = new float[octaveSize];
	float* persistenceBuff = new float[octaveSize];

	float f = 1.0f;

	m_persistenceNoise->noiseBlock(seed,
	                               sx, x0, dx,
	                               sy, y0, dy,
	                               persistenceBuff);

	for (int octave = 0; octave < m_octaves; ++octave)
	{
		m_octaveNoise->noiseBlock(seed + octave,
		                          sx, f*x0, f*dx,
		                          sy, f*y0, f*dy,
		                          octaveBuff);
		for (int i = 0; i < octaveSize; ++i)
		{
			// For performance we could avoid this with an outer index loop and an
			// inner octave loop, but we'd need to keep all octaves in memory,
			// consuming a much larger amount of space.
			float persist = persistenceBuff[i];
			if (persist < 0.0f) { persist = -persist; }
			float g = (float)pow((double)persist, octave);

			float octaveVal = octaveBuff[i];
			if (m_sharp && octaveVal < 0.0f)
			{
				octaveVal = -octaveVal;
			}
			results[i] += g*octaveVal;
		}

		f *= 2.0f;
	}

	delete[] persistenceBuff;
	delete[] octaveBuff;
}

void DynamicFractalNoise::noiseBlock(int seed,
                                     int sx, float x0, float dx,
                                     int sy, float y0, float dy,
                                     int sz, float z0, float dz,
                                     float* results)
{
	int octaveSize = sx*sy*sz;

	for (int i = 0; i < octaveSize; ++i)
	{
		results[i] = 0.0f;
	}

	float* octaveBuff = new float[octaveSize];
	float* persistenceBuff = new float[octaveSize];

	float f = 1.0f;

	m_persistenceNoise->noiseBlock(seed,
	                               sx, x0, dx,
	                               sy, y0, dy,
	                               sz, z0, dz,
	                               persistenceBuff);

	for (int octave = 0; octave < m_octaves; ++octave)
	{
		m_octaveNoise->noiseBlock(seed + octave,
		                          sx, f*x0, f*dx,
		                          sy, f*y0, f*dy,
		                          sz, f*z0, f*dz,
		                          octaveBuff);
		for (int i = 0; i < octaveSize; ++i)
		{
			// For performance we could avoid this with an outer index loop and an
			// inner octave loop, but we'd need to keep all octaves in memory,
			// consuming a much larger amount of space.
			float persist = persistenceBuff[i];
			if (persist < 0.0f) { persist = -persist; }
			float g = (float)pow((double)persist, octave);

			float octaveVal = octaveBuff[i];
			if (m_sharp && octaveVal < 0.0f)
			{
				octaveVal = -octaveVal;
			}
			results[i] += g*octaveVal;
		}

		f *= 2.0f;
	}

	delete[] persistenceBuff;
	delete[] octaveBuff;
}


//// TransformedNoise

float TransformedNoise::noise(int seed, float x, float y)
{
	seed += m_baseSeed;
	x /= m_xSpread;
	y /= m_ySpread;

	return m_offset + m_scale*m_noise->noise(seed, x, y);
}

float TransformedNoise::noise(int seed, float x, float y, float z)
{
	seed += m_baseSeed;
	x /= m_xSpread;
	y /= m_ySpread;
	z /= m_zSpread;

	return m_offset + m_scale*m_noise->noise(seed, x, y, z);
}

void TransformedNoise::noiseBlock(int seed,
                                  int sx, float x0, float dx,
                                  int sy, float y0, float dy,
                                  float* results)
{
	seed += m_baseSeed;
	x0 /= m_xSpread; dx /= m_xSpread;
	y0 /= m_ySpread; dy /= m_ySpread;

	m_noise->noiseBlock(seed, sx, x0, dx, sy, y0, dy, results);

	int size = sx*sy;
	for (int i = 0; i < size; ++i)
	{
		results[i] = m_offset + m_scale*results[i];
	}
}

void TransformedNoise::noiseBlock(int seed,
                                  int sx, float x0, float dx,
                                  int sy, float y0, float dy,
                                  int sz, float z0, float dz,
                                  float* results)
{
	seed += m_baseSeed;
	x0 /= m_xSpread; dx /= m_xSpread;
	y0 /= m_ySpread; dy /= m_ySpread;
	z0 /= m_zSpread; dz /= m_zSpread;

	m_noise->noiseBlock(seed, sx, x0, dx, sy, y0, dy, sz, z0, dz, results);

	int size = sx*sy*sz;
	for (int i = 0; i < size; ++i)
	{
		results[i] = m_offset + m_scale*results[i];
	}
}


//// MixedNoise

float MixedNoise::noise(int seed, float x, float y)
{
	float u = m_noiseU->noise(seed, x, y);
	float v = m_noiseV->noise(seed, x, y);
	return m_a*u + m_b*v + m_c*u*v;
}

float MixedNoise::noise(int seed, float x, float y, float z)
{
	float u = m_noiseU->noise(seed, x, y, z);
	float v = m_noiseV->noise(seed, x, y, z);
	return m_a*u + m_b*v + m_c*u*v;
}

void MixedNoise::noiseBlock(int seed,
                            int sx, float x0, float dx,
                            int sy, float y0, float dy,
                            float* results)
{
	int size = sx*sy;
	float* uData = new float[size];

	m_noiseU->noiseBlock(seed, sx, x0, dx, sy, y0, dy, uData);
	m_noiseV->noiseBlock(seed, sx, x0, dx, sy, y0, dy, results);

	for (int i = 0; i < size; ++i)
	{
		float u = uData[i];
		float v = results[i];
		results[i] = m_a*u + m_b*v + m_c*u*v;
	}

	delete[] uData;
}

void MixedNoise::noiseBlock(int seed,
                            int sx, float x0, float dx,
                            int sy, float y0, float dy,
                            int sz, float z0, float dz,
                            float* results)
{
	int size = sx*sy*sz;
	float* uData = new float[size];

	m_noiseU->noiseBlock(seed, sx, x0, dx, sy, y0, dy, sz, z0, dz, uData);
	m_noiseV->noiseBlock(seed, sx, x0, dx, sy, y0, dy, sz, z0, dz, results);

	for (int i = 0; i < size; ++i)
	{
		float u = uData[i];
		float v = results[i];
		results[i] = m_a*u + m_b*v + m_c*u*v;
	}

	delete[] uData;
}
