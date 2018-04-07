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

#include "irr_v3d.h"
#include <iostream>
#include <vector>
#include "util/string.h"

class Metadata
{
	bool m_modified = false;
public:
	virtual ~Metadata() = default;

	virtual void clear();
	virtual bool empty() const;

	bool operator==(const Metadata &other) const;
	inline bool operator!=(const Metadata &other) const
	{
		return !(*this == other);
	}

	//
	// Key-value related
	//

	size_t size() const;
	bool contains(const std::string &name) const;
	const std::string &getString(const std::string &name, u16 recursion = 0) const;
	bool getStringToRef(const std::string &name, std::string &str, u16 recursion = 0) const;
	virtual bool setString(const std::string &name, const std::string &var);
	inline bool removeString(const std::string &name) { return setString(name, ""); }
	const StringMap &getStrings() const
	{
		return m_stringvars;
	}
	// Add support for variable names in values
	const std::string &resolveString(const std::string &str, u16 recursion = 0) const;

	inline bool isModified() const  { return m_modified; }
	inline void setModified(bool v) { m_modified = v; }
protected:
	StringMap m_stringvars;
};
