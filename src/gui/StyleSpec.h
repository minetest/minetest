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

#include "client/tile.h" // ITextureSource
#include "irrlichttypes_extrabloated.h"
#include "util/string.h"
#include <array>

#pragma once

class StyleSpec
{
public:
	enum Property
	{
		TEXTCOLOR,
		BGCOLOR,
		BGCOLOR_HOVERED,
		BGCOLOR_PRESSED,
		NOCLIP,
		BORDER,
		BGIMG,
		BGIMG_HOVERED,
		BGIMG_MIDDLE,
		BGIMG_PRESSED,
		FGIMG,
		FGIMG_HOVERED,
		FGIMG_PRESSED,
		ALPHA,
		NUM_PROPERTIES,
		NONE
	};

private:
	std::array<bool, NUM_PROPERTIES> property_set{};
	std::array<std::string, NUM_PROPERTIES> properties;

public:
	static Property GetPropertyByName(const std::string &name)
	{
		if (name == "textcolor") {
			return TEXTCOLOR;
		} else if (name == "bgcolor") {
			return BGCOLOR;
		} else if (name == "bgcolor_hovered") {
			return BGCOLOR_HOVERED;
		} else if (name == "bgcolor_pressed") {
			return BGCOLOR_PRESSED;
		} else if (name == "noclip") {
			return NOCLIP;
		} else if (name == "border") {
			return BORDER;
		} else if (name == "bgimg") {
			return BGIMG;
		} else if (name == "bgimg_hovered") {
			return BGIMG_HOVERED;
		} else if (name == "bgimg_middle") {
			return BGIMG_MIDDLE;
		} else if (name == "bgimg_pressed") {
			return BGIMG_PRESSED;
		} else if (name == "fgimg") {
			return FGIMG;
		} else if (name == "fgimg_hovered") {
			return FGIMG_HOVERED;
		} else if (name == "fgimg_pressed") {
			return FGIMG_PRESSED;
		} else if (name == "alpha") {
			return ALPHA;
		} else {
			return NONE;
		}
	}

	std::string get(Property prop, std::string def) const
	{
		const auto &val = properties[prop];
		return val.empty() ? def : val;
	}

	void set(Property prop, const std::string &value)
	{
		properties[prop] = value;
		property_set[prop] = true;
	}

	video::SColor getColor(Property prop, video::SColor def) const
	{
		const auto &val = properties[prop];
		if (val.empty()) {
			return def;
		}

		parseColorString(val, def, false, 0xFF);
		return def;
	}

	video::SColor getColor(Property prop) const
	{
		const auto &val = properties[prop];
		FATAL_ERROR_IF(val.empty(), "Unexpected missing property");

		video::SColor color;
		parseColorString(val, color, false, 0xFF);
		return color;
	}

	irr::core::rect<s32> getRect(Property prop, irr::core::rect<s32> def) const
	{
		const auto &val = properties[prop];
		if (val.empty())
			return def;

		irr::core::rect<s32> rect;
		if (!parseRect(val, &rect))
			return def;

		return rect;
	}

	irr::core::rect<s32> getRect(Property prop) const
	{
		const auto &val = properties[prop];
		FATAL_ERROR_IF(val.empty(), "Unexpected missing property");

		irr::core::rect<s32> rect;
		parseRect(val, &rect);
		return rect;
	}

	video::ITexture *getTexture(Property prop, ISimpleTextureSource *tsrc,
			video::ITexture *def) const
	{
		const auto &val = properties[prop];
		if (val.empty()) {
			return def;
		}

		video::ITexture *texture = tsrc->getTexture(val);

		return texture;
	}

	video::ITexture *getTexture(Property prop, ISimpleTextureSource *tsrc) const
	{
		const auto &val = properties[prop];
		FATAL_ERROR_IF(val.empty(), "Unexpected missing property");

		video::ITexture *texture = tsrc->getTexture(val);

		return texture;
	}

	bool getBool(Property prop, bool def) const
	{
		const auto &val = properties[prop];
		if (val.empty()) {
			return def;
		}

		return is_yes(val);
	}

	inline bool isNotDefault(Property prop) const
	{
		return !properties[prop].empty();
	}

	inline bool hasProperty(Property prop) const { return property_set[prop]; }

	StyleSpec &operator|=(const StyleSpec &other)
	{
		for (size_t i = 0; i < NUM_PROPERTIES; i++) {
			auto prop = (Property)i;
			if (other.hasProperty(prop)) {
				set(prop, other.get(prop, ""));
			}
		}

		return *this;
	}

	StyleSpec operator|(const StyleSpec &other) const
	{
		StyleSpec newspec = *this;
		newspec |= other;
		return newspec;
	}

private:
	bool parseRect(const std::string &value, irr::core::rect<s32> *parsed_rect) const
	{
		irr::core::rect<s32> rect;
		std::vector<std::string> v_rect = split(value, ',');

		if (v_rect.size() == 1) {
			s32 x = stoi(v_rect[0]);
			rect.UpperLeftCorner = irr::core::vector2di(x, x);
			rect.LowerRightCorner = irr::core::vector2di(-x, -x);
		} else if (v_rect.size() == 2) {
			s32 x = stoi(v_rect[0]);
			s32 y =	stoi(v_rect[1]);
			rect.UpperLeftCorner = irr::core::vector2di(x, y);
			rect.LowerRightCorner = irr::core::vector2di(-x, -y);
			// `-x` is interpreted as `w - x`
		} else if (v_rect.size() == 4) {
			rect.UpperLeftCorner = irr::core::vector2di(
					stoi(v_rect[0]), stoi(v_rect[1]));
			rect.LowerRightCorner = irr::core::vector2di(
					stoi(v_rect[2]), stoi(v_rect[3]));
		} else {
			warningstream << "Invalid rectangle string format: \"" << value
					<< "\"" << std::endl;
			return false;
		}

		*parsed_rect = rect;

		return true;
	}
};
