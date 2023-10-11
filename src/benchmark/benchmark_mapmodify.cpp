/*
Minetest
Copyright (C) 2023 sfan5

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
#include "util/container.h"

// Testing the standard library is not useful except to compare
//#define TEST_STDLIB

using TestMap = ModifySafeMap<u16, void*>;

static inline void fill(TestMap &map, size_t n)
{
	map.clear();
	for (size_t i = 0; i < n; i++)
		map.put(i, reinterpret_cast<void*>(0x40000U + i));
}

static inline void remove(TestMap &map, size_t offset, size_t count)
{
	for (size_t i = 0; i < count; i++)
		map.remove(static_cast<u16>(i + offset));
}

#define BENCH_ITERATE(_count) \
	BENCHMARK_ADVANCED("iterate_" #_count)(Catch::Benchmark::Chronometer meter) { \
		TestMap map; \
		fill(map, _count); \
		meter.measure([&] { \
			size_t x = map.size(); \
			for (auto &it : map.iter()) { \
				if (!it.second) \
					continue; \
				x ^= reinterpret_cast<intptr_t>(it.second); \
			} \
			return x; \
		}); \
	};

#define BENCH_DEL_DURING_ITER(_count) \
	BENCHMARK_ADVANCED("remove_during_iterate_" #_count)(Catch::Benchmark::Chronometer meter) { \
		TestMap map; \
		fill(map, _count); \
		meter.measure([&] { \
			for (auto it : map.iter()) { \
				(void)it; \
				remove(map, (_count) / 7, (_count) / 2); /* delete half */ \
				break; \
			} \
		}); \
	};

TEST_CASE("ModifySafeMap") {
	BENCH_ITERATE(50)
	BENCH_ITERATE(400)
	BENCH_ITERATE(1000)

	BENCH_DEL_DURING_ITER(50)
	BENCH_DEL_DURING_ITER(400)
	BENCH_DEL_DURING_ITER(100)
}

using TestMap2 = std::map<u16, void*>;

static inline void fill2(TestMap2 &map, size_t n)
{
	map.clear();
	for (size_t i = 0; i < n; i++)
		map.emplace(i, reinterpret_cast<void*>(0x40000U + i));
}

static inline void remove2(TestMap2 &map, size_t offset, size_t count)
{
	for (size_t i = 0; i < count; i++)
		map.erase(static_cast<TestMap2::key_type>(i + offset));
}

#define BENCH2_ITERATE(_count) \
	BENCHMARK_ADVANCED("iterate_" #_count)(Catch::Benchmark::Chronometer meter) { \
		TestMap2 map; \
		fill2(map, _count); \
		meter.measure([&] { \
			size_t x = map.size(); \
			/* mirrors what ActiveObjectMgr::step used to do */ \
			std::vector<TestMap2::key_type> keys; \
			keys.reserve(x); \
			for (auto &it : map) \
				keys.push_back(it.first); \
			for (auto key : keys) { \
				auto it = map.find(key); \
				if (it == map.end()) \
					continue; \
				x ^= reinterpret_cast<intptr_t>(it->second); \
			} \
			return x; \
		}); \
	};

#define BENCH2_DEL_DURING_ITER(_count) \
	BENCHMARK_ADVANCED("remove_during_iterate_" #_count)(Catch::Benchmark::Chronometer meter) { \
		TestMap2 map; \
		fill2(map, _count); \
		meter.measure([&] { \
			/* no overhead so no fake iteration */ \
			remove2(map, (_count) / 7, (_count) / 2); \
		}); \
	};

#ifdef TEST_STDLIB
TEST_CASE("plain std::map") {
	BENCH2_ITERATE(50)
	BENCH2_ITERATE(400)
	BENCH2_ITERATE(1000)

	BENCH2_DEL_DURING_ITER(50)
	BENCH2_DEL_DURING_ITER(400)
	BENCH2_DEL_DURING_ITER(100)
}
#endif
