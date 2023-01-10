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

#include "game.h"

#include <iomanip>
#include <cmath>
#include "client/renderingengine.h"
#include "camera.h"
#include "client.h"
#include "client/clientevent.h"
#include "client/gameui.h"
#include "client/inputhandler.h"
#include "client/tile.h"     // For TextureSource
#include "client/keys.h"
#include "client/joystick_controller.h"
#include "clientmap.h"
#include "clouds.h"
#include "config.h"
#include "content_cao.h"
#include "content/subgames.h"
#include "client/event_manager.h"
#include "fontengine.h"
#include "itemdef.h"
#include "log.h"
#include "filesys.h"
#include "gameparams.h"
#include "gettext.h"
#include "gui/guiChatConsole.h"
#include "gui/guiFormSpecMenu.h"
#include "gui/guiKeyChangeMenu.h"
#include "gui/guiPasswordChange.h"
#include "gui/guiVolumeChange.h"
#include "gui/mainmenumanager.h"
#include "gui/profilergraph.h"
#include "mapblock.h"
#include "minimap.h"
#include "nodedef.h"         // Needed for determining pointing to nodes
#include "nodemetadata.h"
#include "particles.h"
#include "porting.h"
#include "profiler.h"
#include "raycast.h"
#include "server.h"
#include "settings.h"
#include "shader.h"
#include "sky.h"
#include "translation.h"
#include "util/basic_macros.h"
#include "util/directiontables.h"
#include "util/pointedthing.h"
#include "util/quicktune_shortcutter.h"
#include "irrlicht_changes/static_text.h"
#include "irr_ptr.h"
#include "version.h"
#include "script/scripting_client.h"
#include "hud.h"

#if USE_SOUND
	#include "client/sound_openal.h"
#else
	#include "client/sound.h"
#endif
/*
	Text input system
*/

struct TextDestNodeMetadata : public TextDest
{
	TextDestNodeMetadata(v3s16 p, Client *client)
	{
		m_p = p;
		m_client = client;
	}
	// This is deprecated I guess? -celeron55
	void gotText(const std::wstring &text)
	{
		std::string ntext = wide_to_utf8(text);
		infostream << "Submitting 'text' field of node at (" << m_p.X << ","
			   << m_p.Y << "," << m_p.Z << "): " << ntext << std::endl;
		StringMap fields;
		fields["text"] = ntext;
		m_client->sendNodemetaFields(m_p, "", fields);
	}
	void gotText(const StringMap &fields)
	{
		m_client->sendNodemetaFields(m_p, "", fields);
	}

	v3s16 m_p;
	Client *m_client;
};

struct TextDestPlayerInventory : public TextDest
{
	TextDestPlayerInventory(Client *client)
	{
		m_client = client;
		m_formname.clear();
	}
	TextDestPlayerInventory(Client *client, const std::string &formname)
	{
		m_client = client;
		m_formname = formname;
	}
	void gotText(const StringMap &fields)
	{
		m_client->sendInventoryFields(m_formname, fields);
	}

	Client *m_client;
};

struct LocalFormspecHandler : public TextDest
{
	LocalFormspecHandler(const std::string &formname)
	{
		m_formname = formname;
	}

	LocalFormspecHandler(const std::string &formname, Client *client):
		m_client(client)
	{
		m_formname = formname;
	}

	void gotText(const StringMap &fields)
	{
		if (m_formname == "MT_PAUSE_MENU") {
			if (fields.find("btn_sound") != fields.end()) {
				g_gamecallback->changeVolume();
				return;
			}

			if (fields.find("btn_key_config") != fields.end()) {
				g_gamecallback->keyConfig();
				return;
			}

			if (fields.find("btn_exit_menu") != fields.end()) {
				g_gamecallback->disconnect();
				return;
			}

			if (fields.find("btn_exit_os") != fields.end()) {
				g_gamecallback->exitToOS();
#ifndef __ANDROID__
				RenderingEngine::get_raw_device()->closeDevice();
#endif
				return;
			}

			if (fields.find("btn_change_password") != fields.end()) {
				g_gamecallback->changePassword();
				return;
			}

			return;
		}

		if (m_formname == "MT_DEATH_SCREEN") {
			assert(m_client != 0);
			m_client->sendRespawn();
			return;
		}

		if (m_client->modsLoaded())
			m_client->getScript()->on_formspec_input(m_formname, fields);
	}

	Client *m_client = nullptr;
};

/* Form update callback */

class NodeMetadataFormSource: public IFormSource
{
public:
	NodeMetadataFormSource(ClientMap *map, v3s16 p):
		m_map(map),
		m_p(p)
	{
	}
	const std::string &getForm() const
	{
		static const std::string empty_string = "";
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);

		if (!meta)
			return empty_string;

		return meta->getString("formspec");
	}

	virtual std::string resolveText(const std::string &str)
	{
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);

		if (!meta)
			return str;

		return meta->resolveString(str);
	}

	ClientMap *m_map;
	v3s16 m_p;
};

class PlayerInventoryFormSource: public IFormSource
{
public:
	PlayerInventoryFormSource(Client *client):
		m_client(client)
	{
	}

	const std::string &getForm() const
	{
		LocalPlayer *player = m_client->getEnv().getLocalPlayer();
		return player->inventory_formspec;
	}

	Client *m_client;
};

class NodeDugEvent : public MtEvent
{
public:
	v3s16 p;
	MapNode n;

	NodeDugEvent(v3s16 p, MapNode n):
		p(p),
		n(n)
	{}
	Type getType() const { return NODE_DUG; }
};

class SoundMaker
{
	ISoundManager *m_sound;
	const NodeDefManager *m_ndef;

public:
	bool makes_footstep_sound;
	float m_player_step_timer;
	float m_player_jump_timer;

	SimpleSoundSpec m_player_step_sound;
	SimpleSoundSpec m_player_leftpunch_sound;
	// Second sound made on left punch, currently used for item 'use' sound
	SimpleSoundSpec m_player_leftpunch_sound2;
	SimpleSoundSpec m_player_rightpunch_sound;

	SoundMaker(ISoundManager *sound, const NodeDefManager *ndef):
		m_sound(sound),
		m_ndef(ndef),
		makes_footstep_sound(true),
		m_player_step_timer(0.0f),
		m_player_jump_timer(0.0f)
	{
	}

	void playPlayerStep()
	{
		if (m_player_step_timer <= 0 && m_player_step_sound.exists()) {
			m_player_step_timer = 0.03;
			if (makes_footstep_sound)
				m_sound->playSound(m_player_step_sound);
		}
	}

	void playPlayerJump()
	{
		if (m_player_jump_timer <= 0.0f) {
			m_player_jump_timer = 0.2f;
			m_sound->playSound(SimpleSoundSpec("player_jump", 0.5f));
		}
	}

	static void viewBobbingStep(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker *)data;
		sm->playPlayerStep();
	}

	static void playerRegainGround(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker *)data;
		sm->playPlayerStep();
	}

	static void playerJump(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker *)data;
		sm->playPlayerJump();
	}

	static void cameraPunchLeft(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker *)data;
		sm->m_sound->playSound(sm->m_player_leftpunch_sound);
		sm->m_sound->playSound(sm->m_player_leftpunch_sound2);
	}

	static void cameraPunchRight(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker *)data;
		sm->m_sound->playSound(sm->m_player_rightpunch_sound);
	}

	static void nodeDug(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker *)data;
		NodeDugEvent *nde = (NodeDugEvent *)e;
		sm->m_sound->playSound(sm->m_ndef->get(nde->n).sound_dug);
	}

	static void playerDamage(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker *)data;
		sm->m_sound->playSound(SimpleSoundSpec("player_damage", 0.5));
	}

	static void playerFallingDamage(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker *)data;
		sm->m_sound->playSound(SimpleSoundSpec("player_falling_damage", 0.5));
	}

	void registerReceiver(MtEventManager *mgr)
	{
		mgr->reg(MtEvent::VIEW_BOBBING_STEP, SoundMaker::viewBobbingStep, this);
		mgr->reg(MtEvent::PLAYER_REGAIN_GROUND, SoundMaker::playerRegainGround, this);
		mgr->reg(MtEvent::PLAYER_JUMP, SoundMaker::playerJump, this);
		mgr->reg(MtEvent::CAMERA_PUNCH_LEFT, SoundMaker::cameraPunchLeft, this);
		mgr->reg(MtEvent::CAMERA_PUNCH_RIGHT, SoundMaker::cameraPunchRight, this);
		mgr->reg(MtEvent::NODE_DUG, SoundMaker::nodeDug, this);
		mgr->reg(MtEvent::PLAYER_DAMAGE, SoundMaker::playerDamage, this);
		mgr->reg(MtEvent::PLAYER_FALLING_DAMAGE, SoundMaker::playerFallingDamage, this);
	}

	void step(float dtime)
	{
		m_player_step_timer -= dtime;
		m_player_jump_timer -= dtime;
	}
};

// Locally stored sounds don't need to be preloaded because of this
class GameOnDemandSoundFetcher: public OnDemandSoundFetcher
{
	std::set<std::string> m_fetched;
private:
	void paths_insert(std::set<std::string> &dst_paths,
		const std::string &base,
		const std::string &name)
	{
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".0.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".1.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".2.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".3.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".4.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".5.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".6.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".7.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".8.ogg");
		dst_paths.insert(base + DIR_DELIM + "sounds" + DIR_DELIM + name + ".9.ogg");
	}
public:
	void fetchSounds(const std::string &name,
		std::set<std::string> &dst_paths,
		std::set<std::string> &dst_datas)
	{
		if (m_fetched.count(name))
			return;

		m_fetched.insert(name);

		paths_insert(dst_paths, porting::path_share, name);
		paths_insert(dst_paths, porting::path_user,  name);
	}
};


typedef s32 SamplerLayer_t;


class GameGlobalShaderConstantSetter : public IShaderConstantSetter
{
	Sky *m_sky;
	Client *m_client;
	bool *m_force_fog_off;
	f32 *m_fog_range;
	bool m_fog_enabled;
	CachedPixelShaderSetting<float, 4> m_sky_bg_color;
	CachedPixelShaderSetting<float> m_fog_distance;
	CachedVertexShaderSetting<float> m_animation_timer_vertex;
	CachedPixelShaderSetting<float> m_animation_timer_pixel;
	CachedVertexShaderSetting<float> m_animation_timer_delta_vertex;
	CachedPixelShaderSetting<float> m_animation_timer_delta_pixel;
	CachedPixelShaderSetting<float, 3> m_day_light;
	CachedPixelShaderSetting<float, 4> m_star_color;
	CachedPixelShaderSetting<float, 3> m_eye_position_pixel;
	CachedVertexShaderSetting<float, 3> m_eye_position_vertex;
	CachedPixelShaderSetting<float, 3> m_minimap_yaw;
	CachedPixelShaderSetting<float, 3> m_camera_offset_pixel;
	CachedPixelShaderSetting<float, 3> m_camera_offset_vertex;
	CachedPixelShaderSetting<SamplerLayer_t> m_texture0;
	CachedPixelShaderSetting<SamplerLayer_t> m_texture1;
	CachedPixelShaderSetting<SamplerLayer_t> m_texture2;
	CachedPixelShaderSetting<SamplerLayer_t> m_texture3;
	CachedPixelShaderSetting<float, 2> m_texel_size0;
	std::array<float, 2> m_texel_size0_values;
	CachedStructPixelShaderSetting<float, 7> m_exposure_params_pixel;
	float m_user_exposure_compensation;
	bool m_bloom_enabled;
	CachedPixelShaderSetting<float> m_bloom_intensity_pixel;
	float m_bloom_intensity;
	CachedPixelShaderSetting<float> m_bloom_strength_pixel;
	float m_bloom_strength;
	CachedPixelShaderSetting<float> m_bloom_radius_pixel;
	float m_bloom_radius;
	CachedPixelShaderSetting<float> m_saturation_pixel;

public:
	void onSettingsChange(const std::string &name)
	{
		if (name == "enable_fog")
			m_fog_enabled = g_settings->getBool("enable_fog");
		if (name == "exposure_compensation")
			m_user_exposure_compensation = g_settings->getFloat("exposure_compensation", -1.0f, 1.0f);
		if (name == "bloom_intensity")
			m_bloom_intensity = g_settings->getFloat("bloom_intensity", 0.01f, 1.0f);
		if (name == "bloom_strength_factor")
			m_bloom_strength = RenderingEngine::BASE_BLOOM_STRENGTH * g_settings->getFloat("bloom_strength_factor", 0.1f, 10.0f);
		if (name == "bloom_radius")
			m_bloom_radius = g_settings->getFloat("bloom_radius", 0.1f, 8.0f);
	}

	static void settingsCallback(const std::string &name, void *userdata)
	{
		reinterpret_cast<GameGlobalShaderConstantSetter*>(userdata)->onSettingsChange(name);
	}

	void setSky(Sky *sky) { m_sky = sky; }

	GameGlobalShaderConstantSetter(Sky *sky, bool *force_fog_off,
			f32 *fog_range, Client *client) :
		m_sky(sky),
		m_client(client),
		m_force_fog_off(force_fog_off),
		m_fog_range(fog_range),
		m_sky_bg_color("skyBgColor"),
		m_fog_distance("fogDistance"),
		m_animation_timer_vertex("animationTimer"),
		m_animation_timer_pixel("animationTimer"),
		m_animation_timer_delta_vertex("animationTimerDelta"),
		m_animation_timer_delta_pixel("animationTimerDelta"),
		m_day_light("dayLight"),
		m_star_color("starColor"),
		m_eye_position_pixel("eyePosition"),
		m_eye_position_vertex("eyePosition"),
		m_minimap_yaw("yawVec"),
		m_camera_offset_pixel("cameraOffset"),
		m_camera_offset_vertex("cameraOffset"),
		m_texture0("texture0"),
		m_texture1("texture1"),
		m_texture2("texture2"),
		m_texture3("texture3"),
		m_texel_size0("texelSize0"),
		m_exposure_params_pixel("exposureParams",
				std::array<const char*, 7> {
						"luminanceMin", "luminanceMax", "exposureCorrection",
						"speedDarkBright", "speedBrightDark", "centerWeightPower", "compensationFactor"
				}),
		m_bloom_intensity_pixel("bloomIntensity"),
		m_bloom_strength_pixel("bloomStrength"),
		m_bloom_radius_pixel("bloomRadius"),
		m_saturation_pixel("saturation")
	{
		g_settings->registerChangedCallback("enable_fog", settingsCallback, this);
		g_settings->registerChangedCallback("exposure_compensation", settingsCallback, this);
		g_settings->registerChangedCallback("bloom_intensity", settingsCallback, this);
		g_settings->registerChangedCallback("bloom_strength_factor", settingsCallback, this);
		g_settings->registerChangedCallback("bloom_radius", settingsCallback, this);
		g_settings->registerChangedCallback("saturation", settingsCallback, this);
		m_fog_enabled = g_settings->getBool("enable_fog");
		m_user_exposure_compensation = g_settings->getFloat("exposure_compensation", -1.0f, 1.0f);
		m_bloom_enabled = g_settings->getBool("enable_bloom");
		m_bloom_intensity = g_settings->getFloat("bloom_intensity", 0.01f, 1.0f);
		m_bloom_strength = RenderingEngine::BASE_BLOOM_STRENGTH * g_settings->getFloat("bloom_strength_factor", 0.1f, 10.0f);
		m_bloom_radius = g_settings->getFloat("bloom_radius", 0.1f, 8.0f);
	}

	~GameGlobalShaderConstantSetter()
	{
		g_settings->deregisterChangedCallback("enable_fog", settingsCallback, this);
	}

	void onSetConstants(video::IMaterialRendererServices *services) override
	{
		// Background color
		video::SColor bgcolor = m_sky->getBgColor();
		video::SColorf bgcolorf(bgcolor);
		float bgcolorfa[4] = {
			bgcolorf.r,
			bgcolorf.g,
			bgcolorf.b,
			bgcolorf.a,
		};
		m_sky_bg_color.set(bgcolorfa, services);

		// Fog distance
		float fog_distance = 10000 * BS;

		if (m_fog_enabled && !*m_force_fog_off)
			fog_distance = *m_fog_range;

		m_fog_distance.set(&fog_distance, services);

		u32 daynight_ratio = (float)m_client->getEnv().getDayNightRatio();
		video::SColorf sunlight;
		get_sunlight_color(&sunlight, daynight_ratio);
		float dnc[3] = {
			sunlight.r,
			sunlight.g,
			sunlight.b };
		m_day_light.set(dnc, services);

		video::SColorf star_color = m_sky->getCurrentStarColor();
		float clr[4] = {star_color.r, star_color.g, star_color.b, star_color.a};
		m_star_color.set(clr, services);

		u32 animation_timer = m_client->getEnv().getFrameTime() % 1000000;
		float animation_timer_f = (float)animation_timer / 100000.f;
		m_animation_timer_vertex.set(&animation_timer_f, services);
		m_animation_timer_pixel.set(&animation_timer_f, services);

		float animation_timer_delta_f = (float)m_client->getEnv().getFrameTimeDelta() / 100000.f;
		m_animation_timer_delta_vertex.set(&animation_timer_delta_f, services);
		m_animation_timer_delta_pixel.set(&animation_timer_delta_f, services);

		float eye_position_array[3];
		v3f epos = m_client->getEnv().getLocalPlayer()->getEyePosition();
		epos.getAs3Values(eye_position_array);
		m_eye_position_pixel.set(eye_position_array, services);
		m_eye_position_vertex.set(eye_position_array, services);

		if (m_client->getMinimap()) {
			float minimap_yaw_array[3];
			v3f minimap_yaw = m_client->getMinimap()->getYawVec();
			minimap_yaw.getAs3Values(minimap_yaw_array);
			m_minimap_yaw.set(minimap_yaw_array, services);
		}

		float camera_offset_array[3];
		v3f offset = intToFloat(m_client->getCamera()->getOffset(), BS);
		offset.getAs3Values(camera_offset_array);
		m_camera_offset_pixel.set(camera_offset_array, services);
		m_camera_offset_vertex.set(camera_offset_array, services);

		SamplerLayer_t tex_id;
		tex_id = 0;
		m_texture0.set(&tex_id, services);
		tex_id = 1;
		m_texture1.set(&tex_id, services);
		tex_id = 2;
		m_texture2.set(&tex_id, services);
		tex_id = 3;
		m_texture3.set(&tex_id, services);

		m_texel_size0.set(m_texel_size0_values.data(), services);

		const AutoExposure &exposure_params = m_client->getEnv().getLocalPlayer()->getLighting().exposure;
		std::array<float, 7> exposure_buffer = {
			std::pow(2.0f, exposure_params.luminance_min),
			std::pow(2.0f, exposure_params.luminance_max),
			exposure_params.exposure_correction,
			exposure_params.speed_dark_bright,
			exposure_params.speed_bright_dark,
			exposure_params.center_weight_power,
			powf(2.f, m_user_exposure_compensation)
		};
		m_exposure_params_pixel.set(exposure_buffer.data(), services);

		if (m_bloom_enabled) {
			m_bloom_intensity_pixel.set(&m_bloom_intensity, services);
			m_bloom_radius_pixel.set(&m_bloom_radius, services);
			m_bloom_strength_pixel.set(&m_bloom_strength, services);
		}
		float saturation = m_client->getEnv().getLocalPlayer()->getLighting().saturation;
		m_saturation_pixel.set(&saturation, services);
	}

