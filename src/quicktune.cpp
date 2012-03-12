/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "quicktune.h"
#include <jmutex.h>
#include <jmutexautolock.h>

static std::map<std::string, QuicktuneValue> g_values;
static std::vector<std::string> g_names;
JMutex *g_mutex = NULL;

static void makeMutex()
{
	if(!g_mutex){
		g_mutex = new JMutex();
		g_mutex->Init();
	}
}

std::vector<std::string> getQuicktuneNames()
{
	return g_names;
}

/*std::map<std::string, QuicktuneValue> getQuicktuneValues()
{
	makeMutex();
	JMutexAutoLock lock(*g_mutex);
	return g_values;
}*/

QuicktuneValue getQuicktuneValue(const std::string &name)
{
	makeMutex();
	JMutexAutoLock lock(*g_mutex);
	std::map<std::string, QuicktuneValue>::iterator i = g_values.find(name);
	if(i == g_values.end()){
		QuicktuneValue val;
		val.type = QUICKTUNE_NONE;
		return val;
	}
	return i->second;
}

void setQuicktuneValue(const std::string &name, const QuicktuneValue &val)
{
	makeMutex();
	JMutexAutoLock lock(*g_mutex);
	g_values[name] = val;
}

void updateQuicktuneValue(const std::string &name, QuicktuneValue &val)
{
	makeMutex();
	JMutexAutoLock lock(*g_mutex);
	std::map<std::string, QuicktuneValue>::iterator i = g_values.find(name);
	if(i == g_values.end()){
		g_values[name] = val;
		g_names.push_back(name);
		return;
	}
	QuicktuneValue &ref = i->second;
	switch(val.type){
	case QUICKTUNE_NONE:
		break;
	case QUICKTUNE_FLOAT:
		val.value_float.current = ref.value_float.current;
		break;
	}
}

