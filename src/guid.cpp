/*
Minetest
Copyright (C) 2021, DS

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

#include "guid.h"
#include <sstream>

#define DIGITS_PER_U64 19
#define MAX_NUM_IN_U64 9999999999999999999ul

GUIDGenerator::Validity GUIDGenerator::seed(const GUID &first)
{
	Validity v = getValidity(first);
	if (v != Valid)
		return v;

	m_next_v.clear();

	size_t num_len = first.size() - 6;

	while (num_len > DIGITS_PER_U64) {
		num_len -= DIGITS_PER_U64;
		m_next_v.push_back(std::stoull(first.substr(6 + num_len, DIGITS_PER_U64)));
	}

	m_next_v.push_back(std::stoull(first.substr(6, num_len)));

	return Valid;
}

GUID GUIDGenerator::generateNext()
{
	if (m_next_v.empty())
		return "";

	GUID ret = peekNext();

	// increment m_next_v

	if (m_next_v[0] < MAX_NUM_IN_U64) {
		m_next_v[0] += 1ul;

	} else { // very unlikely
		m_next_v[0] = 0ul;
		size_t s = m_next_v.size();
		for (size_t i = 1; true; i++) {
			if (i == s) {
				m_next_v.push_back(1ul);
				break;
			}
			if (m_next_v[i] < MAX_NUM_IN_U64) {
				m_next_v[i] += 1ul;
				break;
			}
			// very unlikely
			m_next_v[i] = 0ul;
			continue;
		}
	}

	return ret;
}

GUID GUIDGenerator::peekNext() const
{
	if (m_next_v.empty())
		return "";

	std::ostringstream id_strm;

	id_strm << "v1guid";

	for (auto it = m_next_v.rbegin(); it != m_next_v.rend(); it++)
		id_strm << *it;

	return id_strm.str();
}

GUIDGenerator::Validity GUIDGenerator::getValidity(const GUID &id)
{
	if (id.empty())
		return Old;

	size_t s = id.size();

	if (s < 7 || id.substr(0, 6) != "v1guid" || id[6] == '0')
		return Invalid;

	for (size_t i = 7; i < s; i++)
		if (!isdigit(id[i]))
			return Invalid;

	return Valid;
}
