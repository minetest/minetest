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

// Basic metadata interface
class IMetadata
{
public:
	virtual ~IMetadata() = default;

	virtual void clear() = 0;

	bool operator==(const IMetadata &other) const;
	inline bool operator!=(const IMetadata &other) const
	{
		return !(*this == other);
	}

	//
	// Key-value related
	//

	virtual bool contains(const std::string &name) const = 0;

	// May (not must!) put a string in `place` and return a reference to that string.
	const std::string &getString(const std::string &name, std::string *place,
			u16 recursion = 0) const;

	// If the entry is present, puts the value in str and returns true;
	// otherwise just returns false.
	bool getStringToRef(const std::string &name, std::string &str, u16 recursion = 0) const;

	// Returns whether the metadata was (potentially) changed.
	virtual bool setString(const std::string &name, const std::string &var) = 0;

	inline bool removeString(const std::string &name) { return setString(name, ""); }

	// May (not must!) put strings in `place` and return a reference to these strings.
	virtual const StringMap &getStrings(StringMap *place) const = 0;

	// May (not must!) put keys in `place` and return a reference to these keys.
	virtual const std::vector<std::string> &getKeys(std::vector<std::string> *place) const = 0;

	// Add support for variable names in values. Uses place like getString.
	const std::string &resolveString(const std::string &str, std::string *place,
			u16 recursion = 0, bool deprecated = false) const;

protected:
	// Returns nullptr to indicate absence of value. Uses place like getString.
	virtual const std::string *getStringRaw(const std::string &name,
			std::string *place) const = 0;
};

// Simple metadata parent class (in-memory storage)
class SimpleMetadata: public virtual IMetadata
{
	bool m_modified = false;
public:
	virtual ~SimpleMetadata() = default;

	virtual void clear() override;
	virtual bool empty() const;

	//
	// Key-value related
	//

	size_t size() const;
	bool contains(const std::string &name) const override;
	virtual bool setString(const std::string &name, const std::string &var) override;
	const StringMap &getStrings(StringMap *) const override final;
	const std::vector<std::string> &getKeys(std::vector<std::string> *place)
		const override final;

	// Simple version of getters, possible due to in-memory storage:

	inline const std::string &getString(const std::string &name, u16 recursion = 0) const
	{
		return IMetadata::getString(name, nullptr, recursion);
	}

	inline const std::string &resolveString(const std::string &str, u16 recursion = 0) const
	{
		return IMetadata::resolveString(str, nullptr, recursion);
	}

	inline const StringMap &getStrings() const
	{
		return SimpleMetadata::getStrings(nullptr);
	}

	inline bool isModified() const  { return m_modified; }
	inline void setModified(bool v) { m_modified = v; }

protected:
	StringMap m_stringvars;

	const std::string *getStringRaw(const std::string &name,
			std::string *) const override final;
};
