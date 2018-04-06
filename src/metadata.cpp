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
#include "log.h"

/*
	Metadata
*/

void Metadata::clear()
{
	m_stringvars.clear();
	m_modified = true;
}

bool Metadata::empty() const
{
	return m_stringvars.empty();
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

	for (const auto &sv : m_stringvars) {
		if (!other.contains(sv.first) || other.getString(sv.first) != sv.second)
			return false;
	}

	return true;
}

const std::string &Metadata::getString(const std::string &name, u16 recursion) const
{
	StringMap::const_iterator it = m_stringvars.find(name);
	if (it == m_stringvars.end()) {
		static const std::string empty_string = std::string("");
		return empty_string;
	}

	return resolveString(it->second, recursion);
}

bool Metadata::getStringToRef(
		const std::string &name, std::string &str, u16 recursion) const
{
	StringMap::const_iterator it = m_stringvars.find(name);
	if (it == m_stringvars.end()) {
		return false;
	}

	str = resolveString(it->second, recursion);
	return true;
}

/**
 * Sets var to name key in the metadata storage
 *
 * @param name
 * @param var
 * @return true if key-value pair is created or changed
 */
bool Metadata::setString(const std::string &name, const std::string &var)
{
	if (var.empty()) {
		m_stringvars.erase(name);
		return true;
	}

	StringMap::iterator it = m_stringvars.find(name);
	if (it != m_stringvars.end() && it->second == var) {
		return false;
	}

	m_stringvars[name] = var;
	m_modified = true;
	return true;
}

const std::string &Metadata::resolveString(const std::string &str, u16 recursion) const
{
	if (recursion <= 1 && str.substr(0, 2) == "${" && str[str.length() - 1] == '}') {
		return getString(str.substr(2, str.length() - 3), recursion + 1);
	}

	return str;
}
