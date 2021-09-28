/*
Minetest
Copyright (C) 2020 Hugues Ross <hugues.ross@gmail.com>

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

#include "texture_override.h"

#include "log.h"
#include "util/string.h"
#include <algorithm>
#include <fstream>

#define override_cast static_cast<override_t>

TextureOverrideSource::TextureOverrideSource(std::string filepath)
{
	std::ifstream infile(filepath.c_str());
	std::string line;
	int line_index = 0;
	while (std::getline(infile, line)) {
		line_index++;

		// Also trim '\r' on DOS-style files
		line = trim(line);

		// Ignore empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;

		std::vector<std::string> splitted = str_split(line, ' ');
		if (splitted.size() != 3) {
			warningstream << filepath << ":" << line_index
					<< " Syntax error in texture override \"" << line
					<< "\": Expected 3 arguments, got " << splitted.size()
					<< std::endl;
			continue;
		}

		TextureOverride texture_override = {};
		texture_override.id = splitted[0];
		texture_override.texture = splitted[2];

		// Parse the target mask
		std::vector<std::string> targets = str_split(splitted[1], ',');
		for (const std::string &target : targets) {
			if (target == "top")
				texture_override.target |= override_cast(OverrideTarget::TOP);
			else if (target == "bottom")
				texture_override.target |= override_cast(OverrideTarget::BOTTOM);
			else if (target == "left")
				texture_override.target |= override_cast(OverrideTarget::LEFT);
			else if (target == "right")
				texture_override.target |= override_cast(OverrideTarget::RIGHT);
			else if (target == "front")
				texture_override.target |= override_cast(OverrideTarget::FRONT);
			else if (target == "back")
				texture_override.target |= override_cast(OverrideTarget::BACK);
			else if (target == "inventory")
				texture_override.target |= override_cast(OverrideTarget::INVENTORY);
			else if (target == "wield")
				texture_override.target |= override_cast(OverrideTarget::WIELD);
			else if (target == "special1")
				texture_override.target |= override_cast(OverrideTarget::SPECIAL_1);
			else if (target == "special2")
				texture_override.target |= override_cast(OverrideTarget::SPECIAL_2);
			else if (target == "special3")
				texture_override.target |= override_cast(OverrideTarget::SPECIAL_3);
			else if (target == "special4")
				texture_override.target |= override_cast(OverrideTarget::SPECIAL_4);
			else if (target == "special5")
				texture_override.target |= override_cast(OverrideTarget::SPECIAL_5);
			else if (target == "special6")
				texture_override.target |= override_cast(OverrideTarget::SPECIAL_6);
			else if (target == "sides")
				texture_override.target |= override_cast(OverrideTarget::SIDES);
			else if (target == "all" || target == "*")
				texture_override.target |= override_cast(OverrideTarget::ALL_FACES);
			else {
				// Report invalid target
				warningstream << filepath << ":" << line_index
						<< " Syntax error in texture override \"" << line
						<< "\": Unknown target \"" << target << "\""
						<< std::endl;
			}
		}

		// If there are no valid targets, skip adding this override
		if (texture_override.target == override_cast(OverrideTarget::INVALID)) {
			continue;
		}

		m_overrides.push_back(texture_override);
	}
}

//! Get all overrides that apply to item definitions
std::vector<TextureOverride> TextureOverrideSource::getItemTextureOverrides()
{
	std::vector<TextureOverride> found_overrides;

	for (const TextureOverride &texture_override : m_overrides) {
		if (texture_override.hasTarget(OverrideTarget::ITEM_TARGETS))
			found_overrides.push_back(texture_override);
	}

	return found_overrides;
}

//! Get all overrides that apply to node definitions
std::vector<TextureOverride> TextureOverrideSource::getNodeTileOverrides()
{
	std::vector<TextureOverride> found_overrides;

	for (const TextureOverride &texture_override : m_overrides) {
		if (texture_override.hasTarget(OverrideTarget::NODE_TARGETS))
			found_overrides.push_back(texture_override);
	}

	return found_overrides;
}
