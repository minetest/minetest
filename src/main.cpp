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

#ifdef NDEBUG
	/*#ifdef _WIN32
		#pragma message ("Disabling unit tests")
	#else
		#warning "Disabling unit tests"
	#endif*/
	// Disable unit tests
	#define ENABLE_TESTS 0
#else
	// Enable unit tests
	#define ENABLE_TESTS 1
#endif

#ifdef _MSC_VER
#ifndef SERVER // Dedicated server isn't linked with Irrlicht
	#pragma comment(lib, "Irrlicht.lib")
	// This would get rid of the console window
	//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif
	#pragma comment(lib, "zlibwapi.lib")
	#pragma comment(lib, "Shell32.lib")
#endif

#include "irrlicht.h" // createDevice

#include "main.h"
#include "mainmenumanager.h"
#include <iostream>
#include <fstream>
#include <locale.h>
#include "irrlichttypes_extrabloated.h"
#include "debug.h"
#include "test.h"
#include "clouds.h"
#include "server.h"
#include "constants.h"
#include "porting.h"
#include "gettime.h"
#include "filesys.h"
#include "config.h"
#include "version.h"
#include "guiMainMenu.h"
#include "game.h"
#include "keycode.h"
#include "tile.h"
#include "chat.h"
#include "defaultsettings.h"
#include "gettext.h"
#include "settings.h"
#include "profiler.h"
#include "log.h"
#include "mods.h"
#include "util/string.h"
#include "subgame.h"
#include "quicktune.h"
#include "serverlist.h"
#include "httpfetch.h"
#include "guiEngine.h"
#include "mapsector.h"
#include "player.h"
#include "fontengine.h"

#include "database-sqlite3.h"
#ifdef USE_LEVELDB
#include "database-leveldb.h"
#endif

#if USE_REDIS
#include "database-redis.h"
#endif

#ifdef HAVE_TOUCHSCREENGUI
#include "touchscreengui.h"
#endif

/*
	Settings.
	These are loaded from the config file.
*/
static Settings main_settings;
Settings *g_settings = &main_settings;
std::string g_settings_path;

// Global profiler
Profiler main_profiler;
Profiler *g_profiler = &main_profiler;

// Menu clouds are created later
Clouds *g_menuclouds = 0;
irr::scene::ISceneManager *g_menucloudsmgr = 0;

/*
	Debug streams
*/

// Connection
std::ostream *dout_con_ptr = &dummyout;
std::ostream *derr_con_ptr = &verbosestream;

// Server
std::ostream *dout_server_ptr = &infostream;
std::ostream *derr_server_ptr = &errorstream;

// Client
std::ostream *dout_client_ptr = &infostream;
std::ostream *derr_client_ptr = &errorstream;

#define DEBUGFILE "debug.txt"
#define DEFAULT_SERVER_PORT 30000

typedef std::map<std::string, ValueSpec> OptionList;

struct GameParams {
	u16 socket_port;
	std::string world_path;
	SubgameSpec game_spec;
	bool is_dedicated_server;
	int log_level;
};

/**********************************************************************
 * Private functions
 **********************************************************************/

static bool get_cmdline_opts(int argc, char *argv[], Settings *cmd_args);
static void set_allowed_options(OptionList *allowed_options);

static void print_help(const OptionList &allowed_options);
static void print_allowed_options(const OptionList &allowed_options);
static void print_version();
static void print_worldspecs(const std::vector<WorldSpec> &worldspecs,
							 std::ostream &os);
static void print_modified_quicktune_values();

static void list_game_ids();
static void list_worlds();
static void setup_log_params(const Settings &cmd_args);
static bool create_userdata_path();
static bool init_common(int *log_level, const Settings &cmd_args, int argc, char *argv[]);
static void startup_message();
static bool read_config_file(const Settings &cmd_args);
static void init_debug_streams(int *log_level, const Settings &cmd_args);

static bool game_configure(GameParams *game_params, const Settings &cmd_args);
static void game_configure_port(GameParams *game_params, const Settings &cmd_args);

static bool game_configure_world(GameParams *game_params, const Settings &cmd_args);
static bool get_world_from_cmdline(GameParams *game_params, const Settings &cmd_args);
static bool get_world_from_config(GameParams *game_params, const Settings &cmd_args);
static bool auto_select_world(GameParams *game_params);
static std::string get_clean_world_path(const std::string &path);

static bool game_configure_subgame(GameParams *game_params, const Settings &cmd_args);
static bool get_game_from_cmdline(GameParams *game_params, const Settings &cmd_args);
static bool determine_subgame(GameParams *game_params);

static bool run_dedicated_server(const GameParams &game_params, const Settings &cmd_args);
static bool migrate_database(const GameParams &game_params, const Settings &cmd_args,
		Server *server);

#ifndef SERVER
static bool print_video_modes();
static void speed_tests();
#endif

/**********************************************************************/

#ifndef SERVER
/*
	Random stuff
*/

/* mainmenumanager.h */

gui::IGUIEnvironment* guienv = NULL;
gui::IGUIStaticText *guiroot = NULL;
MainMenuManager g_menumgr;

bool noMenuActive()
{
	return (g_menumgr.menuCount() == 0);
}

// Passed to menus to allow disconnecting and exiting
MainGameCallback *g_gamecallback = NULL;
#endif

/*
	gettime.h implementation
*/

#ifdef SERVER

u32 getTimeMs()
{
	/* Use imprecise system calls directly (from porting.h) */
	return porting::getTime(PRECISION_MILLI);
}

u32 getTime(TimePrecision prec)
{
	return porting::getTime(prec);
}

#else

// A small helper class
class TimeGetter
{
public:
	virtual u32 getTime(TimePrecision prec) = 0;
};

// A precise irrlicht one
class IrrlichtTimeGetter: public TimeGetter
{
public:
	IrrlichtTimeGetter(IrrlichtDevice *device):
		m_device(device)
	{}
	u32 getTime(TimePrecision prec)
	{
		if (prec == PRECISION_MILLI) {
			if (m_device == NULL)
				return 0;
			return m_device->getTimer()->getRealTime();
		} else {
			return porting::getTime(prec);
		}
	}
private:
	IrrlichtDevice *m_device;
};
// Not so precise one which works without irrlicht
class SimpleTimeGetter: public TimeGetter
{
public:
	u32 getTime(TimePrecision prec)
	{
		return porting::getTime(prec);
	}
};

