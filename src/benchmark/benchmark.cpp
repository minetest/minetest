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

#include "benchmark/benchmark.h"

// This must be set in just this file
#define CATCH_CONFIG_RUNNER
#include "benchmark_setup.h"

int run_benchmarks()
{
	int argc = 1;
	const char *argv[] = { "MinetestBenchmark", NULL };
	int errCount = Catch::Session().run(argc, argv);
	return errCount ? 1 : 0;
}
