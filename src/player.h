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

#ifndef PLAYER_HEADER
#define PLAYER_HEADER

#include "irrlichttypes_bloated.h"
#include "inventory.h"
#include "constants.h" // BS
#include "threading/mutex.h"
#include <list>

#define PLAYERNAME_SIZE 20

#define PLAYERNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
#define PLAYERNAME_ALLOWED_CHARS_USER_EXPL "'a' to 'z', 'A' to 'Z', '0' to '9', '-', '_'"

struct PlayerControl
{
	PlayerControl()
	{
		up = false;
		down = false;
		left = false;
		right = false;
		jump = false;
		aux1 = false;
		sneak = false;
		LMB = false;
		RMB = false;
		pitch = 0;
		yaw = 0;
		sidew_move_joystick_axis = .0f;
		forw_move_joystick_axis = .0f;
	}

	PlayerControl(
		bool a_up,
		bool a_down,
		bool a_left,
		bool a_right,
		bool a_jump,
		bool a_aux1,
		bool a_sneak,
		bool a_zoom,
		bool a_LMB,
		bool a_RMB,
		float a_pitch,
		float a_yaw,
		float a_sidew_move_joystick_axis,
		float a_forw_move_joystick_axis
	)
	{
		up = a_up;
		down = a_down;
		left = a_left;
		right = a_right;
		jump = a_jump;
		aux1 = a_aux1;
		sneak = a_sneak;
		zoom = a_zoom;
		LMB = a_LMB;
		RMB = a_RMB;
		pitch = a_pitch;
		yaw = a_yaw;
		sidew_move_joystick_axis = a_sidew_move_joystick_axis;
		forw_move_joystick_axis = a_forw_move_joystick_axis;
	}
	bool up;
	bool down;
	bool left;
	bool right;
	bool jump;
	bool aux1;
	bool sneak;
	bool zoom;
	bool LMB;
	bool RMB;
	float pitch;
	float yaw;
	float sidew_move_joystick_axis;
	float forw_move_joystick_axis;
};

class Map;
class IGameDef;
struct CollisionInfo;
class PlayerSAO;
struct HudElement;
class Environment;

// IMPORTANT:
// Do *not* perform an assignment or copy operation on a Player or
// RemotePlayer object!  This will copy the lock held for HUD synchronization
class Player
{
public:

	Player(const char *name, IItemDefManager *idef);
	virtual ~Player() = 0;

	virtual void move(f32 dtime, Environment *env, f32 pos_max_d)
	{}
	virtual void move(f32 dtime, Environment *env, f32 pos_max_d,
			std::vector<CollisionInfo> *collision_info)
	{}

	v3f getSpeed()
	{
		return m_speed;
	}

	void setSpeed(v3f speed)
	{
		m_speed = speed;
	}

	v3f getPosition()
	{
		return m_position;
	}

	v3s16 getLightPosition() const;

	v3f getEyeOffset()
	{
		float eye_height = camera_barely_in_ceiling ? 1.5f : 1.625f;
		return v3f(0, BS * eye_height, 0);
	}

	v3f getEyePosition()
	{
		return m_position + getEyeOffset();
	}

	virtual void setPosition(const v3f &position)
	{
		m_position = position;
	}

	virtual void setPitch(f32 pitch)
	{
		m_pitch = pitch;
	}

	virtual void setYaw(f32 yaw)
	{
		m_yaw = yaw;
	}

	f32 getPitch() const { return m_pitch; }
	f32 getYaw() const { return m_yaw; }
	u16 getBreath() const { return m_breath; }

	virtual void setBreath(u16 breath) { m_breath = breath; }

	f32 getRadPitch() const { return m_pitch * core::DEGTORAD; }
	f32 getRadYaw() const { return m_yaw * core::DEGTORAD; }
	const char *getName() const { return m_name; }
	aabb3f getCollisionbox() const { return m_collisionbox; }

	u32 getFreeHudID()
	{
		size_t size = hud.size();
		for (size_t i = 0; i != size; i++) {
			if (!hud[i])
				return i;
		}
		return size;
	}

	void setLocalAnimations(v2s32 frames[4], float frame_speed)
	{
		for (int i = 0; i < 4; i++)
			local_animations[i] = frames[i];
		local_animation_speed = frame_speed;
	}

	void getLocalAnimations(v2s32 *frames, float *frame_speed)
	{
		for (int i = 0; i < 4; i++)
			frames[i] = local_animations[i];
		*frame_speed = local_animation_speed;
	}

	virtual bool isLocal() const { return false; }

	bool camera_barely_in_ceiling;
	v3f eye_offset_first;
	v3f eye_offset_third;

	Inventory inventory;

	f32 movement_acceleration_default;
	f32 movement_acceleration_air;
	f32 movement_acceleration_fast;
	f32 movement_speed_walk;
	f32 movement_speed_crouch;
	f32 movement_speed_fast;
	f32 movement_speed_climb;
	f32 movement_speed_jump;
	f32 movement_liquid_fluidity;
	f32 movement_liquid_fluidity_smooth;
	f32 movement_liquid_sink;
	f32 movement_gravity;

