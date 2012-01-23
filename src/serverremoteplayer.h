/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SERVERREMOTEPLAYER_HEADER
#define SERVERREMOTEPLAYER_HEADER

#include "player.h"
#include "serverobject.h"
#include "content_object.h" // Object type IDs

/*
	Player on the server
*/

class ServerRemotePlayer : public Player, public ServerActiveObject
{
public:
	ServerRemotePlayer(ServerEnvironment *env);
	ServerRemotePlayer(ServerEnvironment *env, v3f pos_, u16 peer_id_,
			const char *name_);

	virtual ~ServerRemotePlayer();

	virtual bool isLocal() const
	{ return false; }

	virtual void move(f32 dtime, Map &map, f32 pos_max_d)
	{
	}
	
	virtual void setPosition(const v3f &position);
	
	/* ServerActiveObject interface */

	u8 getType() const
	{return ACTIVEOBJECT_TYPE_PLAYER;}
	
	// Called after id has been set and has been inserted in environment
	void addedToEnvironment();
	// Called before removing from environment
	void removingFromEnvironment();
	
	bool environmentDeletes() const
	{ return false; }

	virtual bool unlimitedTransferDistance() const;
	
	bool isStaticAllowed() const
	{ return false; }

	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	void punch(ServerActiveObject *puncher, float time_from_last_punch);
	void rightClick(ServerActiveObject *clicker);
	void setPos(v3f pos);
	void moveTo(v3f pos, bool continuous);
	virtual std::string getDescription()
	{return std::string("player ")+getName();}

	virtual Inventory* getInventory();
	virtual const Inventory* getInventory() const;
	virtual InventoryLocation getInventoryLocation() const;
	virtual void setInventoryModified();
	virtual std::string getWieldList() const;
	virtual int getWieldIndex() const;
	virtual void setWieldIndex(int i);

	virtual void setHP(s16 hp_);
	virtual s16 getHP();
	
	v3f m_last_good_position;
	float m_last_good_position_age;
	int m_wield_index;
	bool m_inventory_not_sent;
	bool m_hp_not_sent;
	bool m_is_in_environment;
	// Incremented by step(), read and reset by Server
	float m_time_from_last_punch;

private:
	bool m_position_not_sent;
};

#endif

