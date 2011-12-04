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
#include "tooldef.h"
#include "environment.h"
#include "materials.h"

ServerRemotePlayer::ServerRemotePlayer(ServerEnvironment *env):
	Player(env->getGameDef()),
	ServerActiveObject(env, v3f(0,0,0)),
	m_last_good_position(0,0,0),
	m_last_good_position_age(0),
	m_inventory_not_sent(false),
	m_hp_not_sent(false),
	m_respawn_active(false),
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
	clearAddToInventoryLater();
}

void ServerRemotePlayer::setPosition(const v3f &position)
{
	Player::setPosition(position);
	ServerActiveObject::setBasePosition(position);
	m_position_not_sent = true;
}

InventoryItem* ServerRemotePlayer::getWieldedItem()
{
	InventoryList *list = inventory.getList("main");
	if (list)
		return list->getItem(m_selected_item);
	return NULL;
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

	ToolDiggingProperties tp;
	puncher->getWieldDiggingProperties(&tp);

	HittingProperties hitprop = getHittingProperties(&mp, &tp,
			time_from_last_punch);
	
	actionstream<<"Player "<<getName()<<" punched by "
			<<puncher->getDescription()<<", damage "<<hitprop.hp
			<<" HP"<<std::endl;
	
	setHP(getHP() - hitprop.hp);
	puncher->damageWieldedItem(hitprop.wear);
	
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

void ServerRemotePlayer::getWieldDiggingProperties(ToolDiggingProperties *dst)
{
	IGameDef *gamedef = m_env->getGameDef();
	IToolDefManager *tdef = gamedef->tdef();

	InventoryItem *item = getWieldedItem();
	if(item == NULL || std::string(item->getName()) != "ToolItem"){
		*dst = ToolDiggingProperties();
		return;
	}
	ToolItem *titem = (ToolItem*)item;
	*dst = tdef->getDiggingProperties(titem->getToolName());
}

void ServerRemotePlayer::damageWieldedItem(u16 amount)
{
	infostream<<"Damaging "<<getName()<<"'s wielded item for amount="
			<<amount<<std::endl;
	InventoryList *list = inventory.getList("main");
	if(!list)
		return;
	InventoryItem *item = list->getItem(m_selected_item);
	if(item && (std::string)item->getName() == "ToolItem"){
		ToolItem *titem = (ToolItem*)item;
		bool weared_out = titem->addWear(amount);
		if(weared_out)
			list->deleteItem(m_selected_item);
	}
}
bool ServerRemotePlayer::addToInventory(InventoryItem *item)
{
	infostream<<"Adding "<<item->getName()<<" into "<<getName()
			<<"'s inventory"<<std::endl;
	
	InventoryList *ilist = inventory.getList("main");
	if(ilist == NULL)
		return false;
	
	// In creative mode, just delete the item
	if(g_settings->getBool("creative_mode")){
		return false;
	}

	// Skip if inventory has no free space
	if(ilist->roomForItem(item) == false)
	{
		infostream<<"Player inventory has no free space"<<std::endl;
		return false;
	}

	// Add to inventory
	InventoryItem *leftover = ilist->addItem(item);
	assert(!leftover);
	
	m_inventory_not_sent = true;

	return true;
}
void ServerRemotePlayer::addToInventoryLater(InventoryItem *item)
{
	infostream<<"Adding (later) "<<item->getName()<<" into "<<getName()
			<<"'s inventory"<<std::endl;
	m_additional_items.push_back(item);
}
void ServerRemotePlayer::clearAddToInventoryLater()
{
	for (std::vector<InventoryItem*>::iterator
			i = m_additional_items.begin();
			i != m_additional_items.end(); i++)
	{
		delete *i;
	}
	m_additional_items.clear();
}
void ServerRemotePlayer::completeAddToInventoryLater(u16 preferred_index)
{
	InventoryList *ilist = inventory.getList("main");
	if(ilist == NULL)
	{
		clearAddToInventoryLater();
		return;
	}
	
	// In creative mode, just delete the items
	if(g_settings->getBool("creative_mode"))
	{
		clearAddToInventoryLater();
		return;
	}
	
	for (std::vector<InventoryItem*>::iterator
			i = m_additional_items.begin();
			i != m_additional_items.end(); i++)
	{
		InventoryItem *item = *i;
		InventoryItem *leftover = item;
		leftover = ilist->addItem(preferred_index, leftover);
		leftover = ilist->addItem(leftover);
		delete leftover;
	}
	m_additional_items.clear();
	m_inventory_not_sent = true;
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
}
s16 ServerRemotePlayer::getHP()
{
	return hp;
}