// A pointer to a global instance of the time getter
// TODO: why?
TimeGetter *g_timegetter = NULL;

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
#endif

class StderrLogOutput: public ILogOutput
{
public:
	/* line: Full line with timestamp, level and thread */
	void printLog(const std::string &line)
	{
		std::cerr << line << std::endl;
	}
} main_stderr_log_out;

class DstreamNoStderrLogOutput: public ILogOutput
{
public:
	/* line: Full line with timestamp, level and thread */
	void printLog(const std::string &line)
	{
		dstream_no_stderr << line << std::endl;
	}
} main_dstream_no_stderr_log_out;

#ifndef SERVER

/*
	Event handler for Irrlicht

	NOTE: Everything possible should be moved out from here,
	      probably to InputHandler and the_game
*/

class MyEventReceiver : public IEventReceiver
{
public:
	// This is the one method that we have to implement
	virtual bool OnEvent(const SEvent& event)
	{
		/*
			React to nothing here if a menu is active
		*/
		if (noMenuActive() == false) {
#ifdef HAVE_TOUCHSCREENGUI
			if (m_touchscreengui != 0) {
				m_touchscreengui->Toggle(false);
			}
#endif
			return g_menumgr.preprocessEvent(event);
		}

		// Remember whether each key is down or up
		if (event.EventType == irr::EET_KEY_INPUT_EVENT) {
			if (event.KeyInput.PressedDown) {
				keyIsDown.set(event.KeyInput);
				keyWasDown.set(event.KeyInput);
			} else {
				keyIsDown.unset(event.KeyInput);
			}
		}

#ifdef HAVE_TOUCHSCREENGUI
		// case of touchscreengui we have to handle different events
		if ((m_touchscreengui != 0) &&
				(event.EventType == irr::EET_TOUCH_INPUT_EVENT)) {
			m_touchscreengui->translateEvent(event);
			return true;
		}
#endif
		// handle mouse events
		if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
			if (noMenuActive() == false) {
				left_active = false;
				middle_active = false;
				right_active = false;
			} else {
				left_active = event.MouseInput.isLeftPressed();
				middle_active = event.MouseInput.isMiddlePressed();
				right_active = event.MouseInput.isRightPressed();

				if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
					leftclicked = true;
				}
				if (event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) {
					rightclicked = true;
				}
				if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
					leftreleased = true;
				}
				if (event.MouseInput.Event == EMIE_RMOUSE_LEFT_UP) {
					rightreleased = true;
				}
				if (event.MouseInput.Event == EMIE_MOUSE_WHEEL) {
					mouse_wheel += event.MouseInput.Wheel;
				}
			}
		}
		if (event.EventType == irr::EET_LOG_TEXT_EVENT) {
			dstream << std::string("Irrlicht log: ") + std::string(event.LogEvent.Text)
			        << std::endl;
			return true;
		}
		/* always return false in order to continue processing events */
		return false;
	}

	bool IsKeyDown(const KeyPress &keyCode) const
	{
		return keyIsDown[keyCode];
	}

	// Checks whether a key was down and resets the state
	bool WasKeyDown(const KeyPress &keyCode)
	{
		bool b = keyWasDown[keyCode];
		if (b)
			keyWasDown.unset(keyCode);
		return b;
	}

	s32 getMouseWheel()
	{
		s32 a = mouse_wheel;
		mouse_wheel = 0;
		return a;
	}

	void clearInput()
	{
		keyIsDown.clear();
		keyWasDown.clear();

		leftclicked = false;
		rightclicked = false;
		leftreleased = false;
		rightreleased = false;

		left_active = false;
		middle_active = false;
		right_active = false;

		mouse_wheel = 0;
	}

	MyEventReceiver()
	{
		clearInput();
#ifdef HAVE_TOUCHSCREENGUI
		m_touchscreengui = NULL;
#endif
	}

	bool leftclicked;
	bool rightclicked;
	bool leftreleased;
	bool rightreleased;

	bool left_active;
	bool middle_active;
	bool right_active;

	s32 mouse_wheel;

#ifdef HAVE_TOUCHSCREENGUI
	TouchScreenGUI* m_touchscreengui;
#endif

private:
	// The current state of keys
	KeyList keyIsDown;
	// Whether a key has been pressed or not
	KeyList keyWasDown;
};

/*
	Separated input handler
*/

class RealInputHandler : public InputHandler
{
public:
	RealInputHandler(IrrlichtDevice *device, MyEventReceiver *receiver):
		m_device(device),
		m_receiver(receiver),
		m_mousepos(0,0)
	{
	}
	virtual bool isKeyDown(const KeyPress &keyCode)
	{
		return m_receiver->IsKeyDown(keyCode);
	}
	virtual bool wasKeyDown(const KeyPress &keyCode)
	{
		return m_receiver->WasKeyDown(keyCode);
	}
	virtual v2s32 getMousePos()
	{
		if (m_device->getCursorControl()) {
			return m_device->getCursorControl()->getPosition();
		}
		else {
			return m_mousepos;
		}
	}
	virtual void setMousePos(s32 x, s32 y)
	{
		if (m_device->getCursorControl()) {
			m_device->getCursorControl()->setPosition(x, y);
		}
		else {
			m_mousepos = v2s32(x,y);
		}
	}

	virtual bool getLeftState()
	{
		return m_receiver->left_active;
	}
	virtual bool getRightState()
	{
		return m_receiver->right_active;
	}

	virtual bool getLeftClicked()
	{
		return m_receiver->leftclicked;
	}
	virtual bool getRightClicked()
	{
		return m_receiver->rightclicked;
	}
	virtual void resetLeftClicked()
	{
		m_receiver->leftclicked = false;
	}
	virtual void resetRightClicked()
	{
		m_receiver->rightclicked = false;
	}

	virtual bool getLeftReleased()
	{
		return m_receiver->leftreleased;
	}
	virtual bool getRightReleased()
	{
		return m_receiver->rightreleased;
	}
	virtual void resetLeftReleased()
	{
		m_receiver->leftreleased = false;
	}
	virtual void resetRightReleased()
	{
		m_receiver->rightreleased = false;
	}

	virtual s32 getMouseWheel()
	{
		return m_receiver->getMouseWheel();
	}

	void clear()
	{
		m_receiver->clearInput();
	}
private:
	IrrlichtDevice  *m_device;
	MyEventReceiver *m_receiver;
	v2s32           m_mousepos;
};

