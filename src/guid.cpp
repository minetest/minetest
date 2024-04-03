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

#include "serverenvironment.h"
#include "util/base64.h"

GUIdGenerator::GUIdGenerator() :
	m_uniform(0, UINT64_MAX)
{
	m_rand_usable = m_rand.entropy() > 0.01;
}

GUId GUIdGenerator::next(const std::string &prefix)
{
	std::stringstream s_guid;

	u64 a[2];
	if (m_rand_usable) {
		a[0] = m_uniform(m_rand);
		a[1] = m_uniform(m_rand);
	}
	else {
		a[0] = (static_cast<u64>(m_env->getGameTime()) << 32) + m_next;
		a[1] = m_uniform(m_rand);
		m_next++;
	}
	s_guid << prefix << base64_encode(std::string_view(reinterpret_cast<char *>(a), 16));

	return s_guid.str();
}