	void onSetMaterial(const video::SMaterial &material)
	{
		video::ITexture *texture = material.getTexture(0);
		if (texture) {
			core::dimension2du size = texture->getSize();
			m_texel_size0_values[0] = 1.f / size.Width;
			m_texel_size0_values[1] = 1.f / size.Height;
		}
		else {
			m_texel_size0_values[0] = 0.f;
			m_texel_size0_values[1] = 0.f;
		}
	}
};


class GameGlobalShaderConstantSetterFactory : public IShaderConstantSetterFactory
{
	Sky *m_sky;
	bool *m_force_fog_off;
	f32 *m_fog_range;
	Client *m_client;
	std::vector<GameGlobalShaderConstantSetter *> created_nosky;
public:
	GameGlobalShaderConstantSetterFactory(bool *force_fog_off,
			f32 *fog_range, Client *client) :
		m_sky(NULL),
		m_force_fog_off(force_fog_off),
		m_fog_range(fog_range),
		m_client(client)
	{}

	void setSky(Sky *sky) {
		m_sky = sky;
		for (GameGlobalShaderConstantSetter *ggscs : created_nosky) {
			ggscs->setSky(m_sky);
		}
		created_nosky.clear();
	}

	virtual IShaderConstantSetter* create()
	{
		auto *scs = new GameGlobalShaderConstantSetter(
				m_sky, m_force_fog_off, m_fog_range, m_client);
		if (!m_sky)
			created_nosky.push_back(scs);
		return scs;
	}
};

#ifdef HAVE_TOUCHSCREENGUI
#define SIZE_TAG "size[11,5.5]"
#else
#define SIZE_TAG "size[11,5.5,true]" // Fixed size on desktop
#endif

/****************************************************************************
 ****************************************************************************/

const static float object_hit_delay = 0.2;

struct FpsControl {
	FpsControl() : last_time(0), busy_time(0), sleep_time(0) {}

	void reset();

	void limit(IrrlichtDevice *device, f32 *dtime);

	u32 getBusyMs() const { return busy_time / 1000; }

	// all values in microseconds (us)
	u64 last_time, busy_time, sleep_time;
};


/* The reason the following structs are not anonymous structs within the
 * class is that they are not used by the majority of member functions and
 * many functions that do require objects of thse types do not modify them
 * (so they can be passed as a const qualified parameter)
 */

struct GameRunData {
	u16 dig_index;
	u16 new_playeritem;
	PointedThing pointed_old;
	bool digging;
	bool punching;
	bool btn_down_for_dig;
	bool dig_instantly;
	bool digging_blocked;
	bool reset_jump_timer;
	float nodig_delay_timer;
	float dig_time;
	float dig_time_complete;
	float repeat_place_timer;
	float object_hit_delay_timer;
	float time_from_last_punch;
	ClientActiveObject *selected_object;

	float jump_timer_up;          // from key up until key down
	float jump_timer_down;        // since last key down
	float jump_timer_down_before; // from key down until key down again

	float damage_flash;
	float update_draw_list_timer;
	float touch_blocks_timer;

	f32 fog_range;

	v3f update_draw_list_last_cam_dir;

	float time_of_day_smooth;
};

class Game;

struct ClientEventHandler
{
	void (Game::*handler)(ClientEvent *, CameraOrientation *);
};

/****************************************************************************
 THE GAME
 ****************************************************************************/

using PausedNodesList = std::vector<std::pair<irr_ptr<scene::IAnimatedMeshSceneNode>, float>>;

/* This is not intended to be a public class. If a public class becomes
 * desirable then it may be better to create another 'wrapper' class that
 * hides most of the stuff in this class (nothing in this class is required
 * by any other file) but exposes the public methods/data only.
 */
class Game {
public:
	Game();
	~Game();

	bool startup(bool *kill,
			InputHandler *input,
			RenderingEngine *rendering_engine,
			const GameStartData &game_params,
			std::string &error_message,
			bool *reconnect,
			ChatBackend *chat_backend);

	void run();
	void shutdown();

protected:

	// Basic initialisation
	bool init(const std::string &map_dir, const std::string &address,
			u16 port, const SubgameSpec &gamespec);
	bool initSound();
	bool createSingleplayerServer(const std::string &map_dir,
			const SubgameSpec &gamespec, u16 port);

	// Client creation
	bool createClient(const GameStartData &start_data);
	bool initGui();

	// Client connection
	bool connectToServer(const GameStartData &start_data,
			bool *connect_ok, bool *aborted);
	bool getServerContent(bool *aborted);

	// Main loop

	void updateInteractTimers(f32 dtime);
	bool checkConnection();
	bool handleCallbacks();
	void processQueues();
	void updateProfilers(const RunStats &stats, const FpsControl &draw_times, f32 dtime);
	void updateDebugState();
	void updateStats(RunStats *stats, const FpsControl &draw_times, f32 dtime);
	void updateProfilerGraphs(ProfilerGraph *graph);

	// Input related
	void processUserInput(f32 dtime);
	void processKeyInput();
	void processItemSelection(u16 *new_playeritem);

	void dropSelectedItem(bool single_item = false);
	void openInventory();
	void openConsole(float scale, const wchar_t *line=NULL);
	void toggleFreeMove();
	void toggleFreeMoveAlt();
	void togglePitchMove();
	void toggleFast();
	void toggleNoClip();
	void toggleCinematic();
	void toggleBlockBounds();
	void toggleAutoforward();

	void toggleMinimap(bool shift_pressed);
	void toggleFog();
	void toggleDebug();
	void toggleUpdateCamera();

	void increaseViewRange();
	void decreaseViewRange();
	void toggleFullViewRange();
	void checkZoomEnabled();

	void updateCameraDirection(CameraOrientation *cam, float dtime);
	void updateCameraOrientation(CameraOrientation *cam, float dtime);
	void updatePlayerControl(const CameraOrientation &cam);
	void step(f32 dtime);
	void processClientEvents(CameraOrientation *cam);
	void updateCamera(f32 dtime);
	void updateSound(f32 dtime);
	void processPlayerInteraction(f32 dtime, bool show_hud);
	/*!
	 * Returns the object or node the player is pointing at.
	 * Also updates the selected thing in the Hud.
	 *
	 * @param[in]  shootline         the shootline, starting from
	 * the camera position. This also gives the maximal distance
	 * of the search.
	 * @param[in]  liquids_pointable if false, liquids are ignored
	 * @param[in]  look_for_object   if false, objects are ignored
	 * @param[in]  camera_offset     offset of the camera
	 * @param[out] selected_object   the selected object or
	 * NULL if not found
	 */
	PointedThing updatePointedThing(
			const core::line3d<f32> &shootline, bool liquids_pointable,
			bool look_for_object, const v3s16 &camera_offset);
	void handlePointingAtNothing(const ItemStack &playerItem);
	void handlePointingAtNode(const PointedThing &pointed,
			const ItemStack &selected_item, const ItemStack &hand_item, f32 dtime);
	void handlePointingAtObject(const PointedThing &pointed, const ItemStack &playeritem,
			const v3f &player_position, bool show_debug);
	void handleDigging(const PointedThing &pointed, const v3s16 &nodepos,
			const ItemStack &selected_item, const ItemStack &hand_item, f32 dtime);
	void updateFrame(ProfilerGraph *graph, RunStats *stats, f32 dtime,
			const CameraOrientation &cam);
	void updateShadows();

	// Misc
	void showOverlayMessage(const char *msg, float dtime, int percent,
			bool draw_clouds = true);

	static void settingChangedCallback(const std::string &setting_name, void *data);
	void readSettings();

	inline bool isKeyDown(GameKeyType k)
	{
		return input->isKeyDown(k);
	}
	inline bool wasKeyDown(GameKeyType k)
	{
		return input->wasKeyDown(k);
	}
	inline bool wasKeyPressed(GameKeyType k)
	{
		return input->wasKeyPressed(k);
	}
	inline bool wasKeyReleased(GameKeyType k)
	{
		return input->wasKeyReleased(k);
	}

#ifdef __ANDROID__
	void handleAndroidChatInput();
#endif

private:
	struct Flags {
		bool force_fog_off = false;
		bool disable_camera_update = false;
	};

	void showDeathFormspec();
	void showPauseMenu();

	void pauseAnimation();
	void resumeAnimation();

