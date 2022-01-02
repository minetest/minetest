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

#include "gui/mainmenumanager.h"
#include "clouds.h"
#include "server.h"
#include "filesys.h"
#include "gui/guiMainMenu.h"
#include "game.h"
#include "player.h"
#include "chat.h"
#include "gettext.h"
#include "profiler.h"
#include "serverlist.h"
#include "gui/guiEngine.h"
#include "fontengine.h"
#include "clientlauncher.h"
#include "version.h"
#include "renderingengine.h"
#include "network/networkexceptions.h"

#if USE_SOUND
	#include "sound_openal.h"
#endif

/* mainmenumanager.h
 */
gui::IGUIEnvironment *guienv = nullptr;
gui::IGUIStaticText *guiroot = nullptr;
MainMenuManager g_menumgr;

bool isMenuActive()
{
	return g_menumgr.menuCount() != 0;
}

// Passed to menus to allow disconnecting and exiting
MainGameCallback *g_gamecallback = nullptr;

#if 0
// This can be helpful for the next code cleanup
static void dump_start_data(const GameStartData &data)
{
	std::cout <<
		"\ndedicated   " << (int)data.is_dedicated_server <<
		"\nport        " << data.socket_port <<
		"\nworld_path  " << data.world_spec.path <<
		"\nworld game  " << data.world_spec.gameid <<
		"\ngame path   " << data.game_spec.path <<
		"\nplayer name " << data.name <<
		"\naddress     " << data.address << std::endl;
}
#endif

ClientLauncher::~ClientLauncher()
{
	delete input;

	delete receiver;

	delete g_fontengine;
	delete g_gamecallback;

	delete m_rendering_engine;

#if USE_SOUND
	g_sound_manager_singleton.reset();
#endif
}


bool ClientLauncher::run(GameStartData &start_data, const Settings &cmd_args)
{
	/* This function is called when a client must be started.
	 * Covered cases:
	 *   - Singleplayer (address but map provided)
	 *   - Join server (no map but address provided)
	 *   - Local server (for main menu only)
	*/

	init_args(start_data, cmd_args);

#if USE_SOUND
	if (g_settings->getBool("enable_sound"))
		g_sound_manager_singleton = createSoundManagerSingleton();
#endif

	if (!init_engine()) {
		errorstream << "Could not initialize game engine." << std::endl;
		return false;
	}

	// Speed tests (done after irrlicht is loaded to get timer)
	if (cmd_args.getFlag("speedtests")) {
		dstream << "Running speed tests" << std::endl;
		speed_tests();
		return true;
	}

	if (m_rendering_engine->get_video_driver() == NULL) {
		errorstream << "Could not initialize video driver." << std::endl;
		return false;
	}

	m_rendering_engine->setupTopLevelWindow(PROJECT_NAME_C);

	/*
		This changes the minimum allowed number of vertices in a VBO.
		Default is 500.
	*/
	//driver->setMinHardwareBufferVertexCount(50);

	// Create game callback for menus
	g_gamecallback = new MainGameCallback();

	m_rendering_engine->setResizable(true);

	init_input();

	m_rendering_engine->get_scene_manager()->getParameters()->
		setAttribute(scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);

	guienv = m_rendering_engine->get_gui_env();
	skin = guienv->getSkin();
	skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255, 255, 255, 255));
	skin->setColor(gui::EGDC_3D_LIGHT, video::SColor(0, 0, 0, 0));
	skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(255, 30, 30, 30));
	skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(255, 0, 0, 0));
	skin->setColor(gui::EGDC_HIGH_LIGHT, video::SColor(255, 70, 120, 50));
	skin->setColor(gui::EGDC_HIGH_LIGHT_TEXT, video::SColor(255, 255, 255, 255));
