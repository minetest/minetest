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

#include "irrlichttypes.h" // must be included before anything irrlicht, see comment in the file
#include "irrlicht.h" // createDevice
#include "irrlichttypes_extrabloated.h"
#include "chat_interface.h"
#include "debug.h"
#include "unittest/test.h"
#include "server.h"
#include "filesys.h"
#include "version.h"
#include "client/game.h"
#include "defaultsettings.h"
#include "gettext.h"
#include "log.h"
#include "util/quicktune.h"
#include "httpfetch.h"
#include "gameparams.h"
#include "database/database.h"
#include "config.h"
#include "player.h"
#include "porting.h"
#include "network/socket.h"
#include "mapblock.h"
#if USE_CURSES
	#include "terminal_chat_console.h"
#endif
#ifndef SERVER
#include "gui/guiMainMenu.h"
#include "client/clientlauncher.h"
#include "gui/guiEngine.h"
#include "gui/mainmenumanager.h"
#endif
#ifdef HAVE_TOUCHSCREENGUI
	#include "gui/touchscreengui.h"
#endif

// for version information only
extern "C" {
#if USE_LUAJIT
	#include <luajit.h>
#else
	#include <lua.h>
#endif
}

#if !defined(__cpp_rtti) || !defined(__cpp_exceptions)
#error Minetest cannot be built without exceptions or RTTI
#endif

#define DEBUGFILE "debug.txt"
#define DEFAULT_SERVER_PORT 30000

typedef std::map<std::string, ValueSpec> OptionList;

/**********************************************************************
 * Private functions
 **********************************************************************/

static bool get_cmdline_opts(int argc, char *argv[], Settings *cmd_args);
static void set_allowed_options(OptionList *allowed_options);

static void print_help(const OptionList &allowed_options);
static void print_allowed_options(const OptionList &allowed_options);
static void print_version();
static void print_worldspecs(const std::vector<WorldSpec> &worldspecs,
	std::ostream &os, bool print_name = true, bool print_path = true);
static void print_modified_quicktune_values();

static void list_game_ids();
static void list_worlds(bool print_name, bool print_path);
static bool setup_log_params(const Settings &cmd_args);
static bool create_userdata_path();
static bool init_common(const Settings &cmd_args, int argc, char *argv[]);
static void uninit_common();
static void startup_message();
static bool read_config_file(const Settings &cmd_args);
static void init_log_streams(const Settings &cmd_args);

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
static bool migrate_map_database(const GameParams &game_params, const Settings &cmd_args);
static bool recompress_map_database(const GameParams &game_params, const Settings &cmd_args, const Address &addr);

/**********************************************************************/


FileLogOutput file_log_output;

static OptionList allowed_options;

