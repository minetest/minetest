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

#include "util/serialize.h"
#include "nodedef.h"
#include "remoteplayer.h"
#include "server.h"
#include "scripting_server.h"
#include "genericobject.h"
#include "server/object/LagPool.h"
#include "server/object/PlayerHPChangeReason.h"
#include "server/object/PlayerSAO.h"

PlayerSAO::PlayerSAO(ServerEnvironment *env_, RemotePlayer *player_, session_t peer_id_,
		bool is_singleplayer):
	UnitSAO(env_, v3f(0,0,0)),
	m_player(player_),
	m_peer_id(peer_id_),
	m_is_singleplayer(is_singleplayer)
{
	assert(m_peer_id != 0);	// pre-condition

	m_prop.hp_max = PLAYER_MAX_HP_DEFAULT;
	m_prop.breath_max = PLAYER_MAX_BREATH_DEFAULT;
	m_prop.physical = false;
	m_prop.weight = 75;
	m_prop.collisionbox = aabb3f(-0.3f, 0.0f, -0.3f, 0.3f, 1.77f, 0.3f);
	m_prop.selectionbox = aabb3f(-0.3f, 0.0f, -0.3f, 0.3f, 1.77f, 0.3f);
	m_prop.pointable = true;
	// Start of default appearance, this should be overwritten by Lua
	m_prop.visual = "upright_sprite";
	m_prop.visual_size = v2f(1, 2);
	m_prop.textures.clear();
	m_prop.textures.emplace_back("player.png");
	m_prop.textures.emplace_back("player_back.png");
	m_prop.colors.clear();
	m_prop.colors.emplace_back(255, 255, 255, 255);
	m_prop.spritediv = v2s16(1,1);
	m_prop.eye_height = 1.625f;
	// End of default appearance
	m_prop.is_visible = true;
	m_prop.backface_culling = false;
	m_prop.makes_footstep_sound = true;
	m_prop.stepheight = PLAYER_DEFAULT_STEPHEIGHT * BS;
	m_hp = m_prop.hp_max;
	m_breath = m_prop.breath_max;
	// Disable zoom in survival mode using a value of 0
	m_prop.zoom_fov = g_settings->getBool("creative_mode") ? 15.0f : 0.0f;
}

PlayerSAO::~PlayerSAO()
{
	if(m_inventory != &m_player->inventory)
		delete m_inventory;
}

void PlayerSAO::finalize(RemotePlayer *player, const std::set<std::string> &privs)
{
	assert(player);
	m_player = player;
	m_privs = privs;
	m_inventory = &m_player->inventory;
}

v3f PlayerSAO::getEyeOffset() const
{
	return v3f(0, BS * m_prop.eye_height, 0);
}

std::string PlayerSAO::getDescription()
{
	return std::string("player ") + m_player->getName();
}

// Called after id has been set and has been inserted in environment
void PlayerSAO::addedToEnvironment(u32 dtime_s)
{
	ServerActiveObject::addedToEnvironment(dtime_s);
	ServerActiveObject::setBasePosition(m_base_position);
	m_player->setPlayerSAO(this);
	m_player->setPeerId(m_peer_id);
	m_last_good_position = m_base_position;
}

// Called before removing from environment
void PlayerSAO::removingFromEnvironment()
{
	ServerActiveObject::removingFromEnvironment();
	if (m_player->getPlayerSAO() == this) {
		unlinkPlayerSessionAndSave();
		for (u32 attached_particle_spawner : m_attached_particle_spawners) {
			m_env->deleteParticleSpawner(attached_particle_spawner, false);
		}
	}
}