class RandomInputHandler : public InputHandler
{
public:
	RandomInputHandler()
	{
		leftdown = false;
		rightdown = false;
		leftclicked = false;
		rightclicked = false;
		leftreleased = false;
		rightreleased = false;
		keydown.clear();
	}
	virtual bool isKeyDown(const KeyPress &keyCode)
	{
		return keydown[keyCode];
	}
	virtual bool wasKeyDown(const KeyPress &keyCode)
	{
		return false;
	}
	virtual v2s32 getMousePos()
	{
		return mousepos;
	}
	virtual void setMousePos(s32 x, s32 y)
	{
		mousepos = v2s32(x, y);
	}

	virtual bool getLeftState()
	{
		return leftdown;
	}
	virtual bool getRightState()
	{
		return rightdown;
	}

	virtual bool getLeftClicked()
	{
		return leftclicked;
	}
	virtual bool getRightClicked()
	{
		return rightclicked;
	}
	virtual void resetLeftClicked()
	{
		leftclicked = false;
	}
	virtual void resetRightClicked()
	{
		rightclicked = false;
	}

	virtual bool getLeftReleased()
	{
		return leftreleased;
	}
	virtual bool getRightReleased()
	{
		return rightreleased;
	}
	virtual void resetLeftReleased()
	{
		leftreleased = false;
	}
	virtual void resetRightReleased()
	{
		rightreleased = false;
	}

	virtual s32 getMouseWheel()
	{
		return 0;
	}

	virtual void step(float dtime)
	{
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_jump"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_special1"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_forward"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_left"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 20);
				mousespeed = v2s32(Rand(-20, 20), Rand(-15, 20));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 30);
				leftdown = !leftdown;
				if (leftdown)
					leftclicked = true;
				if (!leftdown)
					leftreleased = true;
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 15);
				rightdown = !rightdown;
				if (rightdown)
					rightclicked = true;
				if (!rightdown)
					rightreleased = true;
			}
		}
		mousepos += mousespeed;
	}

	s32 Rand(s32 min, s32 max)
	{
		return (myrand()%(max-min+1))+min;
	}
private:
	KeyList keydown;
	v2s32 mousepos;
	v2s32 mousespeed;
	bool leftdown;
	bool rightdown;
	bool leftclicked;
	bool rightclicked;
	bool leftreleased;
	bool rightreleased;
};


class ClientLauncher
{
public:
	ClientLauncher() :
		list_video_modes(false),
		skip_main_menu(false),
		use_freetype(false),
		random_input(false),
		address(""),
		playername(""),
		password(""),
		device(NULL),
		input(NULL),
		receiver(NULL),
		skin(NULL),
		font(NULL),
		simple_singleplayer_mode(false),
		current_playername("inv£lid"),
		current_password(""),
		current_address("does-not-exist"),
		current_port(0)
	{}

	~ClientLauncher();

	bool run(GameParams &game_params, const Settings &cmd_args);

protected:
	void init_args(GameParams &game_params, const Settings &cmd_args);
	bool init_engine(int log_level);

	bool launch_game(std::wstring *error_message, GameParams &game_params,
			const Settings &cmd_args);

	void main_menu(MainMenuData *menudata);
	bool create_engine_device(int log_level);

	bool list_video_modes;
	bool skip_main_menu;
	bool use_freetype;
	bool random_input;
	std::string address;
	std::string playername;
	std::string password;
	IrrlichtDevice *device;
	InputHandler *input;
	MyEventReceiver *receiver;
	gui::IGUISkin *skin;
	gui::IGUIFont *font;
	scene::ISceneManager *smgr;
	SubgameSpec gamespec;
	WorldSpec worldspec;
	bool simple_singleplayer_mode;

	// These are set up based on the menu and other things
	// TODO: Are these required since there's already playername, password, etc
	std::string current_playername;
	std::string current_password;
	std::string current_address;
	int current_port;
};

#endif // !SERVER

static OptionList allowed_options;

int main(int argc, char *argv[])
{
	int retval;

	debug_set_exception_handler();

	log_add_output_maxlev(&main_stderr_log_out, LMT_ACTION);
	log_add_output_all_levs(&main_dstream_no_stderr_log_out);

	log_register_thread("main");

	Settings cmd_args;
	bool cmd_args_ok = get_cmdline_opts(argc, argv, &cmd_args);
	if (!cmd_args_ok
			|| cmd_args.getFlag("help")
			|| cmd_args.exists("nonopt1")) {
		print_help(allowed_options);
		return cmd_args_ok ? 0 : 1;
	}

	if (cmd_args.getFlag("version")) {
		print_version();
		return 0;
	}

	setup_log_params(cmd_args);

	porting::signal_handler_init();
	porting::initializePaths();

	if (!create_userdata_path()) {
		errorstream << "Cannot create user data directory" << std::endl;
		return 1;
	}

	// Initialize debug stacks
	debug_stacks_init();
	DSTACK(__FUNCTION_NAME);

	// Debug handler
	BEGIN_DEBUG_EXCEPTION_HANDLER

	// List gameids if requested
	if (cmd_args.exists("gameid") && cmd_args.get("gameid") == "list") {
		list_game_ids();
		return 0;
	}

	// List worlds if requested
	if (cmd_args.exists("world") && cmd_args.get("world") == "list") {
		list_worlds();
		return 0;
	}

	GameParams game_params;
	if (!init_common(&game_params.log_level, cmd_args, argc, argv))
		return 1;

#ifndef __ANDROID__
	// Run unit tests
	if ((ENABLE_TESTS && cmd_args.getFlag("disable-unittests") == false)
			|| cmd_args.getFlag("enable-unittests") == true) {
		run_tests();
	}
#endif

#ifdef SERVER
	game_params.is_dedicated_server = true;
#else
	game_params.is_dedicated_server = cmd_args.getFlag("server");
#endif

	if (!game_configure(&game_params, cmd_args))
		return 1;

	assert(game_params.world_path != "");

	infostream << "Using commanded world path ["
	           << game_params.world_path << "]" << std::endl;

	//Run dedicated server if asked to or no other option
	g_settings->set("server_dedicated",
			game_params.is_dedicated_server ? "true" : "false");

	if (game_params.is_dedicated_server)
		return run_dedicated_server(game_params, cmd_args) ? 0 : 1;

#ifndef SERVER
	ClientLauncher launcher;
	retval = launcher.run(game_params, cmd_args) ? 0 : 1;
#else
	retval = 0;
#endif

	// Update configuration file
	if (g_settings_path != "")
		g_settings->updateConfigFile(g_settings_path.c_str());

	print_modified_quicktune_values();

	// Stop httpfetch thread (if started)
	httpfetch_cleanup();

	END_DEBUG_EXCEPTION_HANDLER(errorstream)

	return retval;
}


