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

#ifndef UTIL_TIMETAKER_HEADER
#define UTIL_TIMETAKER_HEADER

#include "../irrlichttypes.h"

/*
	TimeTaker
*/

class TimeTaker
{
public:
	TimeTaker(const char *name, u32 *result=NULL);

	~TimeTaker()
	{
		stop();
	}

	u32 stop(bool quiet=false);

	u32 getTime();

private:
	const char *m_name;
	u32 m_time1;
	bool m_running;
	u32 *m_result;
};

#endif

