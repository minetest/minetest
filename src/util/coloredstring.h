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

#ifndef COLOREDSTRING_HEADER
#define COLOREDSTRING_HEADER

#include <string>
#include <vector>
#include <SColor.h>

using namespace irr::video;

class ColoredString {
public:
	ColoredString();
	ColoredString(const std::wstring &s);
	ColoredString(const std::wstring &string, const std::vector<SColor> &colors);
	void operator=(const wchar_t *str);
	size_t size() const;
	ColoredString substr(size_t pos = 0, size_t len = std::string::npos) const;
	const wchar_t *c_str() const;
	const std::vector<SColor> &getColors() const;
	const std::wstring &getString() const;
private:
	std::wstring m_string;
	std::vector<SColor> m_colors;
};

#endif
