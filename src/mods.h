/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef MODS_HEADER
#define MODS_HEADER

#include "irrlichttypes.h"
#include <irrList.h>
#include <set>
#include <string>
#include <exception>

class ModError : public std::exception
{
public:
	ModError(const std::string &s)
	{
		m_s = "ModError: ";
		m_s += s;
	}
	virtual ~ModError() throw()
	{}
	virtual const char * what() const throw()
	{
		return m_s.c_str();
	}
	std::string m_s;
};

struct ModSpec
{
	std::string name;
	std::string path;
	std::set<std::string> depends;
	std::set<std::string> unsatisfied_depends;

	ModSpec(const std::string &name_="", const std::string path_="",
			const std::set<std::string> &depends_=std::set<std::string>()):
		name(name_),
		path(path_),
		depends(depends_),
		unsatisfied_depends(depends_)
	{}
};

// Get a dependency-sorted list of ModSpecs
core::list<ModSpec> getMods(core::list<std::string> &modspaths)
		throw(ModError);

#endif

