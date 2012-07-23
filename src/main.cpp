/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "server.h"
#include "constants.h"
#include "porting.h"
#include "gettime.h"
#include "guiMessageMenu.h"
#include "filesys.h"
#include "config.h"
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

/*
	Settings.
	These are loaded from the config file.
*/
Settings main_settings;
Settings *g_settings = &main_settings;

// Global profiler
Profiler main_profiler;
Profiler *g_profiler = &main_profiler;

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
	return porting::getTimeMs();
}

#else

// A small helper class
class TimeGetter
{
public:
	virtual u32 getTime() = 0;
};

// A precise irrlicht one
class IrrlichtTimeGetter: public TimeGetter
{
public:
	IrrlichtTimeGetter(IrrlichtDevice *device):
		m_device(device)
	{}
	u32 getTime()
	{
		if(m_device == NULL)
			return 0;
		return m_device->getTimer()->getRealTime();
	}
private:
	IrrlichtDevice *m_device;
};
// Not so precise one which works without irrlicht
class SimpleTimeGetter: public TimeGetter
{
public:
	u32 getTime()
	{
		return porting::getTimeMs();
	}
};

// A pointer to a global instance of the time getter
// TODO: why?
TimeGetter *g_timegetter = NULL;

u32 getTimeMs()
{
	if(g_timegetter == NULL)
		return 0;
	return g_timegetter->getTime();
}

#endif

class StderrLogOutput: public ILogOutput
{
public:
	/* line: Full line with timestamp, level and thread */
	void printLog(const std::string &line)
	{
		std::cerr<<line<<std::endl;
	}
} main_stderr_log_out;

class DstreamNoStderrLogOutput: public ILogOutput
{
public:
	/* line: Full line with timestamp, level and thread */
	void printLog(const std::string &line)
	{
		dstream_no_stderr<<line<<std::endl;
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
		if(noMenuActive() == false)
		{
			return false;
		}

		// Remember whether each key is down or up
		if(event.EventType == irr::EET_KEY_INPUT_EVENT)
		{
			if(event.KeyInput.PressedDown) {
				keyIsDown.set(event.KeyInput);
				keyWasDown.set(event.KeyInput);
			} else {
				keyIsDown.unset(event.KeyInput);
			}
		}

		if(event.EventType == irr::EET_MOUSE_INPUT_EVENT)
		{
			if(noMenuActive() == false)
			{
				left_active = false;
				middle_active = false;
				right_active = false;
			}
			else
			{
				left_active = event.MouseInput.isLeftPressed();
				middle_active = event.MouseInput.isMiddlePressed();
				right_active = event.MouseInput.isRightPressed();

				if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
				{
					leftclicked = true;
				}
				if(event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN)
				{
					rightclicked = true;
				}
				if(event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
				{
					leftreleased = true;
				}
				if(event.MouseInput.Event == EMIE_RMOUSE_LEFT_UP)
				{
					rightreleased = true;
				}
				if(event.MouseInput.Event == EMIE_MOUSE_WHEEL)
				{
					mouse_wheel += event.MouseInput.Wheel;
				}
			}
		}

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
	}

	bool leftclicked;
	bool rightclicked;
	bool leftreleased;
	bool rightreleased;

	bool left_active;
	bool middle_active;
	bool right_active;

	s32 mouse_wheel;

private:
	IrrlichtDevice *m_device;
	
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
		m_receiver(receiver)
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
		return m_device->getCursorControl()->getPosition();
	}
	virtual void setMousePos(s32 x, s32 y)
	{
		m_device->getCursorControl()->setPosition(x, y);
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
	IrrlichtDevice *m_device;
	MyEventReceiver *m_receiver;
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
		mousepos = v2s32(x,y);
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
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_jump"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_special1"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_forward"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_left"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 20);
				mousespeed = v2s32(Rand(-20,20), Rand(-15,20));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 30);
				leftdown = !leftdown;
				if(leftdown)
					leftclicked = true;
				if(!leftdown)
					leftreleased = true;
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 15);
				rightdown = !rightdown;
				if(rightdown)
					rightclicked = true;
				if(!rightdown)
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

void drawMenuBackground(video::IVideoDriver* driver)
{
	core::dimension2d<u32> screensize = driver->getScreenSize();
		
	video::ITexture *bgtexture =
			driver->getTexture(getTexturePath("menubg.png").c_str());
	if(bgtexture)
	{
		s32 scaledsize = 128;
		
		// The important difference between destsize and screensize is
		// that destsize is rounded to whole scaled pixels.
		// These formulas use component-wise multiplication and division of v2u32.
		v2u32 texturesize = bgtexture->getSize();
		v2u32 sourcesize = texturesize * screensize / scaledsize + v2u32(1,1);
		v2u32 destsize = scaledsize * sourcesize / texturesize;
		
		// Default texture wrapping mode in Irrlicht is ETC_REPEAT.
		driver->draw2DImage(bgtexture,
			core::rect<s32>(0, 0, destsize.X, destsize.Y),
			core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
			NULL, NULL, true);
	}
	
	video::ITexture *logotexture =
			driver->getTexture(getTexturePath("menulogo.png").c_str());
	if(logotexture)
	{
		v2s32 logosize(logotexture->getOriginalSize().Width,
				logotexture->getOriginalSize().Height);
		logosize *= 4;

		video::SColor bgcolor(255,50,50,50);
		core::rect<s32> bgrect(0, screensize.Height-logosize.Y-20,
				screensize.Width, screensize.Height);
		driver->draw2DRectangle(bgcolor, bgrect, NULL);

		core::rect<s32> rect(0,0,logosize.X,logosize.Y);
		rect += v2s32(screensize.Width/2,screensize.Height-10-logosize.Y);
		rect -= v2s32(logosize.X/2, 0);
		driver->draw2DImage(logotexture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
			core::dimension2di(logotexture->getSize())),
			NULL, NULL, true);
	}
}

