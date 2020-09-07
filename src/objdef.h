/*
Minetest
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include "util/basic_macros.h"
#include "porting.h"

class IGameDef;
class NodeDefManager;

#define OBJDEF_INVALID_INDEX ((u32)(-1))
#define OBJDEF_INVALID_HANDLE 0
#define OBJDEF_HANDLE_SALT 0x00585e6fu
#define OBJDEF_MAX_ITEMS (1 << 18)
#define OBJDEF_UID_MASK ((1 << 7) - 1)

typedef u32 ObjDefHandle;

enum ObjDefType {
	OBJDEF_GENERIC,
	OBJDEF_BIOME,
	OBJDEF_ORE,
	OBJDEF_DECORATION,
	OBJDEF_SCHEMATIC,
};

class ObjDef {
public:
	virtual ~ObjDef() = default;

	u32 index;
	u32 uid;
	ObjDefHandle handle;
	std::string name;
};

// WARNING: Ownership of ObjDefs is transferred to the ObjDefManager it is
// added/set to.  Note that ObjDefs managed by ObjDefManager are NOT refcounted,
// so the same ObjDef instance must not be referenced multiple
class ObjDefManager {
public:
	ObjDefManager(IGameDef *gamedef, ObjDefType type);
	virtual ~ObjDefManager();
	DISABLE_CLASS_COPY(ObjDefManager);

	virtual const char *getObjectTitle() const { return "ObjDef"; }

	virtual void clear();
	virtual ObjDef *getByName(const std::string &name) const;

	//// Add new/get/set object definitions by handle
	virtual ObjDefHandle add(ObjDef *obj);
	virtual ObjDef *get(ObjDefHandle handle) const;
	virtual ObjDef *set(ObjDefHandle handle, ObjDef *obj);

	//// Raw variants that work on indexes
	virtual u32 addRaw(ObjDef *obj);

	// It is generally assumed that getRaw() will always return a valid object
	// This won't be true if people do odd things such as call setRaw() with NULL
	virtual ObjDef *getRaw(u32 index) const;
	virtual ObjDef *setRaw(u32 index, ObjDef *obj);

	size_t getNumObjects() const { return m_objects.size(); }
	ObjDefType getType() const { return m_objtype; }
	const NodeDefManager *getNodeDef() const { return m_ndef; }

	u32 validateHandle(ObjDefHandle handle) const;
	static ObjDefHandle createHandle(u32 index, ObjDefType type, u32 uid);
	static bool decodeHandle(ObjDefHandle handle, u32 *index,
		ObjDefType *type, u32 *uid);

protected:
	const NodeDefManager *m_ndef;
	std::vector<ObjDef *> m_objects;
	ObjDefType m_objtype;
};