int main(int argc, char *argv[])
{
	int retval;
	debug_set_exception_handler();

	g_logger.registerThread("Main");
	g_logger.addOutputMaxLevel(&stderr_output, LL_ACTION);

	Settings cmd_args;
	bool cmd_args_ok = get_cmdline_opts(argc, argv, &cmd_args);
	if (!cmd_args_ok
			|| cmd_args.getFlag("help")
			|| cmd_args.exists("nonopt1")) {
		porting::attachOrCreateConsole();
		print_help(allowed_options);
		return cmd_args_ok ? 0 : 1;
	}
	if (cmd_args.getFlag("console"))
		porting::attachOrCreateConsole();

	if (cmd_args.getFlag("version")) {
		porting::attachOrCreateConsole();
		print_version();
		return 0;
	}

	if (!setup_log_params(cmd_args))
		return 1;

	porting::signal_handler_init();

#ifdef __ANDROID__
	porting::initAndroid();
	porting::initializePathsAndroid();
#else
	porting::initializePaths();
#endif

	if (!create_userdata_path()) {
		errorstream << "Cannot create user data directory" << std::endl;
		return 1;
	}

	// Debug handler
	BEGIN_DEBUG_EXCEPTION_HANDLER

	// List gameids if requested
	if (cmd_args.exists("gameid") && cmd_args.get("gameid") == "list") {
		list_game_ids();
		return 0;
	}

	// List worlds, world names, and world paths if requested
	if (cmd_args.exists("worldlist")) {
		if (cmd_args.get("worldlist") == "name") {
			list_worlds(true, false);
		} else if (cmd_args.get("worldlist") == "path") {
			list_worlds(false, true);
		} else if (cmd_args.get("worldlist") == "both") {
			list_worlds(true, true);
		} else {
			errorstream << "Invalid --worldlist value: "
				<< cmd_args.get("worldlist") << std::endl;
			return 1;
		}
		return 0;
	}

	if (!init_common(cmd_args, argc, argv))
		return 1;

	if (g_settings->getBool("enable_console"))
		porting::attachOrCreateConsole();

#ifndef __ANDROID__
	// Run unit tests
	if (cmd_args.getFlag("run-unittests")) {
#if BUILD_UNITTESTS
		return run_tests();
#else
		errorstream << "Unittest support is not enabled in this binary. "
			<< "If you want to enable it, compile project with BUILD_UNITTESTS=1 flag."
			<< std::endl;
		return 1;
#endif
	}
#endif

	GameStartData game_params;
#ifdef SERVER
	porting::attachOrCreateConsole();
	game_params.is_dedicated_server = true;
#else
	const bool isServer = cmd_args.getFlag("server");
	if (isServer)
		porting::attachOrCreateConsole();
	game_params.is_dedicated_server = isServer;
#endif

	if (!game_configure(&game_params, cmd_args))
		return 1;

	sanity_check(!game_params.world_path.empty());

	if (game_params.is_dedicated_server)
		return run_dedicated_server(game_params, cmd_args) ? 0 : 1;

#ifndef SERVER
	retval = ClientLauncher().run(game_params, cmd_args) ? 0 : 1;
#else
	retval = 0;
#endif

	// Update configuration file
	if (!g_settings_path.empty())
		g_settings->updateConfigFile(g_settings_path.c_str());

	print_modified_quicktune_values();

	END_DEBUG_EXCEPTION_HANDLER

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
	allowed_options->insert(std::make_pair("run-unittests", ValueSpec(VALUETYPE_FLAG,
			_("Run the unit tests and exit"))));
	allowed_options->insert(std::make_pair("map-dir", ValueSpec(VALUETYPE_STRING,
			_("Same as --world (deprecated)"))));
	allowed_options->insert(std::make_pair("world", ValueSpec(VALUETYPE_STRING,
			_("Set world path (implies local game if used with option --go)"))));
	allowed_options->insert(std::make_pair("worldname", ValueSpec(VALUETYPE_STRING,
			_("Set world by name (implies local game if used with option --go)"))));
	allowed_options->insert(std::make_pair("worldlist", ValueSpec(VALUETYPE_STRING,
			_("Get list of worlds ('path' lists paths, "
			"'name' lists names, 'both' lists both)"))));
	allowed_options->insert(std::make_pair("quiet", ValueSpec(VALUETYPE_FLAG,
			_("Print to console errors only"))));
	allowed_options->insert(std::make_pair("color", ValueSpec(VALUETYPE_STRING,
			_("Coloured logs ('always', 'never' or 'auto'), defaults to 'auto'"
			))));
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
	allowed_options->insert(std::make_pair("migrate-players", ValueSpec(VALUETYPE_STRING,
		_("Migrate from current players backend to another (Only works when using minetestserver or with --server)"))));
	allowed_options->insert(std::make_pair("migrate-auth", ValueSpec(VALUETYPE_STRING,
		_("Migrate from current auth backend to another (Only works when using minetestserver or with --server)"))));
	allowed_options->insert(std::make_pair("migrate-mod-storage", ValueSpec(VALUETYPE_STRING,
		_("Migrate from current mod storage backend to another (Only works when using minetestserver or with --server)"))));
	allowed_options->insert(std::make_pair("terminal", ValueSpec(VALUETYPE_FLAG,
			_("Feature an interactive terminal (Only works when using minetestserver or with --server)"))));
	allowed_options->insert(std::make_pair("recompress", ValueSpec(VALUETYPE_FLAG,
			_("Recompress the blocks of the given map database."))));
#ifndef SERVER
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
	allowed_options->insert(std::make_pair("password-file", ValueSpec(VALUETYPE_STRING,
			_("Set password from contents of file"))));
	allowed_options->insert(std::make_pair("go", ValueSpec(VALUETYPE_FLAG,
			_("Disable main menu"))));
	allowed_options->insert(std::make_pair("console", ValueSpec(VALUETYPE_FLAG,
		_("Starts with the console (Windows only)"))));
#endif

}