#endif

// These are defined global so that they're not optimized too much.
// Can't change them to volatile.
s16 temp16;
f32 tempf;
v3f tempv3f1;
v3f tempv3f2;
std::string tempstring;
std::string tempstring2;

void SpeedTests()
{
	{
		infostream<<"The following test should take around 20ms."<<std::endl;
		TimeTaker timer("Testing std::string speed");
		const u32 jj = 10000;
		for(u32 j=0; j<jj; j++)
		{
			tempstring = "";
			tempstring2 = "";
			const u32 ii = 10;
			for(u32 i=0; i<ii; i++){
				tempstring2 += "asd";
			}
			for(u32 i=0; i<ii+1; i++){
				tempstring += "asd";
				if(tempstring == tempstring2)
					break;
			}
		}
	}
	
	infostream<<"All of the following tests should take around 100ms each."
			<<std::endl;

	{
		TimeTaker timer("Testing floating-point conversion speed");
		tempf = 0.001;
		for(u32 i=0; i<4000000; i++){
			temp16 += tempf;
			tempf += 0.001;
		}
	}
	
	{
		TimeTaker timer("Testing floating-point vector speed");

		tempv3f1 = v3f(1,2,3);
		tempv3f2 = v3f(4,5,6);
		for(u32 i=0; i<10000000; i++){
			tempf += tempv3f1.dotProduct(tempv3f2);
			tempv3f2 += v3f(7,8,9);
		}
	}

	{
		TimeTaker timer("Testing core::map speed");
		
		core::map<v2s16, f32> map1;
		tempf = -324;
		const s16 ii=300;
		for(s16 y=0; y<ii; y++){
			for(s16 x=0; x<ii; x++){
				map1.insert(v2s16(x,y), tempf);
				tempf += 1;
			}
		}
		for(s16 y=ii-1; y>=0; y--){
			for(s16 x=0; x<ii; x++){
				tempf = map1[v2s16(x,y)];
			}
		}
	}

	{
		infostream<<"Around 5000/ms should do well here."<<std::endl;
		TimeTaker timer("Testing mutex speed");
		
		JMutex m;
		m.Init();
		u32 n = 0;
		u32 i = 0;
		do{
			n += 10000;
			for(; i<n; i++){
				m.Lock();
				m.Unlock();
			}
		}
		// Do at least 10ms
		while(timer.getTime() < 10);

		u32 dtime = timer.stop();
		u32 per_ms = n / dtime;
		infostream<<"Done. "<<dtime<<"ms, "
				<<per_ms<<"/ms"<<std::endl;
	}
}

static void print_worldspecs(const std::vector<WorldSpec> &worldspecs,
		std::ostream &os)
{
	for(u32 i=0; i<worldspecs.size(); i++){
		std::string name = worldspecs[i].name;
		std::string path = worldspecs[i].path;
		if(name.find(" ") != std::string::npos)
			name = std::string("'") + name + "'";
		path = std::string("'") + path + "'";
		name = padStringRight(name, 14);
		os<<"  "<<name<<" "<<path<<std::endl;
	}
}

