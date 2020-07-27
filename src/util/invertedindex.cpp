/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <algorithm>

#include "invertedindex.h"
#include "log.h"

u32 InvertedIndex::index(const aabb3f box)
{
	// Assign id to box
	u32 id = m_count++;

	// Update max width.
	v3f width = box.MaxEdge - box.MinEdge;

	if (!width.X || !width.Y || !width.Z)
		// The algorithms will *not* work if there is a collision with a 0-width box.
		return id;

	// Add box to indices.
	findAttributeIndex(COLLISION_BOX_MIN_X, box.MinEdge.X)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MIN_Y, box.MinEdge.Y)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MIN_Z, box.MinEdge.Z)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MAX_X, box.MaxEdge.X)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MAX_Y, box.MaxEdge.Y)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MAX_Z, box.MaxEdge.Z)->push_back(id);

	if (width.X > m_maxWidth.X)
		m_maxWidth.X = width.X;
	if (width.Y > m_maxWidth.Y)
		m_maxWidth.Y = width.Y;
	if (width.Z > m_maxWidth.Z)
		m_maxWidth.Z = width.Z;

	return id;
}

// Find the index of the first entry >= offset
s32 InvertedIndex::lowerAttributeBound(CollisionFace face, f32 offset) const
{
	const std::vector<AttributeIndex> &idx=m_index[face];
	return std::lower_bound(idx.begin(), idx.end(), offset, lowerCompare) - idx.begin();
}

s32 InvertedIndex::lowerAttributeBound(CollisionFace face, f32 offset, u32 begin, u32 end) const
{
	const std::vector<AttributeIndex> &idx=m_index[face];
	return std::lower_bound(idx.begin() + begin, idx.begin() + end, offset, lowerCompare) - idx.begin();
}

s32 InvertedIndex::upperAttributeBound(CollisionFace face, f32 offset) const
{
	const std::vector<AttributeIndex> &idx=m_index[face];
	return std::upper_bound(idx.begin(), idx.end(), offset, upperCompare) - idx.begin();
}

s32 InvertedIndex::upperAttributeBound(CollisionFace face, f32 offset, u32 begin, u32 end) const
{
	const std::vector<AttributeIndex> &idx=m_index[face];
	return std::upper_bound(idx.begin() + begin, idx.begin() + end, offset, upperCompare) - idx.begin();
}

const AttributeIndex *InvertedIndex::getAttributeIndex(CollisionFace face, u32 handle) const
{
	rawstream << "getAttributeIndex " << m_index[face].size() << std::endl;
	return &m_index[face].at(handle);
}

std::vector<u32> *InvertedIndex::findAttributeIndex(CollisionFace face, f32 offset)
{
	std::vector<AttributeIndex> &idx = m_index[face];

	if (idx.empty())
	{
		idx.emplace_back(offset, std::vector<u32>());
		rawstream << "Emplaced " << face << " " << offset << " " << idx.front().second.size() << std::endl;
		return &idx.front().second;
	}

	std::vector<AttributeIndex>::iterator i = std::lower_bound(idx.begin(), idx.end(), offset, lowerCompare);

	if (i != idx.end() && i->first == offset)
		return &i->second;

	std::vector<u32> *vec = &idx.emplace(i, offset, std::vector<u32>())->second;
	rawstream << "Emplaced " << face << " " << offset << " " << vec->size() << std::endl;
	return vec;
}
