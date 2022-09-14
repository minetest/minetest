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

#include "quicktune.h"

class QuicktuneShortcutter
{
private:
	std::vector<std::string> m_names;
	u32 m_selected_i;
	std::string m_message;
public:
	bool hasMessage() const
	{
		return !m_message.empty();
	}

	std::string getMessage()
	{
		std::string s = m_message;
		m_message.clear();
		if (!s.empty())
			return std::string("[quicktune] ") + s;
		return "";
	}
	std::string getSelectedName()
	{
		if(m_selected_i < m_names.size())
			return m_names[m_selected_i];
		return "(nothing)";
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
		val.relativeAdd(0.05);
		m_message = std::string("\"")+getSelectedName()
				+"\" = "+val.getString();
		setQuicktuneValue(getSelectedName(), val);
	}
	void dec()
	{
		QuicktuneValue val = getQuicktuneValue(getSelectedName());
		val.relativeAdd(-0.05);
		m_message = std::string("\"")+getSelectedName()
				+"\" = "+val.getString();
		setQuicktuneValue(getSelectedName(), val);
	}
};
