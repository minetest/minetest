/*
Minetest
Copyright (C) 2022 Minetest Authors

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

#include "benchmark_setup.h"
#include "util/serialize.h"
#include <sstream>
#include <ios>

// Builds a string of exactly `length` characters by repeating `s` (rest cut off)
static std::string makeRepeatTo(const std::string &s, size_t length)
{
	std::string v;
	v.reserve(length + s.size());
	for (size_t i = 0; i < length; i += s.size()) {
		v += s;
	}
	v.resize(length);
	return v;
}

#define BENCH3(_label, _chars, _length, _lengthlabel) \
	BENCHMARK_ADVANCED("serializeJsonStringIfNeeded_" _lengthlabel "_" _label)(Catch::Benchmark::Chronometer meter) { \
		std::string s = makeRepeatTo(_chars, _length); \
		meter.measure([&] { return serializeJsonStringIfNeeded(s); }); \
	}; \
	BENCHMARK_ADVANCED("deSerializeJsonStringIfNeeded_" _lengthlabel "_" _label)(Catch::Benchmark::Chronometer meter) { \
		std::string s = makeRepeatTo(_chars, _length); \
		std::string serialized = serializeJsonStringIfNeeded(s); \
		std::istringstream is(serialized, std::ios::binary); \
		meter.measure([&] { \
			is.clear(); \
			is.seekg(0, std::ios::beg); \
			return deSerializeJsonStringIfNeeded(is); \
		}); \
	};

/* Both with and without a space character (' ') */
#define BENCH2(_label, _chars, _length, _lengthlabel) \
	BENCH3(_label, _chars, _length, _lengthlabel) \
	BENCH3(_label "_with_space", " " _chars, _length, _lengthlabel) \

/* Iterate over input lengths */
#define BENCH1(_label, _chars) \
	BENCH2(_label, _chars, 10, "small") \
	BENCH2(_label, _chars, 10000, "large")

/* Iterate over character sets */
#define BENCH_ALL() \
	BENCH1("alpha", "abcdefghijklmnopqrstuvwxyz") \
	BENCH1("escaped", "\"\\/\b\f\n\r\t") \
	BENCH1("nonascii", "\xf0\xff")

TEST_CASE("benchmark_serialize") {
	BENCH_ALL()
}
