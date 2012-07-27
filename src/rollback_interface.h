/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef ROLLBACK_INTERFACE_HEADER
#define ROLLBACK_INTERFACE_HEADER

#include "irr_v3d.h"
#include <string>
#include <iostream>
#include "exceptions.h"

class Map;
class IGameDef;
struct MapNode;
class InventoryManager;

struct RollbackNode
{
	std::string name;
	int param1;
	int param2;
	std::string meta;

	bool operator==(const RollbackNode &other)
	{
		return (name == other.name && param1 == other.param1 &&
				param2 == other.param2 && meta == other.meta);
	}
	bool operator!=(const RollbackNode &other)
	{
		return !(*this == other);
	}

	RollbackNode():
		param1(0),
		param2(0)
	{}

	RollbackNode(Map *map, v3s16 p, IGameDef *gamedef);
};

struct RollbackAction
{
	enum Type{
		TYPE_NOTHING,
		TYPE_SET_NODE,
		TYPE_MODIFY_INVENTORY_STACK,
	} type;

	int unix_time;
	std::string actor;
	bool actor_is_guess;

	v3s16 p;
	RollbackNode n_old;
	RollbackNode n_new;
	
	std::string inventory_location;
	std::string inventory_list;
	u32 inventory_index;
	bool inventory_add;
	std::string inventory_stack;

	RollbackAction():
		type(TYPE_NOTHING),
		unix_time(0),
		actor_is_guess(false)
	{}

	void setSetNode(v3s16 p_, const RollbackNode &n_old_,
			const RollbackNode &n_new_)
	{
		type = TYPE_SET_NODE;
		p = p_;
		n_old = n_old_;
		n_new = n_new_;
	}

	void setModifyInventoryStack(const std::string &inventory_location_,
			const std::string &inventory_list_, int index_,
			bool add_, const std::string &inventory_stack_)
	{
		type = TYPE_MODIFY_INVENTORY_STACK;
		inventory_location = inventory_location_;
		inventory_list = inventory_list_;
		inventory_index = index_;
		inventory_add = add_;
		inventory_stack = inventory_stack_;
	}
	
	// String should not contain newlines or nulls
	std::string toString() const;
	void fromStream(std::istream &is) throw(SerializationError);
	
	// Eg. flowing water level changes are not important
	bool isImportant(IGameDef *gamedef) const;
	
	bool getPosition(v3s16 *dst) const;

	bool applyRevert(Map *map, InventoryManager *imgr, IGameDef *gamedef) const;
};

class IRollbackReportSink
{
public:
	virtual ~IRollbackReportSink(){}
	virtual void reportAction(const RollbackAction &action) = 0;
	virtual std::string getActor() = 0;
	virtual bool isActorGuess() = 0;
	virtual void setActor(const std::string &actor, bool is_guess) = 0;
	virtual std::string getSuspect(v3s16 p, float nearness_shortcut,
			float min_nearness) = 0;
};

class RollbackScopeActor
{
public:
	RollbackScopeActor(IRollbackReportSink *sink, const std::string &actor,
			bool is_guess=false):
		m_sink(sink)
	{
		if(m_sink){
			m_actor_was = m_sink->getActor();
			m_actor_was_guess = m_sink->isActorGuess();
			m_sink->setActor(actor, is_guess);
		}
	}
	~RollbackScopeActor()
	{
		if(m_sink){
			m_sink->setActor(m_actor_was, m_actor_was_guess);
		}
	}
private:
	IRollbackReportSink *m_sink;
	std::string m_actor_was;
	bool m_actor_was_guess;
};

#endif
