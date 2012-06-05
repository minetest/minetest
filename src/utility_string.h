/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef UTILITY_STRING_HEADER
#define UTILITY_STRING_HEADER

// Note: Some stuff could be moved to here from utility.h

#include <string>

static inline std::string padStringRight(std::string s, size_t len)
{
	if(len > s.size())
		s.insert(s.end(), len - s.size(), ' ');
	return s;
}

// ends: NULL- or ""-terminated array of strings
// Returns "" if no end could be removed.
static inline std::string removeStringEnd(const std::string &s, const char *ends[])
{
	const char **p = ends;
	for(; (*p) && (*p)[0] != '\0'; p++){
		std::string end = *p;
		if(s.size() < end.size())
			continue;
		if(s.substr(s.size()-end.size(), end.size()) == end)
			return s.substr(0, s.size() - end.size());
	}
	return "";
}

#endif

