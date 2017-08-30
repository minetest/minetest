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

#include "enriched_string.h"
#include "util/string.h"
#include "log.h"
using namespace irr::video;

EnrichedString::EnrichedString()
{
	clear();
}

EnrichedString::EnrichedString(const std::wstring &string,
		const std::vector<SColor> &colors):
	m_string(string),
	m_colors(colors)
{}

EnrichedString::EnrichedString(const std::wstring &s, const SColor &color)
{
	clear();
	addAtEnd(translate_string(s), color);
}

EnrichedString::EnrichedString(const wchar_t *str, const SColor &color)
{
	clear();
	addAtEnd(translate_string(std::wstring(str)), color);
}

void EnrichedString::operator=(const wchar_t *str)
{
	clear();
	addAtEnd(translate_string(std::wstring(str)), SColor(255, 255, 255, 255));
}

void EnrichedString::addAtEnd(const std::wstring &s, const SColor &initial_color)
{
	SColor color(initial_color);
	size_t i = 0;
	while (i < s.length()) {
		if (s[i] != L'\x1b') {
			m_string += s[i];
			m_colors.push_back(color);
			++i;
			continue;
		}
		++i;
		size_t start_index = i;
		size_t length;
		if (i == s.length()) {
			break;
		}
		if (s[i] == L'(') {
			++i;
			++start_index;
			while (i < s.length() && s[i] != L')') {
				if (s[i] == L'\\') {
					++i;
				}
				++i;
			}
			length = i - start_index;
			++i;
		} else {
			++i;
			length = 1;
		}
		std::wstring escape_sequence(s, start_index, length);
		std::vector<std::wstring> parts = split(escape_sequence, L'@');
		if (parts[0] == L"c") {
			if (parts.size() < 2) {
				continue;
			}
			parseColorString(wide_to_utf8(parts[1]), color, true);
		} else if (parts[0] == L"b") {
			if (parts.size() < 2) {
				continue;
			}
			parseColorString(wide_to_utf8(parts[1]), m_background, true);
			m_has_background = true;
		}
	}
}

void EnrichedString::addChar(const EnrichedString &source, size_t i)
{
	m_string += source.m_string[i];
	m_colors.push_back(source.m_colors[i]);
}

void EnrichedString::addCharNoColor(wchar_t c)
{
	m_string += c;
	if (m_colors.empty()) {
		m_colors.emplace_back(255, 255, 255, 255);
	} else {
		m_colors.push_back(m_colors[m_colors.size() - 1]);
	}
}

EnrichedString EnrichedString::operator+(const EnrichedString &other) const
{
	std::vector<SColor> result;
	result.insert(result.end(), m_colors.begin(), m_colors.end());
	result.insert(result.end(), other.m_colors.begin(), other.m_colors.end());
	return EnrichedString(m_string + other.m_string, result);
}

void EnrichedString::operator+=(const EnrichedString &other)
{
	m_string += other.m_string;
	m_colors.insert(m_colors.end(), other.m_colors.begin(), other.m_colors.end());
}

EnrichedString EnrichedString::substr(size_t pos, size_t len) const
{
	if (pos == m_string.length()) {
		return EnrichedString();
	}
	if (len == std::string::npos || pos + len > m_string.length()) {
		return EnrichedString(
			m_string.substr(pos, std::string::npos),
			std::vector<SColor>(m_colors.begin() + pos, m_colors.end())
		);
	}

	return EnrichedString(
		m_string.substr(pos, len),
		std::vector<SColor>(m_colors.begin() + pos, m_colors.begin() + pos + len)
	);

}

const wchar_t *EnrichedString::c_str() const
{
	return m_string.c_str();
}

const std::vector<SColor> &EnrichedString::getColors() const
{
	return m_colors;
}

const std::wstring &EnrichedString::getString() const
{
	return m_string;
}
