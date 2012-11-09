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

#ifndef DAYNIGHTRATIO_HEADER
#define DAYNIGHTRATIO_HEADER

inline u32 time_to_daynight_ratio(u32 time_of_day)
{
	s32 t = time_of_day%24000;
	if(t < 4500 || t >= 19500)
		return 150;
	else if(t < 4750 || t >= 19250)
		return 250;
	else if(t < 5000 || t >= 19000)
		return 350;
	else if(t < 5250 || t >= 18750)
		return 500;
	else if(t < 5500 || t >= 18500)
		return 675;
	else if(t < 5750 || t >= 18250)
		return 875;
	else
		return 1000;
}

#endif

