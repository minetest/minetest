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

#include "blockmetadata.h"
#include "exceptions.h"
#include "gamedef.h"
#include "log.h"
#include "util/serialize.h"
#include "util/basic_macros.h"
#include "constants.h" // MAP_BLOCKSIZE
#include <sstream>

/*
	BlockMetadata
*/
void BlockMetadata::serialize(std::ostream &os, u8 blockver, bool disk) const
{
	u8 version = blockver >= 29 ? 1 : 0; // always 1
	writeU8(os, version);

	u32 num_vars = disk ? m_stringvars.size() : countNonPrivate();
	writeU32(os, num_vars);
	for (const auto &sv : m_stringvars) {
		bool priv = isPrivate(sv.first);
		if (!disk && priv)
			continue;

		os << serializeString(sv.first);
		os << serializeLongString(sv.second);
		writeU8(os, priv ? 1 : 0);
	}
}

bool BlockMetadata::deSerialize(std::istream &is)
{
	clear();

	u8 version = readU8(is);
	if (version == 0) {
		return false;
	} else if (version > 1) {
		std::string err_str = std::string(FUNCTION_NAME)
			+ ": version " + itos(version) + " not supported";
		infostream << err_str << std::endl;
		throw SerializationError(err_str);
	}

	u32 num_vars = readU32(is);
	if (num_vars == 0)
		return false;
	for (u32 i = 0; i < num_vars; i++) {
		std::string name = deSerializeString(is);
		std::string var = deSerializeLongString(is);
		m_stringvars[name] = var;
		if (readU8(is) == 1)
			markPrivate(name, true);
	}
	return true;
}

void BlockMetadata::clear()
{
	Metadata::clear();
	m_privatevars.clear();
}


void BlockMetadata::markPrivate(const std::string &name, bool set)
{
	if (set)
		m_privatevars.insert(name);
	else
		m_privatevars.erase(name);
}

u32 BlockMetadata::countNonPrivate() const
{
	// m_privatevars can contain names not actually present
	// DON'T: return m_stringvars.size() - m_privatevars.size();
	u32 n = 0;
	for (const auto &sv : m_stringvars) {
		if (!isPrivate(sv.first))
			n++;
	}
	return n;
}
