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

#include "localplayer.h"
#include <cmath>
#include "event.h"
#include "collision.h"
#include "nodedef.h"
#include "settings.h"
#include "environment.h"
#include "map.h"
#include "client.h"
#include "content_cao.h"

/*
	LocalPlayer
*/

LocalPlayer::LocalPlayer(Client *client, const char *name):
	Player(name, client->idef()),
	m_client(client)
{
}

v3s16 LocalPlayer::getStandingNodePos()
{
	if(m_sneak_node_exists)
		return m_sneak_node;
	return m_standing_node;
}

v3s16 LocalPlayer::getFootstepNodePos()
{
	if (in_liquid_stable)
		// Emit swimming sound if the player is in liquid
		return floatToInt(getPosition(), BS);
	if (touching_ground)
		// BS * 0.05 below the player's feet ensures a 1/16th height
		// nodebox is detected instead of the node below it.
		return floatToInt(getPosition() - v3f(0, BS * 0.05f, 0), BS);
	// A larger distance below is necessary for a footstep sound
	// when landing after a jump or fall. BS * 0.5 ensures water
	// sounds when swimming in 1 node deep water.
	return floatToInt(getPosition() - v3f(0, BS * 0.5f, 0), BS);
}

v3s16 LocalPlayer::getLightPosition() const
{
	return floatToInt(m_position + v3f(0,BS+BS/2,0), BS);
}

v3f LocalPlayer::getEyeOffset() const
{
	float eye_height = camera_barely_in_ceiling ?
		m_eye_height - 0.125f : m_eye_height;
	return v3f(0, BS * eye_height, 0);
}

bool LocalPlayer::checkPrivilege(const std::string &priv) const
{
	return m_client->checkLocalPrivilege(priv);
}

void LocalPlayer::triggerJumpEvent()
{
	m_client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::PLAYER_JUMP));
}

const NodeDefManager *LocalPlayer::getNodeDefManager() const
{
	return m_client ? m_client->ndef() : NULL;
}

void LocalPlayer::handleAttachedMove()
{
	setPosition(overridePosition);
}

float LocalPlayer::getStepHeight() const
{
	// Player object property step height is multiplied by BS in
	// /src/script/common/c_content.cpp and /src/content_sao.cpp
	if (m_cao == nullptr)
		return 0.0f;
	if (touching_ground)
		return m_cao->getStepHeight();
	return (0.2f * BS);
}

void LocalPlayer::reportRegainGround()
{
	m_client->getEventManager()->put(
			new SimpleTriggerEvent(MtEvent::PLAYER_REGAIN_GROUND));

	// Set camera impact value to be used for view bobbing
	camera_impact = getSpeed().Y * -1;
}

void LocalPlayer::calculateCameraInCeiling(Map *map, const NodeDefManager *ndef)
{
	camera_barely_in_ceiling = false;
	v3s16 camera_np = floatToInt(getEyePosition(), BS);
	MapNode n = map->getNodeNoEx(camera_np);
	if (n.getContent() != CONTENT_IGNORE) {
		if (ndef->get(n).walkable && ndef->get(n).solidness == 2) {
			camera_barely_in_ceiling = true;
		}
	}
}
