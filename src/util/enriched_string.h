/*
Copyright (C) 2013 xyz, Ilya Zhuravlev <whatever@xyz.is>
Copyright (C) 2016 Nore, Nathanaël Courant <nore@mesecons.net>

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

#include <string>
#include <vector>
#include <SColor.h>

class EnrichedString {
public:
	EnrichedString();
	EnrichedString(const std::wstring &s,
		const irr::video::SColor &color = irr::video::SColor(255, 255, 255, 255));
	EnrichedString(const wchar_t *str,
		const irr::video::SColor &color = irr::video::SColor(255, 255, 255, 255));
	EnrichedString(const std::wstring &string,
		const std::vector<irr::video::SColor> &colors);
	void clear();
	void operator=(const wchar_t *str);
	void addAtEnd(const std::wstring &s, const irr::video::SColor &color);

	// Adds the character source[i] at the end.
	// An EnrichedString should always be able to be copied
	// to the end of an existing EnrichedString that way.
	void addChar(const EnrichedString &source, size_t i);

	// Adds a single character at the end, without specifying its
	// color. The color used will be the one from the last character.
	void addCharNoColor(wchar_t c);

	EnrichedString substr(size_t pos = 0, size_t len = std::string::npos) const;
	EnrichedString operator+(const EnrichedString &other) const;
	void operator+=(const EnrichedString &other);
	const wchar_t *c_str() const;
	const std::vector<irr::video::SColor> &getColors() const;
	const std::wstring &getString() const;

	void setDefaultColor(const irr::video::SColor &color);
	void updateDefaultColor();
	inline const irr::video::SColor &getDefaultColor() const
	{
		return m_default_color;
	}

	inline bool operator==(const EnrichedString &other) const
	{
		return (m_string == other.m_string && m_colors == other.m_colors);
	}
	inline bool operator!=(const EnrichedString &other) const
	{
		return !(*this == other);
	}
	inline bool empty() const
	{
		return m_string.empty();
	}
	inline size_t size() const
	{
		return m_string.size();
	}

	inline bool hasBackground() const
	{
		return m_has_background;
	}
	inline irr::video::SColor getBackground() const
	{
		return m_background;
	}
	inline void setBackground(const irr::video::SColor &color)
	{
		m_background = color;
		m_has_background = true;
	}

private:
	std::wstring m_string;
	std::vector<irr::video::SColor> m_colors;
	bool m_has_background;
	irr::video::SColor m_default_color;
	irr::video::SColor m_background;
	// This variable defines the length of the default-colored text.
	// Change this to a std::vector if an "end coloring" tag is wanted.
	size_t m_default_length = 0;
};