static void print_help(const OptionList &allowed_options)
{
	std::cout << _("Allowed options:") << std::endl;
	print_allowed_options(allowed_options);
}

static void print_allowed_options(const OptionList &allowed_options)
{
	for (const auto &allowed_option : allowed_options) {
		std::ostringstream os1(std::ios::binary);
		os1 << "  --" << allowed_option.first;
		if (allowed_option.second.type != VALUETYPE_FLAG)
			os1 << _(" <value>");

		std::cout << padStringRight(os1.str(), 30);

		if (allowed_option.second.help)
			std::cout << allowed_option.second.help;

		std::cout << std::endl;
	}
}

static void print_version()
{
	std::cout << PROJECT_NAME_C " " << g_version_hash
		<< " (" << porting::getPlatformName() << ")" << std::endl;
#ifndef SERVER
	std::cout << "Using Irrlicht " IRRLICHT_SDK_VERSION << std::endl;
#endif
#if USE_LUAJIT
	std::cout << "Using " << LUAJIT_VERSION << std::endl;
#else
	std::cout << "Using " << LUA_RELEASE << std::endl;
#endif
	std::cout << g_build_info << std::endl;
}

static void list_game_ids()
{
	std::set<std::string> gameids = getAvailableGameIds();
	for (const std::string &gameid : gameids)
		std::cout << gameid <<std::endl;
}

static void list_worlds(bool print_name, bool print_path)
{
	std::cout << _("Available worlds:") << std::endl;
	std::vector<WorldSpec> worldspecs = getAvailableWorlds();
	print_worldspecs(worldspecs, std::cout, print_name, print_path);
}

static void print_worldspecs(const std::vector<WorldSpec> &worldspecs,
	std::ostream &os, bool print_name, bool print_path)
{
	for (const WorldSpec &worldspec : worldspecs) {
		std::string name = worldspec.name;
		std::string path = worldspec.path;
		if (print_name && print_path) {
			os << "\t" << name << "\t\t" << path << std::endl;
		} else if (print_name) {
			os << "\t" << name << std::endl;
		} else if (print_path) {
			os << "\t" << path << std::endl;
		}
	}
}

static void print_modified_quicktune_values()
{
	bool header_printed = false;
	std::vector<std::string> names = getQuicktuneNames();

	for (const std::string &name : names) {
		QuicktuneValue val = getQuicktuneValue(name);
		if (!val.modified)
			continue;
		if (!header_printed) {
			dstream << "Modified quicktune values:" << std::endl;
			header_printed = true;
		}
		dstream << name << " = " << val.getString() << std::endl;
	}
}

