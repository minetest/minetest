/*
Minetest
Copyright (C) 2018 nerzhul, Loic BLOT <loic.blot@unix-experience.fr>

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

#include "weather.h"
#include "debug.h"
#include <unordered_map>

static std::unordered_map<std::string, Weather::Type> weatherStateMapper = {
	{"normal", Weather::NORMAL},
	{"rain", Weather::RAIN},
	{"huge_clouds", Weather::HUGE_CLOUDS},
	{"storm", Weather::STORM},
	{"wind", Weather::WIND},
	{"snow", Weather::SNOW},
};

namespace Weather
{

void State::setType(const std::string &strType)
{
	const auto &wsm = weatherStateMapper.find(strType);
	FATAL_ERROR_IF(wsm == weatherStateMapper.end(),
		std::string("Invalid weather type set: '" + strType + "'").c_str());
	type = wsm->second;
}

const std::string& State::getTypeStr() const
{
	for (const auto &item : weatherStateMapper) {
		if (item.second == type)
			return item.first;
	}

	FATAL_ERROR("Weather::State stored type is wrong !");
}

std::string State::getTextureFilename() const
{
	if (!texture.empty())
		return texture;

	std::string fallback_name = "weather_";
	fallback_name.append(getTypeStr()).append(".png");
	return fallback_name;
}
}
