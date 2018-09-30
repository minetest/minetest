/*
 * Conversion of f32 to IEEE-754 and vice versa.
 *
 * © Copyright 2018 Pedro Gimeno Fortea.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "ieee_float.h"
#include "log.h"
#include "porting.h"
#include <limits>
#include <math.h>

// Given an unsigned 32-bit integer representing an IEEE-754 single-precision
// float, return the float.
f32 u32Tof32Slow(u32 i)
{
	int exp = (i >> 23) & 0xFF;
	u32 sign = i & 0x80000000UL;
	u32 imant = i & 0x7FFFFFUL;
	if (exp == 0xFF) {
		// Inf/NaN
		if (imant == 0) {
			if (std::numeric_limits<f32>::has_infinity)	
				return sign ? -std::numeric_limits<f32>::infinity() :
					std::numeric_limits<f32>::infinity();
			return 0.f;
		}
		return std::numeric_limits<f32>::has_quiet_NaN ?
			std::numeric_limits<f32>::quiet_NaN() : 0.f;
	}

	if (!exp) {
		// Denormal or zero
		return sign ? -ldexpf((f32)imant, -149) : ldexpf((f32)imant, -149);
	}

	return sign ? -ldexpf((f32)(imant | 0x800000), exp - 150) :
		ldexpf((f32)(imant | 0x800000), exp - 150);
}

// Given a float, return an unsigned 32-bit integer representing the f32
// in IEEE-754 single-precision format.
u32 f32Tou32Slow(f32 f)
{
	u32 signbit = copysignf(1.0f, f) == 1.0f ? 0 : 0x80000000UL;
	u32 imant;
	int exp;
	if (f == 0.f)
		return signbit;
	if (isnanf(f))
		return signbit | 0x7FC00000UL;
	if (isinff(f))
		return signbit | 0x7F800000UL;
	f32 mant = frexpf(f, &exp);
	imant = (u32)floorf((signbit ? -16777216.f : 16777216.f) * mant);
	exp += 126;
	if (exp <= 0) {
		// Denormal
		return signbit | (imant >> (1 - exp));
	}

	if (exp >= 255) {
		// Overflow due to the platform having exponents bigger than IEEE ones.
		// Return signed infinity.
		return signbit | 0x7F800000UL;
	}

	// Regular number
	return signbit | (exp << 23) | (imant & 0x7FFFFFUL);
}


// Test the functions.
// This test needs the following requisites in order to work:
// - The compiler must support u32.
// - The compiler must treat union by overlapping the memory of its fields.
// - The float type must be a 32 bits IEEE-754 single-precision float.
// - The endianness of f32s and integers must match.
// - 0.f/0.f must quietly return a NaN.
FloatType getFloatSerializationType(bool do_full_unittest)
{
	conv_f32_u32_t fui;
	f32 f;
	uint_fast32_t i;
	char buf[128];

	if (sizeof(fui) != 4) {
		infostream << "Floats are not 32 bits in this platform" << std::endl;
		return FLOATTYPE_INVALID;
	}

	fui.u = 0;
	fui.f = -22220490.f;
	if (fui.u != 0xCBA98765UL) {
		infostream << "Invalid endianness or machine not IEEE-754 compliant" << std::endl;
		return FLOATTYPE_INVALID;
	}

	// NaN checks aren't included in the main loop
	if (!isnanf(u32Tof32Slow(0x7FC00000UL))) {
		porting::mt_snprintf(buf, sizeof(buf),
			"u32Tof32Slow(0x7FC00000) failed to produce a NaN, actual: %.9g",
			u32Tof32Slow(0x7FC00000UL));
		infostream << buf << std::endl;
	}
	if (!isnanf(u32Tof32Slow(0xFFC00000UL))) {
		porting::mt_snprintf(buf, sizeof(buf),
			"u32Tof32Slow(0xFFC00000) failed to produce a NaN, actual: %.9g",
			u32Tof32Slow(0xFFC00000UL));
		infostream << buf << std::endl;
	}

	i = f32Tou32Slow(0.0f / 0.0f);
	// check that it corresponds to a NaN encoding
	if ((i & 0x7F800000UL) != 0x7F800000UL || (i & 0x7FFFFF) == 0) {
		porting::mt_snprintf(buf, sizeof(buf),
			"f32_u32Slow(NaN) failed to encode NaN, actual: 0x%X", i);
		infostream << buf << std::endl;
	}

	infostream << "NaN test passed" << std::endl;

	if (do_full_unittest) {
		for (i = 0; i <= 0xFF800000UL; i++) {
			if (i == 0x7F800001UL) {
				// Halfway there
				infostream << "Positive tests passed" << std::endl;
				i = 0x80000000UL;
			}

			fui.u = i;
			f = u32Tof32Slow(i);
			if (fui.f != f) {
				porting::mt_snprintf(buf, sizeof(buf),
					"u32_FloatSlow failed on 0x%X, expected %.9g, actual %.9g\n",
					i, fui.f, f);
				infostream << buf << std::endl;
				return FLOATTYPE_INVALID;
			}
			if (f32Tou32Slow(f) != i) {
				porting::mt_snprintf(buf, sizeof(buf),
					"f32_u32Slow failed on %.9g, expected 0x%X, actual 0x%X\n",
					f, i, f32Tou32Slow(f));
				infostream << buf << std::endl;
				return FLOATTYPE_INVALID;
			}
		}
	}

	infostream << "All tests passed" << std::endl;

	fui.u = 0;
	fui.f = -22220490.f;
	if (fui.u == 0xCBA98765UL)
 		return FLOATTYPE_ABCD;
	else if (fui.u == 0x6587A9CBUL)
		return FLOATTYPE_DCBA;
	else if (fui.u == 0xA9CB6587UL)
		return FLOATTYPE_BADC;
	else if (fui.u == 0x8765CBA9UL)
		return FLOATTYPE_CDAB;

	return FLOATTYPE_INVALID;
}
