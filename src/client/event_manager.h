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

#pragma once

#include "mtevent.h"
#include <list>
#include <map>

class EventManager : public MtEventManager
{
	static void receiverReceive(MtEvent *e, void *data)
	{
		MtEventReceiver *r = (MtEventReceiver *)data;
		r->onEvent(e);
	}
	struct FuncSpec
	{
		event_receive_func f;
		void *d;
		FuncSpec(event_receive_func f, void *d) : f(f), d(d) {}
	};

	struct Dest
	{
		std::list<FuncSpec> funcs{};
	};
	std::map<MtEvent::Type, Dest> m_dest{};

public:
	~EventManager() override = default;

	void put(MtEvent *e) override
	{
		std::map<MtEvent::Type, Dest>::iterator i = m_dest.find(e->getType());
		if (i != m_dest.end()) {
			std::list<FuncSpec> &funcs = i->second.funcs;
			for (FuncSpec &func : funcs) {
				(*(func.f))(e, func.d);
			}
		}
		delete e;
	}
	void reg(MtEvent::Type type, event_receive_func f, void *data) override
	{
		std::map<MtEvent::Type, Dest>::iterator i = m_dest.find(type);
		if (i != m_dest.end()) {
			i->second.funcs.emplace_back(f, data);
		} else {
			Dest dest;
			dest.funcs.emplace_back(f, data);
			m_dest[type] = dest;
		}
	}
	void dereg(MtEvent::Type type, event_receive_func f, void *data) override
	{
		std::map<MtEvent::Type, Dest>::iterator i = m_dest.find(type);
		if (i != m_dest.end()) {
			std::list<FuncSpec> &funcs = i->second.funcs;
			auto j = funcs.begin();
			while (j != funcs.end()) {
				bool remove = (j->f == f && (!data || j->d == data));
				if (remove)
					funcs.erase(j++);
				else
					++j;
			}
		}
	}
};
