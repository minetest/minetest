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

#include "nodetimer.h"
#include "log.h"
#include "util/serialize.h"
#include "constants.h" // MAP_BLOCKSIZE

/*
	NodeTimer
*/

void NodeTimer::serialize(std::ostream &os) const
{
	writeF1000(os, duration);
	writeF1000(os, elapsed);
}

void NodeTimer::deSerialize(std::istream &is)
{
	duration = readF1000(is);
	elapsed = readF1000(is);
}

/*
	NodeTimerList
*/

void NodeTimerList::serialize(std::ostream &os) const
{
	/*
		Version 0 is a placeholder for "nothing to see here; go away."
	*/

	if(m_data.size() == 0){
		writeU8(os, 0); // version
		return;
	}

	writeU8(os, 1); // version
	writeU16(os, m_data.size());

	for(std::map<v3s16, NodeTimer>::const_iterator
			i = m_data.begin();
			i != m_data.end(); i++){
		v3s16 p = i->first;
		NodeTimer t = i->second;

		u16 p16 = p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X;
		writeU16(os, p16);
		t.serialize(os);
	}
}

void NodeTimerList::deSerialize(std::istream &is)
{
	m_data.clear();

	u8 version = readU8(is);
	if(version == 0)
		return;
	if(version != 1)
		throw SerializationError("unsupported NodeTimerList version");

	u16 count = readU16(is);

	for(u16 i=0; i<count; i++)
	{
		u16 p16 = readU16(is);

		v3s16 p(0,0,0);
		p.Z += p16 / MAP_BLOCKSIZE / MAP_BLOCKSIZE;
		p16 -= p.Z * MAP_BLOCKSIZE * MAP_BLOCKSIZE;
		p.Y += p16 / MAP_BLOCKSIZE;
		p16 -= p.Y * MAP_BLOCKSIZE;
		p.X += p16;

		NodeTimer t;
		t.deSerialize(is);

		if(t.duration <= 0)
		{
			infostream<<"WARNING: NodeTimerList::deSerialize(): "
					<<"invalid data at position"
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<"): Ignoring."
					<<std::endl;
			continue;
		}

		if(m_data.find(p) != m_data.end())
		{
			infostream<<"WARNING: NodeTimerList::deSerialize(): "
					<<"already set data at position"
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<"): Ignoring."
					<<std::endl;
			continue;
		}

		m_data.insert(std::make_pair(p, t));
	}
}

std::map<v3s16, f32> NodeTimerList::step(float dtime)
{
	std::map<v3s16, f32> elapsed_timers;
	// Increment timers
	for(std::map<v3s16, NodeTimer>::iterator
			i = m_data.begin();
			i != m_data.end(); i++){
		v3s16 p = i->first;
		NodeTimer t = i->second;
		t.elapsed += dtime;
		if(t.elapsed >= t.duration)
			elapsed_timers.insert(std::make_pair(p, t.elapsed));
		else
			i->second = t;
	}
	// Delete elapsed timers
	for(std::map<v3s16, f32>::const_iterator
			i = elapsed_timers.begin();
			i != elapsed_timers.end(); i++){
		v3s16 p = i->first;
		m_data.erase(p);
	}
	return elapsed_timers;
}
