/*
Copyright (C) 2013 xyz, Ilya Zhuravlev <whatever@xyz.is>

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

#include "coloredstring.h"
#include "util/string.h"

ColoredString::ColoredString()
{}

ColoredString::ColoredString(const std::wstring &string, const std::vector<SColor> &colors):
	m_string(string),
	m_colors(colors)
{}

ColoredString::ColoredString(const std::wstring &s) {
	m_string = colorizeText(s, m_colors, SColor(255, 255, 255, 255));
}

void ColoredString::operator=(const wchar_t *str) {
	m_string = colorizeText(str, m_colors, SColor(255, 255, 255, 255));
}

size_t ColoredString::size() const {
	return m_string.size();
}

ColoredString ColoredString::substr(size_t pos, size_t len) const {
	if (pos == m_string.length())
		return ColoredString();
	if (len == std::string::npos || pos + len > m_string.length()) {
		return ColoredString(
		           m_string.substr(pos, std::string::npos),
		           std::vector<SColor>(m_colors.begin() + pos, m_colors.end())
		       );
	} else {
		return ColoredString(
		           m_string.substr(pos, len),
		           std::vector<SColor>(m_colors.begin() + pos, m_colors.begin() + pos + len)
		       );
	}
}

const wchar_t *ColoredString::c_str() const {
	return m_string.c_str();
}

const std::vector<SColor> &ColoredString::getColors() const {
	return m_colors;
}

const std::wstring &ColoredString::getString() const {
	return m_string;
}