#ifdef HAVE_TOUCHSCREENGUI
	float density = RenderingEngine::getDisplayDensity();
	skin->setSize(gui::EGDS_CHECK_BOX_WIDTH, (s32)(17.0f * density));
	skin->setSize(gui::EGDS_SCROLLBAR_SIZE, (s32)(14.0f * density));
	skin->setSize(gui::EGDS_WINDOW_BUTTON_WIDTH, (s32)(15.0f * density));
	if (density > 1.5f) {
		std::string sprite_path = porting::path_user + "/textures/base/pack/";
		if (density > 3.5f)
			sprite_path.append("checkbox_64.png");
		else if (density > 2.0f)
			sprite_path.append("checkbox_32.png");
		else
			sprite_path.append("checkbox_16.png");
		// Texture dimensions should be a power of 2
		gui::IGUISpriteBank *sprites = skin->getSpriteBank();
		video::IVideoDriver *driver = m_rendering_engine->get_video_driver();
		video::ITexture *sprite_texture = driver->getTexture(sprite_path.c_str());
		if (sprite_texture) {
			s32 sprite_id = sprites->addTextureAsSprite(sprite_texture);
			if (sprite_id != -1)
				skin->setIcon(gui::EGDI_CHECK_BOX_CHECKED, sprite_id);
		}
	}
#endif
	g_fontengine = new FontEngine(guienv);
	FATAL_ERROR_IF(g_fontengine == NULL, "Font engine creation failed.");

	// Irrlicht 1.8 input colours
	skin->setColor(gui::EGDC_EDITABLE, video::SColor(255, 128, 128, 128));
	skin->setColor(gui::EGDC_FOCUSED_EDITABLE, video::SColor(255, 96, 134, 49));

	// Create the menu clouds
	if (!g_menucloudsmgr)
		g_menucloudsmgr = m_rendering_engine->get_scene_manager()->createNewSceneManager();
	if (!g_menuclouds)
		g_menuclouds = new Clouds(g_menucloudsmgr, -1, rand());
	g_menuclouds->setHeight(100.0f);
	g_menuclouds->update(v3f(0, 0, 0), video::SColor(255, 240, 240, 255));
	scene::ICameraSceneNode* camera;
	camera = g_menucloudsmgr->addCameraSceneNode(NULL, v3f(0, 0, 0), v3f(0, 60, 100));
	camera->setFarValue(10000);

	/*
		GUI stuff
	*/

	ChatBackend chat_backend;

	// If an error occurs, this is set to something by menu().
	// It is then displayed before the menu shows on the next call to menu()
	std::string error_message;
	bool reconnect_requested = false;

	bool first_loop = true;

	/*
		Menu-game loop
	*/
	bool retval = true;
	bool *kill = porting::signal_handler_killstatus();

	while (m_rendering_engine->run() && !*kill &&
		!g_gamecallback->shutdown_requested) {
		// Set the window caption
		const wchar_t *text = wgettext("Main Menu");
		m_rendering_engine->get_raw_device()->
			setWindowCaption((utf8_to_wide(PROJECT_NAME_C) +
			L" " + utf8_to_wide(g_version_hash) +
			L" [" + text + L"]").c_str());
		delete[] text;

		try {	// This is used for catching disconnects

			m_rendering_engine->get_gui_env()->clear();

			/*
				We need some kind of a root node to be able to add
				custom gui elements directly on the screen.
				Otherwise they won't be automatically drawn.
			*/
			guiroot = m_rendering_engine->get_gui_env()->addStaticText(L"",
				core::rect<s32>(0, 0, 10000, 10000));

			bool game_has_run = launch_game(error_message, reconnect_requested,
				start_data, cmd_args);

			// Reset the reconnect_requested flag
			reconnect_requested = false;

			// If skip_main_menu, we only want to startup once
			if (skip_main_menu && !first_loop)
				break;

			first_loop = false;

			if (!game_has_run) {
				if (skip_main_menu)
					break;

				continue;
			}

			// Break out of menu-game loop to shut down cleanly
			if (!m_rendering_engine->run() || *kill) {
				if (!g_settings_path.empty())
					g_settings->updateConfigFile(g_settings_path.c_str());
				break;
			}

			m_rendering_engine->get_video_driver()->setTextureCreationFlag(
					video::ETCF_CREATE_MIP_MAPS, g_settings->getBool("mip_map"));

#ifdef HAVE_TOUCHSCREENGUI
			receiver->m_touchscreengui = new TouchScreenGUI(m_rendering_engine->get_raw_device(), receiver);
			g_touchscreengui = receiver->m_touchscreengui;
#endif

			the_game(
				kill,
				input,
				m_rendering_engine,
				start_data,
				error_message,
				chat_backend,
				&reconnect_requested
			);
		} //try
		catch (con::PeerNotFoundException &e) {
			error_message = gettext("Connection error (timed out?)");
			errorstream << error_message << std::endl;
		}

#ifdef NDEBUG
		catch (std::exception &e) {
			std::string error_message = "Some exception: \"";
			error_message += e.what();
			error_message += "\"";
			errorstream << error_message << std::endl;
		}
#endif

		m_rendering_engine->get_scene_manager()->clear();

#ifdef HAVE_TOUCHSCREENGUI
		delete g_touchscreengui;
		g_touchscreengui = NULL;
		receiver->m_touchscreengui = NULL;
#endif

		// If no main menu, show error and exit
		if (skip_main_menu) {
			if (!error_message.empty()) {
				verbosestream << "error_message = "
				              << error_message << std::endl;
				retval = false;
			}
			break;
		}
	} // Menu-game loop

	g_menuclouds->drop();
	g_menucloudsmgr->drop();

	return retval;
}

