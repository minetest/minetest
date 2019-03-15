/*
Minetest
Copyright (C) 2019 rubenwardy

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

#include "irrlichttypes_extrabloated.h"

#pragma once


class StyleSpec
{
public:
	enum Property {
		NONE = 0,
		TEXTCOLOR,
		BGCOLOR,
		NUM_PROPERTIES
	};

private:
	std::unordered_map<Property, std::string> properties;

public:
	static Property GetPropertyByName(const std::string &name) {
		if (name == "textcolor") {
			return TEXTCOLOR;
		} else if (name == "bgcolor") {
			return BGCOLOR;
		} else {
			return NONE;
		}
	}

	std::string get(Property prop, std::string def) const {
		auto it = properties.find(prop);
		if (it == properties.end()) {
			return def;
		}

		return it->second;
	}

	void set(Property prop, std::string value) {
		properties[prop] = std::move(value);
	}

	video::SColor getColor(Property prop, video::SColor def) const {
		auto it = properties.find(prop);
		if (it == properties.end()) {
			return def;
		}

		parseColorString(it->second, def, false, 0xFF);
		return def;
	}

	video::SColor getColor(Property prop) const {
		auto it = properties.find(prop);
		FATAL_ERROR_IF(it == properties.end(), "Unexpected missing property");

		video::SColor color;
		parseColorString(it->second, color, false, 0xFF);
		return color;
	}

	bool hasProperty(Property prop) const {
		return properties.find(prop) != properties.end();
	}

	StyleSpec &operator|=(const StyleSpec &other) {
		for (size_t i = 1; i < NUM_PROPERTIES; i++) {
			auto prop = (Property)i;
			if (other.hasProperty(prop)) {
				properties[prop] = other.get(prop, "");
			}
		}

		return *this;
	}

	StyleSpec operator|(const StyleSpec &other) const {
		StyleSpec newspec = *this;
		newspec |= other;
		return newspec;
	}
};

