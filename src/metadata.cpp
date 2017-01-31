/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "metadata.h"
#include "exceptions.h"
#include "gamedef.h"
#include "log.h"
#include <sstream>

/*
	Metadata
*/

void Metadata::clear()
{
	m_stringvars.clear();
}

bool Metadata::empty() const
{
	return m_stringvars.size() == 0;
}

size_t Metadata::size() const
{
	return m_stringvars.size();
}

bool Metadata::contains(const std::string &name) const
{
	return m_stringvars.find(name) != m_stringvars.end();
}

bool Metadata::operator==(const Metadata &other) const
{
	if (size() != other.size())
		return false;

	for (StringMap::const_iterator it = m_stringvars.begin();
			it != m_stringvars.end(); ++it) {
		if (!other.contains(it->first) ||
				other.getString(it->first) != it->second)
			return false;
	}

	return true;
}

const std::string &Metadata::getString(const std::string &name,
		u16 recursion) const
{
	StringMap::const_iterator it = m_stringvars.find(name);
	if (it == m_stringvars.end()) {
		static const std::string empty_string = std::string("");
		return empty_string;
	}

	return resolveString(it->second, recursion);
}

void Metadata::setString(const std::string &name, const std::string &var)
{
	if (var.empty()) {
		m_stringvars.erase(name);
	} else {
		m_stringvars[name] = var;
	}
}

const std::string &Metadata::resolveString(const std::string &str,
		u16 recursion) const
{
	if (recursion <= 1 &&
			str.substr(0, 2) == "${" && str[str.length() - 1] == '}') {
		return getString(str.substr(2, str.length() - 3), recursion + 1);
	} else {
		return str;
	}
}