/*****************************************************************************
 * Startup / Init
 *****************************************************************************/


static bool get_cmdline_opts(int argc, char *argv[], Settings *cmd_args)
{
	set_allowed_options(&allowed_options);

	return cmd_args->parseCommandLine(argc, argv, allowed_options);
}

static void set_allowed_options(OptionList *allowed_options)
{
	allowed_options->clear();

	allowed_options->insert(std::make_pair("help", ValueSpec(VALUETYPE_FLAG,
			_("Show allowed options"))));
	allowed_options->insert(std::make_pair("version", ValueSpec(VALUETYPE_FLAG,
			_("Show version information"))));
	allowed_options->insert(std::make_pair("config", ValueSpec(VALUETYPE_STRING,
			_("Load configuration from specified file"))));
	allowed_options->insert(std::make_pair("port", ValueSpec(VALUETYPE_STRING,
			_("Set network port (UDP)"))));
	allowed_options->insert(std::make_pair("disable-unittests", ValueSpec(VALUETYPE_FLAG,
			_("Disable unit tests"))));
	allowed_options->insert(std::make_pair("enable-unittests", ValueSpec(VALUETYPE_FLAG,
			_("Enable unit tests"))));
	allowed_options->insert(std::make_pair("map-dir", ValueSpec(VALUETYPE_STRING,
			_("Same as --world (deprecated)"))));
	allowed_options->insert(std::make_pair("world", ValueSpec(VALUETYPE_STRING,
			_("Set world path (implies local game) ('list' lists all)"))));
	allowed_options->insert(std::make_pair("worldname", ValueSpec(VALUETYPE_STRING,
			_("Set world by name (implies local game)"))));
	allowed_options->insert(std::make_pair("quiet", ValueSpec(VALUETYPE_FLAG,
			_("Print to console errors only"))));
	allowed_options->insert(std::make_pair("info", ValueSpec(VALUETYPE_FLAG,
			_("Print more information to console"))));
	allowed_options->insert(std::make_pair("verbose",  ValueSpec(VALUETYPE_FLAG,
			_("Print even more information to console"))));
	allowed_options->insert(std::make_pair("trace", ValueSpec(VALUETYPE_FLAG,
			_("Print enormous amounts of information to log and console"))));
	allowed_options->insert(std::make_pair("logfile", ValueSpec(VALUETYPE_STRING,
			_("Set logfile path ('' = no logging)"))));
	allowed_options->insert(std::make_pair("gameid", ValueSpec(VALUETYPE_STRING,
			_("Set gameid (\"--gameid list\" prints available ones)"))));
	allowed_options->insert(std::make_pair("migrate", ValueSpec(VALUETYPE_STRING,
			_("Migrate from current map backend to another (Only works when using minetestserver or with --server)"))));
#ifndef SERVER
	allowed_options->insert(std::make_pair("videomodes", ValueSpec(VALUETYPE_FLAG,
			_("Show available video modes"))));
	allowed_options->insert(std::make_pair("speedtests", ValueSpec(VALUETYPE_FLAG,
			_("Run speed tests"))));
	allowed_options->insert(std::make_pair("address", ValueSpec(VALUETYPE_STRING,
			_("Address to connect to. ('' = local game)"))));
	allowed_options->insert(std::make_pair("random-input", ValueSpec(VALUETYPE_FLAG,
			_("Enable random user input, for testing"))));
	allowed_options->insert(std::make_pair("server", ValueSpec(VALUETYPE_FLAG,
			_("Run dedicated server"))));
	allowed_options->insert(std::make_pair("name", ValueSpec(VALUETYPE_STRING,
			_("Set player name"))));
	allowed_options->insert(std::make_pair("password", ValueSpec(VALUETYPE_STRING,
			_("Set password"))));
	allowed_options->insert(std::make_pair("go", ValueSpec(VALUETYPE_FLAG,
			_("Disable main menu"))));
#endif

}

static void print_help(const OptionList &allowed_options)
{
	dstream << _("Allowed options:") << std::endl;
	print_allowed_options(allowed_options);
}

static void print_allowed_options(const OptionList &allowed_options)
{
	for (OptionList::const_iterator i = allowed_options.begin();
			i != allowed_options.end(); ++i) {
		std::ostringstream os1(std::ios::binary);
		os1 << "  --" << i->first;
		if (i->second.type != VALUETYPE_FLAG)
			os1 << _(" <value>");

		dstream << padStringRight(os1.str(), 24);

		if (i->second.help != NULL)
			dstream << i->second.help;

		dstream << std::endl;
	}
}

static void print_version()
{
#ifdef SERVER
	dstream << "minetestserver " << minetest_version_hash << std::endl;
#else
	dstream << "Minetest " << minetest_version_hash << std::endl;
	dstream << "Using Irrlicht " << IRRLICHT_SDK_VERSION << std::endl;
#endif
	dstream << "Build info: " << minetest_build_info << std::endl;
}

static void list_game_ids()
{
	std::set<std::string> gameids = getAvailableGameIds();
	for (std::set<std::string>::const_iterator i = gameids.begin();
			i != gameids.end(); i++)
		dstream << (*i) <<std::endl;
}

static void list_worlds()
{
	dstream << _("Available worlds:") << std::endl;
	std::vector<WorldSpec> worldspecs = getAvailableWorlds();
	print_worldspecs(worldspecs, dstream);
}

static void print_worldspecs(const std::vector<WorldSpec> &worldspecs,
							 std::ostream &os)
{
	for (size_t i = 0; i < worldspecs.size(); i++) {
		std::string name = worldspecs[i].name;
		std::string path = worldspecs[i].path;
		if (name.find(" ") != std::string::npos)
			name = std::string("'") + name + "'";
		path = std::string("'") + path + "'";
		name = padStringRight(name, 14);
		os << "  " << name << " " << path << std::endl;
	}
}