std::string PlayerSAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);

	// Protocol >= 15
	writeU8(os, 1); // version
	os << serializeString(m_player->getName()); // name
	writeU8(os, 1); // is_player
	writeS16(os, getId()); // id
	writeV3F1000(os, m_base_position);
	writeV3F1000(os, m_rotation);
	writeS16(os, getHP());

	std::ostringstream msg_os(std::ios::binary);
	msg_os << serializeLongString(getPropertyPacket()); // message 1
	msg_os << serializeLongString(gob_cmd_update_armor_groups(m_armor_groups)); // 2
	msg_os << serializeLongString(gob_cmd_update_animation(
		m_animation_range, m_animation_speed, m_animation_blend, m_animation_loop)); // 3
	for (std::unordered_map<std::string, core::vector2d<v3f>>::const_iterator
			ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii) {
		msg_os << serializeLongString(gob_cmd_update_bone_position((*ii).first,
			(*ii).second.X, (*ii).second.Y)); // m_bone_position.size
	}
	msg_os << serializeLongString(gob_cmd_update_attachment(m_attachment_parent_id,
		m_attachment_bone, m_attachment_position, m_attachment_rotation)); // 4
	msg_os << serializeLongString(gob_cmd_update_physics_override(m_physics_override_speed,
			m_physics_override_jump, m_physics_override_gravity, m_physics_override_sneak,
			m_physics_override_sneak_glitch, m_physics_override_new_move)); // 5
	// (GENERIC_CMD_UPDATE_NAMETAG_ATTRIBUTES) : Deprecated, for backwards compatibility only.
	msg_os << serializeLongString(gob_cmd_update_nametag_attributes(m_prop.nametag_color)); // 6
	int message_count = 6 + m_bone_position.size();
	for (std::unordered_set<int>::const_iterator ii = m_attachment_child_ids.begin();
			ii != m_attachment_child_ids.end(); ++ii) {
		if (ServerActiveObject *obj = m_env->getActiveObject(*ii)) {
			message_count++;
			msg_os << serializeLongString(gob_cmd_update_infant(*ii, obj->getSendType(),
				obj->getClientInitializationData(protocol_version)));
		}
	}

	writeU8(os, message_count);
	os.write(msg_os.str().c_str(), msg_os.str().size());

	// return result
	return os.str();
}

void PlayerSAO::getStaticData(std::string * result) const
{
	FATAL_ERROR("Deprecated function");
}

