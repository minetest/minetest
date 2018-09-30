/*
 * Conversion of float to IEEE-754 and vice versa.
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
#include "log.h"
#include "porting.h"
#include <stdint.h>
#include <math.h>

// Given an unsigned 32-bit integer representing an IEEE-754 single-precision
// float, return the float.
float u32_FloatSlow(uint32_t i)
{
	float res;
	int exp = (i >> 23) & 0xFF;
	uint32_t sign = i & 0x80000000UL;
	uint32_t imant = i & 0x7FFFFFUL;
	if (exp == 0xFF) {
		// Inf/NaN
		if (imant == 0)
			return sign ? -1.f/0.f : 1.f/0.f;
		return 0.f/0.f; // always return indeterminate
	}

	if (!exp) {
		// Denormal or zero
		return sign ? -ldexpf((float)imant, -149) : ldexpf((float)imant, -149);
	}

	return sign ? -ldexpf((float)(imant | 0x800000), exp - 150) :
		ldexpf((float)(imant | 0x800000), exp - 150);
}

// Given a float, return an unsigned 32-bit integer representing the float
// in IEEE-754 single-precision format.
uint32_t float_u32Slow(float f)
{
	uint32_t signbit = copysignf(1.0f, f) == 1.0f ? 0 : 0x80000000UL;
	uint32_t imant;
	int exp;
	if (f == 0.f)
		return signbit;
	if (isnanf(f))
		return signbit | 0x7FC00000UL;
	if (isinff(f))
		return signbit | 0x7F800000UL;
	float mant = frexpf(f, &exp);
	imant = (uint32_t)floorf((signbit ? -16777216.f : 16777216.f) * mant);
	exp += 126;
	if (exp <= 0) {
		// Denormal
		return signbit | (imant >> 1 - exp);
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
// - The compiler must support uint32_t.
// - The compiler must treat union by overlapping the memory of its fields.
// - The float type must be a 32 bits IEEE-754 single-precision float.
// - The endianness of floats and integers must match.
// - 0.f/0.f must quietly return a NaN.
int getFloatSerializationType()
{
	union {
		float f;
		uint32_t i;
	} fui;

	float f;
	uint_fast32_t i;
	char buf[128];

	if (sizeof(fui) != 4) {
		infostream << "Floats are not 32 bits in this platform" << std::endl;
		return 1;
	}

	fui.i = 0;
	fui.f = -22220490.f;
	if (fui.i != 0xCBA98765UL) {
		infostream << "Invalid endianness or machine not IEEE-754 compliant" << std::endl;
		return 1;
	}

	// NaN checks aren't included in the main loop
	if (!isnanf(u32_FloatSlow(0x7FC00000UL))) {
		porting::mt_snprintf(buf, sizeof(buf),
			"u32_FloatSlow(0x7FC00000) failed to produce a NaN, actual: %.9g",
			u32_FloatSlow(0x7FC00000UL));
		infostream << buf << std::endl;
	}
	if (!isnanf(u32_FloatSlow(0xFFC00000UL))) {
		porting::mt_snprintf(buf, sizeof(buf),
			"u32_FloatSlow(0xFFC00000) failed to produce a NaN, actual: %.9g",
			u32_FloatSlow(0xFFC00000UL));
		infostream << buf << std::endl;
	}

	i = float_u32Slow(0.0f / 0.0f);
	// check that it corresponds to a NaN encoding
	if ((i & 0x7F800000UL) != 0x7F800000UL || (i & 0x7FFFFF) == 0) {
		porting::mt_snprintf(buf, sizeof(buf),
			"float_u32Slow(NaN) failed to encode NaN, actual: 0x%X", i);
		infostream << buf << std::endl;
	}

	infostream << "NaN test passed" << std::endl;

	for (i = 0; i <= 0xFF800000UL; i++) {
		if (i == 0x7F800001UL) {
			// Halfway there
			infostream << "Positive tests passed" << std::endl;
			i = 0x80000000UL;
		}

		fui.i = i;
		f = u32_FloatSlow(i);
		if (fui.f != f) {
			porting::mt_snprintf(buf, sizeof(buf),
				"u32_FloatSlow failed on 0x%X, expected %.9g, actual %.9g\n",
				i, fui.f, f);
			infostream << buf << std::endl;
			return 1;
		}
		if (float_u32Slow(f) != i) {
			porting::mt_snprintf(buf, sizeof(buf),
				"float_u32Slow failed on %.9g, expected 0x%X, actual 0x%X\n",
				f, i, float_u32Slow(f));
			infostream << buf << std::endl;
			return 1;
		}
	}

	infostream << "All tests passed" << std::endl;
	return 0;
}