static void print_modified_quicktune_values()
{
	bool header_printed = false;
	std::vector<std::string> names = getQuicktuneNames();

	for (u32 i = 0; i < names.size(); i++) {
		QuicktuneValue val = getQuicktuneValue(names[i]);
		if (!val.modified)
			continue;
		if (!header_printed) {
			dstream << "Modified quicktune values:" << std::endl;
			header_printed = true;
		}
		dstream << names[i] << " = " << val.getString() << std::endl;
	}
}

static void setup_log_params(const Settings &cmd_args)
{
	// Quiet mode, print errors only
	if (cmd_args.getFlag("quiet")) {
		log_remove_output(&main_stderr_log_out);
		log_add_output_maxlev(&main_stderr_log_out, LMT_ERROR);
	}

	// If trace is enabled, enable logging of certain things
	if (cmd_args.getFlag("trace")) {
		dstream << _("Enabling trace level debug output") << std::endl;
		log_trace_level_enabled = true;
		dout_con_ptr = &verbosestream; // this is somewhat old crap
		socket_enable_debug_output = true; // socket doesn't use log.h
	}

	// In certain cases, output info level on stderr
	if (cmd_args.getFlag("info") || cmd_args.getFlag("verbose") ||
			cmd_args.getFlag("trace") || cmd_args.getFlag("speedtests"))
		log_add_output(&main_stderr_log_out, LMT_INFO);

	// In certain cases, output verbose level on stderr
	if (cmd_args.getFlag("verbose") || cmd_args.getFlag("trace"))
		log_add_output(&main_stderr_log_out, LMT_VERBOSE);
}

static bool create_userdata_path()
{
	bool success;

#ifdef __ANDROID__
	porting::initAndroid();

	porting::setExternalStorageDir(porting::jnienv);
	if (!fs::PathExists(porting::path_user)) {
		success = fs::CreateDir(porting::path_user);
	} else {
		success = true;
	}
	porting::copyAssets();
#else
	// Create user data directory
	success = fs::CreateDir(porting::path_user);
#endif

	infostream << "path_share = " << porting::path_share << std::endl;
	infostream << "path_user  = " << porting::path_user << std::endl;

	return success;
}

static bool init_common(int *log_level, const Settings &cmd_args, int argc, char *argv[])
{
	startup_message();
	set_default_settings(g_settings);

	// Initialize sockets
	sockets_init();
	atexit(sockets_cleanup);

	if (!read_config_file(cmd_args))
		return false;

	init_debug_streams(log_level, cmd_args);

	// Initialize random seed
	srand(time(0));
	mysrand(time(0));

	// Initialize HTTP fetcher
	httpfetch_init(g_settings->getS32("curl_parallel_limit"));

#ifdef _MSC_VER
	init_gettext((porting::path_share + DIR_DELIM + "locale").c_str(),
		g_settings->get("language"), argc, argv);
#else
	init_gettext((porting::path_share + DIR_DELIM + "locale").c_str(),
		g_settings->get("language"));
#endif

	return true;
}

static void startup_message()
{
	infostream << PROJECT_NAME << " " << _("with")
	           << " SER_FMT_VER_HIGHEST_READ="
               << (int)SER_FMT_VER_HIGHEST_READ << ", "
               << minetest_build_info << std::endl;
}

static bool read_config_file(const Settings &cmd_args)
{
	// Path of configuration file in use
	assert(g_settings_path == "");	// Sanity check

	if (cmd_args.exists("config")) {
		bool r = g_settings->readConfigFile(cmd_args.get("config").c_str());
		if (!r) {
			errorstream << "Could not read configuration from \""
			            << cmd_args.get("config") << "\"" << std::endl;
			return false;
		}
		g_settings_path = cmd_args.get("config");
	} else {
		std::vector<std::string> filenames;
		filenames.push_back(porting::path_user + DIR_DELIM + "minetest.conf");
		// Legacy configuration file location
		filenames.push_back(porting::path_user +
				DIR_DELIM + ".." + DIR_DELIM + "minetest.conf");

#if RUN_IN_PLACE
		// Try also from a lower level (to aid having the same configuration
		// for many RUN_IN_PLACE installs)
		filenames.push_back(porting::path_user +
				DIR_DELIM + ".." + DIR_DELIM + ".." + DIR_DELIM + "minetest.conf");
#endif

		for (size_t i = 0; i < filenames.size(); i++) {
			bool r = g_settings->readConfigFile(filenames[i].c_str());
			if (r) {
				g_settings_path = filenames[i];
				break;
			}
		}

		// If no path found, use the first one (menu creates the file)
		if (g_settings_path == "")
			g_settings_path = filenames[0];
	}

	return true;
}

static void init_debug_streams(int *log_level, const Settings &cmd_args)
{
#if RUN_IN_PLACE
	std::string logfile = DEBUGFILE;
#else
	std::string logfile = porting::path_user + DIR_DELIM + DEBUGFILE;
#endif
	if (cmd_args.exists("logfile"))
		logfile = cmd_args.get("logfile");

	log_remove_output(&main_dstream_no_stderr_log_out);
	*log_level = g_settings->getS32("debug_log_level");

	if (*log_level == 0) //no logging
		logfile = "";
	if (*log_level < 0) {
		dstream << "WARNING: Supplied debug_log_level < 0; Using 0" << std::endl;
		*log_level = 0;
	} else if (*log_level > LMT_NUM_VALUES) {
		dstream << "WARNING: Supplied debug_log_level > " << LMT_NUM_VALUES
		        << "; Using " << LMT_NUM_VALUES << std::endl;
		*log_level = LMT_NUM_VALUES;
	}

	log_add_output_maxlev(&main_dstream_no_stderr_log_out,
			(LogMessageLevel)(*log_level - 1));

	debugstreams_init(false, logfile == "" ? NULL : logfile.c_str());

	infostream << "logfile = " << logfile << std::endl;

	atexit(debugstreams_deinit);
}

static bool game_configure(GameParams *game_params, const Settings &cmd_args)
{
	game_configure_port(game_params, cmd_args);

	if (!game_configure_world(game_params, cmd_args)) {
		errorstream << "No world path specified or found." << std::endl;
		return false;
	}

	game_configure_subgame(game_params, cmd_args);

	return true;
}

static void game_configure_port(GameParams *game_params, const Settings &cmd_args)
{
	if (cmd_args.exists("port"))
		game_params->socket_port = cmd_args.getU16("port");
	else
		game_params->socket_port = g_settings->getU16("port");

	if (game_params->socket_port == 0)
		game_params->socket_port = DEFAULT_SERVER_PORT;
}

