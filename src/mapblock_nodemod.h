/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MAPBLOCK_NODEMOD_HEADER
#define MAPBLOCK_NODEMOD_HEADER

enum NodeModType
{
	NODEMOD_NONE,
	NODEMOD_CHANGECONTENT, //param is content id
	NODEMOD_CRACK // param is crack progression
};

struct NodeMod
{
	NodeMod(enum NodeModType a_type=NODEMOD_NONE, u16 a_param=0)
	{
		type = a_type;
		param = a_param;
	}
	bool operator==(const NodeMod &other)
	{
		return (type == other.type && param == other.param);
	}
	enum NodeModType type;
	u16 param;
};

class NodeModMap
{
public:
	/*
		returns true if the mod was different last time
	*/
	bool set(v3s16 p, const NodeMod &mod)
	{
		// See if old is different, cancel if it is not different.
		core::map<v3s16, NodeMod>::Node *n = m_mods.find(p);
		if(n)
		{
			NodeMod old = n->getValue();
			if(old == mod)
				return false;

			n->setValue(mod);
		}
		else
		{
			m_mods.insert(p, mod);
		}
		
		return true;
	}
	// Returns true if there was one
	bool get(v3s16 p, NodeMod *mod)
	{
		core::map<v3s16, NodeMod>::Node *n;
		n = m_mods.find(p);
		if(n == NULL)
			return false;
		if(mod)
			*mod = n->getValue();
		return true;
	}
	bool clear(v3s16 p)
	{
		if(m_mods.find(p))
		{
			m_mods.remove(p);
			return true;
		}
		return false;
	}
	bool clear()
	{
		if(m_mods.size() == 0)
			return false;
		m_mods.clear();
		return true;
	}
	void copy(NodeModMap &dest)
	{
		dest.m_mods.clear();

		for(core::map<v3s16, NodeMod>::Iterator
				i = m_mods.getIterator();
				i.atEnd() == false; i++)
		{
			dest.m_mods.insert(i.getNode()->getKey(), i.getNode()->getValue());
		}
	}

private:
	core::map<v3s16, NodeMod> m_mods;
};

#endif

