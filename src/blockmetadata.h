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

#pragma once

#include <unordered_set>
#include "metadata.h"

/*
	BlockMetadata stores arbitary amounts of data for mapblocks.
*/

class BlockMetadata : public Metadata
{
public:
	BlockMetadata() = default;
	~BlockMetadata() = default;

	void serialize(std::ostream &os, u8 blockver, bool disk=true) const;
	void deSerialize(std::istream &is);

	void clear();

	inline bool isPrivate(const std::string &name) const
	{
		return m_privatevars.count(name) != 0;
	}
	void markPrivate(const std::string &name, bool set);

private:
	u32 countNonPrivate() const;

	std::unordered_set<std::string> m_privatevars;
};
