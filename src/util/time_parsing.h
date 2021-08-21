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

#pragma once

#include "Optional.h"
#include <string>

/** Parses a human-written time value.
 *
 * Possible time units are the SI stuff (ie. "d" for days, "s" for seconds, "a"
 * for years (1a=365.2425d)) with decimal or binary prefixes (ie. k, Ki, M, Mi).
 * Any double numbers are allowed, just not NaN.
 * Whitespace is allowed at the front and back and between the number and the unit,
 * but not between the unit's prefix and the rest of the unit.
 * Examples:
 * "42 d", "5Ms ", "-10a", " +0x3.0p13Kih"
 *
 * @param str the string to be parsed
 * @return time in seconds or nothing
 */
Optional<double> parse_difftime(const std::string &str);
