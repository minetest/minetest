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

#include "timetaker.h"

#include "../gettime.h"
#include "../log.h"
#include <ostream>

TimeTaker::TimeTaker(const char *name, u32 *result)
{
	m_name = name;
	m_result = result;
	m_running = true;
	m_time1 = getTimeMs();
}

u32 TimeTaker::stop(bool quiet)
{
	if(m_running)
	{
		u32 time2 = getTimeMs();
		u32 dtime = time2 - m_time1;
		if(m_result != NULL)
		{
			(*m_result) += dtime;
		}
		else
		{
			if(quiet == false)
				infostream<<m_name<<" took "<<dtime<<"ms"<<std::endl;
		}
		m_running = false;
		return dtime;
	}
	return 0;
}

u32 TimeTaker::getTime()
{
	u32 time2 = getTimeMs();
	u32 dtime = time2 - m_time1;
	return dtime;
}

