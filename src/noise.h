/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef NOISE_HEADER
#define NOISE_HEADER

#include "irr_v3d.h"


struct NoiseParams
{
	float offset;
	float scale;
	v3f spread;
	int seed;
	int octaves;
	float persist;
};


// Convenience macros for getting/setting NoiseParams in Settings
#define getNoiseParams(x) getStruct<NoiseParams>((x), "f,f,v3,s32,s32,f")
#define setNoiseParams(x, y) setStruct((x), "f,f,v3,s32,s32,f", (y))


static inline float easeCurve(float t)
{
	return t*t*t*(t*(6.0f*t - 15.0f) + 10.0f);
}


class Noise;
Noise* createDefaultBaseNoise();


/** Class interface for generating 2D and 3D noise.  Classes that implement
  * this interface may provide pseudo-random noise based on lattice point
  * values (value-based Minetest noise), lattice point gradients (Perlin Noise
  * and Simplex Noise), or transformed input coordinates and outputs (Decorator
  * Design Pattern).  Interpolation between gridpoints could be done using
  * cubic linear or ease-based interpolation (lagacy Minetest noise) or
  * simplex-based falloff and summation (Simplex Noise).
  *
  * There are a couple of requirements for classes implementing this interface:
  *
  * 1. All noise generation methods MUST be thread safe, so that they can be
  *    called on the same object from multiple threads concurrently.  Thread
  *    safety need not be guaranteed for other functions such as configuration
  *    methods, but the class documentation should make concurrency issues
  *    clear for API users.
  * 2. For performance, only the block-based noise generation functions may use
  *    dynamic memory allocation (new/delete).
  **/
class Noise
{
	protected:
		Noise() {}
		Noise(const Noise&) {}
		Noise& operator=(const Noise&) { return *this; }

	public:
		virtual ~Noise() {}

		/** Computes an interpolated noise value from the seed and the given (2D)
		  * vector coordinates.
		  *
		  * @param seed
		  *    A position-independent seed for the pseudo-random hash.
		  * @param x
		  *    The point's x coordinate.
		  * @param y
		  *    The point's y coordinate.
		  * @return
		  *    A floating-point noise value.
		  **/
		virtual float noise(int seed, float x, float y) =0;

		/** Computes an interpolated noise value from the seed and the given (3D)
		  * vector coordinates.
		  *
		  * @param seed
		  *    A position-independent seed for the pseudo-random hash.
		  * @param x
		  *    The point's x coordinate.
		  * @param y
		  *    The point's y coordinate.
		  * @param z
		  *    The point's z coordinate.
		  * @return
		  *    A floating-point noise value.
		  **/
		virtual float noise(int seed, float x, float y, float z) =0;

		/** Computes an entire block of noise data at regularly spaced points in
		  * (2D) space.
		  *
		  * @param seed
		  *    A position-independent seed for the pseudo-random hash.
		  * @param sx
		  *    The number of points in the block in the x direction.  Must be
		  *    positive.
		  * @param x0
		  *    The starting x coordinate.
		  * @param dx
		  *    The amount to step the x coordinate for each x-index in the block.
		  *    Must be positive.
		  * @param sy
		  *    The number of points in the block in the x direction.  Must be
		  *    positive.
		  * @param y0
		  *    The starting y coordinate.
		  * @param dy
		  *    The amount to step the y coordinate for each y-index in the block.
		  *    Must be positive.
		  * @param results
		  *    An array in which the results of the noise computations are
		  *    stored.   Must be at least sx*sy in size.  The point with
		  *    coordinate indices (xi, yi) has the spatial coordinates
		  *    (x0 + xi*dx, y0 + yi*dy) and its noise value will be stored at
		  *    array index xi + sx*yi.
		  **/
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results) =0;

		/** Computes an entire block of noise data at regularly spaced points in
		  * (3D) space.
		  *
		  * @param seed
		  *    A position-independent seed for the pseudo-random hash.
		  * @param sx
		  *    The number of points in the block in the x direction.  Must be
		  *    positive.
		  * @param x0
		  *    The starting x coordinate.
		  * @param dx
		  *    The amount to step the x coordinate for each x-index in the block.
		  *    Must be positive.
		  * @param sy
		  *    The number of points in the block in the y direction.  Must be
		  *    positive.
		  * @param y0
		  *    The starting y coordinate.
		  * @param dy
		  *    The amount to step the y coordinate for each y-index in the block.
		  *    Must be positive.
		  * @param sz
		  *    The number of points in the block in the z direction.  Must be
		  *    positive.
		  * @param z0
		  *    The starting z coordinate.
		  * @param dz
		  *    The amount to step the z coordinate for each z-index in the block.
		  *    Must be positive.
		  * @param results
		  *    An array in which the results of the noise computations are
		  *    stored.   Must be at least sx*sy*sz in size.  The point with
		  *    coordinate indices (xi, yi, zi) has the spatial coordinates
		  *    (x0 + xi*dx, y0 + yi*dy, z0 + zi*dz) and its noise value will be
		  *    stored at array index xi + sx*(yi + sy*zi).
		  **/
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results) =0;
};


