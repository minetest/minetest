/*
Minetest
Copyright (C) 2021 DS

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

#include "time_parsing.h"
#include "string.h"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>

Optional<double> parse_difftime(const std::string &str)
{
	// read number
	char *num_end;
	double num = std::strtod(str.c_str(), &num_end);
	if (num_end == str.c_str() || // no number
			std::isnan(num)) { // disallow NaN
		return nullopt;
	}

	// trim whitespace
	std::string str_unit = trim(num_end);

	// no unit is only allowed if num=0.0 or num=+-inf
	if (str_unit.empty()) {
		if (num == 0.0 || std::isinf(num))
			return num;
		else
			return nullopt;
	}

	// now there is an optinal prefix and a unit, ie. "ks"
	// we first remove the unit because eg. m (milli) is prefix of min (minute)

	// the units with their conversion ratio to seconds:
	constexpr std::pair<const char *, double> units[] = {
			{"s", 1.0},
			{"min", 60.0},
			{"h", 60.0*60.0},
			{"d", 60.0*60.0*24.0},
			{"a", 60.0*60.0*24.0*365.2425},
		};

	double unit_to_seconds_ratio = 0.0;
	std::string str_unit_prefix;
	for (auto &&unit : units) {
		if (str_ends_with(str_unit, unit.first)) {
			unit_to_seconds_ratio = unit.second;
			str_unit_prefix = str_unit.substr(0, str_unit.size() - std::strlen(unit.first));
			break;
		}
	}
	if (unit_to_seconds_ratio == 0.0) {
		// found no unit
		return nullopt;
	}

	// read the prefix
	// the prefixes with their ratios:
	constexpr std::pair<const char *, double> prefixes[] = {
			{"", 1.0},
			{"Ki", 0x1.0p10},
			{"Mi", 0x1.0p20},
			{"Gi", 0x1.0p30},
			{"Ti", 0x1.0p40},
			{"Pi", 0x1.0p50},
			{"Ei", 0x1.0p60},
			{"Zi", 0x1.0p70},
			{"Yi", 0x1.0p80},
			{"da", 1.0e1},
			{"h", 1.0e2},
			{"k", 1.0e3},
			{"K", 1.0e3},
			{"M", 1.0e6},
			{"G", 1.0e9},
			{"T", 1.0e12},
			{"P", 1.0e15},
			{"E", 1.0e18},
			{"Z", 1.0e21},
			{"Y", 1.0e24},
			{"d", 1.0e-1},
			{"c", 1.0e-2},
			{"m", 1.0e-3},
			{"u", 1.0e-6},
			{"n", 1.0e-9},
			{"p", 1.0e-12},
			{"f", 1.0e-15},
			{"a", 1.0e-18},
			{"z", 1.0e-21},
			{"y", 1.0e-24},
		};

	double prefix_ratio = 0.0;
	for (auto &&prefix : prefixes) {
		if (str_unit_prefix == prefix.first) {
			prefix_ratio = prefix.second;
			break;
		}
	}
	if (prefix_ratio == 0.0) {
		// something invalid
		return nullopt;
	}

	// return the combined result in seconds
	return num * prefix_ratio * unit_to_seconds_ratio;
}