static bool game_configure_world(GameParams *game_params, const Settings &cmd_args)
{
	if (get_world_from_cmdline(game_params, cmd_args))
		return true;
	if (get_world_from_config(game_params, cmd_args))
		return true;

	return auto_select_world(game_params);
}

static bool get_world_from_cmdline(GameParams *game_params, const Settings &cmd_args)
{
	std::string commanded_world = "";

	// World name
	std::string commanded_worldname = "";
	if (cmd_args.exists("worldname"))
		commanded_worldname = cmd_args.get("worldname");

	// If a world name was specified, convert it to a path
	if (commanded_worldname != "") {
		// Get information about available worlds
		std::vector<WorldSpec> worldspecs = getAvailableWorlds();
		bool found = false;
		for (u32 i = 0; i < worldspecs.size(); i++) {
			std::string name = worldspecs[i].name;
			if (name == commanded_worldname) {
				dstream << _("Using world specified by --worldname on the "
					"command line") << std::endl;
				commanded_world = worldspecs[i].path;
				found = true;
				break;
			}
		}
		if (!found) {
			dstream << _("World") << " '" << commanded_worldname
			        << _("' not available. Available worlds:") << std::endl;
			print_worldspecs(worldspecs, dstream);
			return false;
		}

		game_params->world_path = get_clean_world_path(commanded_world);
		return commanded_world != "";
	}

	if (cmd_args.exists("world"))
		commanded_world = cmd_args.get("world");
	else if (cmd_args.exists("map-dir"))
		commanded_world = cmd_args.get("map-dir");
	else if (cmd_args.exists("nonopt0")) // First nameless argument
		commanded_world = cmd_args.get("nonopt0");

	game_params->world_path = get_clean_world_path(commanded_world);
	return commanded_world != "";
}

static bool get_world_from_config(GameParams *game_params, const Settings &cmd_args)
{
	// World directory
	std::string commanded_world = "";

	if (g_settings->exists("map-dir"))
		commanded_world = g_settings->get("map-dir");

	game_params->world_path = get_clean_world_path(commanded_world);

	return commanded_world != "";
}

static bool auto_select_world(GameParams *game_params)
{
	// No world was specified; try to select it automatically
	// Get information about available worlds

	verbosestream << _("Determining world path") << std::endl;

	std::vector<WorldSpec> worldspecs = getAvailableWorlds();
	std::string world_path;

	// If there is only a single world, use it
	if (worldspecs.size() == 1) {
		world_path = worldspecs[0].path;
		dstream <<_("Automatically selecting world at") << " ["
		        << world_path << "]" << std::endl;
	// If there are multiple worlds, list them
	} else if (worldspecs.size() > 1 && game_params->is_dedicated_server) {
		dstream << _("Multiple worlds are available.") << std::endl;
		dstream << _("Please select one using --worldname <name>"
				" or --world <path>") << std::endl;
		print_worldspecs(worldspecs, dstream);
		return false;
	// If there are no worlds, automatically create a new one
	} else {
		// This is the ultimate default world path
		world_path = porting::path_user + DIR_DELIM + "worlds" +
				DIR_DELIM + "world";
		infostream << "Creating default world at ["
		           << world_path << "]" << std::endl;
	}

	assert(world_path != "");
	game_params->world_path = world_path;
	return true;
}

static std::string get_clean_world_path(const std::string &path)
{
	const std::string worldmt = "world.mt";
	std::string clean_path;

	if (path.size() > worldmt.size()
			&& path.substr(path.size() - worldmt.size()) == worldmt) {
		dstream << _("Supplied world.mt file - stripping it off.") << std::endl;
		clean_path = path.substr(0, path.size() - worldmt.size());
	} else {
		clean_path = path;
	}
	return path;
}


static bool game_configure_subgame(GameParams *game_params, const Settings &cmd_args)
{
	bool success;

	success = get_game_from_cmdline(game_params, cmd_args);
	if (!success)
		success = determine_subgame(game_params);

	return success;
}

static bool get_game_from_cmdline(GameParams *game_params, const Settings &cmd_args)
{
	SubgameSpec commanded_gamespec;

	if (cmd_args.exists("gameid")) {
		std::string gameid = cmd_args.get("gameid");
		commanded_gamespec = findSubgame(gameid);
		if (!commanded_gamespec.isValid()) {
			errorstream << "Game \"" << gameid << "\" not found" << std::endl;
			return false;
		}
		dstream << _("Using game specified by --gameid on the command line")
		        << std::endl;
		game_params->game_spec = commanded_gamespec;
		return true;
	}

	return false;
}

static bool determine_subgame(GameParams *game_params)
{
	SubgameSpec gamespec;

	assert(game_params->world_path != "");	// pre-condition

	verbosestream << _("Determining gameid/gamespec") << std::endl;
	// If world doesn't exist
	if (game_params->world_path != ""
			&& !getWorldExists(game_params->world_path)) {
		// Try to take gamespec from command line
		if (game_params->game_spec.isValid()) {
			gamespec = game_params->game_spec;
			infostream << "Using commanded gameid [" << gamespec.id << "]" << std::endl;
		} else { // Otherwise we will be using "minetest"
			gamespec = findSubgame(g_settings->get("default_game"));
			infostream << "Using default gameid [" << gamespec.id << "]" << std::endl;
			if (!gamespec.isValid()) {
				errorstream << "Subgame specified in default_game ["
				            << g_settings->get("default_game")
				            << "] is invalid." << std::endl;
				return false;
			}
		}
	} else { // World exists
		std::string world_gameid = getWorldGameId(game_params->world_path, false);
		// If commanded to use a gameid, do so
		if (game_params->game_spec.isValid()) {
			gamespec = game_params->game_spec;
			if (game_params->game_spec.id != world_gameid) {
				errorstream << "WARNING: Using commanded gameid ["
				            << gamespec.id << "]" << " instead of world gameid ["
				            << world_gameid << "]" << std::endl;
			}
		} else {
			// If world contains an embedded game, use it;
			// Otherwise find world from local system.
			gamespec = findWorldSubgame(game_params->world_path);
			infostream << "Using world gameid [" << gamespec.id << "]" << std::endl;
		}
	}

	if (!gamespec.isValid()) {
		errorstream << "Subgame [" << gamespec.id << "] could not be found."
		            << std::endl;
		return false;
	}

	game_params->game_spec = gamespec;
	return true;
}


/*****************************************************************************
 * Dedicated server
 *****************************************************************************/
