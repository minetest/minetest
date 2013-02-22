/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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


#include "container.h"
#include "serialize.h"

template <>
void UniqueQueue<v3s16>::serialize(std::ostream &os) const
{
	int num = m_list.size();
	writeU32(os, num);
	for(core::list<v3s16>::ConstIterator it = m_list.begin();
		it != m_list.end(); ++it){
		writeV3S16(os,(*it));
	}
	return;
}

template <>
void UniqueQueue<v3s16>::deserialize(std::istream &is)
{
	m_list.clear();
	m_map.clear();
	int num = readU32(is);
	for(int i = 0; i<num; i++)
	{
		v3s16 p = readV3S16(is);
		push_back(p);
	}
	return;
}
