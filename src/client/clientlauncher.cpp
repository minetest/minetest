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

#include "mainmenumanager.h"
#include "debug.h"
#include "clouds.h"
#include "server.h"
#include "filesys.h"
#include "guiMainMenu.h"
#include "game.h"
#include "chat.h"
#include "gettext.h"
#include "profiler.h"
#include "log.h"
#include "serverlist.h"
#include "guiEngine.h"
#include "player.h"
#include "fontengine.h"
#include "clientlauncher.h"

/* mainmenumanager.h
 */
gui::IGUIEnvironment *guienv = NULL;
gui::IGUIStaticText *guiroot = NULL;
MainMenuManager g_menumgr;

bool noMenuActive()
{
	return g_menumgr.menuCount() == 0;
}

// Passed to menus to allow disconnecting and exiting
MainGameCallback *g_gamecallback = NULL;


// Instance of the time getter
static TimeGetter *g_timegetter = NULL;

u32 getTimeMs()
{
	if (g_timegetter == NULL)
		return 0;
	return g_timegetter->getTime(PRECISION_MILLI);
}

u32 getTime(TimePrecision prec) {
	if (g_timegetter == NULL)
		return 0;
	return g_timegetter->getTime(prec);
}

ClientLauncher::~ClientLauncher()
{
	if (receiver)
		delete receiver;

	if (input)
		delete input;

	if (g_fontengine)
		delete g_fontengine;

	if (device)
		device->drop();
}


bool ClientLauncher::run(GameParams &game_params, const Settings &cmd_args)
{
	init_args(game_params, cmd_args);

	// List video modes if requested
	if (list_video_modes)
		return print_video_modes();

	if (!init_engine()) {
		errorstream << "Could not initialize game engine." << std::endl;
		return false;
	}

	// Create time getter
	g_timegetter = new IrrlichtTimeGetter(device);

	// Speed tests (done after irrlicht is loaded to get timer)
	if (cmd_args.getFlag("speedtests")) {
		dstream << "Running speed tests" << std::endl;
		speed_tests();
		return true;
	}

	video::IVideoDriver *video_driver = device->getVideoDriver();
	if (video_driver == NULL) {
		errorstream << "Could not initialize video driver." << std::endl;
		return false;
	}

	porting::setXorgClassHint(video_driver->getExposedVideoData(), PROJECT_NAME_C);

	/*
		This changes the minimum allowed number of vertices in a VBO.
		Default is 500.
	*/
	//driver->setMinHardwareBufferVertexCount(50);

	// Create game callback for menus
	g_gamecallback = new MainGameCallback(device);

	device->setResizable(true);

	if (random_input)
		input = new RandomInputHandler();
	else
		input = new RealInputHandler(device, receiver);

	smgr = device->getSceneManager();
	smgr->getParameters()->setAttribute(scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);

	guienv = device->getGUIEnvironment();
	skin = guienv->getSkin();
	skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255, 255, 255, 255));
	skin->setColor(gui::EGDC_3D_LIGHT, video::SColor(0, 0, 0, 0));
	skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(255, 30, 30, 30));
	skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(255, 0, 0, 0));
	skin->setColor(gui::EGDC_HIGH_LIGHT, video::SColor(255, 70, 120, 50));
	skin->setColor(gui::EGDC_HIGH_LIGHT_TEXT, video::SColor(255, 255, 255, 255));

	g_fontengine = new FontEngine(g_settings, guienv);
	FATAL_ERROR_IF(g_fontengine == NULL, "Font engine creation failed.");

#if (IRRLICHT_VERSION_MAJOR >= 1 && IRRLICHT_VERSION_MINOR >= 8) || IRRLICHT_VERSION_MAJOR >= 2
	// Irrlicht 1.8 input colours
	skin->setColor(gui::EGDC_EDITABLE, video::SColor(255, 128, 128, 128));
	skin->setColor(gui::EGDC_FOCUSED_EDITABLE, video::SColor(255, 96, 134, 49));