static bool setup_log_params(const Settings &cmd_args)
{
	// Quiet mode, print errors only
	if (cmd_args.getFlag("quiet")) {
		g_logger.removeOutput(&stderr_output);
		g_logger.addOutputMaxLevel(&stderr_output, LL_ERROR);
	}

	// Coloured log messages (see log.h)
	std::string color_mode;
	if (cmd_args.exists("color")) {
		color_mode = cmd_args.get("color");
#if !defined(_WIN32)
	} else {
		char *color_mode_env = getenv("MT_LOGCOLOR");
		if (color_mode_env)
			color_mode = color_mode_env;
#endif
	}
	if (color_mode != "") {
		if (color_mode == "auto") {
			Logger::color_mode = LOG_COLOR_AUTO;
		} else if (color_mode == "always") {
			Logger::color_mode = LOG_COLOR_ALWAYS;
		} else if (color_mode == "never") {
			Logger::color_mode = LOG_COLOR_NEVER;
		} else {
			errorstream << "Invalid color mode: " << color_mode << std::endl;
			return false;
		}
	}

	// If trace is enabled, enable logging of certain things
	if (cmd_args.getFlag("trace")) {
		dstream << _("Enabling trace level debug output") << std::endl;
		g_logger.setTraceEnabled(true);
		dout_con_ptr = &verbosestream; // This is somewhat old
		socket_enable_debug_output = true; // Sockets doesn't use log.h
	}

	// In certain cases, output info level on stderr
	if (cmd_args.getFlag("info") || cmd_args.getFlag("verbose") ||
			cmd_args.getFlag("trace") || cmd_args.getFlag("speedtests"))
		g_logger.addOutput(&stderr_output, LL_INFO);

	// In certain cases, output verbose level on stderr
	if (cmd_args.getFlag("verbose") || cmd_args.getFlag("trace"))
		g_logger.addOutput(&stderr_output, LL_VERBOSE);

	return true;
}

static bool create_userdata_path()
{
	bool success;

#ifdef __ANDROID__
	if (!fs::PathExists(porting::path_user)) {
		success = fs::CreateDir(porting::path_user);
	} else {
		success = true;
	}
#else
	// Create user data directory
	success = fs::CreateDir(porting::path_user);
#endif

	return success;
}

static bool init_common(const Settings &cmd_args, int argc, char *argv[])
{
	startup_message();
	set_default_settings();

	sockets_init();

	// Initialize g_settings
	Settings::createLayer(SL_GLOBAL);

	// Set cleanup callback(s) to run at process exit
	atexit(uninit_common);

	if (!read_config_file(cmd_args))
		return false;

	init_log_streams(cmd_args);

	// Initialize random seed
	srand(time(0));
	mysrand(time(0));

	// Initialize HTTP fetcher
	httpfetch_init(g_settings->getS32("curl_parallel_limit"));

	init_gettext(porting::path_locale.c_str(),
		g_settings->get("language"), argc, argv);

	return true;
}

static void uninit_common()
{
	httpfetch_cleanup();

	sockets_cleanup();

	// It'd actually be okay to leak these but we want to please valgrind...
	for (int i = 0; i < (int)SL_TOTAL_COUNT; i++)
		delete Settings::getLayer((SettingsLayer)i);
}

static void startup_message()
{
	infostream << PROJECT_NAME << " " << _("with")
	           << " SER_FMT_VER_HIGHEST_READ="
               << (int)SER_FMT_VER_HIGHEST_READ << ", "
               << g_build_info << std::endl;
}

static bool read_config_file(const Settings &cmd_args)
{
	// Path of configuration file in use
	sanity_check(g_settings_path == "");	// Sanity check

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

		for (const std::string &filename : filenames) {
			bool r = g_settings->readConfigFile(filename.c_str());
			if (r) {
				g_settings_path = filename;
				break;
			}
		}

		// If no path found, use the first one (menu creates the file)
		if (g_settings_path.empty())
			g_settings_path = filenames[0];
	}

	return true;
}

