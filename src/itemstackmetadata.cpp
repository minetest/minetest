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

#include "itemstackmetadata.h"
#include "exceptions.h"
#include "gamedef.h"
#include "inventory.h"
#include "log.h"
#include "util/serialize.h"
#include "util/strfnd.h"
#include "constants.h" // MAP_BLOCKSIZE
#include <sstream>

/*
	ItemStackMetadata
*/

std::string ItemStackMetadata::serialize() const
{
	std::ostringstream os;
	os << "$";
	for (StringMap::const_iterator
			it = m_stringvars.begin();
			it != m_stringvars.end(); ++it) {
		os << it->first << "=";
		os << serializeJsonString(it->second) << "\n";
	}
	return os.str();
}

void ItemStackMetadata::deSerialize(std::string &in)
{
	m_stringvars.clear();
	//to indicate new serialisation version, a dollar sign has to be the first char in the serialized string. if not, apply legacy deserialization
	if (in[0] == '$') {
		Strfnd fnd(in);
		fnd.to(1);
		while ( !fnd.at_end() ) {
			std::string name = fnd.next("=");
			std::string var_json=fnd.next("\n");
			std::istringstream var_stream(var_json);
			std::string var = deSerializeJsonString(var_stream);
			m_stringvars[name] = var;
		}
	}else{
		//legacy: store value in empty field ("")
		m_stringvars[""]=in;
	}
}

void ItemStackMetadata::clear()
{
	m_stringvars.clear();
}
bool ItemStackMetadata::isEmpty() const
{
	return m_stringvars.empty();
}

std::string ItemStackMetadata::getString(const std::string &name,
	unsigned short recursion) const
{
	StringMap::const_iterator it = m_stringvars.find(name);
	if (it == m_stringvars.end())
		return "";

	return resolveString(it->second, recursion);
}

void ItemStackMetadata::setString(const std::string &name, const std::string &var)
{
	if (var.empty()) {
		m_stringvars.erase(name);
	} else {
		m_stringvars[name] = var;
	}
}

std::string ItemStackMetadata::resolveString(const std::string &str,
	unsigned short recursion) const
{
	if (recursion > 1) {
		return str;
	}
	if (str.substr(0, 2) == "${" && str[str.length() - 1] == '}') {
		return getString(str.substr(2, str.length() - 3), recursion + 1);
	}
	return str;
}

