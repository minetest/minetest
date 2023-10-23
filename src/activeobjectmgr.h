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

#include <map>
#include <memory>
#include <vector>
#include "debug.h"
#include "irrlichttypes.h"
#include "util/basic_macros.h"

class TestClientActiveObjectMgr;
class TestServerActiveObjectMgr;

template <typename T>
class ActiveObjectMgr
{
	friend class ::TestClientActiveObjectMgr;
	friend class ::TestServerActiveObjectMgr;

public:
	ActiveObjectMgr() = default;
	DISABLE_CLASS_COPY(ActiveObjectMgr);

	virtual ~ActiveObjectMgr()
	{
		SANITY_CHECK(m_active_objects.empty());
		// Note: Do not call clear() here. The derived class is already half
		// destructed.
	}

	virtual void step(float dtime, const std::function<void(T *)> &f) = 0;
	virtual bool registerObject(std::unique_ptr<T> obj) = 0;
	virtual void removeObject(u16 id) = 0;

	void clear()
	{
		while (!m_active_objects.empty())
			removeObject(m_active_objects.begin()->first);
	}

	T *getActiveObject(u16 id)
	{
		auto it = m_active_objects.find(id);
		return it != m_active_objects.end() ? it->second.get() : nullptr;
	}

	std::vector<u16> getAllIds() const
	{
		std::vector<u16> ids;
		ids.reserve(m_active_objects.size());
		for (auto &it : m_active_objects) {
			ids.push_back(it.first);
		}
		return ids;
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

	// ordered to fix #10985
	// Note: ActiveObjects can access the ActiveObjectMgr. Only erase objects using
	// removeObject()!
	std::map<u16, std::unique_ptr<T>> m_active_objects;
};
