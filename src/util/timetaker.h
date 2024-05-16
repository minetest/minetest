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

#pragma once

#include <string>
#include "irrlichttypes.h"

enum TimePrecision : s8
{
	PRECISION_SECONDS,
	PRECISION_MILLI,
	PRECISION_MICRO,
	PRECISION_NANO,
};

constexpr const char *TimePrecision_units[] = {
	"s"  /* PRECISION_SECONDS */,
	"ms" /* PRECISION_MILLI */,
	"us" /* PRECISION_MICRO */,
	"ns" /* PRECISION_NANO */,
};

/*
	TimeTaker
*/

// Note: this class should be kept lightweight

class TimeTaker
{
public:
	TimeTaker(const std::string &name, u64 *result = nullptr,
		TimePrecision prec = PRECISION_MILLI)
	{
		if (result)
			m_result = result;
		else
			m_name = name;
		m_precision = prec;
		start();
	}

	~TimeTaker()
	{
		stop();
	}

	u64 stop(bool quiet=false);

	u64 getTimerTime();

private:
	void start();

	std::string m_name;
	u64 *m_result = nullptr;
	u64 m_time1;
	bool m_running = true;
	TimePrecision m_precision;
};
