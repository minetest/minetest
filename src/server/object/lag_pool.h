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

class LagPool
{
	float m_pool = 15.0f;
	float m_max = 15.0f;
public:
	LagPool() = default;

	void setMax(float new_max)
	{
		m_max = new_max;
		if(m_pool > new_max)
			m_pool = new_max;
	}

	void add(float dtime)
	{
		m_pool -= dtime;
		if(m_pool < 0)
			m_pool = 0;
	}

	void empty()
	{
		m_pool = m_max;
	}

	bool grab(float dtime)
	{
		if(dtime <= 0)
			return true;
		if(m_pool + dtime > m_max)
			return false;
		m_pool += dtime;
		return true;
	}
};

