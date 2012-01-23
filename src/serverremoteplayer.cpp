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

#include "serverremoteplayer.h"
#include "main.h" // For g_settings
#include "settings.h"
#include "log.h"
#include "gamedef.h"
#include "inventory.h"
#include "environment.h"
#include "materials.h"

ServerRemotePlayer::ServerRemotePlayer(ServerEnvironment *env):
	Player(env->getGameDef()),
	ServerActiveObject(env, v3f(0,0,0)),
	m_last_good_position(0,0,0),
	m_last_good_position_age(0),
	m_wield_index(0),
	m_inventory_not_sent(false),
	m_hp_not_sent(false),
	m_is_in_environment(false),
	m_time_from_last_punch(0),
	m_position_not_sent(false)
{
}
ServerRemotePlayer::ServerRemotePlayer(ServerEnvironment *env, v3f pos_, u16 peer_id_,
		const char *name_):
	Player(env->getGameDef()),
	ServerActiveObject(env, pos_),
	m_last_good_position(0,0,0),
	m_last_good_position_age(0),
	m_wield_index(0),
	m_inventory_not_sent(false),
	m_hp_not_sent(false),
	m_is_in_environment(false),
	m_time_from_last_punch(0),
	m_position_not_sent(false)
{
	setPosition(pos_);
	peer_id = peer_id_;
	updateName(name_);
}
ServerRemotePlayer::~ServerRemotePlayer()
{
}

void ServerRemotePlayer::setPosition(const v3f &position)
{
	Player::setPosition(position);
	ServerActiveObject::setBasePosition(position);
	m_position_not_sent = true;
}

Inventory* ServerRemotePlayer::getInventory()
{
	return &inventory;
}

const Inventory* ServerRemotePlayer::getInventory() const
{
	return &inventory;
}

InventoryLocation ServerRemotePlayer::getInventoryLocation() const
{
	InventoryLocation loc;
	loc.setPlayer(getName());
	return loc;
}

void ServerRemotePlayer::setInventoryModified()
{
	m_inventory_not_sent = true;
}

std::string ServerRemotePlayer::getWieldList() const
{
	return "main";
}

int ServerRemotePlayer::getWieldIndex() const
{
	return m_wield_index;
}

void ServerRemotePlayer::setWieldIndex(int i)
{
	m_wield_index = i;
}

/* ServerActiveObject interface */

void ServerRemotePlayer::addedToEnvironment()
{
	assert(!m_is_in_environment);
	m_is_in_environment = true;
}

void ServerRemotePlayer::removingFromEnvironment()
{
	assert(m_is_in_environment);
	m_is_in_environment = false;
}

bool ServerRemotePlayer::unlimitedTransferDistance() const
{
	return g_settings->getBool("unlimited_player_transfer_distance");
}

void ServerRemotePlayer::step(float dtime, bool send_recommended)
{
	m_time_from_last_punch += dtime;
	
	if(send_recommended == false)
		return;
	
	if(m_position_not_sent)
	{
		m_position_not_sent = false;

		std::ostringstream os(std::ios::binary);
		// command (0 = update position)
		writeU8(os, 0);
		// pos
		writeV3F1000(os, getPosition());
		// yaw
		writeF1000(os, getYaw());
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

std::string ServerRemotePlayer::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// name
	os<<serializeString(getName());
	// pos
	writeV3F1000(os, getPosition());
	// yaw
	writeF1000(os, getYaw());
	// dead
	writeU8(os, getHP() == 0);
	return os.str();
}

std::string ServerRemotePlayer::getStaticData()
{
	assert(0);
	return "";
}

void ServerRemotePlayer::punch(ServerActiveObject *puncher,
		float time_from_last_punch)
{
	if(!puncher)
		return;
	
	// No effect if PvP disabled
	if(g_settings->getBool("enable_pvp") == false){
		if(puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER)
			return;
	}
	
	// "Material" properties of a player
	MaterialProperties mp;
	mp.diggability = DIGGABLE_NORMAL;
	mp.crackiness = -0.5;
	mp.cuttability = 0.5;

	IItemDefManager *idef = m_env->getGameDef()->idef();
	ItemStack punchitem = puncher->getWieldedItem();
	ToolDiggingProperties tp =
		punchitem.getToolDiggingProperties(idef);

	HittingProperties hitprop = getHittingProperties(&mp, &tp,
			time_from_last_punch);
	
	actionstream<<"Player "<<getName()<<" punched by "
			<<puncher->getDescription()<<", damage "<<hitprop.hp
			<<" HP"<<std::endl;
	
	setHP(getHP() - hitprop.hp);
	punchitem.addWear(hitprop.wear, idef);
	puncher->setWieldedItem(punchitem);
	
	if(hitprop.hp != 0)
	{
		std::ostringstream os(std::ios::binary);
		// command (1 = punched)
		writeU8(os, 1);
		// damage
		writeS16(os, hitprop.hp);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

void ServerRemotePlayer::rightClick(ServerActiveObject *clicker)
{
}

void ServerRemotePlayer::setPos(v3f pos)
{
	setPosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	m_last_good_position_age = 0;
}
void ServerRemotePlayer::moveTo(v3f pos, bool continuous)
{
	setPosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	m_last_good_position_age = 0;
}

void ServerRemotePlayer::setHP(s16 hp_)
{
	s16 oldhp = hp;

	// FIXME: don't hardcode maximum HP, make configurable per object
	if(hp_ < 0)
		hp_ = 0;
	else if(hp_ > 20)
		hp_ = 20;
	hp = hp_;

	if(hp != oldhp)
		m_hp_not_sent = true;

	// On death or reincarnation send an active object message
	if((hp == 0) != (oldhp == 0))
	{
		std::ostringstream os(std::ios::binary);
		// command (2 = update death state)
		writeU8(os, 2);
		// dead?
		writeU8(os, hp == 0);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}
s16 ServerRemotePlayer::getHP()
{
	return hp;
}


