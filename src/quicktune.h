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

#ifndef QUICKTUNE_HEADER
#define QUICKTUNE_HEADER

#include "irrlichttypes.h"
#include <string>
#include <map>
#include <vector>
#include "utility.h"

enum QuicktuneValueType{
	QUICKTUNE_NONE,
	QUICKTUNE_FLOAT
};
struct QuicktuneValue
{
	QuicktuneValueType type;
	union{
		struct{
			float current;
			float min;
			float max;
		} value_float;
	};
};

std::vector<std::string> getQuicktuneNames();
//std::map<std::string, QuicktuneValue> getQuicktuneNames();
QuicktuneValue getQuicktuneValue(const std::string &name);
void setQuicktuneValue(const std::string &name, const QuicktuneValue &val);

class QuicktuneShortcutter
{
private:
	std::vector<std::string> m_names;
	u32 m_selected_i;
	std::string m_message;
public:
	std::string getMessage()
	{
		std::string s = m_message;
		m_message = "";
		return s;
	}
	std::string getSelectedName()
	{
		if(m_selected_i < m_names.size())
			return m_names[m_selected_i];
		return "";
	}
	void next()
	{
		m_names = getQuicktuneNames();
		if(m_selected_i < m_names.size()-1)
			m_selected_i++;
		else
			m_selected_i = 0;
		m_message = std::string("Selected \"")+getSelectedName()+"\"";
	}
	void prev()
	{
		m_names = getQuicktuneNames();
		if(m_selected_i > 0)
			m_selected_i--;
		else
			m_selected_i = m_names.size()-1;
		m_message = std::string("Selected \"")+getSelectedName()+"\"";
	}
	void inc()
	{
		QuicktuneValue val = getQuicktuneValue(getSelectedName());
		switch(val.type){
		case QUICKTUNE_NONE:
			break;
		case QUICKTUNE_FLOAT:
			val.value_float.current += 0.05 * (val.value_float.max - val.value_float.min);
			if(val.value_float.current > val.value_float.max)
				val.value_float.current = val.value_float.max;
			m_message = std::string("\"")+getSelectedName()
					+"\" = "+ftos(val.value_float.current);
			break;
		default:
			m_message = std::string("\"")+getSelectedName()
					+"\" has unknown value type";
		}
		setQuicktuneValue(getSelectedName(), val);
	}
	void dec()
	{
		QuicktuneValue val = getQuicktuneValue(getSelectedName());
		switch(val.type){
		case QUICKTUNE_NONE:
			break;
		case QUICKTUNE_FLOAT:
			val.value_float.current -= 0.05 * (val.value_float.max - val.value_float.min);
			if(val.value_float.current < val.value_float.max)
				val.value_float.current = val.value_float.max;
			m_message = std::string("\"")+getSelectedName()
					+"\" = "+ftos(val.value_float.current);
			break;
		default:
			m_message = std::string("\"")+getSelectedName()
					+"\" has unknown value type";
		}
		setQuicktuneValue(getSelectedName(), val);
	}
};

void updateQuicktuneValue(const std::string &name, QuicktuneValue &val);

#ifndef NDEBUG

#define QUICKTUNE_FLOAT(var, min, max, name){\
	QuicktuneValue qv;\
	qv.type = QUICKTUNE_FLOAT;\
	qv.value_float.current = var;\
	qv.value_float.min = min;\
	qv.value_float.min = max;\
	updateQuicktune(name, qv);\
	var = qv.value_float.current;\
}

#else // NDEBUG

#define QUICKTUNE_FLOAT(var, min, max, name){}

#endif

#endif