#endif

	// Create the menu clouds
	if (!g_menucloudsmgr)
		g_menucloudsmgr = smgr->createNewSceneManager();
	if (!g_menuclouds)
		g_menuclouds = new Clouds(g_menucloudsmgr->getRootSceneNode(),
				g_menucloudsmgr, -1, rand(), 100);
	g_menuclouds->update(v2f(0, 0), video::SColor(255, 200, 200, 255));
	scene::ICameraSceneNode* camera;
	camera = g_menucloudsmgr->addCameraSceneNode(0,
				v3f(0, 0, 0), v3f(0, 60, 100));
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

	while (device->run() && !*kill && !g_gamecallback->shutdown_requested)
	{
		// Set the window caption
		const wchar_t *text = wgettext("Main Menu");
		device->setWindowCaption((utf8_to_wide(PROJECT_NAME_C) + L" [" + text + L"]").c_str());
		delete[] text;

		try {	// This is used for catching disconnects

			guienv->clear();

			/*
				We need some kind of a root node to be able to add
				custom gui elements directly on the screen.
				Otherwise they won't be automatically drawn.
			*/
			guiroot = guienv->addStaticText(L"", core::rect<s32>(0, 0, 10000, 10000));

			bool game_has_run = launch_game(error_message, reconnect_requested,
				game_params, cmd_args);

			// Reset the reconnect_requested flag
			reconnect_requested = false;

			// If skip_main_menu, we only want to startup once
			if (skip_main_menu && !first_loop)
				break;

			first_loop = false;

			if (!game_has_run) {
				if (skip_main_menu)
					break;
				else
					continue;
			}

			// Break out of menu-game loop to shut down cleanly
			if (!device->run() || *kill) {
				if (g_settings_path != "")
					g_settings->updateConfigFile(g_settings_path.c_str());
				break;
			}

			if (current_playername.length() > PLAYERNAME_SIZE-1) {
				error_message = gettext("Player name too long.");
				playername = current_playername.substr(0, PLAYERNAME_SIZE-1);
				g_settings->set("name", playername);
				continue;
			}

			device->getVideoDriver()->setTextureCreationFlag(
					video::ETCF_CREATE_MIP_MAPS, g_settings->getBool("mip_map"));

#ifdef HAVE_TOUCHSCREENGUI
			receiver->m_touchscreengui = new TouchScreenGUI(device, receiver);
			g_touchscreengui = receiver->m_touchscreengui;
#endif

			the_game(
				kill,
				random_input,
				input,
				device,
				worldspec.path,
				current_playername,
				current_password,
				current_address,
				current_port,
				error_message,
				chat_backend,
				&reconnect_requested,
				gamespec,
				simple_singleplayer_mode
			);
			smgr->clear();

#ifdef HAVE_TOUCHSCREENGUI
			delete g_touchscreengui;
			g_touchscreengui = NULL;
			receiver->m_touchscreengui = NULL;
#endif

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

void ClientLauncher::init_args(GameParams &game_params, const Settings &cmd_args)
{

	skip_main_menu = cmd_args.getFlag("go");

	// FIXME: This is confusing (but correct)

	/* If world_path is set then override it unless skipping the main menu using
	 * the --go command line param. Else, give preference to the address
	 * supplied on the command line
	 */
	address = g_settings->get("address");
	if (game_params.world_path != "" && !skip_main_menu)
		address = "";
	else if (cmd_args.exists("address"))
		address = cmd_args.get("address");

	playername = g_settings->get("name");
	if (cmd_args.exists("name"))
		playername = cmd_args.get("name");

	list_video_modes = cmd_args.getFlag("videomodes");

	use_freetype = g_settings->getBool("freetype");

	random_input = g_settings->getBool("random_input")
			|| cmd_args.getFlag("random-input");
}

bool ClientLauncher::init_engine()
{
	receiver = new MyEventReceiver();
	create_engine_device();
	return device != NULL;
}

bool ClientLauncher::launch_game(std::string &error_message,
		bool reconnect_requested, GameParams &game_params,
		const Settings &cmd_args)
{
	// Initialize menu data
	MainMenuData menudata;
	menudata.address                         = address;
	menudata.name                            = playername;
	menudata.password                        = password;
	menudata.port                            = itos(game_params.socket_port);
	menudata.script_data.errormessage        = error_message;
	menudata.script_data.reconnect_requested = reconnect_requested;

	error_message.clear();

	if (cmd_args.exists("password"))
		menudata.password = cmd_args.get("password");

	menudata.enable_public = g_settings->getBool("server_announce");

	// If a world was commanded, append and select it
	if (game_params.world_path != "") {
		worldspec.gameid = getWorldGameId(game_params.world_path, true);
		worldspec.name = _("[--world parameter]");

		if (worldspec.gameid == "") {	// Create new
			worldspec.gameid = g_settings->get("default_game");
			worldspec.name += " [new]";
		}
		worldspec.path = game_params.world_path;
	}

	/* Show the GUI menu
	 */
	if (!skip_main_menu) {
		main_menu(&menudata);

		// Skip further loading if there was an exit signal.
		if (*porting::signal_handler_killstatus())
			return false;

		address = menudata.address;
		int newport = stoi(menudata.port);
		if (newport != 0)
			game_params.socket_port = newport;

		simple_singleplayer_mode = menudata.simple_singleplayer_mode;

		std::vector<WorldSpec> worldspecs = getAvailableWorlds();

		if (menudata.selected_world >= 0
				&& menudata.selected_world < (int)worldspecs.size()) {
			g_settings->set("selected_world_path",
					worldspecs[menudata.selected_world].path);
			worldspec = worldspecs[menudata.selected_world];
		}
	}

	if (!menudata.script_data.errormessage.empty()) {
		/* The calling function will pass this back into this function upon the
		 * next iteration (if any) causing it to be displayed by the GUI
		 */
		error_message = menudata.script_data.errormessage;
		return false;
	}

	if (menudata.name == "")
		menudata.name = std::string("Guest") + itos(myrand_range(1000, 9999));
	else
		playername = menudata.name;

	password = menudata.password;

	g_settings->set("name", playername);

	current_playername = playername;
	current_password   = password;
	current_address    = address;
	current_port       = game_params.socket_port;

	// If using simple singleplayer mode, override
	if (simple_singleplayer_mode) {
		assert(skip_main_menu == false);
		current_playername = "singleplayer";
		current_password = "";
		current_address = "";
		current_port = myrand_range(49152, 65535);
	} else if (address != "") {
		ServerListSpec server;
		server["name"] = menudata.servername;
		server["address"] = menudata.address;
		server["port"] = menudata.port;
		server["description"] = menudata.serverdescription;
		ServerList::insert(server);
	}

	infostream << "Selected world: " << worldspec.name
	           << " [" << worldspec.path << "]" << std::endl;

	if (current_address == "") { // If local game
		if (worldspec.path == "") {
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
		gamespec = findWorldSubgame(worldspec.path);
		if (!gamespec.isValid() && !game_params.game_spec.isValid()) {
			error_message = gettext("Could not find or load game \"")
					+ worldspec.gameid + "\"";
			errorstream << error_message << std::endl;
			return false;
		}

		if (porting::signal_handler_killstatus())
			return true;

		if (game_params.game_spec.isValid() &&
				game_params.game_spec.id != worldspec.gameid) {
			warningstream << "Overriding gamespec from \""
			            << worldspec.gameid << "\" to \""
			            << game_params.game_spec.id << "\"" << std::endl;
			gamespec = game_params.game_spec;
		}

		if (!gamespec.isValid()) {
			error_message = gettext("Invalid gamespec.");
			error_message += " (world.gameid=" + worldspec.gameid + ")";
			errorstream << error_message << std::endl;
			return false;
		}
	}

	return true;
}

void ClientLauncher::main_menu(MainMenuData *menudata)
{
	bool *kill = porting::signal_handler_killstatus();
	video::IVideoDriver *driver = device->getVideoDriver();

	infostream << "Waiting for other menus" << std::endl;
	while (device->run() && *kill == false) {
		if (noMenuActive())
			break;
		driver->beginScene(true, true, video::SColor(255, 128, 128, 128));
		guienv->drawAll();
		driver->endScene();
		// On some computers framerate doesn't seem to be automatically limited
		sleep_ms(25);
	}
	infostream << "Waited for other menus" << std::endl;

	// Cursor can be non-visible when coming from the game
#ifndef ANDROID
	device->getCursorControl()->setVisible(true);
#endif

	/* show main menu */
	GUIEngine mymenu(device, guiroot, &g_menumgr, smgr, menudata, *kill);

	smgr->clear();	/* leave scene manager in a clean state */
}

bool ClientLauncher::create_engine_device()
{
	// Resolution selection
	bool fullscreen = g_settings->getBool("fullscreen");
	u16 screenW = g_settings->getU16("screenW");
	u16 screenH = g_settings->getU16("screenH");

	// bpp, fsaa, vsync
	bool vsync = g_settings->getBool("vsync");
	u16 bits = g_settings->getU16("fullscreen_bpp");
	u16 fsaa = g_settings->getU16("fsaa");

	// stereo buffer required for pageflip stereo
	bool stereo_buffer = g_settings->get("3d_mode") == "pageflip";

	// Determine driver
	video::E_DRIVER_TYPE driverType = video::EDT_OPENGL;
	std::string driverstring = g_settings->get("video_driver");
	std::vector<video::E_DRIVER_TYPE> drivers
		= porting::getSupportedVideoDrivers();
	u32 i;
	for (i = 0; i != drivers.size(); i++) {
		if (!strcasecmp(driverstring.c_str(),
			porting::getVideoDriverName(drivers[i]))) {
			driverType = drivers[i];
			break;
		}
	}
	if (i == drivers.size()) {
		errorstream << "Invalid video_driver specified; "
			"defaulting to opengl" << std::endl;
	}

	SIrrlichtCreationParameters params = SIrrlichtCreationParameters();
	params.DriverType    = driverType;
	params.WindowSize    = core::dimension2d<u32>(screenW, screenH);
	params.Bits          = bits;
	params.AntiAlias     = fsaa;
	params.Fullscreen    = fullscreen;
	params.Stencilbuffer = false;
	params.Stereobuffer  = stereo_buffer;
	params.Vsync         = vsync;
	params.EventReceiver = receiver;
	params.HighPrecisionFPU = g_settings->getBool("high_precision_fpu");
	params.ZBufferBits   = 24;
#ifdef __ANDROID__
	params.PrivateData = porting::app_global;
	params.OGLES2ShaderPath = std::string(porting::path_user + DIR_DELIM +
			"media" + DIR_DELIM + "Shaders" + DIR_DELIM).c_str();
#endif

	device = createDeviceEx(params);

	if (device) {
		porting::initIrrlicht(device);
	}

	return device != NULL;
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
	tempstring = std::string();
	tempstring2 = std::string();

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

		Mutex m;
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

bool ClientLauncher::print_video_modes()
{
	IrrlichtDevice *nulldevice;

	bool vsync = g_settings->getBool("vsync");
	u16 fsaa = g_settings->getU16("fsaa");
	MyEventReceiver* receiver = new MyEventReceiver();

	SIrrlichtCreationParameters params = SIrrlichtCreationParameters();
	params.DriverType    = video::EDT_NULL;
	params.WindowSize    = core::dimension2d<u32>(640, 480);
	params.Bits          = 24;
	params.AntiAlias     = fsaa;
	params.Fullscreen    = false;
	params.Stencilbuffer = false;
	params.Vsync         = vsync;
	params.EventReceiver = receiver;
	params.HighPrecisionFPU = g_settings->getBool("high_precision_fpu");

	nulldevice = createDeviceEx(params);

	if (nulldevice == NULL) {
		delete receiver;
		return false;
	}

	std::cout << _("Available video modes (WxHxD):") << std::endl;

	video::IVideoModeList *videomode_list = nulldevice->getVideoModeList();

	if (videomode_list != NULL) {
		s32 videomode_count = videomode_list->getVideoModeCount();
		core::dimension2d<u32> videomode_res;
		s32 videomode_depth;
		for (s32 i = 0; i < videomode_count; ++i) {
			videomode_res = videomode_list->getVideoModeResolution(i);
			videomode_depth = videomode_list->getVideoModeDepth(i);
			std::cout << videomode_res.Width << "x" << videomode_res.Height
			        << "x" << videomode_depth << std::endl;
		}

		std::cout << _("Active video mode (WxHxD):") << std::endl;
		videomode_res = videomode_list->getDesktopResolution();
		videomode_depth = videomode_list->getDesktopDepth();
		std::cout << videomode_res.Width << "x" << videomode_res.Height
		        << "x" << videomode_depth << std::endl;

	}

	nulldevice->drop();
	delete receiver;

	return videomode_list != NULL;
}