void ClientLauncher::init_args(GameStartData &start_data, const Settings &cmd_args)
{
	skip_main_menu = cmd_args.getFlag("go");

	start_data.address = g_settings->get("address");
	if (cmd_args.exists("address")) {
		// Join a remote server
		start_data.address = cmd_args.get("address");
		start_data.world_path.clear();
		start_data.name = g_settings->get("name");
	}
	if (!start_data.world_path.empty()) {
		// Start a singleplayer instance
		start_data.address = "";
	}

	if (cmd_args.exists("name"))
		start_data.name = cmd_args.get("name");

	random_input = g_settings->getBool("random_input")
			|| cmd_args.getFlag("random-input");
}

bool ClientLauncher::init_engine()
{
	receiver = new MyEventReceiver();
	m_rendering_engine = new RenderingEngine(receiver);
	return m_rendering_engine->get_raw_device() != nullptr;
}

void ClientLauncher::init_input()
{
	if (random_input)
		input = new RandomInputHandler();
	else
		input = new RealInputHandler(receiver);

	if (g_settings->getBool("enable_joysticks")) {
		irr::core::array<irr::SJoystickInfo> infos;
		std::vector<irr::SJoystickInfo> joystick_infos;

		// Make sure this is called maximum once per
		// irrlicht device, otherwise it will give you
		// multiple events for the same joystick.
		if (m_rendering_engine->get_raw_device()->activateJoysticks(infos)) {
			infostream << "Joystick support enabled" << std::endl;
			joystick_infos.reserve(infos.size());
			for (u32 i = 0; i < infos.size(); i++) {
				joystick_infos.push_back(infos[i]);
			}
			input->joystick.onJoystickConnect(joystick_infos);
		} else {
			errorstream << "Could not activate joystick support." << std::endl;
		}
	}
}

