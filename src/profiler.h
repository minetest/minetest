/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef PROFILER_HEADER
#define PROFILER_HEADER

#include "common_irrlicht.h"
#include <string>
#include "utility.h"
#include <jmutex.h>
#include <jmutexautolock.h>

/*
	Time profiler
*/

class Profiler
{
public:
	Profiler()
	{
		m_mutex.Init();
	}

	void add(const std::string &name, u32 duration)
	{
		JMutexAutoLock lock(m_mutex);
		core::map<std::string, u32>::Node *n = m_data.find(name);
		if(n == NULL)
		{
			m_data[name] = duration;
		}
		else
		{
			n->setValue(n->getValue()+duration);
		}
	}

	void clear()
	{
		JMutexAutoLock lock(m_mutex);
		for(core::map<std::string, u32>::Iterator
				i = m_data.getIterator();
				i.atEnd() == false; i++)
		{
			i.getNode()->setValue(0);
		}
	}

	void print(std::ostream &o)
	{
		JMutexAutoLock lock(m_mutex);
		for(core::map<std::string, u32>::Iterator
				i = m_data.getIterator();
				i.atEnd() == false; i++)
		{
			std::string name = i.getNode()->getKey();
			o<<name<<": ";
			s32 clampsize = 40;
			s32 space = clampsize-name.size();
			for(s32 j=0; j<space; j++)
			{
				if(j%2 == 0 && j < space - 1)
					o<<"-";
				else
					o<<" ";
			}
			o<<i.getNode()->getValue();
			o<<std::endl;
		}
	}

private:
	JMutex m_mutex;
	core::map<std::string, u32> m_data;
};

class ScopeProfiler
{
public:
	ScopeProfiler(Profiler *profiler, const std::string &name):
		m_profiler(profiler),
		m_name(name),
		m_timer(NULL)
	{
		if(m_profiler)
			m_timer = new TimeTaker(m_name.c_str());
	}
	// name is copied
	ScopeProfiler(Profiler *profiler, const char *name):
		m_profiler(profiler),
		m_name(name),
		m_timer(NULL)
	{
		if(m_profiler)
			m_timer = new TimeTaker(m_name.c_str());
	}
	~ScopeProfiler()
	{
		if(m_timer)
		{
			u32 duration = m_timer->stop(true);
			if(m_profiler)
				m_profiler->add(m_name, duration);
			delete m_timer;
		}
	}
private:
	Profiler *m_profiler;
	std::string m_name;
	TimeTaker *m_timer;
};

#endif

