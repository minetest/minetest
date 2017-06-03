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

#include <fstream>
#include "environment.h"
#include "collision.h"
#include "serverobject.h"
#include "scripting_server.h"
#include "server.h"
#include "daynightratio.h"
#include "emerge.h"


Environment::Environment(IGameDef *gamedef):
	m_time_of_day_speed(0),
	m_time_of_day(9000),
	m_time_of_day_f(9000./24000),
	m_time_conversion_skew(0.0f),
	m_enable_day_night_ratio_override(false),
	m_day_night_ratio_override(0.0f),
	m_gamedef(gamedef)
{
	m_cache_enable_shaders = g_settings->getBool("enable_shaders");
	m_cache_active_block_mgmt_interval = g_settings->getFloat("active_block_mgmt_interval");
	m_cache_abm_interval = g_settings->getFloat("abm_interval");
	m_cache_nodetimer_interval = g_settings->getFloat("nodetimer_interval");
}

Environment::~Environment()
{
}

u32 Environment::getDayNightRatio()
{
	MutexAutoLock lock(this->m_time_lock);
	if (m_enable_day_night_ratio_override)
		return m_day_night_ratio_override;
	return time_to_daynight_ratio(m_time_of_day_f * 24000, m_cache_enable_shaders);
}

void Environment::setTimeOfDaySpeed(float speed)
{
	m_time_of_day_speed = speed;
}

void Environment::setDayNightRatioOverride(bool enable, u32 value)
{
	MutexAutoLock lock(this->m_time_lock);
	m_enable_day_night_ratio_override = enable;
	m_day_night_ratio_override = value;
}

void Environment::setTimeOfDay(u32 time)
{
	MutexAutoLock lock(this->m_time_lock);
	if (m_time_of_day > time)
		++m_day_count;
	m_time_of_day = time;
	m_time_of_day_f = (float)time / 24000.0;
}

u32 Environment::getTimeOfDay()
{
	MutexAutoLock lock(this->m_time_lock);
	return m_time_of_day;
}

float Environment::getTimeOfDayF()
{
	MutexAutoLock lock(this->m_time_lock);
	return m_time_of_day_f;
}

void Environment::stepTimeOfDay(float dtime)
{
	MutexAutoLock lock(this->m_time_lock);

	// Cached in order to prevent the two reads we do to give
	// different results (can be written by code not under the lock)
	f32 cached_time_of_day_speed = m_time_of_day_speed;

	f32 speed = cached_time_of_day_speed * 24000. / (24. * 3600);
	m_time_conversion_skew += dtime;
	u32 units = (u32)(m_time_conversion_skew * speed);
	bool sync_f = false;
	if (units > 0) {
		// Sync at overflow
		if (m_time_of_day + units >= 24000) {
			sync_f = true;
			++m_day_count;
		}
		m_time_of_day = (m_time_of_day + units) % 24000;
		if (sync_f)
			m_time_of_day_f = (float)m_time_of_day / 24000.0;
	}
	if (speed > 0) {
		m_time_conversion_skew -= (f32)units / speed;
	}
	if (!sync_f) {
		m_time_of_day_f += cached_time_of_day_speed / 24 / 3600 * dtime;
		if (m_time_of_day_f > 1.0)
			m_time_of_day_f -= 1.0;
		if (m_time_of_day_f < 0.0)
			m_time_of_day_f += 1.0;
	}
}

u32 Environment::getDayCount()
{
	// Atomic<u32> counter
	return m_day_count;
}
