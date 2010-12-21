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

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "utility.h"
#include "irrlichtwrapper.h"
#include "gettime.h"

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
				std::cout<<m_name<<" took "<<dtime<<"ms"<<std::endl;
		}
		m_running = false;
		return dtime;
	}
	return 0;
}

const v3s16 g_26dirs[26] =
{
	// +right, +top, +back
	v3s16( 0, 0, 1), // back
	v3s16( 0, 1, 0), // top
	v3s16( 1, 0, 0), // right
	v3s16( 0, 0,-1), // front
	v3s16( 0,-1, 0), // bottom
	v3s16(-1, 0, 0), // left
	// 6
	v3s16(-1, 1, 0), // top left
	v3s16( 1, 1, 0), // top right
	v3s16( 0, 1, 1), // top back
	v3s16( 0, 1,-1), // top front
	v3s16(-1, 0, 1), // back left
	v3s16( 1, 0, 1), // back right
	v3s16(-1, 0,-1), // front left
	v3s16( 1, 0,-1), // front right
	v3s16(-1,-1, 0), // bottom left
	v3s16( 1,-1, 0), // bottom right
	v3s16( 0,-1, 1), // bottom back
	v3s16( 0,-1,-1), // bottom front
	// 18
	v3s16(-1, 1, 1), // top back-left
	v3s16( 1, 1, 1), // top back-right
	v3s16(-1, 1,-1), // top front-left
	v3s16( 1, 1,-1), // top front-right
	v3s16(-1,-1, 1), // bottom back-left
	v3s16( 1,-1, 1), // bottom back-right
	v3s16(-1,-1,-1), // bottom front-left
	v3s16( 1,-1,-1), // bottom front-right
	// 26
};