/** Implements the interpolated value noise used by Minetest 0.4.5 and prior.
  * This class no data, and its objects have no state.  It is fully thread
  * safe.
  **/
class ValueNoise : public virtual Noise
{
	public:
		ValueNoise() {}

		virtual ~ValueNoise() {}

		virtual float noise(int seed, float x, float y);
		virtual float noise(int seed, float x, float y, float z);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results);
};


/** Implements Improved Perlin Noise (cubic-interpolated gradient-based noise).
  * This class no data, and its objects have no state.  It is fully thread
  * safe.
  **/
class PerlinNoise : public virtual Noise
{
	public:
		PerlinNoise() {}

		virtual ~PerlinNoise() {}

		virtual float noise(int seed, float x, float y);
		virtual float noise(int seed, float x, float y, float z);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results);
};


/** Implements Simplex Noise (simplex-interpolated gradient-based noise, Ken
  * Perlin's successor to Improved Perlin Noise).  This class no data, and its
  * objects have no state.  It is fully thread safe.
  **/
class SimplexNoise : public virtual Noise
{
	public:
		SimplexNoise() {}

		virtual ~SimplexNoise() {}

		virtual float noise(int seed, float x, float y);
		virtual float noise(int seed, float x, float y, float z);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results);
};


/** Computes fractal noise by computing multiple octaves of noise, with each
  * successive octave being scaled by another power of a persistence factor.
  * Each octave of noise is computed using another wrapped Noise object, using
  * a Decorator Design Pattern.
  *
  * The code constructing instances of this class must ensure that the lifetime
  * of the wrapped Noise instance is at least as long as the lifetime of the
  * wrapping object.
  *
  * The noise generation methods of this class are themselves thread safe.
  * The configuration functions that set transformation values are NOT safe to
  * be called from multiple threads concurrently, nor may they be called while
  * other threads are calling the noise generation methods.  In other words,
  * an instance should be fully configured before being handed to multiple
  * threads for use in noise generation.
  **/
class FractalNoise : public virtual Noise
{
	private:
		Noise* m_noise;
		int m_octaves;
		float m_persistence;
		bool m_normalized;
		bool m_sharp;

	public:
		/** Constructor that specifies all configuration parameters.
		  *
		  * @param noise
		  *    The underlying Noise object this object is to wrap.  The caller
		  *    retains ownership of the object (so the caller must perform any
		  *    necessary cleanup/deletion).  Must not be NULL.
		  * @param octaves
		  *    Number of octaves of noise to compute.  Must be positive.
		  * @param persistence
		  *    The factor by which to scale each successive octave of noise after
		  *    the first (the nth octave after the first has an overall scaling
		  *    factor of persistence to the nth power).  Must be positive.
		  * @param normalized
		  *    If true, result noise values will be multiplied by a factor which
		  *    accounts for the number of octaves and the persistence, returning
		  *    their range to that of the underlying Noise object's.
		  * @param sharp
		  *    If true, the absolute value of each octaves' noise values are
		  *    taken before adding them to the total.  Note that this also
		  *    may affect the range of the noise range (for example, if the
		  *    underlying Noise object's range is [-1, 1) and normalized is true,
		  *    sharp noise will have the output range [0, 1]).
		  **/
		FractalNoise(Noise* noise,
		             int octaves,
		             float persistence,
		             bool normalized = false,
		             bool sharp = false)
		{
			m_noise = noise;
			m_octaves = octaves;
			m_persistence = persistence;
			m_normalized = normalized;
			m_sharp = sharp;
		}