int main(int argc, char *argv[])
{
	int retval = 0;

	/*
		Initialization
	*/

	log_add_output_maxlev(&main_stderr_log_out, LMT_ACTION);
	log_add_output_all_levs(&main_dstream_no_stderr_log_out);

	log_register_thread("main");

	// Set locale. This is for forcing '.' as the decimal point.
	std::locale::global(std::locale("C"));
	// This enables printing all characters in bitmap font
	setlocale(LC_CTYPE, "en_US");

	/*
		Parse command line
	*/
	
	// List all allowed options
	core::map<std::string, ValueSpec> allowed_options;
	allowed_options.insert("help", ValueSpec(VALUETYPE_FLAG,
			_("Show allowed options")));
	allowed_options.insert("config", ValueSpec(VALUETYPE_STRING,
			_("Load configuration from specified file")));
	allowed_options.insert("port", ValueSpec(VALUETYPE_STRING,
			_("Set network port (UDP)")));
	allowed_options.insert("disable-unittests", ValueSpec(VALUETYPE_FLAG,
			_("Disable unit tests")));
	allowed_options.insert("enable-unittests", ValueSpec(VALUETYPE_FLAG,
			_("Enable unit tests")));
	allowed_options.insert("map-dir", ValueSpec(VALUETYPE_STRING,
			_("Same as --world (deprecated)")));
	allowed_options.insert("world", ValueSpec(VALUETYPE_STRING,
			_("Set world path (implies local game) ('list' lists all)")));
	allowed_options.insert("worldname", ValueSpec(VALUETYPE_STRING,
			_("Set world by name (implies local game)")));
	allowed_options.insert("info", ValueSpec(VALUETYPE_FLAG,
			_("Print more information to console")));
	allowed_options.insert("verbose", ValueSpec(VALUETYPE_FLAG,
			_("Print even more information to console")));
	allowed_options.insert("trace", ValueSpec(VALUETYPE_FLAG,
			_("Print enormous amounts of information to log and console")));
	allowed_options.insert("logfile", ValueSpec(VALUETYPE_STRING,
			_("Set logfile path ('' = no logging)")));
	allowed_options.insert("gameid", ValueSpec(VALUETYPE_STRING,
			_("Set gameid (\"--gameid list\" prints available ones)")));
#ifndef SERVER
	allowed_options.insert("speedtests", ValueSpec(VALUETYPE_FLAG,
			_("Run speed tests")));
	allowed_options.insert("address", ValueSpec(VALUETYPE_STRING,
			_("Address to connect to. ('' = local game)")));
	allowed_options.insert("random-input", ValueSpec(VALUETYPE_FLAG,
			_("Enable random user input, for testing")));
	allowed_options.insert("server", ValueSpec(VALUETYPE_FLAG,
			_("Run dedicated server")));
	allowed_options.insert("name", ValueSpec(VALUETYPE_STRING,
			_("Set player name")));
	allowed_options.insert("password", ValueSpec(VALUETYPE_STRING,
			_("Set password")));
	allowed_options.insert("go", ValueSpec(VALUETYPE_FLAG,
			_("Disable main menu")));
#endif

	Settings cmd_args;
	
	bool ret = cmd_args.parseCommandLine(argc, argv, allowed_options);

	if(ret == false || cmd_args.getFlag("help") || cmd_args.exists("nonopt1"))
	{
		dstream<<_("Allowed options:")<<std::endl;
		for(core::map<std::string, ValueSpec>::Iterator
				i = allowed_options.getIterator();
				i.atEnd() == false; i++)
		{
			std::ostringstream os1(std::ios::binary);
			os1<<"  --"<<i.getNode()->getKey();
			if(i.getNode()->getValue().type == VALUETYPE_FLAG)
				{}
			else
				os1<<_(" <value>");
			dstream<<padStringRight(os1.str(), 24);

			if(i.getNode()->getValue().help != NULL)
				dstream<<i.getNode()->getValue().help;
			dstream<<std::endl;
		}

		return cmd_args.getFlag("help") ? 0 : 1;
	}
	
	/*
		Low-level initialization
	*/
	
	// If trace is enabled, enable logging of certain things
	if(cmd_args.getFlag("trace")){
		dstream<<_("Enabling trace level debug output")<<std::endl;
		log_trace_level_enabled = true;
		dout_con_ptr = &verbosestream; // this is somewhat old crap
		socket_enable_debug_output = true; // socket doesn't use log.h
	}
	// In certain cases, output info level on stderr
	if(cmd_args.getFlag("info") || cmd_args.getFlag("verbose") ||
			cmd_args.getFlag("trace") || cmd_args.getFlag("speedtests"))
		log_add_output(&main_stderr_log_out, LMT_INFO);
	// In certain cases, output verbose level on stderr
	if(cmd_args.getFlag("verbose") || cmd_args.getFlag("trace"))
		log_add_output(&main_stderr_log_out, LMT_VERBOSE);

	porting::signal_handler_init();
	bool &kill = *porting::signal_handler_killstatus();
	
	porting::initializePaths();

	// Create user data directory
	fs::CreateDir(porting::path_user);

	init_gettext((porting::path_share+DIR_DELIM+".."+DIR_DELIM+"locale").c_str());
	
	// Initialize debug streams
#define DEBUGFILE "debug.txt"
#if RUN_IN_PLACE
	std::string logfile = DEBUGFILE;
#else
	std::string logfile = porting::path_user+DIR_DELIM+DEBUGFILE;
#endif
	if(cmd_args.exists("logfile"))
		logfile = cmd_args.get("logfile");
	if(logfile != "")
		debugstreams_init(false, logfile.c_str());
	else
		debugstreams_init(false, NULL);

	infostream<<"logfile    = "<<logfile<<std::endl;
	infostream<<"path_share = "<<porting::path_share<<std::endl;
	infostream<<"path_user  = "<<porting::path_user<<std::endl;

	// Initialize debug stacks
	debug_stacks_init();
	DSTACK(__FUNCTION_NAME);

	// Debug handler
	BEGIN_DEBUG_EXCEPTION_HANDLER
	
	// List gameids if requested
	if(cmd_args.exists("gameid") && cmd_args.get("gameid") == "list")
	{
		std::set<std::string> gameids = getAvailableGameIds();
		for(std::set<std::string>::const_iterator i = gameids.begin();
				i != gameids.end(); i++)
			dstream<<(*i)<<std::endl;
		return 0;
	}
	
	// List worlds if requested
	if(cmd_args.exists("world") && cmd_args.get("world") == "list"){
		dstream<<_("Available worlds:")<<std::endl;
		std::vector<WorldSpec> worldspecs = getAvailableWorlds();
		print_worldspecs(worldspecs, dstream);
		return 0;
	}
	
	// Print startup message
	infostream<<PROJECT_NAME<<
			" "<<_("with")<<" SER_FMT_VER_HIGHEST="<<(int)SER_FMT_VER_HIGHEST
			<<", "<<BUILD_INFO
			<<std::endl;
	
	/*
		Basic initialization
	*/

	// Initialize default settings
	set_default_settings(g_settings);
	
	// Initialize sockets
	sockets_init();
	atexit(sockets_cleanup);
	
	/*
		Read config file
	*/
	
	// Path of configuration file in use
	std::string configpath = "";
	
	if(cmd_args.exists("config"))
	{
		bool r = g_settings->readConfigFile(cmd_args.get("config").c_str());
		if(r == false)
		{
			errorstream<<"Could not read configuration from \""
					<<cmd_args.get("config")<<"\""<<std::endl;
			return 1;
		}
		configpath = cmd_args.get("config");
	}
	else
	{
		core::array<std::string> filenames;
		filenames.push_back(porting::path_user +
				DIR_DELIM + "minetest.conf");
		// Legacy configuration file location
		filenames.push_back(porting::path_user +
				DIR_DELIM + ".." + DIR_DELIM + "minetest.conf");
#if RUN_IN_PLACE
		// Try also from a lower level (to aid having the same configuration
		// for many RUN_IN_PLACE installs)
		filenames.push_back(porting::path_user +
				DIR_DELIM + ".." + DIR_DELIM + ".." + DIR_DELIM + "minetest.conf");
#endif

		for(u32 i=0; i<filenames.size(); i++)
		{
			bool r = g_settings->readConfigFile(filenames[i].c_str());
			if(r)
			{
				configpath = filenames[i];
				break;
			}
		}
		
		// If no path found, use the first one (menu creates the file)
		if(configpath == "")
			configpath = filenames[0];
	}

	// Initialize random seed
	srand(time(0));
	mysrand(time(0));

	/*
		Run unit tests
	*/

	if((ENABLE_TESTS && cmd_args.getFlag("disable-unittests") == false)
			|| cmd_args.getFlag("enable-unittests") == true)
	{
		run_tests();
	}
	
	/*
		Game parameters
	*/

	// Port
	u16 port = 30000;
	if(cmd_args.exists("port"))
		port = cmd_args.getU16("port");
	else if(g_settings->exists("port"))
		port = g_settings->getU16("port");
	if(port == 0)
		port = 30000;
	
	// World directory
	std::string commanded_world = "";
	if(cmd_args.exists("world"))
		commanded_world = cmd_args.get("world");
	else if(cmd_args.exists("map-dir"))
		commanded_world = cmd_args.get("map-dir");
	else if(cmd_args.exists("nonopt0")) // First nameless argument
		commanded_world = cmd_args.get("nonopt0");
	else if(g_settings->exists("map-dir"))
		commanded_world = g_settings->get("map-dir");
	
	// World name
	std::string commanded_worldname = "";
	if(cmd_args.exists("worldname"))
		commanded_worldname = cmd_args.get("worldname");
	
	// Strip world.mt from commanded_world
	{
		std::string worldmt = "world.mt";
		if(commanded_world.size() > worldmt.size() &&
				commanded_world.substr(commanded_world.size()-worldmt.size())
				== worldmt){
			dstream<<_("Supplied world.mt file - stripping it off.")<<std::endl;
			commanded_world = commanded_world.substr(
					0, commanded_world.size()-worldmt.size());
		}
	}
	
	// If a world name was specified, convert it to a path
	if(commanded_worldname != ""){
		// Get information about available worlds
		std::vector<WorldSpec> worldspecs = getAvailableWorlds();
		bool found = false;
		for(u32 i=0; i<worldspecs.size(); i++){
			std::string name = worldspecs[i].name;
			if(name == commanded_worldname){
				if(commanded_world != ""){
					dstream<<_("--worldname takes precedence over previously "
							"selected world.")<<std::endl;
				}
				commanded_world = worldspecs[i].path;
				found = true;
				break;
			}
		}
		if(!found){
			dstream<<_("World")<<" '"<<commanded_worldname<<_("' not "
					"available. Available worlds:")<<std::endl;
			print_worldspecs(worldspecs, dstream);
			return 1;
		}
	}

	// Gamespec
	SubgameSpec commanded_gamespec;
	if(cmd_args.exists("gameid")){
		std::string gameid = cmd_args.get("gameid");
		commanded_gamespec = findSubgame(gameid);
		if(!commanded_gamespec.isValid()){
			errorstream<<"Game \""<<gameid<<"\" not found"<<std::endl;
			return 1;
		}
	}

	/*
		Run dedicated server if asked to or no other option
	*/
#ifdef SERVER
	bool run_dedicated_server = true;
#else
	bool run_dedicated_server = cmd_args.getFlag("server");
#endif
	if(run_dedicated_server)
	{
		DSTACK("Dedicated server branch");
		// Create time getter if built with Irrlicht
#ifndef SERVER
		g_timegetter = new SimpleTimeGetter();
#endif

		// World directory
		std::string world_path;
		verbosestream<<_("Determining world path")<<std::endl;
		bool is_legacy_world = false;
		// If a world was commanded, use it
		if(commanded_world != ""){
			world_path = commanded_world;
			infostream<<"Using commanded world path ["<<world_path<<"]"
					<<std::endl;
		}
		// No world was specified; try to select it automatically
		else
		{
			// Get information about available worlds
			std::vector<WorldSpec> worldspecs = getAvailableWorlds();
			// If a world name was specified, select it
			if(commanded_worldname != ""){
				world_path = "";
				for(u32 i=0; i<worldspecs.size(); i++){
					std::string name = worldspecs[i].name;
					if(name == commanded_worldname){
						world_path = worldspecs[i].path;
						break;
					}
				}
				if(world_path == ""){
					dstream<<_("World")<<" '"<<commanded_worldname<<"' "<<_("not "
							"available. Available worlds:")<<std::endl;
					print_worldspecs(worldspecs, dstream);
					return 1;
				}
			}
			// If there is only a single world, use it
			if(worldspecs.size() == 1){
				world_path = worldspecs[0].path;
				dstream<<_("Automatically selecting world at")<<" ["
						<<world_path<<"]"<<std::endl;
			// If there are multiple worlds, list them
			} else if(worldspecs.size() > 1){
				dstream<<_("Multiple worlds are available.")<<std::endl;
				dstream<<_("Please select one using --worldname <name>"
						" or --world <path>")<<std::endl;
				print_worldspecs(worldspecs, dstream);
				return 1;
			// If there are no worlds, automatically create a new one
			} else {
				// This is the ultimate default world path
				world_path = porting::path_user + DIR_DELIM + "worlds" +
						DIR_DELIM + "world";
				infostream<<"Creating default world at ["
						<<world_path<<"]"<<std::endl;
			}
		}

		if(world_path == ""){
			errorstream<<"No world path specified or found."<<std::endl;
			return 1;
		}
		verbosestream<<_("Using world path")<<" ["<<world_path<<"]"<<std::endl;

		// We need a gamespec.
		SubgameSpec gamespec;
		verbosestream<<_("Determining gameid/gamespec")<<std::endl;
		// If world doesn't exist
		if(!getWorldExists(world_path))
		{
			// Try to take gamespec from command line
			if(commanded_gamespec.isValid()){
				gamespec = commanded_gamespec;
				infostream<<"Using commanded gameid ["<<gamespec.id<<"]"<<std::endl;
			}
			// Otherwise we will be using "minetest"
			else{
				gamespec = findSubgame(g_settings->get("default_game"));
				infostream<<"Using default gameid ["<<gamespec.id<<"]"<<std::endl;
			}
		}
		// World exists
		else
		{
			std::string world_gameid = getWorldGameId(world_path, is_legacy_world);
			// If commanded to use a gameid, do so
			if(commanded_gamespec.isValid()){
				gamespec = commanded_gamespec;
				if(commanded_gamespec.id != world_gameid){
					errorstream<<"WARNING: Using commanded gameid ["
							<<gamespec.id<<"]"<<" instead of world gameid ["
							<<world_gameid<<"]"<<std::endl;
				}
			} else{
				// If world contains an embedded game, use it;
				// Otherwise find world from local system.
				gamespec = findWorldSubgame(world_path);
				infostream<<"Using world gameid ["<<gamespec.id<<"]"<<std::endl;
			}
		}
		if(!gamespec.isValid()){
			errorstream<<"Subgame ["<<gamespec.id<<"] could not be found."
					<<std::endl;
			return 1;
		}
		verbosestream<<_("Using gameid")<<" ["<<gamespec.id<<"]"<<std::endl;

		// Create server
		Server server(world_path, configpath, gamespec, false);
		server.start(port);
		
		// Run server
		dedicated_server_loop(server, kill);

		return 0;
	}

#ifndef SERVER // Exclude from dedicated server build

	/*
		More parameters
	*/
	
	std::string address = g_settings->get("address");
	if(commanded_world != "")
		address = "";
	else if(cmd_args.exists("address"))
		address = cmd_args.get("address");
	
	std::string playername = g_settings->get("name");
	if(cmd_args.exists("name"))
		playername = cmd_args.get("name");
	
	bool skip_main_menu = cmd_args.getFlag("go");

	/*
		Device initialization
	*/

	// Resolution selection
	
	bool fullscreen = g_settings->getBool("fullscreen");
	u16 screenW = g_settings->getU16("screenW");
	u16 screenH = g_settings->getU16("screenH");

	// bpp, fsaa, vsync

	bool vsync = g_settings->getBool("vsync");
	u16 bits = g_settings->getU16("fullscreen_bpp");
	u16 fsaa = g_settings->getU16("fsaa");

	// Determine driver

	video::E_DRIVER_TYPE driverType;
	
	std::string driverstring = g_settings->get("video_driver");

	if(driverstring == "null")
		driverType = video::EDT_NULL;
	else if(driverstring == "software")
		driverType = video::EDT_SOFTWARE;
	else if(driverstring == "burningsvideo")
		driverType = video::EDT_BURNINGSVIDEO;
	else if(driverstring == "direct3d8")
		driverType = video::EDT_DIRECT3D8;
	else if(driverstring == "direct3d9")
		driverType = video::EDT_DIRECT3D9;
	else if(driverstring == "opengl")
		driverType = video::EDT_OPENGL;
	else
	{
		errorstream<<"WARNING: Invalid video_driver specified; defaulting "
				"to opengl"<<std::endl;
		driverType = video::EDT_OPENGL;
	}

	/*
		Create device and exit if creation failed
	*/

	MyEventReceiver receiver;

	IrrlichtDevice *device;

	SIrrlichtCreationParameters params = SIrrlichtCreationParameters();
	params.DriverType    = driverType;
	params.WindowSize    = core::dimension2d<u32>(screenW, screenH);
	params.Bits          = bits;
	params.AntiAlias     = fsaa;
	params.Fullscreen    = fullscreen;
	params.Stencilbuffer = false;
	params.Vsync         = vsync;
	params.EventReceiver = &receiver;

	device = createDeviceEx(params);

	if (device == 0)
		return 1; // could not create selected driver.
	
	/*
		Continue initialization
	*/

	video::IVideoDriver* driver = device->getVideoDriver();

	// Disable mipmaps (because some of them look ugly)
	driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

	/*
		This changes the minimum allowed number of vertices in a VBO.
		Default is 500.
	*/
	//driver->setMinHardwareBufferVertexCount(50);

	// Create time getter
	g_timegetter = new IrrlichtTimeGetter(device);
	
	// Create game callback for menus
	g_gamecallback = new MainGameCallback(device);
	
	/*
		Speed tests (done after irrlicht is loaded to get timer)
	*/
	if(cmd_args.getFlag("speedtests"))
	{
		dstream<<"Running speed tests"<<std::endl;
		SpeedTests();
		return 0;
	}
	
	device->setResizable(true);

	bool random_input = g_settings->getBool("random_input")
			|| cmd_args.getFlag("random-input");
	InputHandler *input = NULL;
	if(random_input)
		input = new RandomInputHandler();
	else
		input = new RealInputHandler(device, &receiver);
	
	scene::ISceneManager* smgr = device->getSceneManager();

	guienv = device->getGUIEnvironment();
	gui::IGUISkin* skin = guienv->getSkin();
	gui::IGUIFont* font = guienv->getFont(getTexturePath("fontlucida.png").c_str());
	if(font)
		skin->setFont(font);
	else
		errorstream<<"WARNING: Font file was not found."
				" Using default font."<<std::endl;
	// If font was not found, this will get us one
	font = skin->getFont();
	assert(font);
	
	u32 text_height = font->getDimension(L"Hello, world!").Height;
	infostream<<"text_height="<<text_height<<std::endl;

	//skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255,0,0,0));
	skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255,255,255,255));
	//skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(0,0,0,0));
	//skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(0,0,0,0));
	skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(255,0,0,0));
	skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(255,0,0,0));
	skin->setColor(gui::EGDC_HIGH_LIGHT, video::SColor(255,70,100,50));
	skin->setColor(gui::EGDC_HIGH_LIGHT_TEXT, video::SColor(255,255,255,255));
	
	/*
		GUI stuff
	*/

	ChatBackend chat_backend;

	/*
		If an error occurs, this is set to something and the
		menu-game loop is restarted. It is then displayed before
		the menu.
	*/
	std::wstring error_message = L"";

	// The password entered during the menu screen,
	std::string password;

	bool first_loop = true;

	/*
		Menu-game loop
	*/
	while(device->run() && kill == false)
	{
		// Set the window caption
		device->setWindowCaption((std::wstring(L"Minetest [")+wgettext("Main Menu")+L"]").c_str());

		// This is used for catching disconnects
		try
		{

			/*
				Clear everything from the GUIEnvironment
			*/
			guienv->clear();
			
			/*
				We need some kind of a root node to be able to add
				custom gui elements directly on the screen.
				Otherwise they won't be automatically drawn.
			*/
			guiroot = guienv->addStaticText(L"",
					core::rect<s32>(0, 0, 10000, 10000));
			
			SubgameSpec gamespec;
			WorldSpec worldspec;
			bool simple_singleplayer_mode = false;

			// These are set up based on the menu and other things
			std::string current_playername = "inv£lid";
			std::string current_password = "";
			std::string current_address = "does-not-exist";
			int current_port = 0;

			/*
				Out-of-game menu loop.

				Loop quits when menu returns proper parameters.
			*/
			while(kill == false)
			{
				// If skip_main_menu, only go through here once
				if(skip_main_menu && !first_loop){
					kill = true;
					break;
				}
				first_loop = false;
				
				// Cursor can be non-visible when coming from the game
				device->getCursorControl()->setVisible(true);
				// Some stuff are left to scene manager when coming from the game
				// (map at least?)
				smgr->clear();
				
				// Initialize menu data
				MainMenuData menudata;
				if(g_settings->exists("selected_mainmenu_tab"))
					menudata.selected_tab = g_settings->getS32("selected_mainmenu_tab");
				menudata.address = narrow_to_wide(address);
				menudata.name = narrow_to_wide(playername);
				menudata.port = narrow_to_wide(itos(port));
				if(cmd_args.exists("password"))
					menudata.password = narrow_to_wide(cmd_args.get("password"));
				menudata.fancy_trees = g_settings->getBool("new_style_leaves");
				menudata.smooth_lighting = g_settings->getBool("smooth_lighting");
				menudata.clouds_3d = g_settings->getBool("enable_3d_clouds");
				menudata.opaque_water = g_settings->getBool("opaque_water");
				menudata.creative_mode = g_settings->getBool("creative_mode");
				menudata.enable_damage = g_settings->getBool("enable_damage");
				// Default to selecting nothing
				menudata.selected_world = -1;
				// Get world listing for the menu
				std::vector<WorldSpec> worldspecs = getAvailableWorlds();
				// If there is only one world, select it
				if(worldspecs.size() == 1){
					menudata.selected_world = 0;
				}
				// Otherwise try to select according to selected_world_path
				else if(g_settings->exists("selected_world_path")){
					std::string trypath = g_settings->get("selected_world_path");
					for(u32 i=0; i<worldspecs.size(); i++){
						if(worldspecs[i].path == trypath){
							menudata.selected_world = i;
							break;
						}
					}
				}
				// If a world was commanded, append and select it
				if(commanded_world != ""){
					std::string gameid = getWorldGameId(commanded_world, true);
					std::string name = _("[--world parameter]");
					if(gameid == ""){
						gameid = g_settings->get("default_game");
						name += " [new]";
					}
					WorldSpec spec(commanded_world, name, gameid);
					worldspecs.push_back(spec);
					menudata.selected_world = worldspecs.size()-1;
				}
				// Copy worldspecs to menu
				menudata.worlds = worldspecs;

				if(skip_main_menu == false)
				{
					video::IVideoDriver* driver = device->getVideoDriver();
					
					infostream<<"Waiting for other menus"<<std::endl;
					while(device->run() && kill == false)
					{
						if(noMenuActive())
							break;
						driver->beginScene(true, true,
								video::SColor(255,128,128,128));
						drawMenuBackground(driver);
						guienv->drawAll();
						driver->endScene();
						// On some computers framerate doesn't seem to be
						// automatically limited
						sleep_ms(25);
					}
					infostream<<"Waited for other menus"<<std::endl;

					GUIMainMenu *menu =
							new GUIMainMenu(guienv, guiroot, -1, 
								&g_menumgr, &menudata, g_gamecallback);
					menu->allowFocusRemoval(true);

					if(error_message != L"")
					{
						verbosestream<<"error_message = "
								<<wide_to_narrow(error_message)<<std::endl;

						GUIMessageMenu *menu2 =
								new GUIMessageMenu(guienv, guiroot, -1, 
									&g_menumgr, error_message.c_str());
						menu2->drop();
						error_message = L"";
					}

					infostream<<"Created main menu"<<std::endl;

					while(device->run() && kill == false)
					{
						if(menu->getStatus() == true)
							break;

						//driver->beginScene(true, true, video::SColor(255,0,0,0));
						driver->beginScene(true, true, video::SColor(255,128,128,128));

						drawMenuBackground(driver);

						guienv->drawAll();
						
						driver->endScene();
						
						// On some computers framerate doesn't seem to be
						// automatically limited
						sleep_ms(25);
					}
					
					infostream<<"Dropping main menu"<<std::endl;

					menu->drop();
				}

				playername = wide_to_narrow(menudata.name);
				password = translatePassword(playername, menudata.password);
				//infostream<<"Main: password hash: '"<<password<<"'"<<std::endl;

				address = wide_to_narrow(menudata.address);
				int newport = stoi(wide_to_narrow(menudata.port));
				if(newport != 0)
					port = newport;
				simple_singleplayer_mode = menudata.simple_singleplayer_mode;
				// Save settings
				g_settings->setS32("selected_mainmenu_tab", menudata.selected_tab);
				g_settings->set("new_style_leaves", itos(menudata.fancy_trees));
				g_settings->set("smooth_lighting", itos(menudata.smooth_lighting));
				g_settings->set("enable_3d_clouds", itos(menudata.clouds_3d));
				g_settings->set("opaque_water", itos(menudata.opaque_water));
				g_settings->set("creative_mode", itos(menudata.creative_mode));
				g_settings->set("enable_damage", itos(menudata.enable_damage));
				g_settings->set("name", playername);
				g_settings->set("address", address);
				g_settings->set("port", itos(port));
				if(menudata.selected_world != -1)
					g_settings->set("selected_world_path",
							worldspecs[menudata.selected_world].path);
				
				// Break out of menu-game loop to shut down cleanly
				if(device->run() == false || kill == true)
					break;
				
				current_playername = playername;
				current_password = password;
				current_address = address;
				current_port = port;

				// If using simple singleplayer mode, override
				if(simple_singleplayer_mode){
					current_playername = "singleplayer";
					current_password = "";
					current_address = "";
					current_port = 30011;
				}
				
				// Set world path to selected one
				if(menudata.selected_world != -1){
					worldspec = worldspecs[menudata.selected_world];
					infostream<<"Selected world: "<<worldspec.name
							<<" ["<<worldspec.path<<"]"<<std::endl;
				}

				// Only refresh if so requested
				if(menudata.only_refresh){
					infostream<<"Refreshing menu"<<std::endl;
					continue;
				}
				
				// Create new world if requested
				if(menudata.create_world_name != L"")
				{
					std::string path = porting::path_user + DIR_DELIM
							"worlds" + DIR_DELIM
							+ wide_to_narrow(menudata.create_world_name);
					// Create world if it doesn't exist
					if(!initializeWorld(path, menudata.create_world_gameid)){
						error_message = wgettext("Failed to initialize world");
						errorstream<<wide_to_narrow(error_message)<<std::endl;
						continue;
					}
					g_settings->set("selected_world_path", path);
					continue;
				}

				// If local game
				if(current_address == "")
				{
					if(menudata.selected_world == -1){
						error_message = wgettext("No world selected and no address "
								"provided. Nothing to do.");
						errorstream<<wide_to_narrow(error_message)<<std::endl;
						continue;
					}
					// Load gamespec for required game
					gamespec = findWorldSubgame(worldspec.path);
					if(!gamespec.isValid() && !commanded_gamespec.isValid()){
						error_message = wgettext("Could not find or load game \"")
								+ narrow_to_wide(worldspec.gameid) + L"\"";
						errorstream<<wide_to_narrow(error_message)<<std::endl;
						continue;
					}
					if(commanded_gamespec.isValid() &&
							commanded_gamespec.id != worldspec.gameid){
						errorstream<<"WARNING: Overriding gamespec from \""
								<<worldspec.gameid<<"\" to \""
								<<commanded_gamespec.id<<"\""<<std::endl;
						gamespec = commanded_gamespec;
					}

					if(!gamespec.isValid()){
						error_message = wgettext("Invalid gamespec.");
						error_message += L" (world_gameid="
								+narrow_to_wide(worldspec.gameid)+L")";
						errorstream<<wide_to_narrow(error_message)<<std::endl;
						continue;
					}
				}

				// Continue to game
				break;
			}
			
			// Break out of menu-game loop to shut down cleanly
			if(device->run() == false || kill == true)
				break;

			/*
				Run game
			*/
			the_game(
				kill,
				random_input,
				input,
				device,
				font,
				worldspec.path,
				current_playername,
				current_password,
				current_address,
				current_port,
				error_message,
				configpath,
				chat_backend,
				gamespec,
				simple_singleplayer_mode
			);

		} //try
		catch(con::PeerNotFoundException &e)
		{
			error_message = wgettext("Connection error (timed out?)");
			errorstream<<wide_to_narrow(error_message)<<std::endl;
		}
		catch(ServerError &e)
		{
			error_message = narrow_to_wide(e.what());
			errorstream<<wide_to_narrow(error_message)<<std::endl;
		}
		catch(ModError &e)
		{
			errorstream<<e.what()<<std::endl;
			error_message = narrow_to_wide(e.what()) + wgettext("\nCheck debug.txt for details.");
		}
#ifdef NDEBUG
		catch(std::exception &e)
		{
			std::string narrow_message = "Some exception: \"";
			narrow_message += e.what();
			narrow_message += "\"";
			errorstream<<narrow_message<<std::endl;
			error_message = narrow_to_wide(narrow_message);
		}
#endif

		// If no main menu, show error and exit
		if(skip_main_menu)
		{
			if(error_message != L""){
				verbosestream<<"error_message = "
						<<wide_to_narrow(error_message)<<std::endl;
				retval = 1;
			}
			break;
		}
	} // Menu-game loop
	
	delete input;

	/*
		In the end, delete the Irrlicht device.
	*/
	device->drop();

#endif // !SERVER
	
	// Update configuration file
	if(configpath != "")
		g_settings->updateConfigFile(configpath.c_str());
	
	// Print modified quicktune values
	{
		bool header_printed = false;
		std::vector<std::string> names = getQuicktuneNames();
		for(u32 i=0; i<names.size(); i++){
			QuicktuneValue val = getQuicktuneValue(names[i]);
			if(!val.modified)
				continue;
			if(!header_printed){
				dstream<<"Modified quicktune values:"<<std::endl;
				header_printed = true;
			}
			dstream<<names[i]<<" = "<<val.getString()<<std::endl;
		}
	}

	END_DEBUG_EXCEPTION_HANDLER(errorstream)
	
	debugstreams_deinit();
	
	return retval;
}

//END

