/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef NAMEIDMAPPING_HEADER
#define NAMEIDMAPPING_HEADER

#include <string>
#include <iostream>
#include <set>
#include <map>
#include "irrlichttypes.h"

class NameIdMapping
{
public:
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
	
	void clear(){
		m_id_to_name.clear();
		m_name_to_id.clear();
	}
	void set(u16 id, const std::string &name){
		m_id_to_name[id] = name;
		m_name_to_id[name] = id;
	}
	void removeId(u16 id){
		std::string name;
		bool found = getName(id, name);
		if(!found) return;
		m_id_to_name.erase(id);
		m_name_to_id.erase(name);
	}
	void eraseName(const std::string &name){
		u16 id;
		bool found = getId(name, id);
		if(!found) return;
		m_id_to_name.erase(id);
		m_name_to_id.erase(name);
	}
	bool getName(u16 id, std::string &result) const{
		std::map<u16, std::string>::const_iterator i;
		i = m_id_to_name.find(id);
		if(i == m_id_to_name.end())
			return false;
		result = i->second;
		return true;
	}
	bool getId(const std::string &name, u16 &result) const{
		std::map<std::string, u16>::const_iterator i;
		i = m_name_to_id.find(name);
		if(i == m_name_to_id.end())
			return false;
		result = i->second;
		return true;
	}
	u16 size() const{
		return m_id_to_name.size();
	}
private:
	std::map<u16, std::string> m_id_to_name;
	std::map<std::string, u16> m_name_to_id;
};

#endif