static bool run_dedicated_server(const GameParams &game_params, const Settings &cmd_args)
{
	DSTACK("Dedicated server branch");

	verbosestream << _("Using world path") << " ["
	              << game_params.world_path << "]" << std::endl;
	verbosestream << _("Using gameid") << " ["
	              << game_params.game_spec.id << "]" << std::endl;

	// Bind address
	std::string bind_str = g_settings->get("bind_address");
	Address bind_addr(0, 0, 0, 0, game_params.socket_port);

	if (g_settings->getBool("ipv6_server")) {
		bind_addr.setAddress((IPv6AddressBytes*) NULL);
	}
	try {
		bind_addr.Resolve(bind_str.c_str());
	} catch (ResolveError &e) {
		infostream << "Resolving bind address \"" << bind_str
		           << "\" failed: " << e.what()
		           << " -- Listening on all addresses." << std::endl;
	}
	if (bind_addr.isIPv6() && !g_settings->getBool("enable_ipv6")) {
		errorstream << "Unable to listen on "
		            << bind_addr.serializeString()
		            << L" because IPv6 is disabled" << std::endl;
		return false;
	}

	// Create server
	Server server(game_params.world_path,
			game_params.game_spec, false, bind_addr.isIPv6());

	// Database migration
	if (cmd_args.exists("migrate"))
		return migrate_database(game_params, cmd_args, &server);

	server.start(bind_addr);

	// Run server
	bool &kill = *porting::signal_handler_killstatus();
	dedicated_server_loop(server, kill);

	return true;
}

static bool migrate_database(const GameParams &game_params, const Settings &cmd_args,
		Server *server)
{
	Settings world_mt;
	bool success = world_mt.readConfigFile((game_params.world_path
			+ DIR_DELIM + "world.mt").c_str());
	if (!success) {
		errorstream << "Cannot read world.mt" << std::endl;
		return false;
	}

	if (!world_mt.exists("backend")) {
		errorstream << "Please specify your current backend in world.mt file:"
		            << std::endl << "	backend = {sqlite3|leveldb|redis|dummy}"
		            << std::endl;
		return false;
	}

	std::string backend = world_mt.get("backend");
	Database *new_db;
	std::string migrate_to = cmd_args.get("migrate");

	if (backend == migrate_to) {
		errorstream << "Cannot migrate: new backend is same as the old one"
		            << std::endl;
		return false;
	}

	if (migrate_to == "sqlite3")
		new_db = new Database_SQLite3(&(ServerMap&)server->getMap(),
				game_params.world_path);
#if USE_LEVELDB
	else if (migrate_to == "leveldb")
		new_db = new Database_LevelDB(&(ServerMap&)server->getMap(),
				game_params.world_path);
#endif
#if USE_REDIS
	else if (migrate_to == "redis")
		new_db = new Database_Redis(&(ServerMap&)server->getMap(),
				game_params.world_path);
#endif
	else {
		errorstream << "Migration to " << migrate_to << " is not supported"
		            << std::endl;
		return false;
	}

	std::list<v3s16> blocks;
	ServerMap &old_map = ((ServerMap&)server->getMap());
	old_map.listAllLoadableBlocks(blocks);
	int count = 0;
	new_db->beginSave();
	for (std::list<v3s16>::iterator i = blocks.begin(); i != blocks.end(); i++) {
		MapBlock *block = old_map.loadBlock(*i);
		if (!block) {
			errorstream << "Failed to load block " << PP(*i) << ", skipping it.";
		} else {
			old_map.saveBlock(block, new_db);
			MapSector *sector = old_map.getSectorNoGenerate(v2s16(i->X, i->Z));
			sector->deleteBlock(block);
		}
		++count;
		if (count % 500 == 0)
		   actionstream << "Migrated " << count << " blocks "
			   << (100.0 * count / blocks.size()) << "% completed" << std::endl;
	}
	new_db->endSave();
	delete new_db;

	actionstream << "Successfully migrated " << count << " blocks" << std::endl;
	world_mt.set("backend", migrate_to);
	if (!world_mt.updateConfigFile(
				(game_params.world_path+ DIR_DELIM + "world.mt").c_str()))
		errorstream << "Failed to update world.mt!" << std::endl;
	else
		actionstream << "world.mt updated" << std::endl;

	return true;
}


/*****************************************************************************
 * Client
 *****************************************************************************/
