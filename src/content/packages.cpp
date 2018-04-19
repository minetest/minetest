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

#include "content/packages.h"
#include "log.h"
#include "filesys.h"
#include "porting.h"
#include "settings.h"
#include "content/mods.h"
#include "content/subgames.h"

#if USE_CURL
std::vector<Package> getPackagesFromURL(const std::string &url)
{
	std::vector<std::string> extra_headers;
	extra_headers.emplace_back("Accept: application/json");

	Json::Value json = fetchJsonValue(url, &extra_headers);
	if (!json.isArray()) {
		errorstream << "Invalid JSON download " << std::endl;
		return std::vector<Package>();
	}

	std::vector<Package> packages;

	// Note: `unsigned int` is required to index JSON
	for (unsigned int i = 0; i < json.size(); ++i) {
		Package package;

		package.name = json[i]["name"].asString();
		package.title = json[i]["title"].asString();
		package.author = json[i]["author"].asString();
		package.type = json[i]["type"].asString();
		package.shortDesc = json[i]["shortDesc"].asString();
		package.url = json[i]["url"].asString();
		package.release = json[i]["release"].asInt();

		Json::Value jScreenshots = json[i]["screenshots"];
		for (unsigned int j = 0; j < jScreenshots.size(); ++j) {
			package.screenshots.push_back(jScreenshots[j].asString());
		}

		if (package.valid()) {
			packages.push_back(package);
		} else {
			errorstream << "Invalid package at " << i << std::endl;
		}
	}

	return packages;
}

#endif