void PlayerSAO::step(float dtime, bool send_recommended)
{
	if (m_drowning_interval.step(dtime, 2.0f)) {
		// Get nose/mouth position, approximate with eye position
		v3s16 p = floatToInt(getEyePosition(), BS);
		MapNode n = m_env->getMap().getNodeNoEx(p);
		const ContentFeatures &c = m_env->getGameDef()->ndef()->get(n);
		// If node generates drown
		if (c.drowning > 0 && m_hp > 0) {
			if (m_breath > 0)
				setBreath(m_breath - 1);

			// No more breath, damage player
			if (m_breath == 0) {
				PlayerHPChangeReason reason(PlayerHPChangeReason::DROWNING);
				setHP(m_hp - c.drowning, reason);
				m_env->getGameDef()->SendPlayerHPOrDie(this, reason);
			}
		}
	}

	if (m_breathing_interval.step(dtime, 0.5f)) {
		// Get nose/mouth position, approximate with eye position
		v3s16 p = floatToInt(getEyePosition(), BS);
		MapNode n = m_env->getMap().getNodeNoEx(p);
		const ContentFeatures &c = m_env->getGameDef()->ndef()->get(n);
		// If player is alive & no drowning, breathe
		if (m_hp > 0 && m_breath < m_prop.breath_max && c.drowning == 0)
			setBreath(m_breath + 1);
	}

	if (m_node_hurt_interval.step(dtime, 1.0f)) {
		u32 damage_per_second = 0;
		// Lowest and highest damage points are 0.1 within collisionbox
		float dam_top = m_prop.collisionbox.MaxEdge.Y - 0.1f;

		// Sequence of damage points, starting 0.1 above feet and progressing
		// upwards in 1 node intervals, stopping below top damage point.
		for (float dam_height = 0.1f; dam_height < dam_top; dam_height++) {
			v3s16 p = floatToInt(m_base_position +
				v3f(0.0f, dam_height * BS, 0.0f), BS);
			MapNode n = m_env->getMap().getNodeNoEx(p);
			damage_per_second = std::max(damage_per_second,
				m_env->getGameDef()->ndef()->get(n).damage_per_second);
		}

		// Top damage point
		v3s16 ptop = floatToInt(m_base_position +
			v3f(0.0f, dam_top * BS, 0.0f), BS);
		MapNode ntop = m_env->getMap().getNodeNoEx(ptop);
		damage_per_second = std::max(damage_per_second,
			m_env->getGameDef()->ndef()->get(ntop).damage_per_second);

		if (damage_per_second != 0 && m_hp > 0) {
			s16 newhp = ((s32) damage_per_second > m_hp ? 0 : m_hp - damage_per_second);
			PlayerHPChangeReason reason(PlayerHPChangeReason::NODE_DAMAGE);
			setHP(newhp, reason);
			m_env->getGameDef()->SendPlayerHPOrDie(this, reason);
		}
	}

	if (!m_properties_sent) {
		m_properties_sent = true;
		std::string str = getPropertyPacket();
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	// If attached, check that our parent is still there. If it isn't, detach.
	if (m_attachment_parent_id && !isAttached()) {
		m_attachment_parent_id = 0;
		m_attachment_bone = "";
		m_attachment_position = v3f(0.0f, 0.0f, 0.0f);
		m_attachment_rotation = v3f(0.0f, 0.0f, 0.0f);
		setBasePosition(m_last_good_position);
		m_env->getGameDef()->SendMovePlayer(m_peer_id);
	}

	//dstream<<"PlayerSAO::step: dtime: "<<dtime<<std::endl;

	// Set lag pool maximums based on estimated lag
	const float LAG_POOL_MIN = 5.0f;
	float lag_pool_max = m_env->getMaxLagEstimate() * 2.0f;
	if(lag_pool_max < LAG_POOL_MIN)
		lag_pool_max = LAG_POOL_MIN;
	m_dig_pool.setMax(lag_pool_max);
	m_move_pool.setMax(lag_pool_max);

	// Increment cheat prevention timers
	m_dig_pool.add(dtime);
	m_move_pool.add(dtime);
	m_time_from_last_teleport += dtime;
	m_time_from_last_punch += dtime;
	m_nocheat_dig_time += dtime;

	// Each frame, parent position is copied if the object is attached,
	// otherwise it's calculated normally.
	// If the object gets detached this comes into effect automatically from
	// the last known origin.
	if (isAttached()) {
		v3f pos = m_env->getActiveObject(m_attachment_parent_id)->getBasePosition();
		m_last_good_position = pos;
		setBasePosition(pos);
	}

	if (!send_recommended)
		return;

	// If the object is attached client-side, don't waste bandwidth sending its
	// position or rotation to clients.
	if (m_position_not_sent && !isAttached()) {
		m_position_not_sent = false;
		float update_interval = m_env->getSendRecommendedInterval();
		v3f pos;
		if (isAttached()) // Just in case we ever do send attachment position too
			pos = m_env->getActiveObject(m_attachment_parent_id)->getBasePosition();
		else
			pos = m_base_position;

		std::string str = gob_cmd_update_position(
			pos,
			v3f(0.0f, 0.0f, 0.0f),
			v3f(0.0f, 0.0f, 0.0f),
			v3f(0.0f, 0.0f, 0.0f),
			true,
			false,
			update_interval
		);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, str);
		m_messages_out.push(aom);
	}

	if (!m_armor_groups_sent) {
		m_armor_groups_sent = true;
		std::string str = gob_cmd_update_armor_groups(
				m_armor_groups);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	if (!m_physics_override_sent) {
		m_physics_override_sent = true;
		std::string str = gob_cmd_update_physics_override(m_physics_override_speed,
				m_physics_override_jump, m_physics_override_gravity,
				m_physics_override_sneak, m_physics_override_sneak_glitch,
				m_physics_override_new_move);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	if (!m_animation_sent) {
		m_animation_sent = true;
		std::string str = gob_cmd_update_animation(
			m_animation_range, m_animation_speed, m_animation_blend, m_animation_loop);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	if (!m_bone_position_sent) {
		m_bone_position_sent = true;
		for (std::unordered_map<std::string, core::vector2d<v3f>>::const_iterator
				ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii) {
			std::string str = gob_cmd_update_bone_position((*ii).first,
					(*ii).second.X, (*ii).second.Y);
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push(aom);
		}
	}

	if (!m_attachment_sent) {
		m_attachment_sent = true;
		std::string str = gob_cmd_update_attachment(m_attachment_parent_id,
				m_attachment_bone, m_attachment_position, m_attachment_rotation);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}
}

void PlayerSAO::setBasePosition(const v3f &position)
{
	if (m_player && position != m_base_position)
		m_player->setDirty(true);

	// This needs to be ran for attachments too
	ServerActiveObject::setBasePosition(position);

	// Updating is not wanted/required for player migration
	if (m_env) {
		m_position_not_sent = true;
	}
}

void PlayerSAO::setPos(const v3f &pos)
{
	if(isAttached())
		return;

	setBasePosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	m_move_pool.empty();
	m_time_from_last_teleport = 0.0;
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::moveTo(v3f pos, bool continuous)
{
	if(isAttached())
		return;

	setBasePosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	m_move_pool.empty();
	m_time_from_last_teleport = 0.0;
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::setPlayerYaw(const float yaw)
{
	v3f rotation(0, yaw, 0);
	if (m_player && yaw != m_rotation.Y)
		m_player->setDirty(true);

	// Set player model yaw, not look view
	UnitSAO::setRotation(rotation);
}

void PlayerSAO::setFov(const float fov)
{
	if (m_player && fov != m_fov)
		m_player->setDirty(true);

	m_fov = fov;
}

void PlayerSAO::setWantedRange(const s16 range)
{
	if (m_player && range != m_wanted_range)
		m_player->setDirty(true);

	m_wanted_range = range;
}

void PlayerSAO::setPlayerYawAndSend(const float yaw)
{
	setPlayerYaw(yaw);
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::setLookPitch(const float pitch)
{
	if (m_player && pitch != m_pitch)
		m_player->setDirty(true);

	m_pitch = pitch;
}

void PlayerSAO::setLookPitchAndSend(const float pitch)
{
	setLookPitch(pitch);
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

int PlayerSAO::punch(v3f dir,
	const ToolCapabilities *toolcap,
	ServerActiveObject *puncher,
	float time_from_last_punch)
{
	if (!toolcap)
		return 0;

	// No effect if PvP disabled
	if (!g_settings->getBool("enable_pvp")) {
		if (puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			std::string str = gob_cmd_punched(0, getHP());
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push(aom);
			return 0;
		}
	}

	HitParams hitparams = getHitParams(m_armor_groups, toolcap,
			time_from_last_punch);

	std::string punchername = "nil";

	if (puncher != 0)
		punchername = puncher->getDescription();

	PlayerSAO *playersao = m_player->getPlayerSAO();

	bool damage_handled = m_env->getScriptIface()->on_punchplayer(playersao,
				puncher, time_from_last_punch, toolcap, dir,
				hitparams.hp);

	if (!damage_handled) {
		setHP(getHP() - hitparams.hp,
				PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, puncher));
	} else { // override client prediction
		if (puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			std::string str = gob_cmd_punched(0, getHP());
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push(aom);
		}
	}


	actionstream << "Player " << m_player->getName() << " punched by "
			<< punchername;
	if (!damage_handled) {
		actionstream << ", damage " << hitparams.hp << " HP";
	} else {
		actionstream << ", damage handled by lua";
	}
	actionstream << std::endl;

	return hitparams.wear;
}

s16 PlayerSAO::readDamage()
{
	s16 damage = m_damage;
	m_damage = 0;
	return damage;
}

void PlayerSAO::setHP(s16 hp, const PlayerHPChangeReason &reason)
{
	s16 oldhp = m_hp;

	s16 hp_change = m_env->getScriptIface()->on_player_hpchange(this, hp - oldhp, reason);
	if (hp_change == 0)
		return;
	hp = oldhp + hp_change;

	if (hp < 0)
		hp = 0;
	else if (hp > m_prop.hp_max)
		hp = m_prop.hp_max;

	if (hp < oldhp && !g_settings->getBool("enable_damage")) {
		return;
	}

	m_hp = hp;

	if (oldhp > hp)
		m_damage += (oldhp - hp);

	// Update properties on death
	if ((hp == 0) != (oldhp == 0))
		m_properties_sent = false;
}

void PlayerSAO::setBreath(const u16 breath, bool send)
{
	if (m_player && breath != m_breath)
		m_player->setDirty(true);

	m_breath = MYMIN(breath, m_prop.breath_max);

	if (send)
		m_env->getGameDef()->SendPlayerBreath(this);
}

Inventory* PlayerSAO::getInventory()
{
	return m_inventory;
}
const Inventory* PlayerSAO::getInventory() const
{
	return m_inventory;
}

InventoryLocation PlayerSAO::getInventoryLocation() const
{
	InventoryLocation loc;
	loc.setPlayer(m_player->getName());
	return loc;
}

std::string PlayerSAO::getWieldList() const
{
	return "main";
}

ItemStack PlayerSAO::getWieldedItem() const
{
	const Inventory *inv = getInventory();
	ItemStack ret;
	const InventoryList *mlist = inv->getList(getWieldList());
	if (mlist && getWieldIndex() < (s32)mlist->getSize())
		ret = mlist->getItem(getWieldIndex());
	return ret;
}

ItemStack PlayerSAO::getWieldedItemOrHand() const
{
	const Inventory *inv = getInventory();
	ItemStack ret;
	const InventoryList *mlist = inv->getList(getWieldList());
	if (mlist && getWieldIndex() < (s32)mlist->getSize())
		ret = mlist->getItem(getWieldIndex());
	if (ret.name.empty()) {
		const InventoryList *hlist = inv->getList("hand");
		if (hlist)
			ret = hlist->getItem(0);
	}
	return ret;
}

bool PlayerSAO::setWieldedItem(const ItemStack &item)
{
	Inventory *inv = getInventory();
	if (inv) {
		InventoryList *mlist = inv->getList(getWieldList());
		if (mlist) {
			mlist->changeItem(getWieldIndex(), item);
			return true;
		}
	}
	return false;
}

int PlayerSAO::getWieldIndex() const
{
	return m_wield_index;
}

void PlayerSAO::setWieldIndex(int i)
{
	if(i != m_wield_index) {
		m_wield_index = i;
	}
}

void PlayerSAO::disconnected()
{
	m_peer_id = 0;
	m_pending_removal = true;
}

void PlayerSAO::unlinkPlayerSessionAndSave()
{
	assert(m_player->getPlayerSAO() == this);
	m_player->setPeerId(PEER_ID_INEXISTENT);
	m_env->savePlayer(m_player);
	m_player->setPlayerSAO(NULL);
	m_env->removePlayer(m_player);
}

std::string PlayerSAO::getPropertyPacket()
{
	m_prop.is_visible = (true);
	return gob_cmd_set_properties(m_prop);
}

bool PlayerSAO::checkMovementCheat()
{
	if (isAttached() || m_is_singleplayer ||
			g_settings->getBool("disable_anticheat")) {
		m_last_good_position = m_base_position;
		return false;
	}

	bool cheated = false;
	/*
		Check player movements

		NOTE: Actually the server should handle player physics like the
		client does and compare player's position to what is calculated
		on our side. This is required when eg. players fly due to an
		explosion. Altough a node-based alternative might be possible
		too, and much more lightweight.
	*/

	float player_max_walk = 0; // horizontal movement
	float player_max_jump = 0; // vertical upwards movement

	if (m_privs.count("fast") != 0)
		player_max_walk = m_player->movement_speed_fast; // Fast speed
	else
		player_max_walk = m_player->movement_speed_walk; // Normal speed
	player_max_walk *= m_physics_override_speed;
	player_max_jump = m_player->movement_speed_jump * m_physics_override_jump;
	// FIXME: Bouncy nodes cause practically unbound increase in Y speed,
	//        until this can be verified correctly, tolerate higher jumping speeds
	player_max_jump *= 2.0;

	// Don't divide by zero!
	if (player_max_walk < 0.0001f)
		player_max_walk = 0.0001f;
	if (player_max_jump < 0.0001f)
		player_max_jump = 0.0001f;

	v3f diff = (m_base_position - m_last_good_position);
	float d_vert = diff.Y;
	diff.Y = 0;
	float d_horiz = diff.getLength();
	float required_time = d_horiz / player_max_walk;

	// FIXME: Checking downwards movement is not easily possible currently,
	//        the server could calculate speed differences to examine the gravity
	if (d_vert > 0) {
		// In certain cases (water, ladders) walking speed is applied vertically
		float s = MYMAX(player_max_jump, player_max_walk);
		required_time = MYMAX(required_time, d_vert / s);
	}

	if (m_move_pool.grab(required_time)) {
		m_last_good_position = m_base_position;
	} else {
		const float LAG_POOL_MIN = 5.0;
		float lag_pool_max = m_env->getMaxLagEstimate() * 2.0;
		lag_pool_max = MYMAX(lag_pool_max, LAG_POOL_MIN);
		if (m_time_from_last_teleport > lag_pool_max) {
			actionstream << "Player " << m_player->getName()
					<< " moved too fast; resetting position"
					<< std::endl;
			cheated = true;
		}
		setBasePosition(m_last_good_position);
	}
	return cheated;
}

bool PlayerSAO::getCollisionBox(aabb3f *toset) const
{
	//update collision box
	toset->MinEdge = m_prop.collisionbox.MinEdge * BS;
	toset->MaxEdge = m_prop.collisionbox.MaxEdge * BS;

	toset->MinEdge += m_base_position;
	toset->MaxEdge += m_base_position;
	return true;
}

bool PlayerSAO::getSelectionBox(aabb3f *toset) const
{
	if (!m_prop.is_visible || !m_prop.pointable) {
		return false;
	}

	toset->MinEdge = m_prop.selectionbox.MinEdge * BS;
	toset->MaxEdge = m_prop.selectionbox.MaxEdge * BS;

	return true;
}

float PlayerSAO::getZoomFOV() const
{
	return m_prop.zoom_fov;
}