	// ClientEvent handlers
	void handleClientEvent_None(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_PlayerDamage(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_PlayerForceMove(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_Deathscreen(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_ShowFormSpec(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_ShowLocalFormSpec(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_HandleParticleEvent(ClientEvent *event,
		CameraOrientation *cam);
	void handleClientEvent_HudAdd(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_HudRemove(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_HudChange(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_SetSky(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_SetSun(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_SetMoon(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_SetStars(ClientEvent *event, CameraOrientation *cam);
	void handleClientEvent_OverrideDayNigthRatio(ClientEvent *event,
		CameraOrientation *cam);
	void handleClientEvent_CloudParams(ClientEvent *event, CameraOrientation *cam);

	void updateChat(f32 dtime);

	bool nodePlacement(const ItemDefinition &selected_def, const ItemStack &selected_item,
		const v3s16 &nodepos, const v3s16 &neighborpos, const PointedThing &pointed,
		const NodeMetadata *meta);
	static const ClientEventHandler clientEventHandler[CLIENTEVENT_MAX];

	f32 getSensitivityScaleFactor() const;

	InputHandler *input = nullptr;

	Client *client = nullptr;
	Server *server = nullptr;

	IWritableTextureSource *texture_src = nullptr;
	IWritableShaderSource *shader_src = nullptr;

	// When created, these will be filled with data received from the server
	IWritableItemDefManager *itemdef_manager = nullptr;
	NodeDefManager *nodedef_manager = nullptr;

	GameOnDemandSoundFetcher soundfetcher; // useful when testing
	ISoundManager *sound = nullptr;
	bool sound_is_dummy = false;
	SoundMaker *soundmaker = nullptr;

	ChatBackend *chat_backend = nullptr;
	LogOutputBuffer m_chat_log_buf;

	EventManager *eventmgr = nullptr;
	QuicktuneShortcutter *quicktune = nullptr;

	std::unique_ptr<GameUI> m_game_ui;
	GUIChatConsole *gui_chat_console = nullptr; // Free using ->Drop()
	MapDrawControl *draw_control = nullptr;
	Camera *camera = nullptr;
	Clouds *clouds = nullptr;	                  // Free using ->Drop()
	Sky *sky = nullptr;                         // Free using ->Drop()
	Hud *hud = nullptr;
	Minimap *mapper = nullptr;

	// Map server hud ids to client hud ids
	std::unordered_map<u32, u32> m_hud_server_to_client;

	GameRunData runData;
	Flags m_flags;

	/* 'cache'
	   This class does take ownership/responsibily for cleaning up etc of any of
	   these items (e.g. device)
	*/
	IrrlichtDevice *device;
	RenderingEngine *m_rendering_engine;
	video::IVideoDriver *driver;
	scene::ISceneManager *smgr;
	bool *kill;
	std::string *error_message;
	bool *reconnect_requested;
	scene::ISceneNode *skybox;
	PausedNodesList paused_animated_nodes;

	bool simple_singleplayer_mode;
	/* End 'cache' */

	/* Pre-calculated values
	 */
	int crack_animation_length;

	IntervalLimiter profiler_interval;

	/*
	 * TODO: Local caching of settings is not optimal and should at some stage
	 *       be updated to use a global settings object for getting thse values
	 *       (as opposed to the this local caching). This can be addressed in
	 *       a later release.
	 */
	bool m_cache_doubletap_jump;
	bool m_cache_enable_clouds;
	bool m_cache_enable_joysticks;
	bool m_cache_enable_particles;
	bool m_cache_enable_fog;
	bool m_cache_enable_noclip;
	bool m_cache_enable_free_move;
	f32  m_cache_mouse_sensitivity;
	f32  m_cache_joystick_frustum_sensitivity;
	f32  m_repeat_place_time;
	f32  m_cache_cam_smoothing;
	f32  m_cache_fog_start;

	bool m_invert_mouse = false;
	bool m_first_loop_after_window_activation = false;
	bool m_camera_offset_changed = false;

	bool m_does_lost_focus_pause_game = false;

	// if true, (almost) the whole game is paused
	// this happens in pause menu in singleplayer
	bool m_is_paused = false;

#if IRRLICHT_VERSION_MT_REVISION < 5
	int m_reset_HW_buffer_counter = 0;
#endif

#ifdef HAVE_TOUCHSCREENGUI
	bool m_cache_hold_aux1;
	bool m_touch_use_crosshair;
	inline bool isNoCrosshairAllowed() {
		return !m_touch_use_crosshair && camera->getCameraMode() == CAMERA_MODE_FIRST;
	}
#endif
#ifdef __ANDROID__
	bool m_android_chat_open;
#endif
};

Game::Game() :
	m_chat_log_buf(g_logger),
	m_game_ui(new GameUI())
{
	g_settings->registerChangedCallback("doubletap_jump",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("enable_clouds",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("doubletap_joysticks",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("enable_particles",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("enable_fog",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("mouse_sensitivity",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("joystick_frustum_sensitivity",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("repeat_place_time",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("noclip",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("free_move",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("cinematic",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("cinematic_camera_smoothing",
		&settingChangedCallback, this);
	g_settings->registerChangedCallback("camera_smoothing",
		&settingChangedCallback, this);

	readSettings();

#ifdef HAVE_TOUCHSCREENGUI
	m_cache_hold_aux1 = false;	// This is initialised properly later
#endif

}



/****************************************************************************
 MinetestApp Public
 ****************************************************************************/

Game::~Game()
{
	delete client;
	delete soundmaker;
	if (!sound_is_dummy)
		delete sound;

	delete server; // deleted first to stop all server threads

	delete hud;
	delete camera;
	delete quicktune;
	delete eventmgr;
	delete texture_src;
	delete shader_src;
	delete nodedef_manager;
	delete itemdef_manager;
	delete draw_control;

	clearTextureNameCache();

	g_settings->deregisterChangedCallback("doubletap_jump",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("enable_clouds",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("enable_particles",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("enable_fog",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("mouse_sensitivity",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("repeat_place_time",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("noclip",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("free_move",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("cinematic",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("cinematic_camera_smoothing",
		&settingChangedCallback, this);
	g_settings->deregisterChangedCallback("camera_smoothing",
		&settingChangedCallback, this);
}

bool Game::startup(bool *kill,
		InputHandler *input,
		RenderingEngine *rendering_engine,
		const GameStartData &start_data,
		std::string &error_message,
		bool *reconnect,
		ChatBackend *chat_backend)
{

	// "cache"
	m_rendering_engine        = rendering_engine;
	device                    = m_rendering_engine->get_raw_device();
	this->kill                = kill;
	this->error_message       = &error_message;
	reconnect_requested       = reconnect;
	this->input               = input;
	this->chat_backend        = chat_backend;
	simple_singleplayer_mode  = start_data.isSinglePlayer();

	input->keycache.populate();

	driver = device->getVideoDriver();
	smgr = m_rendering_engine->get_scene_manager();

	smgr->getParameters()->setAttribute(scene::OBJ_LOADER_IGNORE_MATERIAL_FILES, true);

	// Reinit runData
	runData = GameRunData();
	runData.time_from_last_punch = 10.0;

	m_game_ui->initFlags();

	m_invert_mouse = g_settings->getBool("invert_mouse");
	m_first_loop_after_window_activation = true;

#ifdef HAVE_TOUCHSCREENGUI
	m_touch_use_crosshair = g_settings->getBool("touch_use_crosshair");
#endif

	g_client_translations->clear();

	// address can change if simple_singleplayer_mode
	if (!init(start_data.world_spec.path, start_data.address,
			start_data.socket_port, start_data.game_spec))
		return false;

	if (!createClient(start_data))
		return false;

	m_rendering_engine->initialize(client, hud);

	return true;
}


void Game::run()
{
	ProfilerGraph graph;
	RunStats stats = {};
	CameraOrientation cam_view_target = {};
	CameraOrientation cam_view = {};
	FpsControl draw_times;
	f32 dtime; // in seconds

	/* Clear the profiler */
	Profiler::GraphValues dummyvalues;
	g_profiler->graphGet(dummyvalues);

	draw_times.reset();

	set_light_table(g_settings->getFloat("display_gamma"));

#ifdef HAVE_TOUCHSCREENGUI
	m_cache_hold_aux1 = g_settings->getBool("fast_move")
			&& client->checkPrivilege("fast");
#endif

	irr::core::dimension2d<u32> previous_screen_size(g_settings->getU16("screen_w"),
		g_settings->getU16("screen_h"));

	while (m_rendering_engine->run()
			&& !(*kill || g_gamecallback->shutdown_requested
			|| (server && server->isShutdownRequested()))) {

		const irr::core::dimension2d<u32> &current_screen_size =
			m_rendering_engine->get_video_driver()->getScreenSize();
		// Verify if window size has changed and save it if it's the case
		// Ensure evaluating settings->getBool after verifying screensize
		// First condition is cheaper
		if (previous_screen_size != current_screen_size &&
				current_screen_size != irr::core::dimension2d<u32>(0,0) &&
				g_settings->getBool("autosave_screensize")) {
			g_settings->setU16("screen_w", current_screen_size.Width);
			g_settings->setU16("screen_h", current_screen_size.Height);
			previous_screen_size = current_screen_size;
		}

		// Calculate dtime =
		//    m_rendering_engine->run() from this iteration
		//  + Sleep time until the wanted FPS are reached
		draw_times.limit(device, &dtime);

		// Prepare render data for next iteration

		updateStats(&stats, draw_times, dtime);
		updateInteractTimers(dtime);

		if (!checkConnection())
			break;
		if (!handleCallbacks())
			break;

		processQueues();

		m_game_ui->clearInfoText();

		updateProfilers(stats, draw_times, dtime);
		processUserInput(dtime);
		// Update camera before player movement to avoid camera lag of one frame
		updateCameraDirection(&cam_view_target, dtime);
		cam_view.camera_yaw += (cam_view_target.camera_yaw -
				cam_view.camera_yaw) * m_cache_cam_smoothing;
		cam_view.camera_pitch += (cam_view_target.camera_pitch -
				cam_view.camera_pitch) * m_cache_cam_smoothing;
		updatePlayerControl(cam_view);

		{
			bool was_paused = m_is_paused;
			m_is_paused = simple_singleplayer_mode && g_menumgr.pausesGame();
			if (m_is_paused)
				dtime = 0.0f;

			if (!was_paused && m_is_paused)
				pauseAnimation();
			else if (was_paused && !m_is_paused)
				resumeAnimation();
		}

		if (!m_is_paused)
			step(dtime);
		processClientEvents(&cam_view_target);
		updateDebugState();
		updateCamera(dtime);
		updateSound(dtime);
		processPlayerInteraction(dtime, m_game_ui->m_flags.show_hud);
		updateFrame(&graph, &stats, dtime, cam_view);
		updateProfilerGraphs(&graph);

		// Update if minimap has been disabled by the server
		m_game_ui->m_flags.show_minimap &= client->shouldShowMinimap();

		if (m_does_lost_focus_pause_game && !device->isWindowFocused() && !isMenuActive()) {
			showPauseMenu();
		}
	}
}


void Game::shutdown()
{
	m_rendering_engine->finalize();

	auto formspec = m_game_ui->getFormspecGUI();
	if (formspec)
		formspec->quitMenu();

#ifdef HAVE_TOUCHSCREENGUI
	g_touchscreengui->hide();
#endif

	showOverlayMessage(N_("Shutting down..."), 0, 0, false);

	if (clouds)
		clouds->drop();

	if (gui_chat_console)
		gui_chat_console->drop();

	if (sky)
		sky->drop();

	/* cleanup menus */
	while (g_menumgr.menuCount() > 0) {
		g_menumgr.m_stack.front()->setVisible(false);
		g_menumgr.deletingMenu(g_menumgr.m_stack.front());
	}

	m_game_ui->deleteFormspec();

	chat_backend->addMessage(L"", L"# Disconnected.");
	chat_backend->addMessage(L"", L"");
	m_chat_log_buf.clear();

	if (client) {
		client->Stop();
		while (!client->isShutdown()) {
			assert(texture_src != NULL);
			assert(shader_src != NULL);
			texture_src->processQueue();
			shader_src->processQueue();
			sleep_ms(100);
		}
	}
}


/****************************************************************************/
/****************************************************************************
 Startup
 ****************************************************************************/
/****************************************************************************/

bool Game::init(
		const std::string &map_dir,
		const std::string &address,
		u16 port,
		const SubgameSpec &gamespec)
{
	texture_src = createTextureSource();

	showOverlayMessage(N_("Loading..."), 0, 0);

	shader_src = createShaderSource();

	itemdef_manager = createItemDefManager();
	nodedef_manager = createNodeDefManager();

	eventmgr = new EventManager();
	quicktune = new QuicktuneShortcutter();

	if (!(texture_src && shader_src && itemdef_manager && nodedef_manager
			&& eventmgr && quicktune))
		return false;

	if (!initSound())
		return false;

	// Create a server if not connecting to an existing one
	if (address.empty()) {
		if (!createSingleplayerServer(map_dir, gamespec, port))
			return false;
	}

	return true;
}

bool Game::initSound()
{
#if USE_SOUND
	if (g_settings->getBool("enable_sound") && g_sound_manager_singleton.get()) {
		infostream << "Attempting to use OpenAL audio" << std::endl;
		sound = createOpenALSoundManager(g_sound_manager_singleton.get(), &soundfetcher);
		if (!sound)
			infostream << "Failed to initialize OpenAL audio" << std::endl;
	} else
		infostream << "Sound disabled." << std::endl;
#endif

	if (!sound) {
		infostream << "Using dummy audio." << std::endl;
		sound = &dummySoundManager;
		sound_is_dummy = true;
	}

	soundmaker = new SoundMaker(sound, nodedef_manager);
	if (!soundmaker)
		return false;

	soundmaker->registerReceiver(eventmgr);

	return true;
}

bool Game::createSingleplayerServer(const std::string &map_dir,
		const SubgameSpec &gamespec, u16 port)
{
	showOverlayMessage(N_("Creating server..."), 0, 5);

	std::string bind_str = g_settings->get("bind_address");
	Address bind_addr(0, 0, 0, 0, port);

	if (g_settings->getBool("ipv6_server")) {
		bind_addr.setAddress((IPv6AddressBytes *) NULL);
	}

	try {
		bind_addr.Resolve(bind_str.c_str());
	} catch (ResolveError &e) {
		infostream << "Resolving bind address \"" << bind_str
			   << "\" failed: " << e.what()
			   << " -- Listening on all addresses." << std::endl;
	}

	if (bind_addr.isIPv6() && !g_settings->getBool("enable_ipv6")) {
		*error_message = fmtgettext("Unable to listen on %s because IPv6 is disabled",
			bind_addr.serializeString().c_str());
		errorstream << *error_message << std::endl;
		return false;
	}

	server = new Server(map_dir, gamespec, simple_singleplayer_mode, bind_addr,
			false, nullptr, error_message);
	server->start();

	return true;
}

bool Game::createClient(const GameStartData &start_data)
{
	showOverlayMessage(N_("Creating client..."), 0, 10);

	draw_control = new MapDrawControl();
	if (!draw_control)
		return false;

	bool could_connect, connect_aborted;
#ifdef HAVE_TOUCHSCREENGUI
	if (g_touchscreengui) {
		g_touchscreengui->init(texture_src);
		g_touchscreengui->hide();
	}
#endif
	if (!connectToServer(start_data, &could_connect, &connect_aborted))
		return false;

	if (!could_connect) {
		if (error_message->empty() && !connect_aborted) {
			// Should not happen if error messages are set properly
			*error_message = gettext("Connection failed for unknown reason");
			errorstream << *error_message << std::endl;
		}
		return false;
	}

	if (!getServerContent(&connect_aborted)) {
		if (error_message->empty() && !connect_aborted) {
			// Should not happen if error messages are set properly
			*error_message = gettext("Connection failed for unknown reason");
			errorstream << *error_message << std::endl;
		}
		return false;
	}

	auto *scsf = new GameGlobalShaderConstantSetterFactory(
			&m_flags.force_fog_off, &runData.fog_range, client);
	shader_src->addShaderConstantSetterFactory(scsf);

	// Update cached textures, meshes and materials
	client->afterContentReceived();

	/* Camera
	 */
	camera = new Camera(*draw_control, client, m_rendering_engine);
	if (client->modsLoaded())
		client->getScript()->on_camera_ready(camera);
	client->setCamera(camera);
#ifdef HAVE_TOUCHSCREENGUI
	if (g_touchscreengui) {
		g_touchscreengui->setUseCrosshair(!isNoCrosshairAllowed());
	}
#endif

	/* Clouds
	 */
	if (m_cache_enable_clouds)
		clouds = new Clouds(smgr, -1, time(0));

	/* Skybox
	 */
	sky = new Sky(-1, m_rendering_engine, texture_src, shader_src);
	scsf->setSky(sky);
	skybox = NULL;	// This is used/set later on in the main run loop

	/* Pre-calculated values
	 */
	video::ITexture *t = texture_src->getTexture("crack_anylength.png");
	if (t) {
		v2u32 size = t->getOriginalSize();
		crack_animation_length = size.Y / size.X;
	} else {
		crack_animation_length = 5;
	}

	if (!initGui())
		return false;

	/* Set window caption
	 */
	std::wstring str = utf8_to_wide(PROJECT_NAME_C);
	str += L" ";
	str += utf8_to_wide(g_version_hash);
	{
		const wchar_t *text = nullptr;
		if (simple_singleplayer_mode)
			text = wgettext("Singleplayer");
		else
			text = wgettext("Multiplayer");
		str += L" [";
		str += text;
		str += L"]";
		delete[] text;
	}
	str += L" [";
	str += driver->getName();
	str += L"]";

	device->setWindowCaption(str.c_str());

	LocalPlayer *player = client->getEnv().getLocalPlayer();
	player->hurt_tilt_timer = 0;
	player->hurt_tilt_strength = 0;

	hud = new Hud(client, player, &player->inventory);

	mapper = client->getMinimap();

	if (mapper && client->modsLoaded())
		client->getScript()->on_minimap_ready(mapper);

	return true;
}

bool Game::initGui()
{
	m_game_ui->init();

	// Remove stale "recent" chat messages from previous connections
	chat_backend->clearRecentChat();

	// Make sure the size of the recent messages buffer is right
	chat_backend->applySettings();

	// Chat backend and console
	gui_chat_console = new GUIChatConsole(guienv, guienv->getRootGUIElement(),
			-1, chat_backend, client, &g_menumgr);

#ifdef HAVE_TOUCHSCREENGUI

	if (g_touchscreengui)
		g_touchscreengui->show();

#endif

	return true;
}

bool Game::connectToServer(const GameStartData &start_data,
		bool *connect_ok, bool *connection_aborted)
{
	*connect_ok = false;	// Let's not be overly optimistic
	*connection_aborted = false;
	bool local_server_mode = false;

	showOverlayMessage(N_("Resolving address..."), 0, 15);

	Address connect_address(0, 0, 0, 0, start_data.socket_port);

	try {
		connect_address.Resolve(start_data.address.c_str());

		if (connect_address.isZero()) { // i.e. INADDR_ANY, IN6ADDR_ANY
			if (connect_address.isIPv6()) {
				IPv6AddressBytes addr_bytes;
				addr_bytes.bytes[15] = 1;
				connect_address.setAddress(&addr_bytes);
			} else {
				connect_address.setAddress(127, 0, 0, 1);
			}
			local_server_mode = true;
		}
	} catch (ResolveError &e) {
		*error_message = fmtgettext("Couldn't resolve address: %s", e.what());

		errorstream << *error_message << std::endl;
		return false;
	}

	if (connect_address.isIPv6() && !g_settings->getBool("enable_ipv6")) {
		*error_message = fmtgettext("Unable to connect to %s because IPv6 is disabled", connect_address.serializeString().c_str());
		errorstream << *error_message << std::endl;
		return false;
	}

	try {
		client = new Client(start_data.name.c_str(),
				start_data.password, start_data.address,
				*draw_control, texture_src, shader_src,
				itemdef_manager, nodedef_manager, sound, eventmgr,
				m_rendering_engine, connect_address.isIPv6(), m_game_ui.get(),
				start_data.allow_login_or_register);
		client->migrateModStorage();
	} catch (const BaseException &e) {
		*error_message = fmtgettext("Error creating client: %s", e.what());
		errorstream << *error_message << std::endl;
		return false;
	}

	client->m_simple_singleplayer_mode = simple_singleplayer_mode;

	infostream << "Connecting to server at ";
	connect_address.print(infostream);
	infostream << std::endl;

	client->connect(connect_address,
		simple_singleplayer_mode || local_server_mode);

	/*
		Wait for server to accept connection
	*/

	try {
		input->clear();

		FpsControl fps_control;
		f32 dtime;
		f32 wait_time = 0; // in seconds

		fps_control.reset();

		while (m_rendering_engine->run()) {

			fps_control.limit(device, &dtime);

			// Update client and server
			client->step(dtime);

			if (server != NULL)
				server->step(dtime);

			// End condition
			if (client->getState() == LC_Init) {
				*connect_ok = true;
				break;
			}

			// Break conditions
			if (*connection_aborted)
				break;

			if (client->accessDenied()) {
				*error_message = fmtgettext("Access denied. Reason: %s", client->accessDeniedReason().c_str());
				*reconnect_requested = client->reconnectRequested();
				errorstream << *error_message << std::endl;
				break;
			}

			if (input->cancelPressed()) {
				*connection_aborted = true;
				infostream << "Connect aborted [Escape]" << std::endl;
				break;
			}

			wait_time += dtime;
			// Only time out if we aren't waiting for the server we started
			if (!start_data.address.empty() && wait_time > 10) {
				*error_message = gettext("Connection timed out.");
				errorstream << *error_message << std::endl;
				break;
			}

			// Update status
			showOverlayMessage(N_("Connecting to server..."), dtime, 20);
		}
	} catch (con::PeerNotFoundException &e) {
		// TODO: Should something be done here? At least an info/error
		// message?
		return false;
	}

	return true;
}

bool Game::getServerContent(bool *aborted)
{
	input->clear();

	FpsControl fps_control;
	f32 dtime; // in seconds

	fps_control.reset();

	while (m_rendering_engine->run()) {

		fps_control.limit(device, &dtime);

		// Update client and server
		client->step(dtime);

		if (server != NULL)
			server->step(dtime);

		// End condition
		if (client->mediaReceived() && client->itemdefReceived() &&
				client->nodedefReceived()) {
			break;
		}

		// Error conditions
		if (!checkConnection())
			return false;

		if (client->getState() < LC_Init) {
			*error_message = gettext("Client disconnected");
			errorstream << *error_message << std::endl;
			return false;
		}

		if (input->cancelPressed()) {
			*aborted = true;
			infostream << "Connect aborted [Escape]" << std::endl;
			return false;
		}

		// Display status
		int progress = 25;

		if (!client->itemdefReceived()) {
			const wchar_t *text = wgettext("Item definitions...");
			progress = 25;
			m_rendering_engine->draw_load_screen(text, guienv, texture_src,
				dtime, progress);
			delete[] text;
		} else if (!client->nodedefReceived()) {
			const wchar_t *text = wgettext("Node definitions...");
			progress = 30;
			m_rendering_engine->draw_load_screen(text, guienv, texture_src,
				dtime, progress);
			delete[] text;
		} else {
			std::ostringstream message;
			std::fixed(message);
			message.precision(0);
			float receive = client->mediaReceiveProgress() * 100;
			message << gettext("Media...");
			if (receive > 0)
				message << " " << receive << "%";
			message.precision(2);

			if ((USE_CURL == 0) ||
					(!g_settings->getBool("enable_remote_media_server"))) {
				float cur = client->getCurRate();
				std::string cur_unit = gettext("KiB/s");

				if (cur > 900) {
					cur /= 1024.0;
					cur_unit = gettext("MiB/s");
				}

				message << " (" << cur << ' ' << cur_unit << ")";
			}

			progress = 30 + client->mediaReceiveProgress() * 35 + 0.5;
			m_rendering_engine->draw_load_screen(utf8_to_wide(message.str()), guienv,
				texture_src, dtime, progress);
		}
	}

	return true;
}


/****************************************************************************/
/****************************************************************************
 Run
 ****************************************************************************/
/****************************************************************************/

inline void Game::updateInteractTimers(f32 dtime)
{
	if (runData.nodig_delay_timer >= 0)
		runData.nodig_delay_timer -= dtime;

	if (runData.object_hit_delay_timer >= 0)
		runData.object_hit_delay_timer -= dtime;

	runData.time_from_last_punch += dtime;
}


/* returns false if game should exit, otherwise true
 */
inline bool Game::checkConnection()
{
	if (client->accessDenied()) {
		*error_message = fmtgettext("Access denied. Reason: %s", client->accessDeniedReason().c_str());
		*reconnect_requested = client->reconnectRequested();
		errorstream << *error_message << std::endl;
		return false;
	}

	return true;
}


/* returns false if game should exit, otherwise true
 */
inline bool Game::handleCallbacks()
{
	if (g_gamecallback->disconnect_requested) {
		g_gamecallback->disconnect_requested = false;
		return false;
	}

	if (g_gamecallback->changepassword_requested) {
		(new GUIPasswordChange(guienv, guiroot, -1,
				       &g_menumgr, client, texture_src))->drop();
		g_gamecallback->changepassword_requested = false;
	}

	if (g_gamecallback->changevolume_requested) {
		(new GUIVolumeChange(guienv, guiroot, -1,
				     &g_menumgr, texture_src))->drop();
		g_gamecallback->changevolume_requested = false;
	}

	if (g_gamecallback->keyconfig_requested) {
		(new GUIKeyChangeMenu(guienv, guiroot, -1,
				      &g_menumgr, texture_src))->drop();
		g_gamecallback->keyconfig_requested = false;
	}

	if (g_gamecallback->keyconfig_changed) {
		input->keycache.populate(); // update the cache with new settings
		g_gamecallback->keyconfig_changed = false;
	}

	return true;
}


void Game::processQueues()
{
	texture_src->processQueue();
	itemdef_manager->processQueue(client);
	shader_src->processQueue();
}

void Game::updateDebugState()
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	// debug UI and wireframe
	bool has_debug = client->checkPrivilege("debug");
	bool has_basic_debug = has_debug || (player->hud_flags & HUD_FLAG_BASIC_DEBUG);

	if (m_game_ui->m_flags.show_basic_debug) {
		if (!has_basic_debug)
			m_game_ui->m_flags.show_basic_debug = false;
	} else if (m_game_ui->m_flags.show_minimal_debug) {
		if (has_basic_debug)
			m_game_ui->m_flags.show_basic_debug = true;
	}
	if (!has_basic_debug)
		hud->disableBlockBounds();
	if (!has_debug)
		draw_control->show_wireframe = false;

	// noclip
	draw_control->allow_noclip = m_cache_enable_noclip && client->checkPrivilege("noclip");
}

void Game::updateProfilers(const RunStats &stats, const FpsControl &draw_times,
		f32 dtime)
{
	float profiler_print_interval =
			g_settings->getFloat("profiler_print_interval");
	bool print_to_log = true;

	if (profiler_print_interval == 0) {
		print_to_log = false;
		profiler_print_interval = 3;
	}

	if (profiler_interval.step(dtime, profiler_print_interval)) {
		if (print_to_log) {
			infostream << "Profiler:" << std::endl;
			g_profiler->print(infostream);
		}

		m_game_ui->updateProfiler();
		g_profiler->clear();
	}

	// Update update graphs
	g_profiler->graphAdd("Time non-rendering [us]",
		draw_times.busy_time - stats.drawtime);

	g_profiler->graphAdd("Sleep [us]", draw_times.sleep_time);
	g_profiler->graphAdd("FPS", 1.0f / dtime);
}

void Game::updateStats(RunStats *stats, const FpsControl &draw_times,
		f32 dtime)
{

	f32 jitter;
	Jitter *jp;

	/* Time average and jitter calculation
	 */
	jp = &stats->dtime_jitter;
	jp->avg = jp->avg * 0.96 + dtime * 0.04;

	jitter = dtime - jp->avg;

	if (jitter > jp->max)
		jp->max = jitter;

	jp->counter += dtime;

	if (jp->counter > 0.0) {
		jp->counter -= 3.0;
		jp->max_sample = jp->max;
		jp->max_fraction = jp->max_sample / (jp->avg + 0.001);
		jp->max = 0.0;
	}

	/* Busytime average and jitter calculation
	 */
	jp = &stats->busy_time_jitter;
	jp->avg = jp->avg + draw_times.getBusyMs() * 0.02;

	jitter = draw_times.getBusyMs() - jp->avg;

	if (jitter > jp->max)
		jp->max = jitter;
	if (jitter < jp->min)
		jp->min = jitter;

	jp->counter += dtime;

	if (jp->counter > 0.0) {
		jp->counter -= 3.0;
		jp->max_sample = jp->max;
		jp->min_sample = jp->min;
		jp->max = 0.0;
		jp->min = 0.0;
	}
}



/****************************************************************************
 Input handling
 ****************************************************************************/

void Game::processUserInput(f32 dtime)
{
	// Reset input if window not active or some menu is active
	if (!device->isWindowActive() || isMenuActive() || guienv->hasFocus(gui_chat_console)) {
		input->clear();
#ifdef HAVE_TOUCHSCREENGUI
		g_touchscreengui->hide();
#endif
	}
#ifdef HAVE_TOUCHSCREENGUI
	else if (g_touchscreengui) {
		/* on touchscreengui step may generate own input events which ain't
		 * what we want in case we just did clear them */
		g_touchscreengui->show();
		g_touchscreengui->step(dtime);
	}
#endif

	if (!guienv->hasFocus(gui_chat_console) && gui_chat_console->isOpen()) {
		gui_chat_console->closeConsoleAtOnce();
	}

	// Input handler step() (used by the random input generator)
	input->step(dtime);

#ifdef __ANDROID__
	auto formspec = m_game_ui->getFormspecGUI();
	if (formspec)
		formspec->getAndroidUIInput();
	else
		handleAndroidChatInput();
#endif

	// Increase timer for double tap of "keymap_jump"
	if (m_cache_doubletap_jump && runData.jump_timer_up <= 0.2f)
		runData.jump_timer_up += dtime;
	if (m_cache_doubletap_jump && runData.jump_timer_down <= 0.4f)
		runData.jump_timer_down += dtime;

	processKeyInput();
	processItemSelection(&runData.new_playeritem);
}


void Game::processKeyInput()
{
	if (wasKeyDown(KeyType::DROP)) {
		dropSelectedItem(isKeyDown(KeyType::SNEAK));
	} else if (wasKeyDown(KeyType::AUTOFORWARD)) {
		toggleAutoforward();
	} else if (wasKeyDown(KeyType::BACKWARD)) {
		if (g_settings->getBool("continuous_forward"))
			toggleAutoforward();
	} else if (wasKeyDown(KeyType::INVENTORY)) {
		openInventory();
	} else if (input->cancelPressed()) {
#ifdef __ANDROID__
		m_android_chat_open = false;
#endif
		if (!gui_chat_console->isOpenInhibited()) {
			showPauseMenu();
		}
	} else if (wasKeyDown(KeyType::CHAT)) {
		openConsole(0.2, L"");
	} else if (wasKeyDown(KeyType::CMD)) {
		openConsole(0.2, L"/");
	} else if (wasKeyDown(KeyType::CMD_LOCAL)) {
		if (client->modsLoaded())
			openConsole(0.2, L".");
		else
			m_game_ui->showTranslatedStatusText("Client side scripting is disabled");
	} else if (wasKeyDown(KeyType::CONSOLE)) {
		openConsole(core::clamp(g_settings->getFloat("console_height"), 0.1f, 1.0f));
	} else if (wasKeyDown(KeyType::FREEMOVE)) {
		toggleFreeMove();
	} else if (wasKeyDown(KeyType::JUMP)) {
		toggleFreeMoveAlt();
	} else if (wasKeyDown(KeyType::PITCHMOVE)) {
		togglePitchMove();
	} else if (wasKeyDown(KeyType::FASTMOVE)) {
		toggleFast();
	} else if (wasKeyDown(KeyType::NOCLIP)) {
		toggleNoClip();
#if USE_SOUND
	} else if (wasKeyDown(KeyType::MUTE)) {
		if (g_settings->getBool("enable_sound")) {
			bool new_mute_sound = !g_settings->getBool("mute_sound");
			g_settings->setBool("mute_sound", new_mute_sound);
			if (new_mute_sound)
				m_game_ui->showTranslatedStatusText("Sound muted");
			else
				m_game_ui->showTranslatedStatusText("Sound unmuted");
		} else {
			m_game_ui->showTranslatedStatusText("Sound system is disabled");
		}
	} else if (wasKeyDown(KeyType::INC_VOLUME)) {
		if (g_settings->getBool("enable_sound")) {
			float new_volume = g_settings->getFloat("sound_volume", 0.0f, 0.9f) + 0.1f;
			g_settings->setFloat("sound_volume", new_volume);
			std::wstring msg = fwgettext("Volume changed to %d%%", myround(new_volume * 100));
			m_game_ui->showStatusText(msg);
		} else {
			m_game_ui->showTranslatedStatusText("Sound system is disabled");
		}
	} else if (wasKeyDown(KeyType::DEC_VOLUME)) {
		if (g_settings->getBool("enable_sound")) {
			float new_volume = g_settings->getFloat("sound_volume", 0.1f, 1.0f) - 0.1f;
			g_settings->setFloat("sound_volume", new_volume);
			std::wstring msg = fwgettext("Volume changed to %d%%", myround(new_volume * 100));
			m_game_ui->showStatusText(msg);
		} else {
			m_game_ui->showTranslatedStatusText("Sound system is disabled");
		}
#else
	} else if (wasKeyDown(KeyType::MUTE) || wasKeyDown(KeyType::INC_VOLUME)
			|| wasKeyDown(KeyType::DEC_VOLUME)) {
		m_game_ui->showTranslatedStatusText("Sound system is not supported on this build");
#endif
	} else if (wasKeyDown(KeyType::CINEMATIC)) {
		toggleCinematic();
	} else if (wasKeyDown(KeyType::SCREENSHOT)) {
		client->makeScreenshot();
	} else if (wasKeyDown(KeyType::TOGGLE_BLOCK_BOUNDS)) {
		toggleBlockBounds();
	} else if (wasKeyDown(KeyType::TOGGLE_HUD)) {
		m_game_ui->toggleHud();
	} else if (wasKeyDown(KeyType::MINIMAP)) {
		toggleMinimap(isKeyDown(KeyType::SNEAK));
	} else if (wasKeyDown(KeyType::TOGGLE_CHAT)) {
		m_game_ui->toggleChat();
	} else if (wasKeyDown(KeyType::TOGGLE_FOG)) {
		toggleFog();
	} else if (wasKeyDown(KeyType::TOGGLE_UPDATE_CAMERA)) {
		toggleUpdateCamera();
	} else if (wasKeyDown(KeyType::TOGGLE_DEBUG)) {
		toggleDebug();
	} else if (wasKeyDown(KeyType::TOGGLE_PROFILER)) {
		m_game_ui->toggleProfiler();
	} else if (wasKeyDown(KeyType::INCREASE_VIEWING_RANGE)) {
		increaseViewRange();
	} else if (wasKeyDown(KeyType::DECREASE_VIEWING_RANGE)) {
		decreaseViewRange();
	} else if (wasKeyDown(KeyType::RANGESELECT)) {
		toggleFullViewRange();
	} else if (wasKeyDown(KeyType::ZOOM)) {
		checkZoomEnabled();
	} else if (wasKeyDown(KeyType::QUICKTUNE_NEXT)) {
		quicktune->next();
	} else if (wasKeyDown(KeyType::QUICKTUNE_PREV)) {
		quicktune->prev();
	} else if (wasKeyDown(KeyType::QUICKTUNE_INC)) {
		quicktune->inc();
	} else if (wasKeyDown(KeyType::QUICKTUNE_DEC)) {
		quicktune->dec();
	}

	if (!isKeyDown(KeyType::JUMP) && runData.reset_jump_timer) {
		runData.reset_jump_timer = false;
		runData.jump_timer_up = 0.0f;
	}

	if (quicktune->hasMessage()) {
		m_game_ui->showStatusText(utf8_to_wide(quicktune->getMessage()));
	}
}

void Game::processItemSelection(u16 *new_playeritem)
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	/* Item selection using mouse wheel
	 */
	*new_playeritem = player->getWieldIndex();
	s32 wheel = input->getMouseWheel();
	u16 max_item = MYMIN(PLAYER_INVENTORY_SIZE - 1,
		    player->hud_hotbar_itemcount - 1);

	s32 dir = wheel;

	if (wasKeyDown(KeyType::HOTBAR_NEXT))
		dir = -1;

	if (wasKeyDown(KeyType::HOTBAR_PREV))
		dir = 1;

	if (dir < 0)
		*new_playeritem = *new_playeritem < max_item ? *new_playeritem + 1 : 0;
	else if (dir > 0)
		*new_playeritem = *new_playeritem > 0 ? *new_playeritem - 1 : max_item;
	// else dir == 0

	/* Item selection using hotbar slot keys
	 */
	for (u16 i = 0; i <= max_item; i++) {
		if (wasKeyDown((GameKeyType) (KeyType::SLOT_1 + i))) {
			*new_playeritem = i;
			break;
		}
	}

	// Clamp selection again in case it wasn't changed but max_item was
	*new_playeritem = MYMIN(*new_playeritem, max_item);
}


void Game::dropSelectedItem(bool single_item)
{
	IDropAction *a = new IDropAction();
	a->count = single_item ? 1 : 0;
	a->from_inv.setCurrentPlayer();
	a->from_list = "main";
	a->from_i = client->getEnv().getLocalPlayer()->getWieldIndex();
	client->inventoryAction(a);
}


void Game::openInventory()
{
	/*
	 * Don't permit to open inventory is CAO or player doesn't exists.
	 * This prevent showing an empty inventory at player load
	 */

	LocalPlayer *player = client->getEnv().getLocalPlayer();
	if (!player || !player->getCAO())
		return;

	infostream << "Game: Launching inventory" << std::endl;

	PlayerInventoryFormSource *fs_src = new PlayerInventoryFormSource(client);

	InventoryLocation inventoryloc;
	inventoryloc.setCurrentPlayer();

	if (client->modsLoaded() && client->getScript()->on_inventory_open(fs_src->m_client->getInventory(inventoryloc))) {
		delete fs_src;
		return;
	}

	if (fs_src->getForm().empty()) {
		delete fs_src;
		return;
	}

	TextDest *txt_dst = new TextDestPlayerInventory(client);
	auto *&formspec = m_game_ui->updateFormspec("");
	GUIFormSpecMenu::create(formspec, client, m_rendering_engine->get_gui_env(),
		&input->joystick, fs_src, txt_dst, client->getFormspecPrepend(), sound);

	formspec->setFormSpec(fs_src->getForm(), inventoryloc);
}


void Game::openConsole(float scale, const wchar_t *line)
{
	assert(scale > 0.0f && scale <= 1.0f);

#ifdef __ANDROID__
	porting::showInputDialog(gettext("ok"), "", "", 2);
	m_android_chat_open = true;
#else
	if (gui_chat_console->isOpenInhibited())
		return;
	gui_chat_console->openConsole(scale);
	if (line) {
		gui_chat_console->setCloseOnEnter(true);
		gui_chat_console->replaceAndAddToHistory(line);
	}
#endif
}

#ifdef __ANDROID__
void Game::handleAndroidChatInput()
{
	if (m_android_chat_open && porting::getInputDialogState() == 0) {
		std::string text = porting::getInputDialogValue();
		client->typeChatMessage(utf8_to_wide(text));
		m_android_chat_open = false;
	}
}
#endif


void Game::toggleFreeMove()
{
	bool free_move = !g_settings->getBool("free_move");
	g_settings->set("free_move", bool_to_cstr(free_move));

	if (free_move) {
		if (client->checkPrivilege("fly")) {
			m_game_ui->showTranslatedStatusText("Fly mode enabled");
		} else {
			m_game_ui->showTranslatedStatusText("Fly mode enabled (note: no 'fly' privilege)");
		}
	} else {
		m_game_ui->showTranslatedStatusText("Fly mode disabled");
	}
}

void Game::toggleFreeMoveAlt()
{
	if (!runData.reset_jump_timer) {
		runData.jump_timer_down_before = runData.jump_timer_down;
		runData.jump_timer_down = 0.0f;
	}

	// key down (0.2 s max.), then key up (0.2 s max.), then key down
	if (m_cache_doubletap_jump && runData.jump_timer_up < 0.2f &&
			runData.jump_timer_down_before < 0.4f) // 0.2 + 0.2
		toggleFreeMove();

	runData.reset_jump_timer = true;
}


void Game::togglePitchMove()
{
	bool pitch_move = !g_settings->getBool("pitch_move");
	g_settings->set("pitch_move", bool_to_cstr(pitch_move));

	if (pitch_move) {
		m_game_ui->showTranslatedStatusText("Pitch move mode enabled");
	} else {
		m_game_ui->showTranslatedStatusText("Pitch move mode disabled");
	}
}


void Game::toggleFast()
{
	bool fast_move = !g_settings->getBool("fast_move");
	bool has_fast_privs = client->checkPrivilege("fast");
	g_settings->set("fast_move", bool_to_cstr(fast_move));

	if (fast_move) {
		if (has_fast_privs) {
			m_game_ui->showTranslatedStatusText("Fast mode enabled");
		} else {
			m_game_ui->showTranslatedStatusText("Fast mode enabled (note: no 'fast' privilege)");
		}
	} else {
		m_game_ui->showTranslatedStatusText("Fast mode disabled");
	}

#ifdef HAVE_TOUCHSCREENGUI
	m_cache_hold_aux1 = fast_move && has_fast_privs;
#endif
}


void Game::toggleNoClip()
{
	bool noclip = !g_settings->getBool("noclip");
	g_settings->set("noclip", bool_to_cstr(noclip));

	if (noclip) {
		if (client->checkPrivilege("noclip")) {
			m_game_ui->showTranslatedStatusText("Noclip mode enabled");
		} else {
			m_game_ui->showTranslatedStatusText("Noclip mode enabled (note: no 'noclip' privilege)");
		}
	} else {
		m_game_ui->showTranslatedStatusText("Noclip mode disabled");
	}
}

void Game::toggleCinematic()
{
	bool cinematic = !g_settings->getBool("cinematic");
	g_settings->set("cinematic", bool_to_cstr(cinematic));

	if (cinematic)
		m_game_ui->showTranslatedStatusText("Cinematic mode enabled");
	else
		m_game_ui->showTranslatedStatusText("Cinematic mode disabled");
}

void Game::toggleBlockBounds()
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();
	if (!(client->checkPrivilege("debug") || (player->hud_flags & HUD_FLAG_BASIC_DEBUG))) {
		m_game_ui->showTranslatedStatusText("Can't show block bounds (disabled by mod or game)");
		return;
	}
	enum Hud::BlockBoundsMode newmode = hud->toggleBlockBounds();
	switch (newmode) {
		case Hud::BLOCK_BOUNDS_OFF:
			m_game_ui->showTranslatedStatusText("Block bounds hidden");
			break;
		case Hud::BLOCK_BOUNDS_CURRENT:
			m_game_ui->showTranslatedStatusText("Block bounds shown for current block");
			break;
		case Hud::BLOCK_BOUNDS_NEAR:
			m_game_ui->showTranslatedStatusText("Block bounds shown for nearby blocks");
			break;
		case Hud::BLOCK_BOUNDS_MAX:
			m_game_ui->showTranslatedStatusText("Block bounds shown for all blocks");
			break;
		default:
			break;
	}
}

// Autoforward by toggling continuous forward.
void Game::toggleAutoforward()
{
	bool autorun_enabled = !g_settings->getBool("continuous_forward");
	g_settings->set("continuous_forward", bool_to_cstr(autorun_enabled));

	if (autorun_enabled)
		m_game_ui->showTranslatedStatusText("Automatic forward enabled");
	else
		m_game_ui->showTranslatedStatusText("Automatic forward disabled");
}

void Game::toggleMinimap(bool shift_pressed)
{
	if (!mapper || !m_game_ui->m_flags.show_hud || !g_settings->getBool("enable_minimap"))
		return;

	if (shift_pressed)
		mapper->toggleMinimapShape();
	else
		mapper->nextMode();

	// TODO: When legacy minimap is deprecated, keep only HUD minimap stuff here

	// Not so satisying code to keep compatibility with old fixed mode system
	// -->
	u32 hud_flags = client->getEnv().getLocalPlayer()->hud_flags;

	if (!(hud_flags & HUD_FLAG_MINIMAP_VISIBLE)) {
		m_game_ui->m_flags.show_minimap = false;
	} else {

	// If radar is disabled, try to find a non radar mode or fall back to 0
		if (!(hud_flags & HUD_FLAG_MINIMAP_RADAR_VISIBLE))
			while (mapper->getModeIndex() &&
					mapper->getModeDef().type == MINIMAP_TYPE_RADAR)
				mapper->nextMode();

		m_game_ui->m_flags.show_minimap = mapper->getModeDef().type !=
				MINIMAP_TYPE_OFF;
	}
	// <--
	// End of 'not so satifying code'
	if ((hud_flags & HUD_FLAG_MINIMAP_VISIBLE) ||
			(hud && hud->hasElementOfType(HUD_ELEM_MINIMAP)))
		m_game_ui->showStatusText(utf8_to_wide(mapper->getModeDef().label));
	else
		m_game_ui->showTranslatedStatusText("Minimap currently disabled by game or mod");
}

void Game::toggleFog()
{
	bool fog_enabled = g_settings->getBool("enable_fog");
	g_settings->setBool("enable_fog", !fog_enabled);
	if (fog_enabled)
		m_game_ui->showTranslatedStatusText("Fog disabled");
	else
		m_game_ui->showTranslatedStatusText("Fog enabled");
}


void Game::toggleDebug()
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();
	bool has_debug = client->checkPrivilege("debug");
	bool has_basic_debug = has_debug || (player->hud_flags & HUD_FLAG_BASIC_DEBUG);
	// Initial: No debug info
	// 1x toggle: Debug text
	// 2x toggle: Debug text with profiler graph
	// 3x toggle: Debug text and wireframe (needs "debug" priv)
	// Next toggle: Back to initial
	//
	// The debug text can be in 2 modes: minimal and basic.
	// * Minimal: Only technical client info that not gameplay-relevant
	// * Basic: Info that might give gameplay advantage, e.g. pos, angle
	// Basic mode is used when player has the debug HUD flag set,
	// otherwise the Minimal mode is used.
	if (!m_game_ui->m_flags.show_minimal_debug) {
		m_game_ui->m_flags.show_minimal_debug = true;
		if (has_basic_debug)
			m_game_ui->m_flags.show_basic_debug = true;
		m_game_ui->m_flags.show_profiler_graph = false;
		draw_control->show_wireframe = false;
		m_game_ui->showTranslatedStatusText("Debug info shown");
	} else if (!m_game_ui->m_flags.show_profiler_graph && !draw_control->show_wireframe) {
		if (has_basic_debug)
			m_game_ui->m_flags.show_basic_debug = true;
		m_game_ui->m_flags.show_profiler_graph = true;
		m_game_ui->showTranslatedStatusText("Profiler graph shown");
	} else if (!draw_control->show_wireframe && client->checkPrivilege("debug")) {
		if (has_basic_debug)
			m_game_ui->m_flags.show_basic_debug = true;
		m_game_ui->m_flags.show_profiler_graph = false;
		draw_control->show_wireframe = true;
		m_game_ui->showTranslatedStatusText("Wireframe shown");
	} else {
		m_game_ui->m_flags.show_minimal_debug = false;
		m_game_ui->m_flags.show_basic_debug = false;
		m_game_ui->m_flags.show_profiler_graph = false;
		draw_control->show_wireframe = false;
		if (has_debug) {
			m_game_ui->showTranslatedStatusText("Debug info, profiler graph, and wireframe hidden");
		} else {
			m_game_ui->showTranslatedStatusText("Debug info and profiler graph hidden");
		}
	}
}


void Game::toggleUpdateCamera()
{
	m_flags.disable_camera_update = !m_flags.disable_camera_update;
	if (m_flags.disable_camera_update)
		m_game_ui->showTranslatedStatusText("Camera update disabled");
	else
		m_game_ui->showTranslatedStatusText("Camera update enabled");
}


void Game::increaseViewRange()
{
	s16 range = g_settings->getS16("viewing_range");
	s16 range_new = range + 10;

	if (range_new > 4000) {
		range_new = 4000;
		std::wstring msg = fwgettext("Viewing range is at maximum: %d", range_new);
		m_game_ui->showStatusText(msg);
	} else {
		std::wstring msg = fwgettext("Viewing range changed to %d", range_new);
		m_game_ui->showStatusText(msg);
	}
	g_settings->set("viewing_range", itos(range_new));
}


void Game::decreaseViewRange()
{
	s16 range = g_settings->getS16("viewing_range");
	s16 range_new = range - 10;

	if (range_new < 20) {
		range_new = 20;
		std::wstring msg = fwgettext("Viewing range is at minimum: %d", range_new);
		m_game_ui->showStatusText(msg);
	} else {
		std::wstring msg = fwgettext("Viewing range changed to %d", range_new);
		m_game_ui->showStatusText(msg);
	}
	g_settings->set("viewing_range", itos(range_new));
}


void Game::toggleFullViewRange()
{
	draw_control->range_all = !draw_control->range_all;
	if (draw_control->range_all)
		m_game_ui->showTranslatedStatusText("Enabled unlimited viewing range");
	else
		m_game_ui->showTranslatedStatusText("Disabled unlimited viewing range");
}


void Game::checkZoomEnabled()
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();
	if (player->getZoomFOV() < 0.001f || player->getFov().fov > 0.0f)
		m_game_ui->showTranslatedStatusText("Zoom currently disabled by game or mod");
}

void Game::updateCameraDirection(CameraOrientation *cam, float dtime)
{
#if !defined(__ANDROID__) && IRRLICHT_VERSION_MT_REVISION >= 9
	if (isMenuActive())
		device->getCursorControl()->setRelativeMode(false);
	else
		device->getCursorControl()->setRelativeMode(true);
#endif

	if ((device->isWindowActive() && device->isWindowFocused()
			&& !isMenuActive()) || input->isRandom()) {

#ifndef __ANDROID__
		if (!input->isRandom()) {
			// Mac OSX gets upset if this is set every frame
			if (device->getCursorControl()->isVisible())
				device->getCursorControl()->setVisible(false);
		}
#endif

		if (m_first_loop_after_window_activation) {
			m_first_loop_after_window_activation = false;

			input->setMousePos(driver->getScreenSize().Width / 2,
				driver->getScreenSize().Height / 2);
		} else {
			updateCameraOrientation(cam, dtime);
		}

	} else {

#ifndef ANDROID
		// Mac OSX gets upset if this is set every frame
		if (!device->getCursorControl()->isVisible())
			device->getCursorControl()->setVisible(true);
#endif

		m_first_loop_after_window_activation = true;

	}
}

// Get the factor to multiply with sensitivity to get the same mouse/joystick
// responsiveness independently of FOV.
f32 Game::getSensitivityScaleFactor() const
{
	f32 fov_y = client->getCamera()->getFovY();

	// Multiply by a constant such that it becomes 1.0 at 72 degree FOV and
	// 16:9 aspect ratio to minimize disruption of existing sensitivity
	// settings.
	return tan(fov_y / 2.0f) * 1.3763818698f;
}

void Game::updateCameraOrientation(CameraOrientation *cam, float dtime)
{
#ifdef HAVE_TOUCHSCREENGUI
	if (g_touchscreengui) {
		cam->camera_yaw   += g_touchscreengui->getYawChange();
		cam->camera_pitch  = g_touchscreengui->getPitch();
	} else {
#endif
		v2s32 center(driver->getScreenSize().Width / 2, driver->getScreenSize().Height / 2);
		v2s32 dist = input->getMousePos() - center;

		if (m_invert_mouse || camera->getCameraMode() == CAMERA_MODE_THIRD_FRONT) {
			dist.Y = -dist.Y;
		}

		f32 sens_scale = getSensitivityScaleFactor();
		cam->camera_yaw   -= dist.X * m_cache_mouse_sensitivity * sens_scale;
		cam->camera_pitch += dist.Y * m_cache_mouse_sensitivity * sens_scale;

		if (dist.X != 0 || dist.Y != 0)
			input->setMousePos(center.X, center.Y);
#ifdef HAVE_TOUCHSCREENGUI
	}
#endif

	if (m_cache_enable_joysticks) {
		f32 sens_scale = getSensitivityScaleFactor();
		f32 c = m_cache_joystick_frustum_sensitivity * dtime * sens_scale;
		cam->camera_yaw -= input->joystick.getAxisWithoutDead(JA_FRUSTUM_HORIZONTAL) * c;
		cam->camera_pitch += input->joystick.getAxisWithoutDead(JA_FRUSTUM_VERTICAL) * c;
	}

	cam->camera_pitch = rangelim(cam->camera_pitch, -89.5, 89.5);
}


void Game::updatePlayerControl(const CameraOrientation &cam)
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	//TimeTaker tt("update player control", NULL, PRECISION_NANO);

	PlayerControl control(
		isKeyDown(KeyType::FORWARD),
		isKeyDown(KeyType::BACKWARD),
		isKeyDown(KeyType::LEFT),
		isKeyDown(KeyType::RIGHT),
		isKeyDown(KeyType::JUMP) || player->getAutojump(),
		isKeyDown(KeyType::AUX1),
		isKeyDown(KeyType::SNEAK),
		isKeyDown(KeyType::ZOOM),
		isKeyDown(KeyType::DIG),
		isKeyDown(KeyType::PLACE),
		cam.camera_pitch,
		cam.camera_yaw,
		input->getMovementSpeed(),
		input->getMovementDirection()
	);

	// autoforward if set: move at maximum speed
	if (player->getPlayerSettings().continuous_forward &&
			client->activeObjectsReceived() && !player->isDead()) {
		control.movement_speed = 1.0f;
		// sideways movement only
		float dx = sin(control.movement_direction);
		control.movement_direction = atan2(dx, 1.0f);
	}

#ifdef HAVE_TOUCHSCREENGUI
	/* For touch, simulate holding down AUX1 (fast move) if the user has
	 * the fast_move setting toggled on. If there is an aux1 key defined for
	 * touch then its meaning is inverted (i.e. holding aux1 means walk and
	 * not fast)
	 */
	if (m_cache_hold_aux1) {
		control.aux1 = control.aux1 ^ true;
	}
#endif

	client->setPlayerControl(control);

	//tt.stop();
}


inline void Game::step(f32 dtime)
{
	if (server)
		server->step(dtime);

	client->step(dtime);
}

static void pauseNodeAnimation(PausedNodesList &paused, scene::ISceneNode *node) {
	if (!node)
		return;
	for (auto &&child: node->getChildren())
		pauseNodeAnimation(paused, child);
	if (node->getType() != scene::ESNT_ANIMATED_MESH)
		return;
	auto animated_node = static_cast<scene::IAnimatedMeshSceneNode *>(node);
	float speed = animated_node->getAnimationSpeed();
	if (!speed)
		return;
	paused.push_back({grab(animated_node), speed});
	animated_node->setAnimationSpeed(0.0f);
}

void Game::pauseAnimation()
{
	pauseNodeAnimation(paused_animated_nodes, smgr->getRootSceneNode());
}

void Game::resumeAnimation()
{
	for (auto &&pair: paused_animated_nodes)
		pair.first->setAnimationSpeed(pair.second);
	paused_animated_nodes.clear();
}

const ClientEventHandler Game::clientEventHandler[CLIENTEVENT_MAX] = {
	{&Game::handleClientEvent_None},
	{&Game::handleClientEvent_PlayerDamage},
	{&Game::handleClientEvent_PlayerForceMove},
	{&Game::handleClientEvent_Deathscreen},
	{&Game::handleClientEvent_ShowFormSpec},
	{&Game::handleClientEvent_ShowLocalFormSpec},
	{&Game::handleClientEvent_HandleParticleEvent},
	{&Game::handleClientEvent_HandleParticleEvent},
	{&Game::handleClientEvent_HandleParticleEvent},
	{&Game::handleClientEvent_HudAdd},
	{&Game::handleClientEvent_HudRemove},
	{&Game::handleClientEvent_HudChange},
	{&Game::handleClientEvent_SetSky},
	{&Game::handleClientEvent_SetSun},
	{&Game::handleClientEvent_SetMoon},
	{&Game::handleClientEvent_SetStars},
	{&Game::handleClientEvent_OverrideDayNigthRatio},
	{&Game::handleClientEvent_CloudParams},
};

void Game::handleClientEvent_None(ClientEvent *event, CameraOrientation *cam)
{
	FATAL_ERROR("ClientEvent type None received");
}

void Game::handleClientEvent_PlayerDamage(ClientEvent *event, CameraOrientation *cam)
{
	if (client->modsLoaded())
		client->getScript()->on_damage_taken(event->player_damage.amount);

	if (!event->player_damage.effect)
		return;

	// Damage flash and hurt tilt are not used at death
	if (client->getHP() > 0) {
		LocalPlayer *player = client->getEnv().getLocalPlayer();

		f32 hp_max = player->getCAO() ?
			player->getCAO()->getProperties().hp_max : PLAYER_MAX_HP_DEFAULT;
		f32 damage_ratio = event->player_damage.amount / hp_max;

		runData.damage_flash += 95.0f + 64.f * damage_ratio;
		runData.damage_flash = MYMIN(runData.damage_flash, 127.0f);

		player->hurt_tilt_timer = 1.5f;
		player->hurt_tilt_strength =
			rangelim(damage_ratio * 5.0f, 1.0f, 4.0f);
	}

	// Play damage sound
	client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::PLAYER_DAMAGE));
}

void Game::handleClientEvent_PlayerForceMove(ClientEvent *event, CameraOrientation *cam)
{
	cam->camera_yaw = event->player_force_move.yaw;
	cam->camera_pitch = event->player_force_move.pitch;
}

void Game::handleClientEvent_Deathscreen(ClientEvent *event, CameraOrientation *cam)
{
	// If client scripting is enabled, deathscreen is handled by CSM code in
	// builtin/client/init.lua
	if (client->modsLoaded())
		client->getScript()->on_death();
	else
		showDeathFormspec();

	/* Handle visualization */
	LocalPlayer *player = client->getEnv().getLocalPlayer();
	runData.damage_flash = 0;
	player->hurt_tilt_timer = 0;
	player->hurt_tilt_strength = 0;
}

void Game::handleClientEvent_ShowFormSpec(ClientEvent *event, CameraOrientation *cam)
{
	if (event->show_formspec.formspec->empty()) {
		auto formspec = m_game_ui->getFormspecGUI();
		if (formspec && (event->show_formspec.formname->empty()
				|| *(event->show_formspec.formname) == m_game_ui->getFormspecName())) {
			formspec->quitMenu();
		}
	} else {
		FormspecFormSource *fs_src =
			new FormspecFormSource(*(event->show_formspec.formspec));
		TextDestPlayerInventory *txt_dst =
			new TextDestPlayerInventory(client, *(event->show_formspec.formname));

		auto *&formspec = m_game_ui->updateFormspec(*(event->show_formspec.formname));
		GUIFormSpecMenu::create(formspec, client, m_rendering_engine->get_gui_env(),
			&input->joystick, fs_src, txt_dst, client->getFormspecPrepend(), sound);
	}

	delete event->show_formspec.formspec;
	delete event->show_formspec.formname;
}

void Game::handleClientEvent_ShowLocalFormSpec(ClientEvent *event, CameraOrientation *cam)
{
	FormspecFormSource *fs_src = new FormspecFormSource(*event->show_formspec.formspec);
	LocalFormspecHandler *txt_dst =
		new LocalFormspecHandler(*event->show_formspec.formname, client);
	GUIFormSpecMenu::create(m_game_ui->getFormspecGUI(), client, m_rendering_engine->get_gui_env(),
			&input->joystick, fs_src, txt_dst, client->getFormspecPrepend(), sound);

	delete event->show_formspec.formspec;
	delete event->show_formspec.formname;
}

void Game::handleClientEvent_HandleParticleEvent(ClientEvent *event,
		CameraOrientation *cam)
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();
	client->getParticleManager()->handleParticleEvent(event, client, player);
}

void Game::handleClientEvent_HudAdd(ClientEvent *event, CameraOrientation *cam)
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	u32 server_id = event->hudadd->server_id;
	// ignore if we already have a HUD with that ID
	auto i = m_hud_server_to_client.find(server_id);
	if (i != m_hud_server_to_client.end()) {
		delete event->hudadd;
		return;
	}

	HudElement *e = new HudElement;
	e->type   = static_cast<HudElementType>(event->hudadd->type);
	e->pos    = event->hudadd->pos;
	e->name   = event->hudadd->name;
	e->scale  = event->hudadd->scale;
	e->text   = event->hudadd->text;
	e->number = event->hudadd->number;
	e->item   = event->hudadd->item;
	e->dir    = event->hudadd->dir;
	e->align  = event->hudadd->align;
	e->offset = event->hudadd->offset;
	e->world_pos = event->hudadd->world_pos;
	e->size      = event->hudadd->size;
	e->z_index   = event->hudadd->z_index;
	e->text2     = event->hudadd->text2;
	e->style     = event->hudadd->style;
	m_hud_server_to_client[server_id] = player->addHud(e);

	delete event->hudadd;
}

void Game::handleClientEvent_HudRemove(ClientEvent *event, CameraOrientation *cam)
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	auto i = m_hud_server_to_client.find(event->hudrm.id);
	if (i != m_hud_server_to_client.end()) {
		HudElement *e = player->removeHud(i->second);
		delete e;
		m_hud_server_to_client.erase(i);
	}

}

void Game::handleClientEvent_HudChange(ClientEvent *event, CameraOrientation *cam)
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	HudElement *e = nullptr;

	auto i = m_hud_server_to_client.find(event->hudchange->id);
	if (i != m_hud_server_to_client.end()) {
		e = player->getHud(i->second);
	}

	if (e == nullptr) {
		delete event->hudchange;
		return;
	}

#define CASE_SET(statval, prop, dataprop) \
	case statval: \
		e->prop = event->hudchange->dataprop; \
		break

	switch (event->hudchange->stat) {
		CASE_SET(HUD_STAT_POS, pos, v2fdata);

		CASE_SET(HUD_STAT_NAME, name, sdata);

		CASE_SET(HUD_STAT_SCALE, scale, v2fdata);

		CASE_SET(HUD_STAT_TEXT, text, sdata);

		CASE_SET(HUD_STAT_NUMBER, number, data);

		CASE_SET(HUD_STAT_ITEM, item, data);

		CASE_SET(HUD_STAT_DIR, dir, data);

		CASE_SET(HUD_STAT_ALIGN, align, v2fdata);

		CASE_SET(HUD_STAT_OFFSET, offset, v2fdata);

		CASE_SET(HUD_STAT_WORLD_POS, world_pos, v3fdata);

		CASE_SET(HUD_STAT_SIZE, size, v2s32data);

		CASE_SET(HUD_STAT_Z_INDEX, z_index, data);

		CASE_SET(HUD_STAT_TEXT2, text2, sdata);

		CASE_SET(HUD_STAT_STYLE, style, data);
	}

#undef CASE_SET

	delete event->hudchange;
}

void Game::handleClientEvent_SetSky(ClientEvent *event, CameraOrientation *cam)
{
	sky->setVisible(false);
	// Whether clouds are visible in front of a custom skybox.
	sky->setCloudsEnabled(event->set_sky->clouds);

	if (skybox) {
		skybox->remove();
		skybox = NULL;
	}
	// Clear the old textures out in case we switch rendering type.
	sky->clearSkyboxTextures();
	// Handle according to type
	if (event->set_sky->type == "regular") {
		// Shows the mesh skybox
		sky->setVisible(true);
		// Update mesh based skybox colours if applicable.
		sky->setSkyColors(event->set_sky->sky_color);
		sky->setHorizonTint(
			event->set_sky->fog_sun_tint,
			event->set_sky->fog_moon_tint,
			event->set_sky->fog_tint_type
		);
	} else if (event->set_sky->type == "skybox" &&
			event->set_sky->textures.size() == 6) {
		// Disable the dyanmic mesh skybox:
		sky->setVisible(false);
		// Set fog colors:
		sky->setFallbackBgColor(event->set_sky->bgcolor);
		// Set sunrise and sunset fog tinting:
		sky->setHorizonTint(
			event->set_sky->fog_sun_tint,
			event->set_sky->fog_moon_tint,
			event->set_sky->fog_tint_type
		);
		// Add textures to skybox.
		for (int i = 0; i < 6; i++)
			sky->addTextureToSkybox(event->set_sky->textures[i], i, texture_src);
	} else {
		// Handle everything else as plain color.
		if (event->set_sky->type != "plain")
			infostream << "Unknown sky type: "
				<< (event->set_sky->type) << std::endl;
		sky->setVisible(false);
		sky->setFallbackBgColor(event->set_sky->bgcolor);
		// Disable directional sun/moon tinting on plain or invalid skyboxes.
		sky->setHorizonTint(
			event->set_sky->bgcolor,
			event->set_sky->bgcolor,
			"custom"
		);
	}

	delete event->set_sky;
}

void Game::handleClientEvent_SetSun(ClientEvent *event, CameraOrientation *cam)
{
	sky->setSunVisible(event->sun_params->visible);
	sky->setSunTexture(event->sun_params->texture,
		event->sun_params->tonemap, texture_src);
	sky->setSunScale(event->sun_params->scale);
	sky->setSunriseVisible(event->sun_params->sunrise_visible);
	sky->setSunriseTexture(event->sun_params->sunrise, texture_src);
	delete event->sun_params;
}

void Game::handleClientEvent_SetMoon(ClientEvent *event, CameraOrientation *cam)
{
	sky->setMoonVisible(event->moon_params->visible);
	sky->setMoonTexture(event->moon_params->texture,
		event->moon_params->tonemap, texture_src);
	sky->setMoonScale(event->moon_params->scale);
	delete event->moon_params;
}

void Game::handleClientEvent_SetStars(ClientEvent *event, CameraOrientation *cam)
{
	sky->setStarsVisible(event->star_params->visible);
	sky->setStarCount(event->star_params->count);
	sky->setStarColor(event->star_params->starcolor);
	sky->setStarScale(event->star_params->scale);
	sky->setStarDayOpacity(event->star_params->day_opacity);
	delete event->star_params;
}

void Game::handleClientEvent_OverrideDayNigthRatio(ClientEvent *event,
		CameraOrientation *cam)
{
	client->getEnv().setDayNightRatioOverride(
		event->override_day_night_ratio.do_override,
		event->override_day_night_ratio.ratio_f * 1000.0f);
}

void Game::handleClientEvent_CloudParams(ClientEvent *event, CameraOrientation *cam)
{
	if (!clouds)
		return;

	clouds->setDensity(event->cloud_params.density);
	clouds->setColorBright(video::SColor(event->cloud_params.color_bright));
	clouds->setColorAmbient(video::SColor(event->cloud_params.color_ambient));
	clouds->setHeight(event->cloud_params.height);
	clouds->setThickness(event->cloud_params.thickness);
	clouds->setSpeed(v2f(event->cloud_params.speed_x, event->cloud_params.speed_y));
}

void Game::processClientEvents(CameraOrientation *cam)
{
	while (client->hasClientEvents()) {
		std::unique_ptr<ClientEvent> event(client->getClientEvent());
		FATAL_ERROR_IF(event->type >= CLIENTEVENT_MAX, "Invalid clientevent type");
		const ClientEventHandler& evHandler = clientEventHandler[event->type];
		(this->*evHandler.handler)(event.get(), cam);
	}
}

void Game::updateChat(f32 dtime)
{
	// Get new messages from error log buffer
	while (!m_chat_log_buf.empty())
		chat_backend->addMessage(L"", utf8_to_wide(m_chat_log_buf.get()));

	// Get new messages from client
	std::wstring message;
	while (client->getChatMessage(message)) {
		chat_backend->addUnparsedMessage(message);
	}

	// Remove old messages
	chat_backend->step(dtime);

	// Display all messages in a static text element
	auto &buf = chat_backend->getRecentBuffer();
	if (buf.getLinesModified()) {
		buf.resetLinesModified();
		m_game_ui->setChatText(chat_backend->getRecentChat(), buf.getLineCount());
	}

	// Make sure that the size is still correct
	m_game_ui->updateChatSize();
}

void Game::updateCamera(f32 dtime)
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	/*
		For interaction purposes, get info about the held item
		- What item is it?
		- Is it a usable item?
		- Can it point to liquids?
	*/
	ItemStack playeritem;
	{
		ItemStack selected, hand;
		playeritem = player->getWieldedItem(&selected, &hand);
	}

	ToolCapabilities playeritem_toolcap =
		playeritem.getToolCapabilities(itemdef_manager);

	v3s16 old_camera_offset = camera->getOffset();

	if (wasKeyDown(KeyType::CAMERA_MODE)) {
		GenericCAO *playercao = player->getCAO();

		// If playercao not loaded, don't change camera
		if (!playercao)
			return;

		camera->toggleCameraMode();

#ifdef HAVE_TOUCHSCREENGUI
		if (g_touchscreengui)
			g_touchscreengui->setUseCrosshair(!isNoCrosshairAllowed());
#endif

		// Make the player visible depending on camera mode.
		playercao->updateMeshCulling();
		playercao->setChildrenVisible(camera->getCameraMode() > CAMERA_MODE_FIRST);
	}

	float full_punch_interval = playeritem_toolcap.full_punch_interval;
	float tool_reload_ratio = runData.time_from_last_punch / full_punch_interval;

	tool_reload_ratio = MYMIN(tool_reload_ratio, 1.0);
	camera->update(player, dtime, tool_reload_ratio);
	camera->step(dtime);

	f32 camera_fov = camera->getFovMax();
	v3s16 camera_offset = camera->getOffset();

	m_camera_offset_changed = (camera_offset != old_camera_offset);

	if (!m_flags.disable_camera_update) {
		v3f camera_position = camera->getPosition();
		v3f camera_direction = camera->getDirection();

		client->getEnv().getClientMap().updateCamera(camera_position,
				camera_direction, camera_fov, camera_offset);

		if (m_camera_offset_changed) {
			client->updateCameraOffset(camera_offset);
			client->getEnv().updateCameraOffset(camera_offset);

			if (clouds)
				clouds->updateCameraOffset(camera_offset);
		}
	}
}


void Game::updateSound(f32 dtime)
{
	// Update sound listener
	v3s16 camera_offset = camera->getOffset();
	sound->updateListener(camera->getCameraNode()->getPosition() + intToFloat(camera_offset, BS),
			      v3f(0, 0, 0), // velocity
			      camera->getDirection(),
			      camera->getCameraNode()->getUpVector());

	bool mute_sound = g_settings->getBool("mute_sound");
	if (mute_sound) {
		sound->setListenerGain(0.0f);
	} else {
		// Check if volume is in the proper range, else fix it.
		float old_volume = g_settings->getFloat("sound_volume");
		float new_volume = rangelim(old_volume, 0.0f, 1.0f);
		sound->setListenerGain(new_volume);

		if (old_volume != new_volume) {
			g_settings->setFloat("sound_volume", new_volume);
		}
	}

	LocalPlayer *player = client->getEnv().getLocalPlayer();

	// Tell the sound maker whether to make footstep sounds
	soundmaker->makes_footstep_sound = player->makes_footstep_sound;

	//	Update sound maker
	if (player->makes_footstep_sound)
		soundmaker->step(dtime);

	ClientMap &map = client->getEnv().getClientMap();
	MapNode n = map.getNode(player->getFootstepNodePos());
	soundmaker->m_player_step_sound = nodedef_manager->get(n).sound_footstep;
}


void Game::processPlayerInteraction(f32 dtime, bool show_hud)
{
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	const v3f camera_direction = camera->getDirection();
	const v3s16 camera_offset  = camera->getOffset();

	/*
		Calculate what block is the crosshair pointing to
	*/

	ItemStack selected_item, hand_item;
	const ItemStack &tool_item = player->getWieldedItem(&selected_item, &hand_item);

	const ItemDefinition &selected_def = selected_item.getDefinition(itemdef_manager);
	f32 d = getToolRange(selected_def, hand_item.getDefinition(itemdef_manager));

	core::line3d<f32> shootline;

	switch (camera->getCameraMode()) {
	case CAMERA_MODE_FIRST:
		// Shoot from camera position, with bobbing
		shootline.start = camera->getPosition();
		break;
	case CAMERA_MODE_THIRD:
		// Shoot from player head, no bobbing
		shootline.start = camera->getHeadPosition();
		break;
	case CAMERA_MODE_THIRD_FRONT:
		shootline.start = camera->getHeadPosition();
		// prevent player pointing anything in front-view
		d = 0;
		break;
	}
	shootline.end = shootline.start + camera_direction * BS * d;

#ifdef HAVE_TOUCHSCREENGUI
	if (g_touchscreengui && isNoCrosshairAllowed()) {
		shootline = g_touchscreengui->getShootline();
		// Scale shootline to the acual distance the player can reach
		shootline.end = shootline.start +
				shootline.getVector().normalize() * BS * d;
		shootline.start += intToFloat(camera_offset, BS);
		shootline.end += intToFloat(camera_offset, BS);
	}
#endif

	PointedThing pointed = updatePointedThing(shootline,
			selected_def.liquids_pointable,
			!runData.btn_down_for_dig,
			camera_offset);

	if (pointed != runData.pointed_old)
		infostream << "Pointing at " << pointed.dump() << std::endl;

	// Note that updating the selection mesh every frame is not particularly efficient,
	// but the halo rendering code is already inefficient so there's no point in optimizing it here
	hud->updateSelectionMesh(camera_offset);

	// Allow digging again if button is not pressed
	if (runData.digging_blocked && !isKeyDown(KeyType::DIG))
		runData.digging_blocked = false;

	/*
		Stop digging when
		- releasing dig button
		- pointing away from node
	*/
	if (runData.digging) {
		if (wasKeyReleased(KeyType::DIG)) {
			infostream << "Dig button released (stopped digging)" << std::endl;
			runData.digging = false;
		} else if (pointed != runData.pointed_old) {
			if (pointed.type == POINTEDTHING_NODE
					&& runData.pointed_old.type == POINTEDTHING_NODE
					&& pointed.node_undersurface
							== runData.pointed_old.node_undersurface) {
				// Still pointing to the same node, but a different face.
				// Don't reset.
			} else {
				infostream << "Pointing away from node (stopped digging)" << std::endl;
				runData.digging = false;
				hud->updateSelectionMesh(camera_offset);
			}
		}

		if (!runData.digging) {
			client->interact(INTERACT_STOP_DIGGING, runData.pointed_old);
			client->setCrack(-1, v3s16(0, 0, 0));
			runData.dig_time = 0.0;
		}
	} else if (runData.dig_instantly && wasKeyReleased(KeyType::DIG)) {
		// Remove e.g. torches faster when clicking instead of holding dig button
		runData.nodig_delay_timer = 0;
		runData.dig_instantly = false;
	}

	if (!runData.digging && runData.btn_down_for_dig && !isKeyDown(KeyType::DIG))
		runData.btn_down_for_dig = false;

	runData.punching = false;

	soundmaker->m_player_leftpunch_sound = SimpleSoundSpec();
	soundmaker->m_player_leftpunch_sound2 = pointed.type != POINTEDTHING_NOTHING ?
		selected_def.sound_use : selected_def.sound_use_air;

	// Prepare for repeating, unless we're not supposed to
	if (isKeyDown(KeyType::PLACE) && !g_settings->getBool("safe_dig_and_place"))
		runData.repeat_place_timer += dtime;
	else
		runData.repeat_place_timer = 0;

	if (selected_def.usable && isKeyDown(KeyType::DIG)) {
		if (wasKeyPressed(KeyType::DIG) && (!client->modsLoaded() ||
				!client->getScript()->on_item_use(selected_item, pointed)))
			client->interact(INTERACT_USE, pointed);
	} else if (pointed.type == POINTEDTHING_NODE) {
		handlePointingAtNode(pointed, selected_item, hand_item, dtime);
	} else if (pointed.type == POINTEDTHING_OBJECT) {
		v3f player_position  = player->getPosition();
		bool basic_debug_allowed = client->checkPrivilege("debug") || (player->hud_flags & HUD_FLAG_BASIC_DEBUG);
		handlePointingAtObject(pointed, tool_item, player_position,
				m_game_ui->m_flags.show_basic_debug && basic_debug_allowed);
	} else if (isKeyDown(KeyType::DIG)) {
		// When button is held down in air, show continuous animation
		runData.punching = true;
		// Run callback even though item is not usable
		if (wasKeyPressed(KeyType::DIG) && client->modsLoaded())
			client->getScript()->on_item_use(selected_item, pointed);
	} else if (wasKeyPressed(KeyType::PLACE)) {
		handlePointingAtNothing(selected_item);
	}

	runData.pointed_old = pointed;

	if (runData.punching || wasKeyPressed(KeyType::DIG))
		camera->setDigging(0); // dig animation

	input->clearWasKeyPressed();
	input->clearWasKeyReleased();
	// Ensure DIG & PLACE are marked as handled
	wasKeyDown(KeyType::DIG);
	wasKeyDown(KeyType::PLACE);

	input->joystick.clearWasKeyPressed(KeyType::DIG);
	input->joystick.clearWasKeyPressed(KeyType::PLACE);

	input->joystick.clearWasKeyReleased(KeyType::DIG);
	input->joystick.clearWasKeyReleased(KeyType::PLACE);
}


PointedThing Game::updatePointedThing(
	const core::line3d<f32> &shootline,
	bool liquids_pointable,
	bool look_for_object,
	const v3s16 &camera_offset)
{
	std::vector<aabb3f> *selectionboxes = hud->getSelectionBoxes();
	selectionboxes->clear();
	hud->setSelectedFaceNormal(v3f());
	static thread_local const bool show_entity_selectionbox = g_settings->getBool(
		"show_entity_selectionbox");

	ClientEnvironment &env = client->getEnv();
	ClientMap &map = env.getClientMap();
	const NodeDefManager *nodedef = map.getNodeDefManager();

	runData.selected_object = NULL;
	hud->pointing_at_object = false;

	RaycastState s(shootline, look_for_object, liquids_pointable);
	PointedThing result;
	env.continueRaycast(&s, &result);
	if (result.type == POINTEDTHING_OBJECT) {
		hud->pointing_at_object = true;

		runData.selected_object = client->getEnv().getActiveObject(result.object_id);
		aabb3f selection_box;
		if (show_entity_selectionbox && runData.selected_object->doShowSelectionBox() &&
				runData.selected_object->getSelectionBox(&selection_box)) {
			v3f pos = runData.selected_object->getPosition();
			selectionboxes->push_back(aabb3f(selection_box));
			hud->setSelectionPos(pos, camera_offset);
			GenericCAO* gcao = dynamic_cast<GenericCAO*>(runData.selected_object);
			if (gcao != nullptr && gcao->getProperties().rotate_selectionbox)
				hud->setSelectionRotation(gcao->getSceneNode()->getAbsoluteTransformation().getRotationDegrees());
			else
				hud->setSelectionRotation(v3f());
		}
		hud->setSelectedFaceNormal(result.raw_intersection_normal);
	} else if (result.type == POINTEDTHING_NODE) {
		// Update selection boxes
		MapNode n = map.getNode(result.node_undersurface);
		std::vector<aabb3f> boxes;
		n.getSelectionBoxes(nodedef, &boxes,
			n.getNeighbors(result.node_undersurface, &map));

		f32 d = 0.002 * BS;
		for (std::vector<aabb3f>::const_iterator i = boxes.begin();
			i != boxes.end(); ++i) {
			aabb3f box = *i;
			box.MinEdge -= v3f(d, d, d);
			box.MaxEdge += v3f(d, d, d);
			selectionboxes->push_back(box);
		}
		hud->setSelectionPos(intToFloat(result.node_undersurface, BS),
			camera_offset);
		hud->setSelectionRotation(v3f());
		hud->setSelectedFaceNormal(result.intersection_normal);
	}

	// Update selection mesh light level and vertex colors
	if (!selectionboxes->empty()) {
		v3f pf = hud->getSelectionPos();
		v3s16 p = floatToInt(pf, BS);

		// Get selection mesh light level
		MapNode n = map.getNode(p);
		u16 node_light = getInteriorLight(n, -1, nodedef);
		u16 light_level = node_light;

		for (const v3s16 &dir : g_6dirs) {
			n = map.getNode(p + dir);
			node_light = getInteriorLight(n, -1, nodedef);
			if (node_light > light_level)
				light_level = node_light;
		}

		u32 daynight_ratio = client->getEnv().getDayNightRatio();
		video::SColor c;
		final_color_blend(&c, light_level, daynight_ratio);

		// Modify final color a bit with time
		u32 timer = client->getEnv().getFrameTime() % 5000;
		float timerf = (float) (irr::core::PI * ((timer / 2500.0) - 0.5));
		float sin_r = 0.08f * std::sin(timerf);
		float sin_g = 0.08f * std::sin(timerf + irr::core::PI * 0.5f);
		float sin_b = 0.08f * std::sin(timerf + irr::core::PI);
		c.setRed(core::clamp(core::round32(c.getRed() * (0.8 + sin_r)), 0, 255));
		c.setGreen(core::clamp(core::round32(c.getGreen() * (0.8 + sin_g)), 0, 255));
		c.setBlue(core::clamp(core::round32(c.getBlue() * (0.8 + sin_b)), 0, 255));

		// Set mesh final color
		hud->setSelectionMeshColor(c);
	}
	return result;
}


void Game::handlePointingAtNothing(const ItemStack &playerItem)
{
	infostream << "Attempted to place item while pointing at nothing" << std::endl;
	PointedThing fauxPointed;
	fauxPointed.type = POINTEDTHING_NOTHING;
	client->interact(INTERACT_ACTIVATE, fauxPointed);
}


void Game::handlePointingAtNode(const PointedThing &pointed,
	const ItemStack &selected_item, const ItemStack &hand_item, f32 dtime)
{
	v3s16 nodepos = pointed.node_undersurface;
	v3s16 neighborpos = pointed.node_abovesurface;

	/*
		Check information text of node
	*/

	ClientMap &map = client->getEnv().getClientMap();

	if (runData.nodig_delay_timer <= 0.0 && isKeyDown(KeyType::DIG)
			&& !runData.digging_blocked
			&& client->checkPrivilege("interact")) {
		handleDigging(pointed, nodepos, selected_item, hand_item, dtime);
	}

	// This should be done after digging handling
	NodeMetadata *meta = map.getNodeMetadata(nodepos);

	if (meta) {
		m_game_ui->setInfoText(unescape_translate(utf8_to_wide(
			meta->getString("infotext"))));
	} else {
		MapNode n = map.getNode(nodepos);

		if (nodedef_manager->get(n).name == "unknown") {
			m_game_ui->setInfoText(L"Unknown node");
		}
	}

	if ((wasKeyPressed(KeyType::PLACE) ||
			runData.repeat_place_timer >= m_repeat_place_time) &&
			client->checkPrivilege("interact")) {
		runData.repeat_place_timer = 0;
		infostream << "Place button pressed while looking at ground" << std::endl;

		// Placing animation (always shown for feedback)
		camera->setDigging(1);

		soundmaker->m_player_rightpunch_sound = SimpleSoundSpec();

		// If the wielded item has node placement prediction,
		// make that happen
		// And also set the sound and send the interact
		// But first check for meta formspec and rightclickable
		auto &def = selected_item.getDefinition(itemdef_manager);
		bool placed = nodePlacement(def, selected_item, nodepos, neighborpos,
			pointed, meta);

		if (placed && client->modsLoaded())
			client->getScript()->on_placenode(pointed, def);
	}
}

bool Game::nodePlacement(const ItemDefinition &selected_def,
	const ItemStack &selected_item, const v3s16 &nodepos, const v3s16 &neighborpos,
	const PointedThing &pointed, const NodeMetadata *meta)
{
	const auto &prediction = selected_def.node_placement_prediction;

	const NodeDefManager *nodedef = client->ndef();
	ClientMap &map = client->getEnv().getClientMap();
	MapNode node;
	bool is_valid_position;

	node = map.getNode(nodepos, &is_valid_position);
	if (!is_valid_position) {
		soundmaker->m_player_rightpunch_sound = selected_def.sound_place_failed;
		return false;
	}

	// formspec in meta
	if (meta && !meta->getString("formspec").empty() && !input->isRandom()
			&& !isKeyDown(KeyType::SNEAK)) {
		// on_rightclick callbacks are called anyway
		if (nodedef_manager->get(map.getNode(nodepos)).rightclickable)
			client->interact(INTERACT_PLACE, pointed);

		infostream << "Launching custom inventory view" << std::endl;

		InventoryLocation inventoryloc;
		inventoryloc.setNodeMeta(nodepos);

		NodeMetadataFormSource *fs_src = new NodeMetadataFormSource(
			&client->getEnv().getClientMap(), nodepos);
		TextDest *txt_dst = new TextDestNodeMetadata(nodepos, client);

		auto *&formspec = m_game_ui->updateFormspec("");
		GUIFormSpecMenu::create(formspec, client, m_rendering_engine->get_gui_env(),
			&input->joystick, fs_src, txt_dst, client->getFormspecPrepend(), sound);

		formspec->setFormSpec(meta->getString("formspec"), inventoryloc);
		return false;
	}

	// on_rightclick callback
	if (prediction.empty() || (nodedef->get(node).rightclickable &&
			!isKeyDown(KeyType::SNEAK))) {
		// Report to server
		client->interact(INTERACT_PLACE, pointed);
		return false;
	}

	verbosestream << "Node placement prediction for "
		<< selected_def.name << " is " << prediction << std::endl;
	v3s16 p = neighborpos;

	// Place inside node itself if buildable_to
	MapNode n_under = map.getNode(nodepos, &is_valid_position);
	if (is_valid_position) {
		if (nodedef->get(n_under).buildable_to) {
			p = nodepos;
		} else {
			node = map.getNode(p, &is_valid_position);
			if (is_valid_position && !nodedef->get(node).buildable_to) {
				soundmaker->m_player_rightpunch_sound = selected_def.sound_place_failed;
				// Report to server
				client->interact(INTERACT_PLACE, pointed);
				return false;
			}
		}
	}

	// Find id of predicted node
	content_t id;
	bool found = nodedef->getId(prediction, id);

	if (!found) {
		errorstream << "Node placement prediction failed for "
			<< selected_def.name << " (places " << prediction
			<< ") - Name not known" << std::endl;
		// Handle this as if prediction was empty
		// Report to server
		client->interact(INTERACT_PLACE, pointed);
		return false;
	}

	const ContentFeatures &predicted_f = nodedef->get(id);

	// Compare core.item_place_node() for what the server does with param2
	MapNode predicted_node(id, 0, 0);

	const u8 place_param2 = selected_def.place_param2;

	if (place_param2) {
		predicted_node.setParam2(place_param2);
	} else if (predicted_f.param_type_2 == CPT2_WALLMOUNTED ||
			predicted_f.param_type_2 == CPT2_COLORED_WALLMOUNTED) {
		v3s16 dir = nodepos - neighborpos;

		if (abs(dir.Y) > MYMAX(abs(dir.X), abs(dir.Z))) {
			predicted_node.setParam2(dir.Y < 0 ? 1 : 0);
		} else if (abs(dir.X) > abs(dir.Z)) {
			predicted_node.setParam2(dir.X < 0 ? 3 : 2);
		} else {
			predicted_node.setParam2(dir.Z < 0 ? 5 : 4);
		}
	} else if (predicted_f.param_type_2 == CPT2_FACEDIR ||
			predicted_f.param_type_2 == CPT2_COLORED_FACEDIR ||
			predicted_f.param_type_2 == CPT2_4DIR ||
			predicted_f.param_type_2 == CPT2_COLORED_4DIR) {
		v3s16 dir = nodepos - floatToInt(client->getEnv().getLocalPlayer()->getPosition(), BS);

		if (abs(dir.X) > abs(dir.Z)) {
			predicted_node.setParam2(dir.X < 0 ? 3 : 1);
		} else {
			predicted_node.setParam2(dir.Z < 0 ? 2 : 0);
		}
	}

	// Check attachment if node is in group attached_node
	int an = itemgroup_get(predicted_f.groups, "attached_node");
	if (an != 0) {
		v3s16 pp;

		if (an == 3) {
			pp = p + v3s16(0, -1, 0);
		} else if (an == 4) {
			pp = p + v3s16(0, 1, 0);
		} else if (an == 2) {
			if (predicted_f.param_type_2 == CPT2_FACEDIR ||
					predicted_f.param_type_2 == CPT2_COLORED_FACEDIR ||
					predicted_f.param_type_2 == CPT2_4DIR ||
					predicted_f.param_type_2 == CPT2_COLORED_4DIR) {
				pp = p + facedir_dirs[predicted_node.getFaceDir(nodedef)];
			} else {
				pp = p;
			}
		} else if (predicted_f.param_type_2 == CPT2_WALLMOUNTED ||
				predicted_f.param_type_2 == CPT2_COLORED_WALLMOUNTED) {
			pp = p + predicted_node.getWallMountedDir(nodedef);
		} else {
			pp = p + v3s16(0, -1, 0);
		}

		if (!nodedef->get(map.getNode(pp)).walkable) {
			soundmaker->m_player_rightpunch_sound = selected_def.sound_place_failed;
			// Report to server
			client->interact(INTERACT_PLACE, pointed);
			return false;
		}
	}

	// Apply color
	if (!place_param2 && (predicted_f.param_type_2 == CPT2_COLOR
			|| predicted_f.param_type_2 == CPT2_COLORED_FACEDIR
			|| predicted_f.param_type_2 == CPT2_COLORED_4DIR
			|| predicted_f.param_type_2 == CPT2_COLORED_WALLMOUNTED)) {
		const auto &indexstr = selected_item.metadata.
			getString("palette_index", 0);
		if (!indexstr.empty()) {
			s32 index = mystoi(indexstr);
			if (predicted_f.param_type_2 == CPT2_COLOR) {
				predicted_node.setParam2(index);
			} else if (predicted_f.param_type_2 == CPT2_COLORED_WALLMOUNTED) {
				// param2 = pure palette index + other
				predicted_node.setParam2((index & 0xf8) | (predicted_node.getParam2() & 0x07));
			} else if (predicted_f.param_type_2 == CPT2_COLORED_FACEDIR) {
				// param2 = pure palette index + other
				predicted_node.setParam2((index & 0xe0) | (predicted_node.getParam2() & 0x1f));
			} else if (predicted_f.param_type_2 == CPT2_COLORED_4DIR) {
				// param2 = pure palette index + other
				predicted_node.setParam2((index & 0xfc) | (predicted_node.getParam2() & 0x03));
			}
		}
	}

	// Add node to client map
	try {
		LocalPlayer *player = client->getEnv().getLocalPlayer();

		// Don't place node when player would be inside new node
		// NOTE: This is to be eventually implemented by a mod as client-side Lua
		if (!predicted_f.walkable ||
				g_settings->getBool("enable_build_where_you_stand") ||
				(client->checkPrivilege("noclip") && g_settings->getBool("noclip")) ||
				(predicted_f.walkable &&
					neighborpos != player->getStandingNodePos() + v3s16(0, 1, 0) &&
					neighborpos != player->getStandingNodePos() + v3s16(0, 2, 0))) {
			// This triggers the required mesh update too
			client->addNode(p, predicted_node);
			// Report to server
			client->interact(INTERACT_PLACE, pointed);
			// A node is predicted, also play a sound
			soundmaker->m_player_rightpunch_sound = selected_def.sound_place;
			return true;
		} else {
			soundmaker->m_player_rightpunch_sound = selected_def.sound_place_failed;
			return false;
		}
	} catch (const InvalidPositionException &e) {
		errorstream << "Node placement prediction failed for "
			<< selected_def.name << " (places "
			<< prediction << ") - Position not loaded" << std::endl;
		soundmaker->m_player_rightpunch_sound = selected_def.sound_place_failed;
		return false;
	}
}

void Game::handlePointingAtObject(const PointedThing &pointed,
		const ItemStack &tool_item, const v3f &player_position, bool show_debug)
{
	std::wstring infotext = unescape_translate(
		utf8_to_wide(runData.selected_object->infoText()));

	if (show_debug) {
		if (!infotext.empty()) {
			infotext += L"\n";
		}
		infotext += utf8_to_wide(runData.selected_object->debugInfoText());
	}

	m_game_ui->setInfoText(infotext);

	if (isKeyDown(KeyType::DIG)) {
		bool do_punch = false;
		bool do_punch_damage = false;

		if (runData.object_hit_delay_timer <= 0.0) {
			do_punch = true;
			do_punch_damage = true;
			runData.object_hit_delay_timer = object_hit_delay;
		}

		if (wasKeyPressed(KeyType::DIG))
			do_punch = true;

		if (do_punch) {
			infostream << "Punched object" << std::endl;
			runData.punching = true;
		}

		if (do_punch_damage) {
			// Report direct punch
			v3f objpos = runData.selected_object->getPosition();
			v3f dir = (objpos - player_position).normalize();

			bool disable_send = runData.selected_object->directReportPunch(
					dir, &tool_item, runData.time_from_last_punch);
			runData.time_from_last_punch = 0;

			if (!disable_send)
				client->interact(INTERACT_START_DIGGING, pointed);
		}
	} else if (wasKeyDown(KeyType::PLACE)) {
		infostream << "Pressed place button while pointing at object" << std::endl;
		client->interact(INTERACT_PLACE, pointed);  // place
	}
}


void Game::handleDigging(const PointedThing &pointed, const v3s16 &nodepos,
		const ItemStack &selected_item, const ItemStack &hand_item, f32 dtime)
{
	// See also: serverpackethandle.cpp, action == 2
	LocalPlayer *player = client->getEnv().getLocalPlayer();
	ClientMap &map = client->getEnv().getClientMap();
	MapNode n = map.getNode(nodepos);
	const auto &features = nodedef_manager->get(n);

	// NOTE: Similar piece of code exists on the server side for
	// cheat detection.
	// Get digging parameters
	DigParams params = getDigParams(features.groups,
			&selected_item.getToolCapabilities(itemdef_manager),
			selected_item.wear);

	// If can't dig, try hand
	if (!params.diggable) {
		params = getDigParams(features.groups,
				&hand_item.getToolCapabilities(itemdef_manager));
	}

	if (!params.diggable) {
		// I guess nobody will wait for this long
		runData.dig_time_complete = 10000000.0;
	} else {
		runData.dig_time_complete = params.time;

		if (m_cache_enable_particles) {
			client->getParticleManager()->addNodeParticle(client,
					player, nodepos, n, features);
		}
	}

	if (!runData.digging) {
		infostream << "Started digging" << std::endl;
		runData.dig_instantly = runData.dig_time_complete == 0;
		if (client->modsLoaded() && client->getScript()->on_punchnode(nodepos, n))
			return;

		client->interact(INTERACT_START_DIGGING, pointed);
		runData.digging = true;
		runData.btn_down_for_dig = true;
	}

	if (!runData.dig_instantly) {
		runData.dig_index = (float)crack_animation_length
				* runData.dig_time
				/ runData.dig_time_complete;
	} else {
		// This is for e.g. torches
		runData.dig_index = crack_animation_length;
	}

	const auto &sound_dig = features.sound_dig;

	if (sound_dig.exists() && params.diggable) {
		if (sound_dig.name == "__group") {
			if (!params.main_group.empty()) {
				soundmaker->m_player_leftpunch_sound.gain = 0.5;
				soundmaker->m_player_leftpunch_sound.name =
						std::string("default_dig_") +
						params.main_group;
			}
		} else {
			soundmaker->m_player_leftpunch_sound = sound_dig;
		}
	}

	// Don't show cracks if not diggable
	if (runData.dig_time_complete >= 100000.0) {
	} else if (runData.dig_index < crack_animation_length) {
		client->setCrack(runData.dig_index, nodepos);
	} else {
		infostream << "Digging completed" << std::endl;
		client->setCrack(-1, v3s16(0, 0, 0));

		runData.dig_time = 0;
		runData.digging = false;
		// we successfully dug, now block it from repeating if we want to be safe
		if (g_settings->getBool("safe_dig_and_place"))
			runData.digging_blocked = true;

		runData.nodig_delay_timer =
				runData.dig_time_complete / (float)crack_animation_length;

		// We don't want a corresponding delay to very time consuming nodes
		// and nodes without digging time (e.g. torches) get a fixed delay.
		if (runData.nodig_delay_timer > 0.3)
			runData.nodig_delay_timer = 0.3;
		else if (runData.dig_instantly)
			runData.nodig_delay_timer = 0.15;

		if (client->modsLoaded() &&
				client->getScript()->on_dignode(nodepos, n)) {
			return;
		}

		if (features.node_dig_prediction == "air") {
			client->removeNode(nodepos);
		} else if (!features.node_dig_prediction.empty()) {
			content_t id;
			bool found = nodedef_manager->getId(features.node_dig_prediction, id);
			if (found)
				client->addNode(nodepos, id, true);
		}
		// implicit else: no prediction

		client->interact(INTERACT_DIGGING_COMPLETED, pointed);

		if (m_cache_enable_particles) {
			client->getParticleManager()->addDiggingParticles(client,
				player, nodepos, n, features);
		}


		// Send event to trigger sound
		client->getEventManager()->put(new NodeDugEvent(nodepos, n));
	}

	if (runData.dig_time_complete < 100000.0) {
		runData.dig_time += dtime;
	} else {
		runData.dig_time = 0;
		client->setCrack(-1, nodepos);
	}

	camera->setDigging(0);  // Dig animation
}

void Game::updateFrame(ProfilerGraph *graph, RunStats *stats, f32 dtime,
		const CameraOrientation &cam)
{
	TimeTaker tt_update("Game::updateFrame()");
	LocalPlayer *player = client->getEnv().getLocalPlayer();

	/*
		Frame time
	*/

	client->getEnv().updateFrameTime(m_is_paused);

	/*
		Fog range
	*/

	if (draw_control->range_all) {
		runData.fog_range = 100000 * BS;
	} else {
		runData.fog_range = draw_control->wanted_range * BS;
	}

	/*
		Calculate general brightness
	*/
	u32 daynight_ratio = client->getEnv().getDayNightRatio();
	float time_brightness = decode_light_f((float)daynight_ratio / 1000.0);
	float direct_brightness;
	bool sunlight_seen;

	// When in noclip mode force same sky brightness as above ground so you
	// can see properly
	if (draw_control->allow_noclip && m_cache_enable_free_move &&
		client->checkPrivilege("fly")) {
		direct_brightness = time_brightness;
		sunlight_seen = true;
	} else {
		float old_brightness = sky->getBrightness();
		direct_brightness = client->getEnv().getClientMap()
				.getBackgroundBrightness(MYMIN(runData.fog_range * 1.2, 60 * BS),
					daynight_ratio, (int)(old_brightness * 255.5), &sunlight_seen)
				    / 255.0;
	}

	float time_of_day_smooth = runData.time_of_day_smooth;
	float time_of_day = client->getEnv().getTimeOfDayF();

	static const float maxsm = 0.05f;
	static const float todsm = 0.05f;

	if (std::fabs(time_of_day - time_of_day_smooth) > maxsm &&
			std::fabs(time_of_day - time_of_day_smooth + 1.0) > maxsm &&
			std::fabs(time_of_day - time_of_day_smooth - 1.0) > maxsm)
		time_of_day_smooth = time_of_day;

	if (time_of_day_smooth > 0.8 && time_of_day < 0.2)
		time_of_day_smooth = time_of_day_smooth * (1.0 - todsm)
				+ (time_of_day + 1.0) * todsm;
	else
		time_of_day_smooth = time_of_day_smooth * (1.0 - todsm)
				+ time_of_day * todsm;

	runData.time_of_day_smooth = time_of_day_smooth;

	sky->update(time_of_day_smooth, time_brightness, direct_brightness,
			sunlight_seen, camera->getCameraMode(), player->getYaw(),
			player->getPitch());

	/*
		Update clouds
	*/
	if (clouds) {
		if (sky->getCloudsVisible()) {
			clouds->setVisible(true);
			clouds->step(dtime);
			// camera->getPosition is not enough for 3rd person views
			v3f camera_node_position = camera->getCameraNode()->getPosition();
			v3s16 camera_offset      = camera->getOffset();
			camera_node_position.X   = camera_node_position.X + camera_offset.X * BS;
			camera_node_position.Y   = camera_node_position.Y + camera_offset.Y * BS;
			camera_node_position.Z   = camera_node_position.Z + camera_offset.Z * BS;
			clouds->update(camera_node_position,
					sky->getCloudColor());
			if (clouds->isCameraInsideCloud() && m_cache_enable_fog) {
				// if inside clouds, and fog enabled, use that as sky
				// color(s)
				video::SColor clouds_dark = clouds->getColor()
						.getInterpolated(video::SColor(255, 0, 0, 0), 0.9);
				sky->overrideColors(clouds_dark, clouds->getColor());
				sky->setInClouds(true);
				runData.fog_range = std::fmin(runData.fog_range * 0.5f, 32.0f * BS);
				// do not draw clouds after all
				clouds->setVisible(false);
			}
		} else {
			clouds->setVisible(false);
		}
	}

	/*
		Update particles
	*/
	client->getParticleManager()->step(dtime);

	/*
		Fog
	*/

	if (m_cache_enable_fog) {
		driver->setFog(
				sky->getBgColor(),
				video::EFT_FOG_LINEAR,
				runData.fog_range * m_cache_fog_start,
				runData.fog_range * 1.0,
				0.01,
				false, // pixel fog
				true // range fog
		);
	} else {
		driver->setFog(
				sky->getBgColor(),
				video::EFT_FOG_LINEAR,
				100000 * BS,
				110000 * BS,
				0.01f,
				false, // pixel fog
				false // range fog
		);
	}

	/*
		Damage camera tilt
	*/
	if (player->hurt_tilt_timer > 0.0f) {
		player->hurt_tilt_timer -= dtime * 6.0f;

		if (player->hurt_tilt_timer < 0.0f)
			player->hurt_tilt_strength = 0.0f;
	}

	/*
		Update minimap pos and rotation
	*/
	if (mapper && m_game_ui->m_flags.show_hud) {
		mapper->setPos(floatToInt(player->getPosition(), BS));
		mapper->setAngle(player->getYaw());
	}

	/*
		Get chat messages from client
	*/

	updateChat(dtime);

	/*
		Inventory
	*/

	if (player->getWieldIndex() != runData.new_playeritem)
		client->setPlayerItem(runData.new_playeritem);

	if (client->updateWieldedItem()) {
		// Update wielded tool
		ItemStack selected_item, hand_item;
		ItemStack &tool_item = player->getWieldedItem(&selected_item, &hand_item);
		camera->wield(tool_item);
	}

	/*
		Update block draw list every 200ms or when camera direction has
		changed much
	*/
	runData.update_draw_list_timer += dtime;
	runData.touch_blocks_timer += dtime;

	bool draw_list_updated = false;

	float update_draw_list_delta = 0.2f;

	v3f camera_direction = camera->getDirection();
	if (runData.update_draw_list_timer >= update_draw_list_delta
			|| runData.update_draw_list_last_cam_dir.getDistanceFrom(camera_direction) > 0.2
			|| m_camera_offset_changed
			|| client->getEnv().getClientMap().needsUpdateDrawList()) {
		runData.update_draw_list_timer = 0;
		client->getEnv().getClientMap().updateDrawList();
		runData.update_draw_list_last_cam_dir = camera_direction;
		draw_list_updated = true;
	}

	if (runData.touch_blocks_timer > update_draw_list_delta && !draw_list_updated) {
		client->getEnv().getClientMap().touchMapBlocks();
		runData.touch_blocks_timer = 0;
	}

	if (RenderingEngine::get_shadow_renderer()) {
		updateShadows();
	}

	m_game_ui->update(*stats, client, draw_control, cam, runData.pointed_old, gui_chat_console, dtime);

	/*
	   make sure menu is on top
	   1. Delete formspec menu reference if menu was removed
	   2. Else, make sure formspec menu is on top
	*/
	auto formspec = m_game_ui->getFormspecGUI();
	do { // breakable. only runs for one iteration
		if (!formspec)
			break;

		if (formspec->getReferenceCount() == 1) {
			m_game_ui->deleteFormspec();
			break;
		}

		auto &loc = formspec->getFormspecLocation();
		if (loc.type == InventoryLocation::NODEMETA) {
			NodeMetadata *meta = client->getEnv().getClientMap().getNodeMetadata(loc.p);
			if (!meta || meta->getString("formspec").empty()) {
				formspec->quitMenu();
				break;
			}
		}

		if (isMenuActive())
			guiroot->bringToFront(formspec);
	} while (false);

	/*
		==================== Drawing begins ====================
	*/
	const video::SColor skycolor = sky->getSkyColor();

	TimeTaker tt_draw("Draw scene", nullptr, PRECISION_MICRO);
	driver->beginScene(true, true, skycolor);

	bool draw_wield_tool = (m_game_ui->m_flags.show_hud &&
			(player->hud_flags & HUD_FLAG_WIELDITEM_VISIBLE) &&
			(camera->getCameraMode() == CAMERA_MODE_FIRST));
	bool draw_crosshair = (
			(player->hud_flags & HUD_FLAG_CROSSHAIR_VISIBLE) &&
			(camera->getCameraMode() != CAMERA_MODE_THIRD_FRONT));
#ifdef HAVE_TOUCHSCREENGUI
	if (isNoCrosshairAllowed())
		draw_crosshair = false;
#endif
	m_rendering_engine->draw_scene(skycolor, m_game_ui->m_flags.show_hud,
			m_game_ui->m_flags.show_minimap, draw_wield_tool, draw_crosshair);

	/*
		Profiler graph
	*/
	v2u32 screensize = driver->getScreenSize();

	if (m_game_ui->m_flags.show_profiler_graph)
		graph->draw(10, screensize.Y - 10, driver, g_fontengine->getFont());

	/*
		Damage flash
	*/
	if (runData.damage_flash > 0.0f) {
		video::SColor color(runData.damage_flash, 180, 0, 0);
		driver->draw2DRectangle(color,
					core::rect<s32>(0, 0, screensize.X, screensize.Y),
					NULL);

		runData.damage_flash -= 384.0f * dtime;
	}

	/*
		==================== End scene ====================
	*/
#if IRRLICHT_VERSION_MT_REVISION < 5
	if (++m_reset_HW_buffer_counter > 500) {
		/*
		  Periodically remove all mesh HW buffers.

		  Work around for a quirk in Irrlicht where a HW buffer is only
		  released after 20000 iterations (triggered from endScene()).

		  Without this, all loaded but unused meshes will retain their HW
		  buffers for at least 5 minutes, at which point looking up the HW buffers
		  becomes a bottleneck and the framerate drops (as much as 30%).

		  Tests showed that numbers between 50 and 1000 are good, so picked 500.
		  There are no other public Irrlicht APIs that allow interacting with the
		  HW buffers without tracking the status of every individual mesh.

		  The HW buffers for _visible_ meshes will be reinitialized in the next frame.
		*/
		infostream << "Game::updateFrame(): Removing all HW buffers." << std::endl;
		driver->removeAllHardwareBuffers();
		m_reset_HW_buffer_counter = 0;
	}
#endif

	driver->endScene();

	stats->drawtime = tt_draw.stop(true);
	g_profiler->graphAdd("Draw scene [us]", stats->drawtime);
	g_profiler->avg("Game::updateFrame(): update frame [ms]", tt_update.stop(true));
}

/* Log times and stuff for visualization */
inline void Game::updateProfilerGraphs(ProfilerGraph *graph)
{
	Profiler::GraphValues values;
	g_profiler->graphGet(values);
	graph->put(values);
}

/****************************************************************************
 * Shadows
 *****************************************************************************/
void Game::updateShadows()
{
	ShadowRenderer *shadow = RenderingEngine::get_shadow_renderer();
	if (!shadow)
		return;

	float in_timeofday = fmod(runData.time_of_day_smooth, 1.0f);

	float timeoftheday = getWickedTimeOfDay(in_timeofday);
	bool is_day = timeoftheday > 0.25 && timeoftheday < 0.75;
	bool is_shadow_visible = is_day ? sky->getSunVisible() : sky->getMoonVisible();
	shadow->setShadowIntensity(is_shadow_visible ? client->getEnv().getLocalPlayer()->getLighting().shadow_intensity : 0.0f);

	timeoftheday = fmod(timeoftheday + 0.75f, 0.5f) + 0.25f;
	const float offset_constant = 10000.0f;

	v3f light = is_day ? sky->getSunDirection() : sky->getMoonDirection();

	v3f sun_pos = light * offset_constant;

	if (shadow->getDirectionalLightCount() == 0)
		shadow->addDirectionalLight();
	shadow->getDirectionalLight().setDirection(sun_pos);
	shadow->setTimeOfDay(in_timeofday);

	shadow->getDirectionalLight().update_frustum(camera, client, m_camera_offset_changed);
}

/****************************************************************************
 Misc
 ****************************************************************************/

void FpsControl::reset()
{
	last_time = porting::getTimeUs();
}

/*
 * On some computers framerate doesn't seem to be automatically limited
 */
void FpsControl::limit(IrrlichtDevice *device, f32 *dtime)
{
	const float fps_limit = (device->isWindowFocused() && !g_menumgr.pausesGame())
			? g_settings->getFloat("fps_max")
			: g_settings->getFloat("fps_max_unfocused");
	const u64 frametime_min = 1000000.0f / std::max(fps_limit, 1.0f);

	u64 time = porting::getTimeUs();

	if (time > last_time) // Make sure time hasn't overflowed
		busy_time = time - last_time;
	else
		busy_time = 0;

	if (busy_time < frametime_min) {
		sleep_time = frametime_min - busy_time;
		if (sleep_time > 1000)
			sleep_ms(sleep_time / 1000);
	} else {
		sleep_time = 0;
	}

	// Read the timer again to accurately determine how long we actually slept,
	// rather than calculating it by adding sleep_time to time.
	time = porting::getTimeUs();

	if (time > last_time) // Make sure last_time hasn't overflowed
		*dtime = (time - last_time) / 1000000.0f;
	else
		*dtime = 0;

	last_time = time;
}

void Game::showOverlayMessage(const char *msg, float dtime, int percent, bool draw_clouds)
{
	const wchar_t *wmsg = wgettext(msg);
	m_rendering_engine->draw_load_screen(wmsg, guienv, texture_src, dtime, percent,
		draw_clouds);
	delete[] wmsg;
}

void Game::settingChangedCallback(const std::string &setting_name, void *data)
{
	((Game *)data)->readSettings();
}

void Game::readSettings()
{
	m_cache_doubletap_jump               = g_settings->getBool("doubletap_jump");
	m_cache_enable_clouds                = g_settings->getBool("enable_clouds");
	m_cache_enable_joysticks             = g_settings->getBool("enable_joysticks");
	m_cache_enable_particles             = g_settings->getBool("enable_particles");
	m_cache_enable_fog                   = g_settings->getBool("enable_fog");
	m_cache_mouse_sensitivity            = g_settings->getFloat("mouse_sensitivity", 0.001f, 10.0f);
	m_cache_joystick_frustum_sensitivity = std::max(g_settings->getFloat("joystick_frustum_sensitivity"), 0.001f);
	m_repeat_place_time                  = g_settings->getFloat("repeat_place_time", 0.25f, 2.0);

	m_cache_enable_noclip                = g_settings->getBool("noclip");
	m_cache_enable_free_move             = g_settings->getBool("free_move");

	m_cache_fog_start                    = g_settings->getFloat("fog_start");

	m_cache_cam_smoothing = 0;
	if (g_settings->getBool("cinematic"))
		m_cache_cam_smoothing = 1 - g_settings->getFloat("cinematic_camera_smoothing");
	else
		m_cache_cam_smoothing = 1 - g_settings->getFloat("camera_smoothing");

	m_cache_fog_start = rangelim(m_cache_fog_start, 0.0f, 0.99f);
	m_cache_cam_smoothing = rangelim(m_cache_cam_smoothing, 0.01f, 1.0f);
	m_cache_mouse_sensitivity = rangelim(m_cache_mouse_sensitivity, 0.001, 100.0);

	m_does_lost_focus_pause_game = g_settings->getBool("pause_on_lost_focus");
}

/****************************************************************************/
/****************************************************************************
 Shutdown / cleanup
 ****************************************************************************/
/****************************************************************************/

void Game::showDeathFormspec()
{
	static std::string formspec_str =
		std::string("formspec_version[1]") +
		SIZE_TAG
		"bgcolor[#320000b4;true]"
		"label[4.85,1.35;" + gettext("You died") + "]"
		"button_exit[4,3;3,0.5;btn_respawn;" + gettext("Respawn") + "]"
		;

	/* Create menu */
	/* Note: FormspecFormSource and LocalFormspecHandler  *
	 * are deleted by guiFormSpecMenu                     */
	FormspecFormSource *fs_src = new FormspecFormSource(formspec_str);
	LocalFormspecHandler *txt_dst = new LocalFormspecHandler("MT_DEATH_SCREEN", client);

	auto *&formspec = m_game_ui->getFormspecGUI();
	GUIFormSpecMenu::create(formspec, client, m_rendering_engine->get_gui_env(),
		&input->joystick, fs_src, txt_dst, client->getFormspecPrepend(), sound);
	formspec->setFocus("btn_respawn");
}

#define GET_KEY_NAME(KEY) gettext(getKeySetting(#KEY).name())
void Game::showPauseMenu()
{
#ifdef HAVE_TOUCHSCREENGUI
	static const std::string control_text = strgettext("Default Controls:\n"
		"No menu visible:\n"
		"- single tap: button activate\n"
		"- double tap: place/use\n"
		"- slide finger: look around\n"
		"Menu/Inventory visible:\n"
		"- double tap (outside):\n"
		" -->close\n"
		"- touch stack, touch slot:\n"
		" --> move stack\n"
		"- touch&drag, tap 2nd finger\n"
		" --> place single item to slot\n"
		);
#else
	static const std::string control_text_template = strgettext("Controls:\n"
		"- %s: move forwards\n"
		"- %s: move backwards\n"
		"- %s: move left\n"
		"- %s: move right\n"
		"- %s: jump/climb up\n"
		"- %s: dig/punch\n"
		"- %s: place/use\n"
		"- %s: sneak/climb down\n"
		"- %s: drop item\n"
		"- %s: inventory\n"
		"- Mouse: turn/look\n"
		"- Mouse wheel: select item\n"
		"- %s: chat\n"
	);

	char control_text_buf[600];

	porting::mt_snprintf(control_text_buf, sizeof(control_text_buf), control_text_template.c_str(),
		GET_KEY_NAME(keymap_forward),
		GET_KEY_NAME(keymap_backward),
		GET_KEY_NAME(keymap_left),
		GET_KEY_NAME(keymap_right),
		GET_KEY_NAME(keymap_jump),
		GET_KEY_NAME(keymap_dig),
		GET_KEY_NAME(keymap_place),
		GET_KEY_NAME(keymap_sneak),
		GET_KEY_NAME(keymap_drop),
		GET_KEY_NAME(keymap_inventory),
		GET_KEY_NAME(keymap_chat)
	);

	std::string control_text = std::string(control_text_buf);
	str_formspec_escape(control_text);
#endif

	float ypos = simple_singleplayer_mode ? 0.7f : 0.1f;
	std::ostringstream os;

	os << "formspec_version[1]" << SIZE_TAG
		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_continue;"
		<< strgettext("Continue") << "]";

	if (!simple_singleplayer_mode) {
		os << "button_exit[4," << (ypos++) << ";3,0.5;btn_change_password;"
			<< strgettext("Change Password") << "]";
	} else {
		os << "field[4.95,0;5,1.5;;" << strgettext("Game paused") << ";]";
	}

#ifndef __ANDROID__
#if USE_SOUND
	if (g_settings->getBool("enable_sound")) {
		os << "button_exit[4," << (ypos++) << ";3,0.5;btn_sound;"
			<< strgettext("Sound Volume") << "]";
	}
#endif
	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_key_config;"
		<< strgettext("Change Keys")  << "]";
#endif
	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_exit_menu;"
		<< strgettext("Exit to Menu") << "]";
	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_exit_os;"
		<< strgettext("Exit to OS")   << "]"
		<< "textarea[7.5,0.25;3.9,6.25;;" << control_text << ";]"
		<< "textarea[0.4,0.25;3.9,6.25;;" << PROJECT_NAME_C " " VERSION_STRING "\n"
		<< "\n"
		<<  strgettext("Game info:") << "\n";
	const std::string &address = client->getAddressName();
	static const std::string mode = strgettext("- Mode: ");
	if (!simple_singleplayer_mode) {
		Address serverAddress = client->getServerAddress();
		if (!address.empty()) {
			os << mode << strgettext("Remote server") << "\n"
					<< strgettext("- Address: ") << address;
		} else {
			os << mode << strgettext("Hosting server");
		}
		os << "\n" << strgettext("- Port: ") << serverAddress.getPort() << "\n";
	} else {
		os << mode << strgettext("Singleplayer") << "\n";
	}
	if (simple_singleplayer_mode || address.empty()) {
		static const std::string on = strgettext("On");
		static const std::string off = strgettext("Off");
		// Note: Status of enable_damage and creative_mode settings is intentionally
		// NOT shown here because the game might roll its own damage system and/or do
		// a per-player Creative Mode, in which case writing it here would mislead.
		bool damage = g_settings->getBool("enable_damage");
		const std::string &announced = g_settings->getBool("server_announce") ? on : off;
		if (!simple_singleplayer_mode) {
			if (damage) {
				const std::string &pvp = g_settings->getBool("enable_pvp") ? on : off;
				//~ PvP = Player versus Player
				os << strgettext("- PvP: ") << pvp << "\n";
			}
			os << strgettext("- Public: ") << announced << "\n";
			std::string server_name = g_settings->get("server_name");
			str_formspec_escape(server_name);
			if (announced == on && !server_name.empty())
				os << strgettext("- Server Name: ") << server_name;

		}
	}
	os << ";]";

	/* Create menu */
	/* Note: FormspecFormSource and LocalFormspecHandler  *
	 * are deleted by guiFormSpecMenu                     */
	FormspecFormSource *fs_src = new FormspecFormSource(os.str());
	LocalFormspecHandler *txt_dst = new LocalFormspecHandler("MT_PAUSE_MENU");

	auto *&formspec = m_game_ui->getFormspecGUI();
	GUIFormSpecMenu::create(formspec, client, m_rendering_engine->get_gui_env(),
			&input->joystick, fs_src, txt_dst, client->getFormspecPrepend(), sound);
	formspec->setFocus("btn_continue");
	// game will be paused in next step, if in singleplayer (see m_is_paused)
	formspec->doPause = true;
}

/****************************************************************************/
/****************************************************************************
 extern function for launching the game
 ****************************************************************************/
/****************************************************************************/

void the_game(bool *kill,
		InputHandler *input,
		RenderingEngine *rendering_engine,
		const GameStartData &start_data,
		std::string &error_message,
		ChatBackend &chat_backend,
		bool *reconnect_requested) // Used for local game
{
	Game game;

	/* Make a copy of the server address because if a local singleplayer server
	 * is created then this is updated and we don't want to change the value
	 * passed to us by the calling function
	 */

	try {

		if (game.startup(kill, input, rendering_engine, start_data,
				error_message, reconnect_requested, &chat_backend)) {
			game.run();
		}

	} catch (SerializationError &e) {
		const std::string ver_err = fmtgettext("The server is probably running a different version of %s.", PROJECT_NAME_C);
		error_message = strgettext("A serialization error occurred:") +"\n"
				+ e.what() + "\n\n" + ver_err;
		errorstream << error_message << std::endl;
	} catch (ServerError &e) {
		error_message = e.what();
		errorstream << "ServerError: " << error_message << std::endl;
	} catch (ModError &e) {
		// DO NOT TRANSLATE the `ModError`, it's used by ui.lua
		error_message = std::string("ModError: ") + e.what() +
				strgettext("\nCheck debug.txt for details.");
		errorstream << error_message << std::endl;
	}
	game.shutdown();
}