static void init_log_streams(const Settings &cmd_args)
{
	std::string log_filename = porting::path_user + DIR_DELIM + DEBUGFILE;

	if (cmd_args.exists("logfile"))
		log_filename = cmd_args.get("logfile");

	g_logger.removeOutput(&file_log_output);
	std::string conf_loglev = g_settings->get("debug_log_level");

	// Old integer format
	if (std::isdigit(conf_loglev[0])) {
		warningstream << "Deprecated use of debug_log_level with an "
			"integer value; please update your configuration." << std::endl;
		static const char *lev_name[] =
			{"", "error", "action", "info", "verbose"};
		int lev_i = atoi(conf_loglev.c_str());
		if (lev_i < 0 || lev_i >= (int)ARRLEN(lev_name)) {
			warningstream << "Supplied invalid debug_log_level!"
				"  Assuming action level." << std::endl;
			lev_i = 2;
		}
		conf_loglev = lev_name[lev_i];
	}

	if (log_filename.empty() || conf_loglev.empty())  // No logging
		return;

	LogLevel log_level = Logger::stringToLevel(conf_loglev);
	if (log_level == LL_MAX) {
		warningstream << "Supplied unrecognized debug_log_level; "
			"using maximum." << std::endl;
	}

	file_log_output.setFile(log_filename,
		g_settings->getU64("debug_log_size_max") * 1000000);
	g_logger.addOutputMaxLevel(&file_log_output, log_level);
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
	if (cmd_args.exists("port")) {
		game_params->socket_port = cmd_args.getU16("port");
	} else {
		if (game_params->is_dedicated_server)
			game_params->socket_port = g_settings->getU16("port");
		else
			game_params->socket_port = g_settings->getU16("remote_port");
	}

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
	std::string commanded_world;

	// World name
	std::string commanded_worldname;
	if (cmd_args.exists("worldname"))
		commanded_worldname = cmd_args.get("worldname");

	// If a world name was specified, convert it to a path
	if (!commanded_worldname.empty()) {
		// Get information about available worlds
		std::vector<WorldSpec> worldspecs = getAvailableWorlds();
		bool found = false;
		for (const WorldSpec &worldspec : worldspecs) {
			std::string name = worldspec.name;
			if (name == commanded_worldname) {
				dstream << _("Using world specified by --worldname on the "
					"command line") << std::endl;
				commanded_world = worldspec.path;
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
		return !commanded_world.empty();
	}

	if (cmd_args.exists("world"))
		commanded_world = cmd_args.get("world");
	else if (cmd_args.exists("map-dir"))
		commanded_world = cmd_args.get("map-dir");
	else if (cmd_args.exists("nonopt0")) // First nameless argument
		commanded_world = cmd_args.get("nonopt0");

	game_params->world_path = get_clean_world_path(commanded_world);
	return !commanded_world.empty();
}

static bool get_world_from_config(GameParams *game_params, const Settings &cmd_args)
{
	// World directory
	std::string commanded_world;

	if (g_settings->exists("map-dir"))
		commanded_world = g_settings->get("map-dir");

	game_params->world_path = get_clean_world_path(commanded_world);

	return !commanded_world.empty();
}

static bool auto_select_world(GameParams *game_params)
{
	// No world was specified; try to select it automatically
	// Get information about available worlds

	std::vector<WorldSpec> worldspecs = getAvailableWorlds();
	std::string world_path;

	// If there is only a single world, use it
	if (worldspecs.size() == 1) {
		world_path = worldspecs[0].path;
		dstream <<_("Automatically selecting world at") << " ["
		        << world_path << "]" << std::endl;
	// If there are multiple worlds, list them
	} else if (worldspecs.size() > 1 && game_params->is_dedicated_server) {
		std::cerr << _("Multiple worlds are available.") << std::endl;
		std::cerr << _("Please select one using --worldname <name>"
				" or --world <path>") << std::endl;
		print_worldspecs(worldspecs, std::cerr);
		return false;
	// If there are no worlds, automatically create a new one
	} else {
		// This is the ultimate default world path
		world_path = porting::path_user + DIR_DELIM + "worlds" +
				DIR_DELIM + "world";
		infostream << "Using default world at ["
		           << world_path << "]" << std::endl;
	}

	assert(world_path != "");	// Post-condition
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

	assert(game_params->world_path != "");	// Pre-condition

	// If world doesn't exist
	if (!game_params->world_path.empty()
		&& !getWorldExists(game_params->world_path)) {
		// Try to take gamespec from command line
		if (game_params->game_spec.isValid()) {
			gamespec = game_params->game_spec;
			infostream << "Using commanded gameid [" << gamespec.id << "]" << std::endl;
		} else { // Otherwise we will be using "minetest"
			gamespec = findSubgame(g_settings->get("default_game"));
			infostream << "Using default gameid [" << gamespec.id << "]" << std::endl;
			if (!gamespec.isValid()) {
				errorstream << "Game specified in default_game ["
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
				warningstream << "Using commanded gameid ["
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
		errorstream << "Game [" << gamespec.id << "] could not be found."
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

	// Database migration/compression
	if (cmd_args.exists("migrate"))
		return migrate_map_database(game_params, cmd_args);

	if (cmd_args.exists("migrate-players"))
		return ServerEnvironment::migratePlayersDatabase(game_params, cmd_args);

	if (cmd_args.exists("migrate-auth"))
		return ServerEnvironment::migrateAuthDatabase(game_params, cmd_args);

	if (cmd_args.exists("migrate-mod-storage"))
		return Server::migrateModStorageDatabase(game_params, cmd_args);

	if (cmd_args.getFlag("recompress"))
		return recompress_map_database(game_params, cmd_args, bind_addr);

	if (cmd_args.exists("terminal")) {
#if USE_CURSES
		bool name_ok = true;
		std::string admin_nick = g_settings->get("name");

		name_ok = name_ok && !admin_nick.empty();
		name_ok = name_ok && string_allowed(admin_nick, PLAYERNAME_ALLOWED_CHARS);

		if (!name_ok) {
			if (admin_nick.empty()) {
				errorstream << "No name given for admin. "
					<< "Please check your minetest.conf that it "
					<< "contains a 'name = ' to your main admin account."
					<< std::endl;
			} else {
				errorstream << "Name for admin '"
					<< admin_nick << "' is not valid. "
					<< "Please check that it only contains allowed characters. "
					<< "Valid characters are: " << PLAYERNAME_ALLOWED_CHARS_USER_EXPL
					<< std::endl;
			}
			return false;
		}
		ChatInterface iface;
		bool &kill = *porting::signal_handler_killstatus();

		try {
			// Create server
			Server server(game_params.world_path, game_params.game_spec,
					false, bind_addr, true, &iface);

			g_term_console.setup(&iface, &kill, admin_nick);

			g_term_console.start();

			server.start();
			// Run server
			dedicated_server_loop(server, kill);
		} catch (const ModError &e) {
			g_term_console.stopAndWaitforThread();
			errorstream << "ModError: " << e.what() << std::endl;
			return false;
		} catch (const ServerError &e) {
			g_term_console.stopAndWaitforThread();
			errorstream << "ServerError: " << e.what() << std::endl;
			return false;
		}

		// Tell the console to stop, and wait for it to finish,
		// only then leave context and free iface
		g_term_console.stop();
		g_term_console.wait();

		g_term_console.clearKillStatus();
	} else {
#else
		errorstream << "Cmd arg --terminal passed, but "
			<< "compiled without ncurses. Ignoring." << std::endl;
	} {
#endif
		try {
			// Create server
			Server server(game_params.world_path, game_params.game_spec, false,
				bind_addr, true);
			server.start();

			// Run server
			bool &kill = *porting::signal_handler_killstatus();
			dedicated_server_loop(server, kill);

		} catch (const ModError &e) {
			errorstream << "ModError: " << e.what() << std::endl;
			return false;
		} catch (const ServerError &e) {
			errorstream << "ServerError: " << e.what() << std::endl;
			return false;
		}
	}

	return true;
}

static bool migrate_map_database(const GameParams &game_params, const Settings &cmd_args)
{
	std::string migrate_to = cmd_args.get("migrate");
	Settings world_mt;
	std::string world_mt_path = game_params.world_path + DIR_DELIM + "world.mt";
	if (!world_mt.readConfigFile(world_mt_path.c_str())) {
		errorstream << "Cannot read world.mt!" << std::endl;
		return false;
	}

	if (!world_mt.exists("backend")) {
		errorstream << "Please specify your current backend in world.mt:"
			<< std::endl
			<< "	backend = {sqlite3|leveldb|redis|dummy|postgresql}"
			<< std::endl;
		return false;
	}

	std::string backend = world_mt.get("backend");
	if (backend == migrate_to) {
		errorstream << "Cannot migrate: new backend is same"
			<< " as the old one" << std::endl;
		return false;
	}

	MapDatabase *old_db = ServerMap::createDatabase(backend, game_params.world_path, world_mt),
		*new_db = ServerMap::createDatabase(migrate_to, game_params.world_path, world_mt);

	u32 count = 0;
	time_t last_update_time = 0;
	bool &kill = *porting::signal_handler_killstatus();

	std::vector<v3s16> blocks;
	old_db->listAllLoadableBlocks(blocks);
	new_db->beginSave();
	for (std::vector<v3s16>::const_iterator it = blocks.begin(); it != blocks.end(); ++it) {
		if (kill) return false;

		std::string data;
		old_db->loadBlock(*it, &data);
		if (!data.empty()) {
			new_db->saveBlock(*it, data);
		} else {
			errorstream << "Failed to load block " << PP(*it) << ", skipping it." << std::endl;
		}
		if (++count % 0xFF == 0 && time(NULL) - last_update_time >= 1) {
			std::cerr << " Migrated " << count << " blocks, "
				<< (100.0 * count / blocks.size()) << "% completed.\r";
			new_db->endSave();
			new_db->beginSave();
			last_update_time = time(NULL);
		}
	}
	std::cerr << std::endl;
	new_db->endSave();
	delete old_db;
	delete new_db;

	actionstream << "Successfully migrated " << count << " blocks" << std::endl;
	world_mt.set("backend", migrate_to);
	if (!world_mt.updateConfigFile(world_mt_path.c_str()))
		errorstream << "Failed to update world.mt!" << std::endl;
	else
		actionstream << "world.mt updated" << std::endl;

	return true;
}

static bool recompress_map_database(const GameParams &game_params, const Settings &cmd_args, const Address &addr)
{
	Settings world_mt;
	const std::string world_mt_path = game_params.world_path + DIR_DELIM + "world.mt";

	if (!world_mt.readConfigFile(world_mt_path.c_str())) {
		errorstream << "Cannot read world.mt at " << world_mt_path << std::endl;
		return false;
	}
	const std::string &backend = world_mt.get("backend");
	Server server(game_params.world_path, game_params.game_spec, false, addr, false);
	MapDatabase *db = ServerMap::createDatabase(backend, game_params.world_path, world_mt);

	u32 count = 0;
	u64 last_update_time = 0;
	bool &kill = *porting::signal_handler_killstatus();
	const u8 serialize_as_ver = SER_FMT_VER_HIGHEST_WRITE;

	// This is ok because the server doesn't actually run
	std::vector<v3s16> blocks;
	db->listAllLoadableBlocks(blocks);
	db->beginSave();
	std::istringstream iss(std::ios_base::binary);
	std::ostringstream oss(std::ios_base::binary);
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		if (kill) return false;

		std::string data;
		db->loadBlock(*it, &data);
		if (data.empty()) {
			errorstream << "Failed to load block " << PP(*it) << std::endl;
			return false;
		}

		iss.str(data);
		iss.clear();

		MapBlock mb(nullptr, v3s16(0,0,0), &server);
		u8 ver = readU8(iss);
		mb.deSerialize(iss, ver, true);

		oss.str("");
		oss.clear();
		writeU8(oss, serialize_as_ver);
		mb.serialize(oss, serialize_as_ver, true, -1);

		db->saveBlock(*it, oss.str());

		count++;
		if (count % 0xFF == 0 && porting::getTimeS() - last_update_time >= 1) {
			std::cerr << " Recompressed " << count << " blocks, "
				<< (100.0f * count / blocks.size()) << "% completed.\r";
			db->endSave();
			db->beginSave();
			last_update_time = porting::getTimeS();
		}
	}
	std::cerr << std::endl;
	db->endSave();

	actionstream << "Done, " << count << " blocks were recompressed." << std::endl;
	return true;
}