#ifndef SERVER

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

	if (!init_engine(game_params.log_level)) {
		errorstream << "Could not initialize game engine." << std::endl;
		return false;
	}

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

	porting::setXorgClassHint(video_driver->getExposedVideoData(), "Minetest");

	/*
		This changes the minimum allowed number of vertices in a VBO.
		Default is 500.
	*/
	//driver->setMinHardwareBufferVertexCount(50);

	// Create time getter
	g_timegetter = new IrrlichtTimeGetter(device);

	// Create game callback for menus
	g_gamecallback = new MainGameCallback(device);

	device->setResizable(true);

	if (random_input)
		input = new RandomInputHandler();
	else
		input = new RealInputHandler(device, receiver);

	smgr = device->getSceneManager();

	guienv = device->getGUIEnvironment();
	skin = guienv->getSkin();
	skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255, 255, 255, 255));
	skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(255, 0, 0, 0));
	skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(255, 0, 0, 0));
	skin->setColor(gui::EGDC_HIGH_LIGHT, video::SColor(255, 70, 100, 50));
	skin->setColor(gui::EGDC_HIGH_LIGHT_TEXT, video::SColor(255, 255, 255, 255));

	g_fontengine = new FontEngine(g_settings, guienv);
	assert(g_fontengine != NULL);

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
	// It is then displayed before	the menu shows on the next call to menu()
	std::wstring error_message = L"";

	bool first_loop = true;

	/*
		Menu-game loop
	*/
	bool retval = true;
	bool *kill = porting::signal_handler_killstatus();

	while (device->run() && !*kill && !g_gamecallback->shutdown_requested)
	{
		// Set the window caption
		wchar_t *text = wgettext("Main Menu");
		device->setWindowCaption((std::wstring(L"Minetest [") + text + L"]").c_str());
		delete[] text;

		try {	// This is used for catching disconnects

			guienv->clear();

			/*
				We need some kind of a root node to be able to add
				custom gui elements directly on the screen.
				Otherwise they won't be automatically drawn.
			*/
			guiroot = guienv->addStaticText(L"", core::rect<s32>(0, 0, 10000, 10000));

			bool game_has_run = launch_game(&error_message, game_params, cmd_args);

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
				error_message = wgettext("Player name too long.");
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
			error_message = wgettext("Connection error (timed out?)");
			errorstream << wide_to_narrow(error_message) << std::endl;
		}

#ifdef NDEBUG
		catch (std::exception &e) {
			std::string narrow_message = "Some exception: \"";
			narrow_message += e.what();
			narrow_message += "\"";
			errorstream << narrow_message << std::endl;
			error_message = narrow_to_wide(narrow_message);
		}
#endif

		// If no main menu, show error and exit
		if (skip_main_menu) {
			if (error_message != L"") {
				verbosestream << "error_message = "
				              << wide_to_narrow(error_message) << std::endl;
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

bool ClientLauncher::init_engine(int log_level)
{
	receiver = new MyEventReceiver();
	create_engine_device(log_level);
	return device != NULL;
}

bool ClientLauncher::launch_game(std::wstring *error_message,
		GameParams &game_params, const Settings &cmd_args)
{
	// Initialize menu data
	MainMenuData menudata;
	menudata.address      = address;
	menudata.name         = playername;
	menudata.port         = itos(game_params.socket_port);
	menudata.errormessage = wide_to_narrow(*error_message);

	*error_message = L"";

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

	if (menudata.errormessage != "") {
		/* The calling function will pass this back into this function upon the
		 * next iteration (if any) causing it to be displayed by the GUI
		 */
		*error_message = narrow_to_wide(menudata.errormessage);
		return false;
	}

	if (menudata.name == "")
		menudata.name = std::string("Guest") + itos(myrand_range(1000, 9999));
	else
		playername = menudata.name;

	password = translatePassword(playername, narrow_to_wide(menudata.password));

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
			*error_message = wgettext("No world selected and no address "
					"provided. Nothing to do.");
			errorstream << wide_to_narrow(*error_message) << std::endl;
			return false;
		}

		if (!fs::PathExists(worldspec.path)) {
			*error_message = wgettext("Provided world path doesn't exist: ")
					+ narrow_to_wide(worldspec.path);
			errorstream << wide_to_narrow(*error_message) << std::endl;
			return false;
		}

		// Load gamespec for required game
		gamespec = findWorldSubgame(worldspec.path);
		if (!gamespec.isValid() && !game_params.game_spec.isValid()) {
			*error_message = wgettext("Could not find or load game \"")
					+ narrow_to_wide(worldspec.gameid) + L"\"";
			errorstream << wide_to_narrow(*error_message) << std::endl;
			return false;
		}

		if (porting::signal_handler_killstatus())
			return true;

		if (game_params.game_spec.isValid() &&
				game_params.game_spec.id != worldspec.gameid) {
			errorstream << "WARNING: Overriding gamespec from \""
			            << worldspec.gameid << "\" to \""
			            << game_params.game_spec.id << "\"" << std::endl;
			gamespec = game_params.game_spec;
		}

		if (!gamespec.isValid()) {
			*error_message = wgettext("Invalid gamespec.");
			*error_message += L" (world_gameid="
					+ narrow_to_wide(worldspec.gameid) + L")";
			errorstream << wide_to_narrow(*error_message) << std::endl;
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

bool ClientLauncher::create_engine_device(int log_level)
{
	static const irr::ELOG_LEVEL irr_log_level[5] = {
		ELL_NONE,
		ELL_ERROR,
		ELL_WARNING,
		ELL_INFORMATION,
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
		ELL_INFORMATION
#else
		ELL_DEBUG
#endif
	};

	// Resolution selection
	bool fullscreen = g_settings->getBool("fullscreen");
	u16 screenW = g_settings->getU16("screenW");
	u16 screenH = g_settings->getU16("screenH");

	// bpp, fsaa, vsync
	bool vsync = g_settings->getBool("vsync");
	u16 bits = g_settings->getU16("fullscreen_bpp");
	u16 fsaa = g_settings->getU16("fsaa");

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
	params.Vsync         = vsync;
	params.EventReceiver = receiver;
	params.HighPrecisionFPU = g_settings->getBool("high_precision_fpu");
#ifdef __ANDROID__
	params.PrivateData = porting::app_global;
	params.OGLES2ShaderPath = std::string(porting::path_user + DIR_DELIM +
			"media" + DIR_DELIM + "Shaders" + DIR_DELIM).c_str();
#endif

	device = createDeviceEx(params);

	if (device) {
		// Map our log level to irrlicht engine one.
		ILogger* irr_logger = device->getLogger();
		irr_logger->setLogLevel(irr_log_level[log_level]);

		porting::initIrrlicht(device);
	}

	return device != NULL;
}

// Misc functions

static bool print_video_modes()
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

	dstream << _("Available video modes (WxHxD):") << std::endl;

	video::IVideoModeList *videomode_list = nulldevice->getVideoModeList();

	if (videomode_list != NULL) {
		s32 videomode_count = videomode_list->getVideoModeCount();
		core::dimension2d<u32> videomode_res;
		s32 videomode_depth;
		for (s32 i = 0; i < videomode_count; ++i) {
			videomode_res = videomode_list->getVideoModeResolution(i);
			videomode_depth = videomode_list->getVideoModeDepth(i);
			dstream << videomode_res.Width << "x" << videomode_res.Height
			        << "x" << videomode_depth << std::endl;
		}

		dstream << _("Active video mode (WxHxD):") << std::endl;
		videomode_res = videomode_list->getDesktopResolution();
		videomode_depth = videomode_list->getDesktopDepth();
		dstream << videomode_res.Width << "x" << videomode_res.Height
		        << "x" << videomode_depth << std::endl;

	}

	nulldevice->drop();
	delete receiver;

	return videomode_list != NULL;
}

#endif // !SERVER

/*****************************************************************************
 * Performance tests
 *****************************************************************************/
#ifndef SERVER
static void speed_tests()
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

		JMutex m;
		u32 n = 0;
		u32 i = 0;
		do {
			n += 10000;
			for (; i < n; i++) {
				m.Lock();
				m.Unlock();
			}
		}
		// Do at least 10ms
		while(timer.getTimerTime() < 10);

		u32 dtime = timer.stop();
		u32 per_ms = n / dtime;
		infostream << "Done. " << dtime << "ms, " << per_ms << "/ms" << std::endl;
	}
}
#endif