		virtual ~FractalNoise() {}

		Noise* getWrappedNoise() { return m_noise; }
		void setWrappedNoise(Noise* value) { m_noise = value; }

		int getOctaves() { return m_octaves; }
		void setOctaves(int value) { m_octaves = value; }

		float getPersistence() { return m_persistence; }
		void setPersistence(float value) { m_persistence = value; }

		bool isNormalized() { return m_normalized; }
		void setNormalized(bool value) { m_normalized = value; }

		bool isSharp() { return m_sharp; }
		void setSharp(bool value) { m_sharp = value; }

		virtual float noise(int seed, float x, float y);
		virtual float noise(int seed, float x, float y, float z);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results);
};


/** Like FractalNoise, but takes the "persistence" value from another Noise
  * object.  So this class wraps an "octave noise" object for the actual octave
  * values, and "persistence noise" for determining how to mix the octaves.
  *
  * This class expects the generated persistence noise values to be in the
  * range [0, 1] (or a subset), but it takes the absolute value just in case.
  * Giving it noise that generates values with magnitudes greater than 1 may
  * produce very strange results.
  *
  * The code constructing instances of this class must ensure that the lifetime
  * of the wrapped Noise instances is at least as long as the lifetime of the
  * wrapping object.
  *
  * The noise generation methods of this class are themselves thread safe.
  * The configuration functions that set transformation values are NOT safe to
  * be called from multiple threads concurrently, nor may they be called while
  * other threads are calling the noise generation methods.  In other words,
  * an instance should be fully configured before being handed to multiple
  * threads for use in noise generation.
  **/
class DynamicFractalNoise : public virtual Noise
{
	private:
		Noise* m_octaveNoise;
		Noise* m_persistenceNoise;
		int m_octaves;
		bool m_sharp;

	public:
		/** Constructor that specifies all configuration parameters.
		  *
		  * @param octaveNoise
		  *    The underlying Noise object this object is to wrap and use for
		  *    octave data.  The caller retains ownership of the object (so the
		  *    caller must perform any necessary cleanup/deletion).  Must not be
		  *    NULL.
		  * @param persistenceNoise
		  *    The underlying Noise object this object is to wrap and use for
		  *    the fractal persistence value when mixing the octaves.  The caller
		  *    retains ownership of the object (so the caller must perform any
		  *    necessary cleanup/deletion).  Must not be NULL.
		  * @param octaves
		  *    Number of octaves of noise to compute.  Must be positive.
		  * @param sharp
		  *    If true, the absolute value of each octaves' noise values are
		  *    taken before adding them to the total.  Note that this also
		  *    may affect the range of the noise range (for example, if the
		  *    underlying Noise object's range is [-1, 1) and normalized is true,
		  *    sharp noise will have the output range [0, 1]).
		  **/
		DynamicFractalNoise(Noise* octaveNoise,
		                    Noise* persistenceNoise,
		                    int octaves,
		                    bool sharp = false)
		{
			m_octaveNoise = octaveNoise;
			m_persistenceNoise = persistenceNoise;
			m_octaves = octaves;
			m_sharp = sharp;
		}

		virtual ~DynamicFractalNoise() {}

		Noise* getOctaveNoise() { return m_octaveNoise; }
		void setOctaveNoise(Noise* value) { m_octaveNoise = value; }

		Noise* getPersistenceNoise() { return m_persistenceNoise; }
		void setPersistenceNoise(Noise* value) { m_persistenceNoise = value; }

