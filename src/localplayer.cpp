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

void LocalPlayer::applyControl(float dtime, Environment *env)
{
	ControlLogEntry cle;
	cle.setDtime(dtime);

	cle.setFreeMove(g_settings->getBool("free_move"));
	cle.setFastMove(g_settings->getBool("fast_move"));
	cle.setContinuousForward(g_settings->getBool("continuous_forward"));
	cle.setAlwaysFlyFast(g_settings->getBool("always_fly_fast"));
	cle.setAux1Descends(g_settings->getBool("aux1_descends"));

	cle.setUp(control.up);
	cle.setDown(control.down);
	cle.setLeft(control.left);
	cle.setRight(control.right);

	cle.setJump(control.jump);
	cle.setSneak(control.sneak);
	cle.setAux1(control.aux1);

	cle.setPitch(control.pitch);
	cle.setYaw(control.yaw);

	cle.setJoyForw(control.forw_move_joystick_axis / 32767.f);
	cle.setJoySidew(control.sidew_move_joystick_axis / 32767.f);

	applyControlLogEntry(cle, env);

	// append entry to actual log
	m_control_log.add(cle);
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

bool LocalPlayer::isAttached() const
{
	return is_attached;
}

void LocalPlayer::triggerJumpEvent()
{
	MtEvent *e = new SimpleTriggerEvent("PlayerJump");
	m_client->event()->put(e);
}

const NodeDefManager *LocalPlayer::getNodeDefManager() const
{
	return m_client ? m_client->ndef() : NULL;
}

void LocalPlayer::_handleAttachedMove()
{
	setPosition(overridePosition);
}

float LocalPlayer::_getStepHeight() const
{
	// Player object property step height is multiplied by BS in
	// /src/script/common/c_content.cpp and /src/content_sao.cpp
	return (m_cao == nullptr) ? 0.0f :
		(touching_ground ? m_cao->getStepHeight() : (0.2f * BS));
}

void LocalPlayer::reportRegainGround()
{
	MtEvent *e = new SimpleTriggerEvent("PlayerRegainGround");
	m_client->event()->put(e);

	// Set camera impact value to be used for view bobbing
	camera_impact = getSpeed().Y * -1;
}

void LocalPlayer::calculateCameraInCeiling(Map *map, const NodeDefManager *nodemgr)
{
	camera_barely_in_ceiling = false;
	v3s16 camera_np = floatToInt(getEyePosition(), BS);
	MapNode n = map->getNodeNoEx(camera_np);
	if(n.getContent() != CONTENT_IGNORE){
		if(nodemgr->get(n).walkable && nodemgr->get(n).solidness == 2){
			camera_barely_in_ceiling = true;
		}
	}
}

void LocalPlayer::debugVec( const std::string &title, const v3f &v, const std::string &unused) const {
	Player::debugVec(title, v, "LOCAL: ");
}
void LocalPlayer::debugStr(const std::string &str, bool newline, const std::string &unused) const {
	Player::debugStr(str, newline, "LOCAL: ");
}
void LocalPlayer::debugFloat(const std::string &title, const float value, bool newline, const std::string &unused) const {
	Player::debugFloat(title, value, newline, "LOCAL: ");
}
