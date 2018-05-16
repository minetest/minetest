/*
Minetest
Copyright (C) 2018 rubenwardy <rw@rubenwardy.com>

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
#include "config.h"
#include "convert_json.h"
#include "irrlichttypes.h"

struct Package
{
	std::string name; // Technical name
	std::string title;
	std::string author;
	std::string type; // One of "mod", "game", or "txp"

	std::string shortDesc;
	std::string url; // download URL
	u32 release;
	std::vector<std::string> screenshots;

	bool valid()
	{
		return !(name.empty() || title.empty() || author.empty() ||
				type.empty() || url.empty() || release <= 0);
	}
};

#if USE_CURL
std::vector<Package> getPackagesFromURL(const std::string &url);
#else
inline std::vector<Package> getPackagesFromURL(const std::string &url)
{
	return std::vector<Package>();
}
#endif