		int getOctaves() { return m_octaves; }
		void setOctaves(int value) { m_octaves = value; }

		bool isSharp() { return m_sharp; }
		void setSharp(bool value) { m_sharp = value; }

		virtual float noise(int seed, float x, float y);
		virtual float noise(int seed, float x, float y, float z);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results);
};


/** Wraps another Noise object using a Decorator Design Pattern, transforming
  * the seed, input coordinates, and output coordinates according to the
  * settings of the wrapping object.
  *
  * The code constructing instances of this class must ensure that the lifetime
  * of the wrapped Noise instance is at least as long as the lifetime of the
  * wrapping object.
  *
  * The noise generation methods of this class are themselves thread safe.
  * The configuration functions that set transformation values are NOT safe to
  * be called from multiple threads concurrently, nor may they be called while
  * other threads are calling the noise generation methods.  In other words,
  * an instance should be fully configured before being handed to multiple
  * threads for use in noise generation.
  **/
class TransformedNoise : public virtual Noise
{
	private:
		Noise* m_noise;
		float m_offset;
		float m_scale;
		float m_xSpread;
		float m_ySpread;
		float m_zSpread;
		int m_baseSeed;

	public:
		/** Equivalent to TransformedNoise(noise, 0.0f, 1.0f, 1.0f, 1.0f, 1.0, 0).
		  * This is provided for convenience, so that the caller can set one or a
		  * couple values while allowing the rest to default.
		  **/
		TransformedNoise(Noise* noise)
		{
			m_noise = noise;
			m_offset = 0.0f;
			m_scale = 1.0f;
			m_xSpread = 1.0f;
			m_ySpread = 1.0f;
			m_zSpread = 1.0f;
			m_baseSeed = 0;
		}

		/** Equivalent to
		  * TransformedNoise(noise, offset, scale, xSpread, ySpread, 1.0,
		  *                  baseSeed).
		  **/
		TransformedNoise(Noise* noise,
		                 float offset,
		                 float scale,
		                 float xSpread, float ySpread,
		                 int baseSeed)
		{
			m_noise = noise;
			m_offset = offset;
			m_scale = scale;
			m_xSpread = xSpread;
			m_ySpread = ySpread;
			m_zSpread = 1.0f;
			m_baseSeed = baseSeed;
		}

		/** Constructor that specifies all instance configuration parameters.
		  *
		  * @param noise
		  *    The underlying Noise object this object is to wrap.  The caller
		  *    retains ownership of the object (so the caller must perform any
		  *    necessary cleanup/deletion).  Must not be NULL.
		  * @param offset
		  *    The amount by which to offset all noise values after scaling them.
		  * @param scale
		  *    The amount by which to scale all generated noise values.
		  * @param xSpread
		  *    The real distance between lattice points in the x direction.
		  *    Input x values are scaled by the inverse before being passed to
		  *    the underlying noise algorithm.  Must be non-zero.
		  * @param ySpread
		  *    Like xSpread, by in the y direction.
		  * @param zSpread
		  *    Like xSpread, by in the z direction.
		  * @param baseSeed
		  *    A seed value specific to this object.  Added to all seed values
		  *    passed to the noise generation methods.
		  **/
		TransformedNoise(Noise* noise,
		                 float offset,
		                 float scale,
		                 float xSpread, float ySpread, float zSpread,
		                 int baseSeed)
		{
			m_noise = noise;
			m_offset = offset;
			m_scale = scale;
			m_xSpread = xSpread;
			m_ySpread = ySpread;
			m_zSpread = zSpread;
			m_baseSeed = baseSeed;
		}

		virtual ~TransformedNoise() {}

		Noise* getWrappedNoise() { return m_noise; }
		void setWrappedNoise(Noise* value) { m_noise = value; }

		float getOffset() { return m_offset; }
		void setOffset(float value) { m_offset = value; }

		float getScale() { return m_scale; }
		void setScale(float value) { m_scale = value; }

		float getXSpread() { return m_xSpread; }
		void setXSpread(float value) { m_xSpread = value; }

		float getYSpread() { return m_ySpread; }
		void setYSpread(float value) { m_ySpread = value; }

