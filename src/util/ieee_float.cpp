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
#include <cmath>

// Given an unsigned 32-bit integer representing an IEEE-754 single-precision
// float, return the float.
f32 u32Tof32Slow(u32 i)
{
	// clang-format off
	int exp = (i >> 23) & 0xFF;
	u32 sign = i & 0x80000000UL;
	u32 imant = i & 0x7FFFFFUL;
	if (exp == 0xFF) {
		// Inf/NaN
		if (imant == 0) {
			if (std::numeric_limits<f32>::has_infinity)
				return sign ? -std::numeric_limits<f32>::infinity() :
					std::numeric_limits<f32>::infinity();
			return sign ? std::numeric_limits<f32>::max() :
				std::numeric_limits<f32>::lowest();
		}
		return std::numeric_limits<f32>::has_quiet_NaN ?
			std::numeric_limits<f32>::quiet_NaN() : -0.f;
	}

	if (!exp) {
		// Denormal or zero
		return sign ? -ldexpf((f32)imant, -149) : ldexpf((f32)imant, -149);
	}

	return sign ? -ldexpf((f32)(imant | 0x800000UL), exp - 150) :
		ldexpf((f32)(imant | 0x800000UL), exp - 150);
	// clang-format on
}

// Given a float, return an unsigned 32-bit integer representing the f32
// in IEEE-754 single-precision format.
u32 f32Tou32Slow(f32 f)
{
	u32 signbit = std::copysign(1.0f, f) == 1.0f ? 0 : 0x80000000UL;
	if (f == 0.f)
		return signbit;
	if (std::isnan(f))
		return signbit | 0x7FC00000UL;
	if (std::isinf(f))
		return signbit | 0x7F800000UL;
	int exp = 0; // silence warning
	f32 mant = frexpf(f, &exp);
	u32 imant = (u32)std::floor((signbit ? -16777216.f : 16777216.f) * mant);
	exp += 126;
	if (exp <= 0) {
		// Denormal
		return signbit | (exp <= -31 ? 0 : imant >> (1 - exp));
	}

	if (exp >= 255) {
		// Overflow due to the platform having exponents bigger than IEEE ones.
		// Return signed infinity.
		return signbit | 0x7F800000UL;
	}

	// Regular number
	return signbit | (exp << 23) | (imant & 0x7FFFFFUL);
}

// This test needs the following requisites in order to work:
// - The float type must be a 32 bits IEEE-754 single-precision float.
// - The endianness of f32s and integers must match.
FloatType getFloatSerializationType()
{
	// clang-format off
	const f32 cf = -22220490.f;
	const u32 cu = 0xCBA98765UL;
	if (std::numeric_limits<f32>::is_iec559 && sizeof(cf) == 4 &&
			sizeof(cu) == 4 && !memcmp(&cf, &cu, 4)) {
		// u32Tof32Slow and f32Tou32Slow are not needed, use memcpy
		return FLOATTYPE_SYSTEM;
	}

	// Run quick tests to ensure the custom functions provide acceptable results
	warningstream << "floatSerialization: f32 and u32 endianness are "
		"not equal or machine is not IEEE-754 compliant" << std::endl;
	u32 i;
	char buf[128];

	// NaN checks aren't included in the main loop
	if (!std::isnan(u32Tof32Slow(0x7FC00000UL))) {
		porting::mt_snprintf(buf, sizeof(buf),
			"u32Tof32Slow(0x7FC00000) failed to produce a NaN, actual: %.9g",
			u32Tof32Slow(0x7FC00000UL));
		infostream << buf << std::endl;
	}
	if (!std::isnan(u32Tof32Slow(0xFFC00000UL))) {
		porting::mt_snprintf(buf, sizeof(buf),
			"u32Tof32Slow(0xFFC00000) failed to produce a NaN, actual: %.9g",
			u32Tof32Slow(0xFFC00000UL));
		infostream << buf << std::endl;
	}

	i = f32Tou32Slow(std::numeric_limits<f32>::quiet_NaN());
	// check that it corresponds to a NaN encoding
	if ((i & 0x7F800000UL) != 0x7F800000UL || (i & 0x7FFFFFUL) == 0) {
		porting::mt_snprintf(buf, sizeof(buf),
			"f32Tou32Slow(NaN) failed to encode NaN, actual: 0x%X", i);
		infostream << buf << std::endl;
	}

	return FLOATTYPE_SLOW;
	// clang-format on
}