bool ClientLauncher::launch_game(std::string &error_message,
		bool reconnect_requested, GameStartData &start_data,
		const Settings &cmd_args)
{
	// Prepare and check the start data to launch a game
	std::string error_message_lua = error_message;
	error_message.clear();

	if (cmd_args.exists("password"))
		start_data.password = cmd_args.get("password");

	if (cmd_args.exists("password-file")) {
		std::ifstream passfile(cmd_args.get("password-file"));
		if (passfile.good()) {
			getline(passfile, start_data.password);
		} else {
			error_message = gettext("Provided password file "
					"failed to open: ")
					+ cmd_args.get("password-file");
			errorstream << error_message << std::endl;
			return false;
		}
	}

	// If a world was commanded, append and select it
	// This is provieded by "get_world_from_cmdline()", main.cpp
	if (!start_data.world_path.empty()) {
		auto &spec = start_data.world_spec;

		spec.path = start_data.world_path;
		spec.gameid = getWorldGameId(spec.path, true);
		spec.name = _("[--world parameter]");

		if (spec.gameid.empty()) {	// Create new
			spec.gameid = g_settings->get("default_game");
			spec.name += " [new]";
		}
	}

	/* Show the GUI menu
	 */
	std::string server_name, server_description;
	if (!skip_main_menu) {
		// Initialize menu data
		// TODO: Re-use existing structs (GameStartData)
		MainMenuData menudata;
		menudata.address                         = start_data.address;
		menudata.name                            = start_data.name;
		menudata.password                        = start_data.password;
		menudata.port                            = itos(start_data.socket_port);
		menudata.script_data.errormessage        = error_message_lua;
		menudata.script_data.reconnect_requested = reconnect_requested;

		main_menu(&menudata);

		// Skip further loading if there was an exit signal.
		if (*porting::signal_handler_killstatus())
			return false;

		if (!menudata.script_data.errormessage.empty()) {
			/* The calling function will pass this back into this function upon the
			 * next iteration (if any) causing it to be displayed by the GUI
			 */
			error_message = menudata.script_data.errormessage;
			return false;
		}

		int newport = stoi(menudata.port);
		if (newport != 0)
			start_data.socket_port = newport;

		// Update world information using main menu data
		std::vector<WorldSpec> worldspecs = getAvailableWorlds();

		int world_index = menudata.selected_world;
		if (world_index >= 0 && world_index < (int)worldspecs.size()) {
			g_settings->set("selected_world_path",
					worldspecs[world_index].path);
			start_data.world_spec = worldspecs[world_index];
		}

		start_data.name = menudata.name;
		start_data.password = menudata.password;
		start_data.address = std::move(menudata.address);
		server_name = menudata.servername;
		server_description = menudata.serverdescription;

		start_data.local_server = !menudata.simple_singleplayer_mode &&
			start_data.address.empty();
	} else {
		start_data.local_server = !start_data.world_path.empty() &&
			start_data.address.empty() && !start_data.name.empty();
	}

	if (!m_rendering_engine->run())
		return false;

	if (!start_data.isSinglePlayer() && start_data.name.empty()) {
		error_message = gettext("Please choose a name!");
		errorstream << error_message << std::endl;
		return false;
	}

	// If using simple singleplayer mode, override
	if (start_data.isSinglePlayer()) {
		start_data.name = "singleplayer";
		start_data.password = "";
		start_data.socket_port = myrand_range(49152, 65535);
	} else {
		g_settings->set("name", start_data.name);
	}

	if (start_data.name.length() > PLAYERNAME_SIZE - 1) {
		error_message = gettext("Player name too long.");
		start_data.name.resize(PLAYERNAME_SIZE);
		g_settings->set("name", start_data.name);
		return false;
	}

	auto &worldspec = start_data.world_spec;
	infostream << "Selected world: " << worldspec.name
	           << " [" << worldspec.path << "]" << std::endl;

	if (start_data.address.empty()) {
		// For singleplayer and local server
		if (worldspec.path.empty()) {
			error_message = gettext("No world selected and no address "
					"provided. Nothing to do.");
			errorstream << error_message << std::endl;
			return false;
		}

		if (!fs::PathExists(worldspec.path)) {
			error_message = gettext("Provided world path doesn't exist: ")
					+ worldspec.path;
			errorstream << error_message << std::endl;
			return false;
		}

		// Load gamespec for required game
		start_data.game_spec = findWorldSubgame(worldspec.path);
		if (!start_data.game_spec.isValid()) {
			error_message = gettext("Could not find or load game: ")
					+ worldspec.gameid;
			errorstream << error_message << std::endl;
			return false;
		}

		if (porting::signal_handler_killstatus())
			return true;

		if (!start_data.game_spec.isValid()) {
			error_message = gettext("Invalid gamespec.");
			error_message += " (world.gameid=" + worldspec.gameid + ")";
			errorstream << error_message << std::endl;
			return false;
		}
	}

	start_data.world_path = start_data.world_spec.path;
	return true;
}