		float getZSpread() { return m_zSpread; }
		void setZSpread(float value) { m_zSpread = value; }

		void setSpread(float xSpread, float ySpread, float zSpread)
			{ m_xSpread = xSpread; m_ySpread = ySpread; m_zSpread = zSpread; }

		int getBaseSeed() { return m_baseSeed; }
		void setBaseSeed(int value) { m_baseSeed = value; }

		virtual float noise(int seed, float x, float y);
		virtual float noise(int seed, float x, float y, float z);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results);
};


/** Mixes the outputs of two different Noise objects.
  *
  * If the output value from one noise input is u and the output value from the
  * other noise input is v, then the resulting noise will be a*u + b*v + c*u*v.
  *
  * The code constructing instances of this class must ensure that the lifetime
  * of the wrapped Noise instances is at least as long as the lifetime of the
  * wrapping object.
  *
  * The noise generation methods of this class are themselves thread safe.
  * The configuration functions that set transformation values are NOT safe to
  * be called from multiple threads concurrently, nor may they be called while
  * other threads are calling the noise generation methods.  In other words,
  * an instance should be fully configured before being handed to multiple
  * threads for use in noise generation.
  **/
class MixedNoise : public virtual Noise
{
	private:
		Noise* m_noiseU;
		Noise* m_noiseV;
		float m_a;
		float m_b;
		float m_c;

	public:
		/** Constructor that specifies all instance configuration parameters.
		  *
		  * Given Noise inputs u and v and mix parameters a, b, and c, the
		  * resulting noise value will be a*u + b*v + c*u*v.
		  *
		  * @param noiseU
		  *    The first underlying Noise object this object is to wrap.  The
		  *    caller retains ownership of the object (so the caller must perform
		  *    any necessary cleanup/deletion).  Must not be NULL.
		  * @param noiseV
		  *    The second underlying Noise object this object is to wrap.  The
		  *    caller retains ownership of the object (so the caller must perform
		  *    any necessary cleanup/deletion).  Must not be NULL.
		  * @param a
		  *    The first mixing coefficient (for u).
		  * @param b
		  *    The second mixing coefficient (for v).
		  * @param c
		  *    The second mixing coefficient (for u*v).
		  **/
		MixedNoise(Noise* noiseU, Noise* noiseV, float a, float b, float c)
		{
			m_noiseU = noiseU;
			m_noiseV = noiseV;
			m_a = a;
			m_b = b;
			m_c = c;
		}

		virtual ~MixedNoise() {}

		Noise* getNoiseU() { return m_noiseU; }
		void setNoiseU(Noise* value) { m_noiseU = value; }

		Noise* getNoiseV() { return m_noiseV; }
		void setNoiseV(Noise* value) { m_noiseV = value; }

		float getA() { return m_a; }
		void setA(float value) { m_a = value; }

		float getB() { return m_b; }
		void setB(float value) { m_b = value; }

		float getC() { return m_c; }
		void setC(float value) { m_c = value; }

		virtual float noise(int seed, float x, float y);
		virtual float noise(int seed, float x, float y, float z);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results);
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results);
};

/** Wraps another Noise object using a Decorator Design Pattern, transforming
  * the input coordinates according to the settings of the wrapping object.
  *
  * The code constructing instances of this class must ensure that the lifetime
  * of the wrapped Noise instance is at least as long as the lifetime of the
  * wrapping object.
  *
  * The noise generation methods of this class are themselves thread safe.
  * The configuration functions that set transformation values are NOT safe to
  * be called from multiple threads concurrently, nor may they be called while
  * other threads are calling the noise generation methods.  In other words,
  * an instance should be fully configured before being handed to multiple
  * threads for use in noise generation.
  **/
class ShiftedNoise : public virtual Noise
{
	private:
		Noise* m_noise;
		float m_xShift;
		float m_yShift;
		float m_zShift;

