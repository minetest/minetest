/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#ifndef GAME_HPP_
#define GAME_HPP_

#include "client/clientevent.h"
#include "client/gameui.h"
#include "client/inputhandler.h"
#include "gui/profilergraph.h"
#include "server.h"

#include "client/game/GameOnDemandSoundFetcher.h"

class ChatBackend;
class ClientActiveObject;
class Clouds;
class GUIChatConsole;
class GUIFormSpecMenu;
class IWritableShaderSource;
class QuicktuneShortcutter;
class SoundMaker;
class Sky;

#ifdef __ANDROID__
#define SIZE_TAG "size[11,5.5]"
#else
#define SIZE_TAG "size[11,5.5,true]" // Fixed size on desktop
#endif

const float object_hit_delay = 0.2;

struct FpsControl {
	u32 last_time, busy_time, sleep_time;
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
	bool ldown_for_dig;
	bool dig_instantly;
	bool digging_blocked;
	bool left_punch;
	bool update_wielded_item_trigger;
	bool reset_jump_timer;
	float nodig_delay_timer;
	float dig_time;
	float dig_time_complete;
	float repeat_rightclick_timer;
	float object_hit_delay_timer;
	float time_from_last_punch;
	ClientActiveObject *selected_object;

	float jump_timer;
	float damage_flash;
	float update_draw_list_timer;

	f32 fog_range;

	v3f update_draw_list_last_cam_dir;

	float time_of_day_smooth;
};

class Game;

struct ClientEventHandler
{
	void (Game::*handler)(ClientEvent *, CameraOrientation *);
};

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
			bool random_input,
			InputHandler *input,
			const std::string &map_dir,
			const std::string &playername,
			const std::string &password,
			// If address is "", local server is used and address is updated
			std::string *address,
			u16 port,
			std::string &error_message,
			bool *reconnect,
			ChatBackend *chat_backend,
			const SubgameSpec &gamespec,    // Used for local game
			bool simple_singleplayer_mode);

	void run();
	void shutdown();

protected:

	void extendedResourceCleanup();

	// Basic initialisation
	bool init(const std::string &map_dir, std::string *address,
			u16 port,
			const SubgameSpec &gamespec);
	bool initSound();
	bool createSingleplayerServer(const std::string &map_dir,
			const SubgameSpec &gamespec, u16 port, std::string *address);

	// Client creation
	bool createClient(const std::string &playername,
			const std::string &password, std::string *address, u16 port);
	bool initGui();

	// Client connection
	bool connectToServer(const std::string &playername,
			const std::string &password, std::string *address, u16 port,
			bool *connect_ok, bool *aborted);
	bool getServerContent(bool *aborted);

	// Main loop

	void updateInteractTimers(f32 dtime);
	bool checkConnection();
	bool handleCallbacks();
	void processQueues();
	void updateProfilers(const RunStats &stats, const FpsControl &draw_times, f32 dtime);
	void addProfilerGraphs(const RunStats &stats, const FpsControl &draw_times, f32 dtime);
	void updateStats(RunStats *stats, const FpsControl &draw_times, f32 dtime);

	// Input related
	void processUserInput(f32 dtime);
	void processKeyInput();
	void processItemSelection(u16 *new_playeritem);

	void dropSelectedItem(bool single_item = false);
	void openInventory();
	void openConsole(float scale, const wchar_t *line=NULL);
	void toggleFreeMove();
	void toggleFreeMoveAlt();
	void toggleFast();
	void toggleNoClip();
	void toggleCinematic();
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
	void step(f32 *dtime);
	void processClientEvents(CameraOrientation *cam);
	void updateCamera(u32 busy_time, f32 dtime);
	void updateSound(f32 dtime);
	void processPlayerInteraction(f32 dtime, bool show_hud, bool show_debug);
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
		const ItemDefinition &playeritem_def, const ItemStack &playeritem,
		const ToolCapabilities &playeritem_toolcap, f32 dtime);
	void handlePointingAtObject(const PointedThing &pointed, const ItemStack &playeritem,
			const v3f &player_position, bool show_debug);
	void handleDigging(const PointedThing &pointed, const v3s16 &nodepos,
			const ToolCapabilities &playeritem_toolcap, f32 dtime);
	void updateFrame(ProfilerGraph *graph, RunStats *stats, f32 dtime,
			const CameraOrientation &cam);
	void updateProfilerGraphs(ProfilerGraph *graph);

	// Misc
	void limitFps(FpsControl *fps_timings, f32 *dtime);

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
	void handleClientEvent_OverrideDayNigthRatio(ClientEvent *event,
		CameraOrientation *cam);
	void handleClientEvent_CloudParams(ClientEvent *event, CameraOrientation *cam);

	void updateChat(f32 dtime, const v2u32 &screensize);

	bool nodePlacementPrediction(const ItemDefinition &playeritem_def,
		const ItemStack &playeritem, const v3s16 &nodepos, const v3s16 &neighbourpos);
	static const ClientEventHandler clientEventHandler[CLIENTEVENT_MAX];

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

	GUIFormSpecMenu *current_formspec = nullptr;
	//default: "". If other than "", empty show_formspec packets will only close the formspec when the formname matches
	std::string cur_formname;

	EventManager *eventmgr = nullptr;
	QuicktuneShortcutter *quicktune = nullptr;
	bool registration_confirmation_shown = false;

	std::unique_ptr<GameUI> m_game_ui;
	GUIChatConsole *gui_chat_console = nullptr; // Free using ->Drop()
	MapDrawControl *draw_control = nullptr;
	Camera *camera = nullptr;
	Clouds *clouds = nullptr;	                  // Free using ->Drop()
	Sky *sky = nullptr;                         // Free using ->Drop()
	Inventory *local_inventory = nullptr;
	Hud *hud = nullptr;
	Minimap *mapper = nullptr;

	GameRunData runData;
	Flags m_flags;

	/* 'cache'
	   This class does take ownership/responsibily for cleaning up etc of any of
	   these items (e.g. device)
	*/
	IrrlichtDevice *device;
	video::IVideoDriver *driver;
	scene::ISceneManager *smgr;
	bool *kill;
	std::string *error_message;
	bool *reconnect_requested;
	scene::ISceneNode *skybox;

	bool random_input;
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
	f32  m_repeat_right_click_time;
	f32  m_cache_cam_smoothing;
	f32  m_cache_fog_start;

	bool m_invert_mouse = false;
	bool m_first_loop_after_window_activation = false;
	bool m_camera_offset_changed = false;

	bool m_does_lost_focus_pause_game = false;

#ifdef __ANDROID__
	bool m_cache_hold_aux1;
	bool m_android_chat_open;
#endif
};

#endif // GAME_HPP_
