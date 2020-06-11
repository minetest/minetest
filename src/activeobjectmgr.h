/*
Minetest
Copyright (C) 2010-2018 nerzhul, Loic BLOT <loic.blot@unix-experience.fr>

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

#include <unordered_map>
#include "irrlichttypes.h"

class TestClientActiveObjectMgr;
class TestServerActiveObjectMgr;

template <typename T>
class ActiveObjectMgr
{
	friend class ::TestClientActiveObjectMgr;
	friend class ::TestServerActiveObjectMgr;

public:
	virtual void step(float dtime, const std::function<void(T *)> &f) = 0;
	virtual bool registerObject(T *obj) = 0;
	virtual void removeObject(u16 id) = 0;

	T *getActiveObject(u16 id)
	{
		typename std::unordered_map<u16, T *>::const_iterator n =
				m_active_objects.find(id);
		return (n != m_active_objects.end() ? n->second : nullptr);
	}

protected:
	u16 getFreeId() const
	{
		// try to reuse id's as late as possible
		static thread_local u16 last_used_id = 0;
		u16 startid = last_used_id;
		while (!isFreeId(++last_used_id)) {
			if (last_used_id == startid)
				return 0;
		}

		return last_used_id;
	}

	bool isFreeId(u16 id) const
	{
		return id != 0 && m_active_objects.find(id) == m_active_objects.end();
	}

	std::unordered_map<u16, T *> m_active_objects;
};