	public:
		/** Equivalent to ShiftedNoise(noise, 0.0f, 0.0f, 0.0f).  This is
		  * provided for convenience, so that the caller can set one or a couple
		  * values while allowing the rest to default.
		  **/
		ShiftedNoise(Noise* noise)
		{
			m_noise = noise;
			m_xShift = 0.0f;
			m_yShift = 0.0f;
			m_zShift = 0.0f;
		}

		/** Constructor that specifies all instance configuration parameters.
		  *
		  * @param noise
		  *    The underlying Noise object this object is to wrap.  The caller
		  *    retains ownership of the object (so the caller must perform any
		  *    necessary cleanup/deletion).  Must not be NULL.
		  * @param xShift
		  *    The amount to add to the x coordinate input.
		  * @param yShift
		  *    Like xShift, by in the y direction.
		  * @param zShift
		  *    Like xShift, by in the z direction.
		  **/
		ShiftedNoise(Noise* noise, float xShift, float yShift, float zShift)
		{
			m_noise = noise;
			m_xShift = xShift;
			m_yShift = yShift;
			m_zShift = zShift;
		}

		virtual ~ShiftedNoise() {}

		Noise* getWrappedNoise() { return m_noise; }
		void setWrappedNoise(Noise* value) { m_noise = value; }

		float getXShift() { return m_xShift; }
		void setXShift(float value) { m_xShift = value; }

		float getYShift() { return m_yShift; }
		void setYShift(float value) { m_yShift = value; }

		float getZShift() { return m_zShift; }
		void setZShift(float value) { m_zShift = value; }

		bool isShifted()
			{ return (m_xShift != 0.0f || m_yShift != 0.0f || m_zShift != 0.0f); }

		void setShift(float xsh, float ysh, float zsh)
			{ m_xShift = xsh; m_yShift = ysh; m_zShift = zsh; }


		virtual float noise(int seed, float x, float y)
			{ return m_noise->noise(seed, x+m_xShift, y+m_yShift); }
		virtual float noise(int seed, float x, float y, float z)
			{ return m_noise->noise(seed, x+m_xShift, y+m_yShift, z+m_zShift); }
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results)
		{
			return m_noise->noiseBlock(seed,
			                           sx, x0+m_xShift, dx,
			                           sy, y0+m_yShift, dy,
			                           results);
		}

		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results)
		{
			return m_noise->noiseBlock(seed,
			                           sx, x0+m_xShift, dx,
			                           sy, y0+m_yShift, dy,
			                           sz, z0+m_zShift, dz,
			                           results);
		}
};


/** Wraps another Noise object (e.g. a ValueNoise, PerlinNoise, or
  * SimplexNoise object) using a Decorator Design Pattern, transforming the
  * noise according to all parameters given by a NoiseParams configuration
  * object.
  *
  * The code constructing instances of this class must ensure that the lifetime
  * of the wrapped Noise instance is at least as long as the lifetime of the
  * wrapping object.
  *
  * The noise generation methods of this class are themselves thread safe.
  * The configuration functions that set transformation values are NOT safe to
  * be called from multiple threads concurrently, nor may they be called while
  * other threads are calling the noise generation methods.  In other words,
  * an instance should be fully configured before being handed to multiple
  * threads for use in noise generation.
  **/
class ParameterizedNoise : public virtual Noise
{
	private:
		Noise* m_baseNoise;

		NoiseParams m_np;
		FractalNoise m_fractalNoise;
		ShiftedNoise m_shiftedNoise;
		TransformedNoise m_transformedNoise;

		Noise* m_optimizedNoise;

