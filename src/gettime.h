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

#include <ctime>
#include <string>
#include <mutex>

enum TimePrecision
{
	PRECISION_SECONDS,
	PRECISION_MILLI,
	PRECISION_MICRO,
	PRECISION_NANO
};

inline struct tm mt_localtime()
{
	// initialize the time zone on first invocation
	static std::once_flag tz_init;
	std::call_once(tz_init, [] {
#ifdef _WIN32
		_tzset();
#else
		tzset();
#endif
		});

	struct tm ret;
	time_t t = time(NULL);
	// TODO we should check if the function returns NULL, which would mean error
#ifdef _WIN32
	localtime_s(&ret, &t);
#else
	localtime_r(&t, &ret);
#endif
	return ret;
}


inline std::string getTimestamp()
{
	const struct tm tm = mt_localtime();
	char cs[20]; // YYYY-MM-DD HH:MM:SS + '\0'
	strftime(cs, 20, "%Y-%m-%d %H:%M:%S", &tm);
	return cs;
}
