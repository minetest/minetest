/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef CONTENT_SAO_ITEM_H_
#define CONTENT_SAO_ITEM_H_

#include "content_sao.h"

class ItemSAO : public ServerActiveObject
{
public:
	ItemSAO(ServerEnvironment *env, v3f pos,
			const std::string inventorystring);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_ITEM;}
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	InventoryItem* createInventoryItem();
	void punch(ServerActiveObject *puncher, float time_from_last_punch);
	float getMinimumSavedMovement(){ return 0.1*BS; }
private:
	std::string m_inventorystring;
	v3f m_speed_f;
	v3f m_last_sent_position;
	IntervalLimiter m_move_interval;
};

#endif /* CONTENT_SAO_ITEM_H_ */