	public:
		/** Constructor that specifies all the configuratino parameters.
		  *
		  * @param noise
		  *    The underlying Noise object this object is to wrap.  The caller
		  *    retains ownership of the object (so the caller must perform any
		  *    necessary cleanup/deletion).  Must not be NULL.
		  * @param np
		  *    The NoiseParams object specifying how the noise is to be
		  *    transformed and fractalized.  The attributes of this object must
		  *    satisfy the requirements of the corresponding attributes of the
		  *    FractalNoise and TransformedNoise classes.
		  * @param xShift
		  *    Amount by which to shift x coordinate inputs.
		  * @param yShift
		  *    Amount by which to shift y coordinate inputs.
		  * @param zShift
		  *    Amount by which to shift z coordinate inputs.
		  **/
		ParameterizedNoise(Noise* noise,
		                   const NoiseParams& np,
		                   float xShift =0.0f,
		                   float yShift =0.0f,
		                   float zShift =0.0f)
			: m_baseNoise(noise),
			  m_np(np),
			  m_fractalNoise(m_baseNoise, np.octaves, np.persist),
			  m_shiftedNoise(&m_fractalNoise, xShift, yShift, zShift),
			  m_transformedNoise(&m_shiftedNoise,
			                     np.offset,
			                     np.scale,
			                     np.spread.X, np.spread.Y, np.spread.Z,
			                     np.seed),
			  m_optimizedNoise(&m_transformedNoise)
		{
			optimizeNoise();
		}

		virtual ~ParameterizedNoise() {}

		Noise* getWrappedNoise() { return m_baseNoise; }
		void setWrappedNoise(Noise* value) { m_baseNoise = value; }

		NoiseParams getNoiseParameters() { return m_np; }
		void setNoiseParameters(const NoiseParams& np)
		{
			m_np = np;

			m_fractalNoise.setOctaves(np.octaves);
			m_fractalNoise.setPersistence(np.persist);

			m_transformedNoise.setOffset(np.offset);
			m_transformedNoise.setScale(np.scale);
			m_transformedNoise.setXSpread(np.spread.X);
			m_transformedNoise.setYSpread(np.spread.Y);
			m_transformedNoise.setZSpread(np.spread.Z);
			m_transformedNoise.setBaseSeed(np.seed);

			optimizeNoise();
		}

		float getXShift() { return m_shiftedNoise.getXShift(); }
		void setXShift(float value) { m_shiftedNoise.setXShift(value); }

		float getYShift() { return m_shiftedNoise.getYShift(); }
		void setYShift(float value) { m_shiftedNoise.setYShift(value); }

		float getZShift() { return m_shiftedNoise.getZShift(); }
		void setZShift(float value) { m_shiftedNoise.setZShift(value); }

		bool isShifted()
			{ return m_shiftedNoise.isShifted(); }

		void setShift(float xsh, float ysh, float zsh)
		{
			m_shiftedNoise.setShift(xsh, ysh, zsh);
			optimizeNoise();
		}

		virtual float noise(int seed, float x, float y)
			{ return m_optimizedNoise->noise(seed, x, y); }
		virtual float noise(int seed, float x, float y, float z)
			{ return m_optimizedNoise->noise(seed, x, y, z); }
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        float* results)
			{
				return m_optimizedNoise->noiseBlock(seed,
				                                    sx, x0, dx,
				                                    sy, y0, dy,
				                                    results);
			}
		virtual void noiseBlock(int seed,
		                        int sx, float x0, float dx,
		                        int sy, float y0, float dy,
		                        int sz, float z0, float dz,
		                        float* results)
			{
				return m_optimizedNoise->noiseBlock(seed,
				                                    sx, x0, dx,
				                                    sy, y0, dy,
				                                    sz, z0, dz,
				                                    results);
			}

	private:
		void optimizeNoise()
		{
			m_optimizedNoise = m_baseNoise;

			if (m_np.octaves > 1)
			{
				m_fractalNoise.setWrappedNoise(m_optimizedNoise);
				m_optimizedNoise = &m_fractalNoise;
			}

			if (m_shiftedNoise.isShifted())
			{
				m_shiftedNoise.setWrappedNoise(m_optimizedNoise);
				m_optimizedNoise = &m_shiftedNoise;
			}

			if (m_np.offset != 0.0f ||
				 m_np.scale != 1.0f ||
				 m_np.spread.X != 1.0f ||
				 m_np.spread.Y != 1.0f ||
				 m_np.spread.Z != 1.0f ||
				 m_np.seed != 0)
			{
				m_transformedNoise.setWrappedNoise(m_optimizedNoise);
				m_optimizedNoise = &m_transformedNoise;
			}
		}
};


#endif
