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

#ifndef NODETIMER_HEADER
#define NODETIMER_HEADER

#include "irr_v3d.h"
#include <iostream>
#include <map>

/*
	NodeTimer provides per-node timed callback functionality.
	Can be used for:
	- Furnaces, to keep the fire burnin'
	- "activated" nodes that snap back to their original state
	  after a fixed amount of time (mesecons buttons, for example)
*/

class NodeTimer
{
public:
	NodeTimer(): timeout(0.), elapsed(0.) {}
	NodeTimer(f32 timeout_, f32 elapsed_):
		timeout(timeout_), elapsed(elapsed_) {}
	~NodeTimer() {}
	
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
	
	f32 timeout;
	f32 elapsed;
};

/*
	List of timers of all the nodes of a block
*/

class NodeTimerList
{
public:
	NodeTimerList() {}
	~NodeTimerList() {}
	
	void serialize(std::ostream &os, u8 map_format_version) const;
	void deSerialize(std::istream &is, u8 map_format_version);
	
	// Get timer
	NodeTimer get(v3s16 p){
		std::map<v3s16, NodeTimer>::iterator n = m_data.find(p);
		if(n == m_data.end())
			return NodeTimer();
		return n->second;
	}
	// Deletes timer
	void remove(v3s16 p){
		m_data.erase(p);
	}
	// Deletes old timer and sets a new one
	void set(v3s16 p, NodeTimer t){
		m_data[p] = t;
	}
	// Deletes all timers
	void clear(){
		m_data.clear();
	}

	// A step in time. Returns map of elapsed timers.
	std::map<v3s16, NodeTimer> step(float dtime);

private:
	std::map<v3s16, NodeTimer> m_data;
};

#endif

