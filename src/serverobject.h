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

#ifndef SERVEROBJECT_HEADER
#define SERVEROBJECT_HEADER

#include "irrlichttypes_bloated.h"
#include "activeobject.h"
#include "inventorymanager.h"
#include "itemgroup.h"
#include "util/container.h"

/*

Some planning
-------------

* Server environment adds an active object, which gets the id 1
* The active object list is scanned for each client once in a while,
  and it finds out what objects have been added that are not known
  by the client yet. This scan is initiated by the Server class and
  the result ends up directly to the server.
* A network packet is created with the info and sent to the client.
* Environment converts objects to static data and static data to
  objects, based on how close players are to them.

*/

class ServerEnvironment;
struct ItemStack;
class Player;
struct ToolCapabilities;
struct ObjectProperties;

class ServerActiveObject : public ActiveObject
{
public:
	/*
		NOTE: m_env can be NULL, but step() isn't called if it is.
		Prototypes are used that way.
	*/
	ServerActiveObject(ServerEnvironment *env, v3f pos);
	virtual ~ServerActiveObject();

	virtual u8 getSendType() const
	{ return getType(); }

	// Called after id has been set and has been inserted in environment
	virtual void addedToEnvironment(u32 dtime_s){};
	// Called before removing from environment
	virtual void removingFromEnvironment(){};
	// Returns true if object's deletion is the job of the
	// environment
	virtual bool environmentDeletes() const
	{ return true; }

	virtual bool unlimitedTransferDistance() const
	{ return false; }
	
	// Create a certain type of ServerActiveObject
	static ServerActiveObject* create(u8 type,
			ServerEnvironment *env, u16 id, v3f pos,
			const std::string &data);
	
	/*
		Some simple getters/setters
	*/
	v3f getBasePosition(){ return m_base_position; }
	void setBasePosition(v3f pos){ m_base_position = pos; }
	ServerEnvironment* getEnv(){ return m_env; }
	
	/*
		Some more dynamic interface
	*/
	
	virtual void setPos(v3f pos)
		{ setBasePosition(pos); }
	// continuous: if true, object does not stop immediately at pos
	virtual void moveTo(v3f pos, bool continuous)
		{ setBasePosition(pos); }
	// If object has moved less than this and data has not changed,
	// saving to disk may be omitted
	virtual float getMinimumSavedMovement();

	virtual std::string getDescription(){return "SAO";}
	
	/*
		Step object in time.
		Messages added to messages are sent to client over network.

		send_recommended:
			True at around 5-10 times a second, same for all objects.
			This is used to let objects send most of the data at the
			same time so that the data can be combined in a single
			packet.
	*/
	virtual void step(float dtime, bool send_recommended){}
	
	/*
		The return value of this is passed to the client-side object
		when it is created
	*/
	virtual std::string getClientInitializationData(u16 protocol_version){return "";}
	
	/*
		The return value of this is passed to the server-side object
		when it is created (converted from static to active - actually
		the data is the static form)
	*/
	virtual std::string getStaticData()
	{
		assert(isStaticAllowed());
		return "";
	}
	/*
		Return false in here to never save and instead remove object
		on unload. getStaticData() will not be called in that case.
	*/
	virtual bool isStaticAllowed() const
	{return true;}
	
	// Returns tool wear
	virtual int punch(v3f dir,
			const ToolCapabilities *toolcap=NULL,
			ServerActiveObject *puncher=NULL,
			float time_from_last_punch=1000000)
	{ return 0; }
	virtual void rightClick(ServerActiveObject *clicker)
	{}
	virtual void setHP(s16 hp)
	{}
	virtual s16 getHP() const
	{ return 0; }

	virtual void setArmorGroups(const ItemGroupList &armor_groups)
	{}
	virtual void setPhysicsOverride(float physics_override_speed, float physics_override_jump, float physics_override_gravity)
	{}
	virtual void setAnimation(v2f frames, float frame_speed, float frame_blend)
	{}
	virtual void setBonePosition(std::string bone, v3f position, v3f rotation)
	{}
	virtual void setAttachment(int parent_id, std::string bone, v3f position, v3f rotation)
	{}
	virtual ObjectProperties* accessObjectProperties()
	{ return NULL; }
	virtual void notifyObjectPropertiesModified()
	{}

	// Inventory and wielded item
	virtual Inventory* getInventory()
	{ return NULL; }
	virtual const Inventory* getInventory() const
	{ return NULL; }
	virtual InventoryLocation getInventoryLocation() const
	{ return InventoryLocation(); }
	virtual void setInventoryModified()
	{}
	virtual std::string getWieldList() const
	{ return ""; }
	virtual int getWieldIndex() const
	{ return 0; }
	virtual ItemStack getWieldedItem() const;
	virtual bool setWieldedItem(const ItemStack &item);

	/*
		Number of players which know about this object. Object won't be
		deleted until this is 0 to keep the id preserved for the right
		object.
	*/
	u16 m_known_by_count;

	/*
		- Whether this object is to be removed when nobody knows about
		  it anymore.
		- Removal is delayed to preserve the id for the time during which
		  it could be confused to some other object by some client.
		- This is set to true by the step() method when the object wants
		  to be deleted.
		- This can be set to true by anything else too.
	*/
	bool m_removed;
	
	/*
		This is set to true when an object should be removed from the active
		object list but couldn't be removed because the id has to be
		reserved for some client.

		The environment checks this periodically. If this is true and also
		m_known_by_count is true, object is deleted from the active object
		list.
	*/
	bool m_pending_deactivation;
	
	/*
		Whether the object's static data has been stored to a block
	*/
	bool m_static_exists;
	/*
		The block from which the object was loaded from, and in which
		a copy of the static data resides.
	*/
	v3s16 m_static_block;
	
	/*
		Queue of messages to be sent to the client
	*/
	Queue<ActiveObjectMessage> m_messages_out;
	
protected:
	// Used for creating objects based on type
	typedef ServerActiveObject* (*Factory)
			(ServerEnvironment *env, v3f pos,
			const std::string &data);
	static void registerType(u16 type, Factory f);

	ServerEnvironment *m_env;
	v3f m_base_position;

private:
	// Used for creating objects based on type
	static std::map<u16, Factory> m_types;
};

#endif