	v2s32 local_animations[4];
	float local_animation_speed;

	u16 hp;

	u16 peer_id;

	std::string inventory_formspec;

	PlayerControl control;
	const PlayerControl& getPlayerControl() { return control; }

	u32 keyPressed;

	HudElement* getHud(u32 id);
	u32         addHud(HudElement* hud);
	HudElement* removeHud(u32 id);
	void        clearHud();

	u32 hud_flags;
	s32 hud_hotbar_itemcount;
protected:
	char m_name[PLAYERNAME_SIZE];
	u16 m_breath;
	f32 m_pitch;
	f32 m_yaw;
	v3f m_speed;
	v3f m_position;
	aabb3f m_collisionbox;

	std::vector<HudElement *> hud;
private:
	// Protect some critical areas
	// hud for example can be modified by EmergeThread
	// and ServerThread
	Mutex m_mutex;
};

enum RemotePlayerChatResult {
	RPLAYER_CHATRESULT_OK,
	RPLAYER_CHATRESULT_FLOODING,
	RPLAYER_CHATRESULT_KICK,
};
/*
	Player on the server
*/
class RemotePlayer : public Player
{
public:
	RemotePlayer(const char *name, IItemDefManager *idef);
	virtual ~RemotePlayer() {}

	void save(std::string savedir, IGameDef *gamedef);
	void deSerialize(std::istream &is, const std::string &playername);

	PlayerSAO *getPlayerSAO() { return m_sao; }
	void setPlayerSAO(PlayerSAO *sao) { m_sao = sao; }
	void setPosition(const v3f &position);

	const RemotePlayerChatResult canSendChatMessage();

	void setHotbarItemcount(s32 hotbar_itemcount)
	{
		hud_hotbar_itemcount = hotbar_itemcount;
	}

	s32 getHotbarItemcount() const { return hud_hotbar_itemcount; }

	void overrideDayNightRatio(bool do_override, float ratio)
	{
		m_day_night_ratio_do_override = do_override;
		m_day_night_ratio = ratio;
	}

	void getDayNightRatio(bool *do_override, float *ratio)
	{
		*do_override = m_day_night_ratio_do_override;
		*ratio = m_day_night_ratio;
	}

	// Use a function, if isDead can be defined by other conditions
	bool isDead() const { return hp == 0; }

	void setHotbarImage(const std::string &name)
	{
		hud_hotbar_image = name;
	}

	std::string getHotbarImage() const
	{
		return hud_hotbar_image;
	}

	void setHotbarSelectedImage(const std::string &name)
	{
		hud_hotbar_selected_image = name;
	}

	const std::string &getHotbarSelectedImage() const
	{
		return hud_hotbar_selected_image;
	}

	// Deprecated
	f32 getRadPitchDep() const { return -1.0 * m_pitch * core::DEGTORAD; }

	// Deprecated
	f32 getRadYawDep() const { return (m_yaw + 90.) * core::DEGTORAD; }

	void setSky(const video::SColor &bgcolor, const std::string &type,
				const std::vector<std::string> &params)
	{
		m_sky_bgcolor = bgcolor;
		m_sky_type = type;
		m_sky_params = params;
	}

	void getSky(video::SColor *bgcolor, std::string *type,
				std::vector<std::string> *params)
	{
		*bgcolor = m_sky_bgcolor;
		*type = m_sky_type;
		*params = m_sky_params;
	}

	bool checkModified() const { return m_dirty || inventory.checkModified(); }

	void setModified(const bool x)
	{
		m_dirty = x;
		if (!x)
			inventory.setModified(x);
	}

	virtual void setBreath(u16 breath)
	{
		if (breath != m_breath)
			m_dirty = true;
		Player::setBreath(breath);
	}

	virtual void setPitch(f32 pitch)
	{
		if (pitch != m_pitch)
			m_dirty = true;
		Player::setPitch(pitch);
	}

	virtual void setYaw(f32 yaw)
	{
		if (yaw != m_yaw)
			m_dirty = true;
		Player::setYaw(yaw);
	}

	u16 protocol_version;
private:
	/*
		serialize() writes a bunch of text that can contain
		any characters except a '\0', and such an ending that
		deSerialize stops reading exactly at the right point.
	*/
	void serialize(std::ostream &os);

	PlayerSAO *m_sao;
	bool m_dirty;

	static bool m_setting_cache_loaded;
	static float m_setting_chat_message_limit_per_10sec;
	static u16 m_setting_chat_message_limit_trigger_kick;

	u32 m_last_chat_message_sent;
	float m_chat_message_allowance;
	u16 m_message_rate_overhead;

	bool m_day_night_ratio_do_override;
	float m_day_night_ratio;
	std::string hud_hotbar_image;
	std::string hud_hotbar_selected_image;

	std::string m_sky_type;
	video::SColor m_sky_bgcolor;
	std::vector<std::string> m_sky_params;
};

#endif

