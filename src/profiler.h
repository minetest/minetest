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

#ifndef PROFILER_HEADER
#define PROFILER_HEADER

#include "irrlichttypes.h"
#include <string>
#include <map>

#include "jthread/jmutex.h"
#include "jthread/jmutexautolock.h"
#include "util/timetaker.h"
#include "util/numeric.h" // paging()
#include "debug.h" // assert()

/*
	Time profiler
*/

class Profiler
{
public:
	Profiler()
	{
	}

	void add(const std::string &name, float value)
	{
		JMutexAutoLock lock(m_mutex);
		{
			/* No average shall have been used; mark add used as -2 */
			std::map<std::string, int>::iterator n = m_avgcounts.find(name);
			if(n == m_avgcounts.end())
				m_avgcounts[name] = -2;
			else{
				if(n->second == -1)
					n->second = -2;
				assert(n->second == -2);
			}
		}
		{
			std::map<std::string, float>::iterator n = m_data.find(name);
			if(n == m_data.end())
				m_data[name] = value;
			else
				n->second += value;
		}
	}

	void avg(const std::string &name, float value)
	{
		JMutexAutoLock lock(m_mutex);
		{
			std::map<std::string, int>::iterator n = m_avgcounts.find(name);
			if(n == m_avgcounts.end())
				m_avgcounts[name] = 1;
			else{
				/* No add shall have been used */
				assert(n->second != -2);
				n->second = MYMAX(n->second, 0) + 1;
			}
		}
		{
			std::map<std::string, float>::iterator n = m_data.find(name);
			if(n == m_data.end())
				m_data[name] = value;
			else
				n->second += value;
		}
	}

	void clear()
	{
		JMutexAutoLock lock(m_mutex);
		for(std::map<std::string, float>::iterator
				i = m_data.begin();
				i != m_data.end(); ++i)
		{
			i->second = 0;
		}
		m_avgcounts.clear();
	}

	void print(std::ostream &o)
	{
		printPage(o, 1, 1);
	}

	void printPage(std::ostream &o, u32 page, u32 pagecount)
	{
		JMutexAutoLock lock(m_mutex);

		u32 minindex, maxindex;
		paging(m_data.size(), page, pagecount, minindex, maxindex);

		for(std::map<std::string, float>::iterator
				i = m_data.begin();
				i != m_data.end(); ++i)
		{
			if(maxindex == 0)
				break;
			maxindex--;

			if(minindex != 0)
			{
				minindex--;
				continue;
			}

			std::string name = i->first;
			int avgcount = 1;
			std::map<std::string, int>::iterator n = m_avgcounts.find(name);
			if(n != m_avgcounts.end()){
				if(n->second >= 1)
					avgcount = n->second;
			}
			o<<"  "<<name<<": ";
			s32 clampsize = 40;
			s32 space = clampsize - name.size();
			for(s32 j=0; j<space; j++)
			{
				if(j%2 == 0 && j < space - 1)
					o<<"-";
				else
					o<<" ";
			}
			o<<(i->second / avgcount);
			o<<std::endl;
		}
	}

	typedef std::map<std::string, float> GraphValues;

	void graphAdd(const std::string &id, float value)
	{
		JMutexAutoLock lock(m_mutex);
		std::map<std::string, float>::iterator i =
				m_graphvalues.find(id);
		if(i == m_graphvalues.end())
			m_graphvalues[id] = value;
		else
			i->second += value;
	}
	void graphGet(GraphValues &result)
	{
		JMutexAutoLock lock(m_mutex);
		result = m_graphvalues;
		m_graphvalues.clear();
	}

	void remove(const std::string& name)
	{
		JMutexAutoLock lock(m_mutex);
		m_avgcounts.erase(name);
		m_data.erase(name);
	}

private:
	JMutex m_mutex;
	std::map<std::string, float> m_data;
	std::map<std::string, int> m_avgcounts;
	std::map<std::string, float> m_graphvalues;
};

enum ScopeProfilerType{
	SPT_ADD,
	SPT_AVG,
	SPT_GRAPH_ADD
};

class ScopeProfiler
{
public:
	ScopeProfiler(Profiler *profiler, const std::string &name,
			enum ScopeProfilerType type = SPT_ADD):
		m_profiler(profiler),
		m_name(name),
		m_timer(NULL),
		m_type(type)
	{
		if(m_profiler)
			m_timer = new TimeTaker(m_name.c_str());
	}
	// name is copied
	ScopeProfiler(Profiler *profiler, const char *name,
			enum ScopeProfilerType type = SPT_ADD):
		m_profiler(profiler),
		m_name(name),
		m_timer(NULL),
		m_type(type)
	{
		if(m_profiler)
			m_timer = new TimeTaker(m_name.c_str());
	}
	~ScopeProfiler()
	{
		if(m_timer)
		{
			float duration_ms = m_timer->stop(true);
			float duration = duration_ms / 1000.0;
			if(m_profiler){
				switch(m_type){
				case SPT_ADD:
					m_profiler->add(m_name, duration);
					break;
				case SPT_AVG:
					m_profiler->avg(m_name, duration);
					break;
				case SPT_GRAPH_ADD:
					m_profiler->graphAdd(m_name, duration);
					break;
				}
			}
			delete m_timer;
		}
	}
private:
	Profiler *m_profiler;
	std::string m_name;
	TimeTaker *m_timer;
	enum ScopeProfilerType m_type;
};

#endif