void ClientLauncher::main_menu(MainMenuData *menudata)
{
	bool *kill = porting::signal_handler_killstatus();
	video::IVideoDriver *driver = m_rendering_engine->get_video_driver();

	infostream << "Waiting for other menus" << std::endl;
	while (m_rendering_engine->run() && !*kill) {
		if (!isMenuActive())
			break;
		driver->beginScene(true, true, video::SColor(255, 128, 128, 128));
		m_rendering_engine->get_gui_env()->drawAll();
		driver->endScene();
		// On some computers framerate doesn't seem to be automatically limited
		sleep_ms(25);
	}
	infostream << "Waited for other menus" << std::endl;

	// Cursor can be non-visible when coming from the game
#ifndef ANDROID
	m_rendering_engine->get_raw_device()->getCursorControl()->setVisible(true);
#endif

	/* show main menu */
	GUIEngine mymenu(&input->joystick, guiroot, m_rendering_engine, &g_menumgr, menudata, *kill);

	/* leave scene manager in a clean state */
	m_rendering_engine->get_scene_manager()->clear();
}

void ClientLauncher::speed_tests()
{
	// volatile to avoid some potential compiler optimisations
	volatile static s16 temp16;
	volatile static f32 tempf;
	static v3f tempv3f1;
	static v3f tempv3f2;
	static std::string tempstring;
	static std::string tempstring2;

	tempv3f1 = v3f();
	tempv3f2 = v3f();
	tempstring.clear();
	tempstring2.clear();

	{
		infostream << "The following test should take around 20ms." << std::endl;
		TimeTaker timer("Testing std::string speed");
		const u32 jj = 10000;
		for (u32 j = 0; j < jj; j++) {
			tempstring = "";
			tempstring2 = "";
			const u32 ii = 10;
			for (u32 i = 0; i < ii; i++) {
				tempstring2 += "asd";
			}
			for (u32 i = 0; i < ii+1; i++) {
				tempstring += "asd";
				if (tempstring == tempstring2)
					break;
			}
		}
	}

	infostream << "All of the following tests should take around 100ms each."
	           << std::endl;

	{
		TimeTaker timer("Testing floating-point conversion speed");
		tempf = 0.001;
		for (u32 i = 0; i < 4000000; i++) {
			temp16 += tempf;
			tempf += 0.001;
		}
	}

	{
		TimeTaker timer("Testing floating-point vector speed");

		tempv3f1 = v3f(1, 2, 3);
		tempv3f2 = v3f(4, 5, 6);
		for (u32 i = 0; i < 10000000; i++) {
			tempf += tempv3f1.dotProduct(tempv3f2);
			tempv3f2 += v3f(7, 8, 9);
		}
	}

	{
		TimeTaker timer("Testing std::map speed");

		std::map<v2s16, f32> map1;
		tempf = -324;
		const s16 ii = 300;
		for (s16 y = 0; y < ii; y++) {
			for (s16 x = 0; x < ii; x++) {
				map1[v2s16(x, y)] =  tempf;
				tempf += 1;
			}
		}
		for (s16 y = ii - 1; y >= 0; y--) {
			for (s16 x = 0; x < ii; x++) {
				tempf = map1[v2s16(x, y)];
			}
		}
	}

	{
		infostream << "Around 5000/ms should do well here." << std::endl;
		TimeTaker timer("Testing mutex speed");

		std::mutex m;
		u32 n = 0;
		u32 i = 0;
		do {
			n += 10000;
			for (; i < n; i++) {
				m.lock();
				m.unlock();
			}
		}
		// Do at least 10ms
		while(timer.getTimerTime() < 10);

		u32 dtime = timer.stop();
		u32 per_ms = n / dtime;
		infostream << "Done. " << dtime << "ms, " << per_ms << "/ms" << std::endl;
	}
}
