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

#ifndef EVENT_MANAGER_HEADER
#define EVENT_MANAGER_HEADER

#include "event.h"
#include <list>
#include <map>

class EventManager: public MtEventManager
{
	static void receiverReceive(MtEvent *e, void *data)
	{
		MtEventReceiver *r = (MtEventReceiver*)data;
		r->onEvent(e);
	}
	struct FuncSpec{
		event_receive_func f;
		void *d;
		FuncSpec(event_receive_func f, void *d):
			f(f), d(d)
		{}
	};
	struct Dest{
		std::list<FuncSpec> funcs;
	};
	std::map<std::string, Dest> m_dest;

public:
	~EventManager()
	{
	}
	void put(MtEvent *e)
	{
		std::map<std::string, Dest>::iterator i = m_dest.find(e->getType());
		if(i != m_dest.end()){
			std::list<FuncSpec> &funcs = i->second.funcs;
			for(std::list<FuncSpec>::iterator i = funcs.begin();
					i != funcs.end(); i++){
				(*(i->f))(e, i->d);
			}
		}
		delete e;
	}
	void reg(const char *type, event_receive_func f, void *data)
	{
		std::map<std::string, Dest>::iterator i = m_dest.find(type);
		if(i != m_dest.end()){
			i->second.funcs.push_back(FuncSpec(f, data));
		} else{
			std::list<FuncSpec> funcs;
			Dest dest;
			dest.funcs.push_back(FuncSpec(f, data));
			m_dest[type] = dest;
		}
	}
	void dereg(const char *type, event_receive_func f, void *data)
	{
		if(type != NULL){
			std::map<std::string, Dest>::iterator i = m_dest.find(type);
			if(i != m_dest.end()){
				std::list<FuncSpec> &funcs = i->second.funcs;
				std::list<FuncSpec>::iterator j = funcs.begin();
				while(j != funcs.end()){
					bool remove = (j->f == f && (!data || j->d == data));
					if(remove)
						funcs.erase(j++);
					else
						j++;
				}
			}
		} else{
			for(std::map<std::string, Dest>::iterator
					i = m_dest.begin(); i != m_dest.end(); i++){
				std::list<FuncSpec> &funcs = i->second.funcs;
				std::list<FuncSpec>::iterator j = funcs.begin();
				while(j != funcs.end()){
					bool remove = (j->f == f && (!data || j->d == data));
					if(remove)
						funcs.erase(j++);
					else
						j++;
				}
			}
		}
	}
	void reg(MtEventReceiver *r, const char *type)
	{
		reg(type, EventManager::receiverReceive, r);
	}
	void dereg(MtEventReceiver *r, const char *type)
	{
		dereg(type, EventManager::receiverReceive, r);
	}
};

#endif

