/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
=============================== NOTES ==============================
NOTE: Things starting with TODO are sometimes only suggestions.

NOTE: VBO cannot be turned on for fast-changing stuff because there
      is an apparanet memory leak in irrlicht when using it (not sure)

NOTE: iostream.imbue(std::locale("C")) is very slow
NOTE: Global locale is now set at initialization

SUGG: Fix address to be ipv6 compatible

FIXME: When a new sector is generated, it may change the ground level
       of it's and it's neighbors border that two blocks that are
	   above and below each other and that are generated before and
	   after the sector heightmap generation (order doesn't matter),
	   can have a small gap between each other at the border.
SUGGESTION: Use same technique for sector heightmaps as what we're
            using for UnlimitedHeightmap? (getting all neighbors
			when generating)

SUGG: Transfer more blocks in a single packet
SUGG: A blockdata combiner class, to which blocks are added and at
      destruction it sends all the stuff in as few packets as possible.

SUGG: If player is on ground, mainly fetch ground-level blocks
SUGG: Fetch stuff mainly from the viewing direction

SUGG: Expose Connection's seqnums and ACKs to server and client.
      - This enables saving many packets and making a faster connection
	  - This also enables server to check if client has received the
	    most recent block sent, for example.
SUGG: Add a sane bandwidth throttling system to Connection

SUGG: More fine-grained control of client's dumping of blocks from
      memory
	  - ...What does this mean in the first place?

SUGG: A map editing mode (similar to dedicated server mode)

SUGG: Add a time value to the param of footstepped grass and check it
      against a global timer when a block is accessed, to make old
	  steps fade away.

SUGG: Make a copy of close-range environment on client for showing
      on screen, with minimal mutexes to slow down the main loop

SUGG: Make a PACKET_COMBINED which contains many subpackets. Utilize
      it by sending more stuff in a single packet.
	  - Add a packet queue to RemoteClient, from which packets will be
	    combined with object data packets
		- This is not exactly trivial: the object data packets are
		  sometimes very big by themselves

SUGG: Split MapBlockObject serialization to to-client and to-disk
      - This will allow saving ages of rats on disk but not sending
	    them to clients

SUGG: Implement lighting using VoxelManipulator
      - Would it be significantly faster?

FIXME: Rats somehow go underground sometimes (you can see it in water)
       - Does their position get saved to a border value or something?
	   - Does this happen anymore?

SUGG: MovingObject::move and Player::move are basically the same.
      combine them.

SUGG: Implement a "Fast check queue" (a queue with a map for checking
      if something is already in it)
      - Use it in active block queue in water flowing

SUGG: Signs could be done in the same way as torches. For this, blocks
      need an additional metadata field for the texts
	  - This is also needed for item container chests

SUGG: Precalculate lighting translation table at runtime (at startup)

SUGG: A version number to blocks, which increments when the block is
      modified (node add/remove, water update, lighting update)
	  - This can then be used to make sure the most recent version of
	    a block has been sent to client

SUGG: Make the amount of blocks sending to client and the total
	  amount of blocks dynamically limited. Transferring blocks is the
	  main network eater of this system, so it is the one that has
	  to be throttled so that RTTs stay low.

TODO: Untie client network operations from framerate
      - Needs some input queues or something
	  - Not really necessary?

TODO: Combine MapBlock's face caches to so big pieces that VBO
      gets used
      - That is >500 vertices

TODO: Better dungeons
TODO: Cliffs, arcs

TODO: Startup and configuration menu

TODO: There are some lighting-related todos and fixmes in
      ServerMap::emergeBlock

TODO: Proper handling of spawning place (try to find something that
      is not in the middle of an ocean (some land to stand on at
	  least) and save it in map config.

TODO: Players to only be hidden when the client quits.
TODO: - Players to be saved on disk, with inventory
TODO: Players to be saved as text in map/players/<name>
TODO: Player inventory to be saved on disk

TODO: Make fetching sector's blocks more efficient when rendering
      sectors that have very large amounts of blocks (on client)

TODO: Make the video backend selectable

Block object server side:
      - A "near blocks" buffer, in which some nearby blocks are stored.
	  - For all blocks in the buffer, objects are stepped(). This
	    means they are active.
	  - TODO: A global active buffer is needed for the server
	  - TODO: A timestamp to blocks
      - TODO: All blocks going in and out of the buffer are recorded.
	    - TODO: For outgoing blocks, timestamp is written.
	    - TODO: For incoming blocks, time difference is calculated and
	      objects are stepped according to it.

TODO: Copy the text of the last picked sign to inventory in creative
      mode

TODO: Get rid of GotSplitPacketException

TODO: Check what goes wrong with caching map to disk (Kray)

TODO: Remove LazyMeshUpdater. It is not used as supposed.

TODO: TOSERVER_LEAVE

TODO: Better handling of objects and mobs
      - Scripting?
      - There has to be some way to do it with less spaghetti code
	  - Make separate classes for client and server
	    - Client should not discriminate between blocks, server should
	    - Make other players utilize the same framework

TODO: Draw big amounts of torches better (that is, throw them in the
      same meshbuffer (can the meshcollector class be used?))

TODO: Check if the usage of Client::isFetchingBlocks() in
      updateViewingRange() actually does something

Doing now:
======================================================================

TODO: Convert the text input system to use a modal menu... or something

======================================================================

*/

/*
	Setting this to 1 enables a special camera mode that forces
	the renderers to think that the camera statically points from
	the starting place to a static direction.

	This allows one to move around with the player and see what
	is actually drawn behind solid things and behind the player.
*/
#define FIELD_OF_VIEW_TEST 0

#ifdef UNITTEST_DISABLE
	#ifdef _WIN32
		#pragma message ("Disabling unit tests")
	#else
		#warning "Disabling unit tests"
	#endif
	// Disable unit tests
	#define ENABLE_TESTS 0
#else
	// Enable unit tests
	#define ENABLE_TESTS 1
#endif

#ifdef _MSC_VER
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(lib, "jthread.lib")
#pragma comment(lib, "zlibwapi.lib")
// This would get rid of the console window
//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#include <iostream>
#include <fstream>
#include <jmutexautolock.h>
#include <locale.h>
#include "common_irrlicht.h"
#include "debug.h"
#include "map.h"
#include "player.h"
#include "main.h"
#include "test.h"
#include "environment.h"
#include "server.h"
#include "client.h"
#include "serialization.h"
#include "constants.h"
#include "strfnd.h"
#include "porting.h"
#include "irrlichtwrapper.h"
#include "gettime.h"
#include "porting.h"
#include "guiPauseMenu.h"
#include "guiInventoryMenu.h"
#include "guiTextInputMenu.h"

IrrlichtWrapper *g_irrlicht;

// All range-related stuff below is locked behind this
JMutex g_range_mutex;

// Blocks are viewed in this range from the player
s16 g_viewing_range_nodes = 60;
//s16 g_viewing_range_nodes = 0;

// This is updated by the client's fetchBlocks routine
//s16 g_actual_viewing_range_nodes = VIEWING_RANGE_NODES_DEFAULT;

// If true, the preceding value has no meaning and all blocks
// already existing in memory are drawn
bool g_viewing_range_all = false;

// This is the freetime ratio imposed by the dynamic viewing
// range changing code.
// It is controlled by the main loop to the smallest value that
// inhibits glitches (dtime jitter) in the main loop.
//float g_freetime_ratio = FREETIME_RATIO_MAX;

/*
	Settings.
	These are loaded from the config file.
*/

Settings g_settings;

extern void set_default_settings();

/*
	Random stuff
*/

//u16 g_selected_material = 0;
u16 g_selected_item = 0;

IrrlichtDevice *g_device = NULL;
Client *g_client = NULL;

/*
	GUI Stuff
*/
gui::IGUIEnvironment* guienv = NULL;
gui::IGUIStaticText *guiroot = NULL;
int g_active_menu_count = 0;

bool noMenuActive()
{
	return (g_active_menu_count == 0);
}

// Inventory actions from the menu are buffered here before sending
Queue<InventoryAction*> inventory_action_queue;
// This is a copy of the inventory that the client's environment has
Inventory local_inventory;

/*
	Debug streams
*/

// Connection
std::ostream *dout_con_ptr = &dummyout;
std::ostream *derr_con_ptr = &dstream_no_stderr;
//std::ostream *dout_con_ptr = &dstream_no_stderr;
//std::ostream *derr_con_ptr = &dstream_no_stderr;
//std::ostream *dout_con_ptr = &dstream;
//std::ostream *derr_con_ptr = &dstream;

// Server
std::ostream *dout_server_ptr = &dstream;
std::ostream *derr_server_ptr = &dstream;

// Client
std::ostream *dout_client_ptr = &dstream;
std::ostream *derr_client_ptr = &dstream;

/*
	gettime.h implementation
*/

u32 getTimeMs()
{
	/*
		Use irrlicht because it is more precise than porting.h's
		getTimeMs()
	*/
	if(g_irrlicht == NULL)
		return 0;
	return g_irrlicht->getTime();
}

/*
	Text input system
*/

struct TextDestSign : public TextDest
{
	TextDestSign(v3s16 blockpos, s16 id, Client *client)
	{
		m_blockpos = blockpos;
		m_id = id;
		m_client = client;
	}
	void gotText(std::wstring text)
	{
		std::string ntext = wide_to_narrow(text);
		dstream<<"Changing text of a sign object: "
				<<ntext<<std::endl;
		m_client->sendSignText(m_blockpos, m_id, ntext);
	}

	v3s16 m_blockpos;
	s16 m_id;
	Client *m_client;
};

struct TextDestChat : public TextDest
{
	TextDestChat(Client *client)
	{
		m_client = client;
	}
	void gotText(std::wstring text)
	{
		m_client->sendChatMessage(text);
		m_client->addChatMessage(text);
	}

	Client *m_client;
};

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
			clearInput();
			return false;
		}

		// Remember whether each key is down or up
		if(event.EventType == irr::EET_KEY_INPUT_EVENT)
		{
			keyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;

			if(event.KeyInput.PressedDown)
			{
				//dstream<<"Pressed key: "<<(char)event.KeyInput.Key<<std::endl;
				
				/*
					Launch menus
				*/

				if(guienv != NULL && guiroot != NULL && g_device != NULL)
				{
					if(event.KeyInput.Key == irr::KEY_ESCAPE)
					{
						dstream<<DTIME<<"MyEventReceiver: "
								<<"Launching pause menu"<<std::endl;
						// It will delete itself by itself
						(new GUIPauseMenu(guienv, guiroot, -1, g_device,
								&g_active_menu_count))->drop();
						return true;
					}
					if(event.KeyInput.Key == irr::KEY_KEY_I)
					{
						dstream<<DTIME<<"MyEventReceiver: "
								<<"Launching inventory"<<std::endl;
						(new GUIInventoryMenu(guienv, guiroot, -1,
								&local_inventory, &inventory_action_queue,
								&g_active_menu_count))->drop();
						return true;
					}
					if(event.KeyInput.Key == irr::KEY_KEY_T)
					{
						TextDest *dest = new TextDestChat(g_client);

						(new GUITextInputMenu(guienv, guiroot, -1,
								&g_active_menu_count, dest,
								L""))->drop();
					}
				}

				// Material selection
				if(event.KeyInput.Key == irr::KEY_KEY_F)
				{
					if(g_selected_item < PLAYER_INVENTORY_SIZE-1)
						g_selected_item++;
					else
						g_selected_item = 0;
					dstream<<DTIME<<"Selected item: "
							<<g_selected_item<<std::endl;
				}

				// Viewing range selection
				if(event.KeyInput.Key == irr::KEY_KEY_R)
				{
					JMutexAutoLock lock(g_range_mutex);
					if(g_viewing_range_all)
					{
						g_viewing_range_all = false;
						dstream<<DTIME<<"Disabled full viewing range"<<std::endl;
					}
					else
					{
						g_viewing_range_all = true;
						dstream<<DTIME<<"Enabled full viewing range"<<std::endl;
					}
				}

				// Print debug stacks
				if(event.KeyInput.Key == irr::KEY_KEY_P)
				{
					dstream<<"-----------------------------------------"
							<<std::endl;
					dstream<<DTIME<<"Printing debug stacks:"<<std::endl;
					dstream<<"-----------------------------------------"
							<<std::endl;
					debug_stacks_print();
				}
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
				//dstream<<"MyEventReceiver: mouse input"<<std::endl;
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
					/*dstream<<"event.MouseInput.Wheel="
							<<event.MouseInput.Wheel<<std::endl;*/
					if(event.MouseInput.Wheel < 0)
					{
						if(g_selected_item < PLAYER_INVENTORY_SIZE-1)
							g_selected_item++;
						else
							g_selected_item = 0;
					}
					else if(event.MouseInput.Wheel > 0)
					{
						if(g_selected_item > 0)
							g_selected_item--;
						else
							g_selected_item = PLAYER_INVENTORY_SIZE-1;
					}
				}
			}
		}

		return false;
	}

	// This is used to check whether a key is being held down
	virtual bool IsKeyDown(EKEY_CODE keyCode) const
	{
		return keyIsDown[keyCode];
	}

	void clearInput()
	{
		for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
				keyIsDown[i] = false;
		
		leftclicked = false;
		rightclicked = false;
		leftreleased = false;
		rightreleased = false;

		left_active = false;
		middle_active = false;
		right_active = false;
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

private:
	// We use this array to store the current state of each key
	bool keyIsDown[KEY_KEY_CODES_COUNT];
	//s32 mouseX;
	//s32 mouseY;
	IrrlichtDevice *m_device;
};

class InputHandler
{
public:
	InputHandler()
	{
	}
	virtual ~InputHandler()
	{
	}

	virtual bool isKeyDown(EKEY_CODE keyCode) = 0;

	virtual v2s32 getMousePos() = 0;
	virtual void setMousePos(s32 x, s32 y) = 0;

	virtual bool getLeftState() = 0;
	virtual bool getRightState() = 0;

	virtual bool getLeftClicked() = 0;
	virtual bool getRightClicked() = 0;
	virtual void resetLeftClicked() = 0;
	virtual void resetRightClicked() = 0;

	virtual bool getLeftReleased() = 0;
	virtual bool getRightReleased() = 0;
	virtual void resetLeftReleased() = 0;
	virtual void resetRightReleased() = 0;
	
	virtual void step(float dtime) {};

	virtual void clear() {};
};

InputHandler *g_input = NULL;

class RealInputHandler : public InputHandler
{
public:
	RealInputHandler(IrrlichtDevice *device, MyEventReceiver *receiver):
		m_device(device),
		m_receiver(receiver)
	{
	}
	virtual bool isKeyDown(EKEY_CODE keyCode)
	{
		return m_receiver->IsKeyDown(keyCode);
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

	void clear()
	{
		resetRightClicked();
		resetLeftClicked();
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
		leftclicked = false;
		rightclicked = false;
		for(u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
			keydown[i] = false;
	}
	virtual bool isKeyDown(EKEY_CODE keyCode)
	{
		return keydown[keyCode];
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
		return false;
	}
	virtual bool getRightState()
	{
		return false;
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
		return false;
	}
	virtual bool getRightReleased()
	{
		return false;
	}
	virtual void resetLeftReleased()
	{
	}
	virtual void resetRightReleased()
	{
	}

	virtual void step(float dtime)
	{
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1,10);
				/*if(g_selected_material < USEFUL_CONTENT_COUNT-1)
					g_selected_material++;
				else
					g_selected_material = 0;*/
				if(g_selected_item < PLAYER_INVENTORY_SIZE-1)
					g_selected_item++;
				else
					g_selected_item = 0;
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 40);
				keydown[irr::KEY_SPACE] = !keydown[irr::KEY_SPACE];
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 40);
				keydown[irr::KEY_KEY_2] = !keydown[irr::KEY_KEY_2];
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 40);
				keydown[irr::KEY_KEY_W] = !keydown[irr::KEY_KEY_W];
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 40);
				keydown[irr::KEY_KEY_A] = !keydown[irr::KEY_KEY_A];
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
				leftclicked = true;
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1, 20);
				rightclicked = true;
			}
		}
		mousepos += mousespeed;
	}

	s32 Rand(s32 min, s32 max)
	{
		return (rand()%(max-min+1))+min;
	}
private:
	bool keydown[KEY_KEY_CODES_COUNT];
	v2s32 mousepos;
	v2s32 mousespeed;
	bool leftclicked;
	bool rightclicked;
};

void updateViewingRange(f32 frametime, Client *client)
{
	// Range_all messes up frametime_avg
	if(g_viewing_range_all == true)
		return;

	float wanted_fps = g_settings.getFloat("wanted_fps");
	
	// Initialize to the target value
	static float frametime_avg = 1.0/wanted_fps;
	//frametime_avg = frametime_avg * 0.9 + frametime * 0.1;
	frametime_avg = frametime_avg * 0.7 + frametime * 0.3;

	static f32 counter = 0;
	if(counter > 0){
		counter -= frametime;
		return;
	}
	//counter = 1.0; //seconds
	counter = 0.5; //seconds

	//float freetime_ratio = 0.2;
	//float freetime_ratio = 0.4;
	float freetime_ratio = FREETIME_RATIO;

	float frametime_wanted = (1.0/(wanted_fps/(1.0-freetime_ratio)));

	float fraction = sqrt(frametime_avg / frametime_wanted);

	/*float fraction = sqrt(frametime_avg / frametime_wanted) / 2.0
			+ frametime_avg / frametime_wanted / 2.0;*/
	
	//float fraction = frametime_avg / frametime_wanted;

	static bool fraction_is_good = false;
	
	//float fraction_good_threshold = 0.1;
	//float fraction_bad_threshold = 0.25;
	float fraction_good_threshold = 0.075;
	float fraction_bad_threshold = 0.125;
	float fraction_limit;
	// Use high limit if fraction is good AND the fraction would
	// lower the range. We want to keep the range fairly high.
	if(fraction_is_good && fraction > 1.0)
		fraction_limit = fraction_bad_threshold;
	else
		fraction_limit = fraction_good_threshold;

	if(fabs(fraction - 1.0) < fraction_limit)
	{
		fraction_is_good = true;
		return;
	}
	else
	{
		fraction_is_good = false;
	}

	//dstream<<"frametime_avg="<<frametime_avg<<std::endl;
	//dstream<<"frametime_wanted="<<frametime_wanted<<std::endl;
	/*dstream<<"fetching="<<client->isFetchingBlocks()
			<<" faction = "<<fraction<<std::endl;*/

	JMutexAutoLock lock(g_range_mutex);
	
	s16 viewing_range_nodes_min = g_settings.getS16("viewing_range_nodes_min");
	s16 viewing_range_nodes_max = g_settings.getS16("viewing_range_nodes_max");

	s16 n = (float)g_viewing_range_nodes / fraction;
	if(n < viewing_range_nodes_min)
		n = viewing_range_nodes_min;
	if(n > viewing_range_nodes_max)
		n = viewing_range_nodes_max;

	bool can_change = true;

	if(client->isFetchingBlocks() == true && n > g_viewing_range_nodes)
		can_change = false;
	
	if(can_change)
		g_viewing_range_nodes = n;

	/*dstream<<"g_viewing_range_nodes = "
			<<g_viewing_range_nodes<<std::endl;*/
}

class GUIQuickInventory : public IEventReceiver
{
public:
	GUIQuickInventory(
			gui::IGUIEnvironment* env,
			gui::IGUIElement* parent,
			v2s32 pos,
			s32 itemcount,
			Inventory *inventory):
		m_itemcount(itemcount),
		m_inventory(inventory)
	{
		core::rect<s32> imgsize(0,0,48,48);
		core::rect<s32> textsize(0,0,48,16);
		v2s32 spacing(0, 64);
		for(s32 i=0; i<m_itemcount; i++)
		{
			m_images.push_back(env->addImage(
				imgsize + pos + spacing*i
			));
			m_images[i]->setScaleImage(true);
			m_texts.push_back(env->addStaticText(
				L"",
				textsize + pos + spacing*i,
				false, false
			));
			m_texts[i]->setBackgroundColor(
					video::SColor(128,0,0,0));
			m_texts[i]->setTextAlignment(
					gui::EGUIA_CENTER,
					gui::EGUIA_UPPERLEFT);
		}
	}

	virtual bool OnEvent(const SEvent& event)
	{
		return false;
	}

	void setSelection(s32 i)
	{
		m_selection = i;
	}

	void update()
	{
		s32 start = 0;

		start = m_selection - m_itemcount / 2;

		InventoryList *mainlist = m_inventory->getList("main");

		for(s32 i=0; i<m_itemcount; i++)
		{
			s32 j = i + start;

			if(j > (s32)mainlist->getSize() - 1)
				j -= mainlist->getSize();
			if(j < 0)
				j += mainlist->getSize();
			
			InventoryItem *item = mainlist->getItem(j);
			// Null items
			if(item == NULL)
			{
				m_images[i]->setImage(NULL);

				wchar_t t[10];
				if(m_selection == j)
					swprintf(t, 10, L"<-");
				else
					swprintf(t, 10, L"");
				m_texts[i]->setText(t);

				// The next ifs will segfault with a NULL pointer
				continue;
			}
			
			
			m_images[i]->setImage(item->getImage());
			
			wchar_t t[10];
			if(m_selection == j)
				swprintf(t, 10, SWPRINTF_CHARSTRING L" <-", item->getText().c_str());
			else
				swprintf(t, 10, SWPRINTF_CHARSTRING, item->getText().c_str());
			m_texts[i]->setText(t);
		}
	}

private:
	s32 m_itemcount;
	core::array<gui::IGUIStaticText*> m_texts;
	core::array<gui::IGUIImage*> m_images;
	Inventory *m_inventory;
	s32 m_selection;
};

int main(int argc, char *argv[])
{
	/*
		Low-level initialization
	*/

	bool disable_stderr = false;
#ifdef _WIN32
	disable_stderr = true;
#endif

	// Initialize debug streams
	debugstreams_init(disable_stderr, DEBUGFILE);
	// Initialize debug stacks
	debug_stacks_init();

	DSTACK(__FUNCTION_NAME);

	try
	{
	
	/*
		Parse command line
	*/
	
	// List all allowed options
	core::map<std::string, ValueSpec> allowed_options;
	allowed_options.insert("help", ValueSpec(VALUETYPE_FLAG));
	allowed_options.insert("server", ValueSpec(VALUETYPE_FLAG,
			"Run server directly"));
	allowed_options.insert("config", ValueSpec(VALUETYPE_STRING,
			"Load configuration from specified file"));
	allowed_options.insert("port", ValueSpec(VALUETYPE_STRING));
	allowed_options.insert("address", ValueSpec(VALUETYPE_STRING));
	allowed_options.insert("random-input", ValueSpec(VALUETYPE_FLAG));
	allowed_options.insert("disable-unittests", ValueSpec(VALUETYPE_FLAG));
	allowed_options.insert("enable-unittests", ValueSpec(VALUETYPE_FLAG));

	Settings cmd_args;
	
	bool ret = cmd_args.parseCommandLine(argc, argv, allowed_options);

	if(ret == false || cmd_args.getFlag("help"))
	{
		dstream<<"Allowed options:"<<std::endl;
		for(core::map<std::string, ValueSpec>::Iterator
				i = allowed_options.getIterator();
				i.atEnd() == false; i++)
		{
			dstream<<"  --"<<i.getNode()->getKey();
			if(i.getNode()->getValue().type == VALUETYPE_FLAG)
			{
			}
			else
			{
				dstream<<" <value>";
			}
			dstream<<std::endl;

			if(i.getNode()->getValue().help != NULL)
			{
				dstream<<"      "<<i.getNode()->getValue().help
						<<std::endl;
			}
		}

		return cmd_args.getFlag("help") ? 0 : 1;
	}


	/*
		Basic initialization
	*/

	// Initialize default settings
	set_default_settings();
	
	// Print startup message
	dstream<<DTIME<<"minetest-c55"
			" with SER_FMT_VER_HIGHEST="<<(int)SER_FMT_VER_HIGHEST
			<<", ENABLE_TESTS="<<ENABLE_TESTS
			<<std::endl;
	
	// Set locale. This is for forcing '.' as the decimal point.
	std::locale::global(std::locale("C"));
	// This enables printing all characters in bitmap font
	setlocale(LC_CTYPE, "en_US");

	// Initialize sockets
	sockets_init();
	atexit(sockets_cleanup);
	
	/*
		Initialization
	*/

	/*
		Read config file
	*/
	
	// Path of configuration file in use
	std::string configpath = "";
	
	if(cmd_args.exists("config"))
	{
		bool r = g_settings.readConfigFile(cmd_args.get("config").c_str());
		if(r == false)
		{
			dstream<<"Could not read configuration from \""
					<<cmd_args.get("config")<<"\""<<std::endl;
			return 1;
		}
		configpath = cmd_args.get("config");
	}
	else
	{
		const char *filenames[2] =
		{
			"../minetest.conf",
			"../../minetest.conf"
		};

		for(u32 i=0; i<2; i++)
		{
			bool r = g_settings.readConfigFile(filenames[i]);
			if(r)
			{
				configpath = filenames[i];
				break;
			}
		}
	}

	// Initialize random seed
	srand(time(0));

	/*
		Run unit tests
	*/
	if((ENABLE_TESTS && cmd_args.getFlag("disable-unittests") == false)
			|| cmd_args.getFlag("enable-unittests") == true)
	{
		run_tests();
	}
	
	/*
		Global range mutex
	*/
	g_range_mutex.Init();
	assert(g_range_mutex.IsInitialized());

	// Read map parameters from settings

	HMParams hm_params;
	hm_params.blocksize = g_settings.getU16("heightmap_blocksize");
	hm_params.randmax = g_settings.get("height_randmax");
	hm_params.randfactor = g_settings.get("height_randfactor");
	hm_params.base = g_settings.get("height_base");

	MapParams map_params;
	map_params.plants_amount = g_settings.getFloat("plants_amount");
	map_params.ravines_amount = g_settings.getFloat("ravines_amount");

	/*
		Ask some stuff
	*/

	std::cout<<std::endl<<std::endl;
	
	std::cout
	<<"        .__               __                   __   "<<std::endl
	<<"  _____ |__| ____   _____/  |_  ____   _______/  |_ "<<std::endl
	<<" /     \\|  |/    \\_/ __ \\   __\\/ __ \\ /  ___/\\   __\\"<<std::endl
	<<"|  Y Y  \\  |   |  \\  ___/|  | \\  ___/ \\___ \\  |  |  "<<std::endl
	<<"|__|_|  /__|___|  /\\___  >__|  \\___  >____  > |__|  "<<std::endl
	<<"      \\/        \\/     \\/          \\/     \\/        "<<std::endl
	<<std::endl;

	std::cout<<std::endl;
	//char templine[100];
	
	// Port?
	u16 port = 30000;
	if(cmd_args.exists("port"))
	{
		port = cmd_args.getU16("port");
	}
	else
	{
		port = g_settings.getU16Ask("port", "Port", 30000);
		std::cout<<"-> "<<port<<std::endl;
	}
	
	if(cmd_args.getFlag("server"))
	{
		DSTACK("Dedicated server branch");
		
		std::cout<<std::endl;
		std::cout<<"========================"<<std::endl;
		std::cout<<"Running dedicated server"<<std::endl;
		std::cout<<"========================"<<std::endl;
		std::cout<<std::endl;

		Server server("../map", hm_params, map_params);
		server.start(port);
	
		for(;;)
		{
			// This is kind of a hack but can be done like this
			// because server.step() is very light
			sleep_ms(30);
			server.step(0.030);

			static int counter = 0;
			counter--;
			if(counter <= 0)
			{
				counter = 10;

				core::list<PlayerInfo> list = server.getPlayerInfo();
				core::list<PlayerInfo>::Iterator i;
				static u32 sum_old = 0;
				u32 sum = PIChecksum(list);
				if(sum != sum_old)
				{
					std::cout<<DTIME<<"Player info:"<<std::endl;
					for(i=list.begin(); i!=list.end(); i++)
					{
						i->PrintLine(&std::cout);
					}
				}
				sum_old = sum;
			}
		}

		return 0;
	}

	bool hosting = false;
	char connect_name[100] = "";

	if(cmd_args.exists("address"))
	{
		snprintf(connect_name, 100, "%s", cmd_args.get("address").c_str());
	}
	else if(is_yes(g_settings.get("host_game")) == false)
	{
		if(g_settings.get("address") != "")
		{
			std::cout<<g_settings.get("address")<<std::endl;
			snprintf(connect_name, 100, "%s", g_settings.get("address").c_str());
		}
		else
		{
			std::cout<<"Address to connect to [empty = host a game]: ";
			std::cin.getline(connect_name, 100);
		}
	}
	
	if(connect_name[0] == 0){
		snprintf(connect_name, 100, "127.0.0.1");
		hosting = true;
	}
	
	if(hosting)
		std::cout<<"> Hosting game"<<std::endl;
	else
		std::cout<<"> Connecting to "<<connect_name<<std::endl;
	
	char playername[PLAYERNAME_SIZE] = "";
	if(g_settings.get("name") != "")
	{
		snprintf(playername, PLAYERNAME_SIZE, "%s", g_settings.get("name").c_str());
	}
	else
	{
		std::cout<<"Name of player: ";
		std::cin.getline(playername, PLAYERNAME_SIZE);
	}
	std::cout<<"-> \""<<playername<<"\""<<std::endl;

	/*
		Resolution selection
	*/
	
	bool fullscreen = false;
	u16 screenW = atoi(g_settings.get("screenW").c_str());
	u16 screenH = atoi(g_settings.get("screenH").c_str());

	//

	MyEventReceiver receiver;

	video::E_DRIVER_TYPE driverType;

#ifdef _WIN32
	//driverType = video::EDT_DIRECT3D9; // Doesn't seem to work
	driverType = video::EDT_OPENGL;
#else
	driverType = video::EDT_OPENGL;
#endif

	// create device and exit if creation failed

	IrrlichtDevice *device;
	device = createDevice(driverType,
			core::dimension2d<u32>(screenW, screenH),
			16, fullscreen, false, false, &receiver);

	if (device == 0)
		return 1; // could not create selected driver.
	
	g_device = device;
	g_irrlicht = new IrrlichtWrapper(device);

	//g_device = device;
	
	device->setResizable(true);

	bool random_input = g_settings.getBool("random_input")
			|| cmd_args.getFlag("random-input");
	if(random_input)
		g_input = new RandomInputHandler();
	else
		g_input = new RealInputHandler(device, &receiver);
	
	/*
		Continue initialization
	*/

	video::IVideoDriver* driver = device->getVideoDriver();

	/*
		This changes the minimum allowed number of vertices in a VBO
	*/
	//driver->setMinHardwareBufferVertexCount(1);

	scene::ISceneManager* smgr = device->getSceneManager();
	
	guienv = device->getGUIEnvironment();
	gui::IGUISkin* skin = guienv->getSkin();
	gui::IGUIFont* font = guienv->getFont("../data/fontlucida.png");
	if(font)
		skin->setFont(font);
	
	u32 text_height = font->getDimension(L"Hello, world!").Height;
	dstream<<"text_height="<<text_height<<std::endl;

	//skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255,0,0,0));
	skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255,255,255,255));
	//skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(0,0,0,0));
	//skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(0,0,0,0));
	skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(255,0,0,0));
	skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(255,0,0,0));
	
	const wchar_t *text = L"Loading and connecting...";
	core::vector2d<s32> center(screenW/2, screenH/2);
	core::vector2d<s32> textsize(300, text_height);
	core::rect<s32> textrect(center - textsize/2, center + textsize/2);

	gui::IGUIStaticText *gui_loadingtext = guienv->addStaticText(
			text, textrect, false, false);
	gui_loadingtext->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);

	driver->beginScene(true, true, video::SColor(255,0,0,0));
	guienv->drawAll();
	driver->endScene();

	/*
		Preload some textures
	*/

	tile_materials_preload(g_irrlicht);

	/*
		Make a scope here for the client so that it gets removed
		before the irrlicht device
	*/
	{

	std::cout<<DTIME<<"Creating server and client"<<std::endl;
	
	/*
		Create server
	*/
	SharedPtr<Server> server;
	if(hosting){
		server = new Server("../map", hm_params, map_params);
		server->start(port);
	}
	
	/*
		Create client
	*/

	Client client(device, playername,
			g_range_mutex,
			g_viewing_range_nodes,
			g_viewing_range_all);
			
	g_client = &client;
	
	Address connect_address(0,0,0,0, port);
	try{
		connect_address.Resolve(connect_name);
	}
	catch(ResolveError &e)
	{
		std::cout<<DTIME<<"Couldn't resolve address"<<std::endl;
		return 0;
	}
	
	std::cout<<DTIME<<"Connecting to server..."<<std::endl;
	client.connect(connect_address);
	
	try{
		while(client.connectedAndInitialized() == false)
		{
			client.step(0.1);
			if(server != NULL){
				server->step(0.1);
			}
			sleep_ms(100);
		}
	}
	catch(con::PeerNotFoundException &e)
	{
		std::cout<<DTIME<<"Timed out."<<std::endl;
		return 0;
	}

	/*
		Create skybox
	*/
	/*scene::ISceneNode* skybox;
	skybox = smgr->addSkyBoxSceneNode(
		driver->getTexture("../data/skybox2.png"),
		driver->getTexture("../data/skybox3.png"),
		driver->getTexture("../data/skybox1.png"),
		driver->getTexture("../data/skybox1.png"),
		driver->getTexture("../data/skybox1.png"),
		driver->getTexture("../data/skybox1.png"));*/
	
	/*
		Create the camera node
	*/

	scene::ICameraSceneNode* camera = smgr->addCameraSceneNode(
		0, // Camera parent
		v3f(BS*100, BS*2, BS*100), // Look from
		v3f(BS*100+1, BS*2, BS*100), // Look to
		-1 // Camera ID
   	);

	if(camera == NULL)
		return 1;
	
	video::SColor skycolor = video::SColor(255,90,140,200);

	camera->setFOV(FOV_ANGLE);

	// Just so big a value that everything rendered is visible
	camera->setFarValue(100000*BS);

	f32 camera_yaw = 0; // "right/left"
	f32 camera_pitch = 0; // "up/down"

	/*
		Move into game
	*/
	
	gui_loadingtext->remove();

	/*
		Add some gui stuff
	*/

	GUIQuickInventory *quick_inventory = new GUIQuickInventory
			(guienv, NULL, v2s32(10, 70), 5, &local_inventory);
	
	/*
		We need some kind of a root node to be able to add
		custom elements directly on the screen.
		Otherwise they won't be automatically drawn.
	*/
	guiroot = guienv->addStaticText(L"",
			core::rect<s32>(0, 0, 10000, 10000));
	
	// Test the text input system
	/*(new GUITextInputMenu(guienv, guiroot, -1, &g_active_menu_count,
			NULL))->drop();*/

	// First line of debug text
	gui::IGUIStaticText *guitext = guienv->addStaticText(
			L"Minetest-c55",
			core::rect<s32>(5, 5, 795, 5+textsize.Y),
			false, false);
	// Second line of debug text
	gui::IGUIStaticText *guitext2 = guienv->addStaticText(
			L"",
			core::rect<s32>(5, 5+(textsize.Y+5)*1, 795, (5+textsize.Y)*2),
			false, false);
	
	// At the middle of the screen
	// Object infos are shown in this
	gui::IGUIStaticText *guitext_info = guienv->addStaticText(
			L"test",
			core::rect<s32>(100, 70, 100+400, 70+(textsize.Y+5)),
			false, false);
	
	// Chat text
	gui::IGUIStaticText *chat_guitext = guienv->addStaticText(
			L"Chat here\nOther line\nOther line\nOther line\nOther line",
			core::rect<s32>(70, 60, 795, 150),
			false, true);
	core::list<std::wstring> chat_lines;
	//chat_lines.push_back(L"Minetest-c55 up and running!");
	
	/*
		Some statistics are collected in these
	*/
	u32 drawtime = 0;
	u32 beginscenetime = 0;
	u32 scenetime = 0;
	u32 endscenetime = 0;

	/*
		Main loop
	*/

	bool first_loop_after_window_activation = true;

	// Time is in milliseconds
	// NOTE: getRealTime() without run()s causes strange problems in wine
	// NOTE: Have to call run() between calls of this to update the timer
	u32 lasttime = device->getTimer()->getTime();

	while(device->run())
	{
		/*
			Run global IrrlichtWrapper's main thread processing stuff
		*/
		g_irrlicht->Run();

		/*
			Random calculations
		*/
		v2u32 screensize = driver->getScreenSize();
		core::vector2d<s32> displaycenter(screensize.X/2,screensize.Y/2);
		
		// Hilight boxes collected during the loop and displayed
		core::list< core::aabbox3d<f32> > hilightboxes;
		
		// Info text
		std::wstring infotext;

		//TimeTaker //timer1("//timer1");
		
		// Time of frame without fps limit
		float busytime;
		u32 busytime_u32;
		{
			// not using getRealTime is necessary for wine
			u32 time = device->getTimer()->getTime();
			if(time > lasttime)
				busytime_u32 = time - lasttime;
			else
				busytime_u32 = 0;
			busytime = busytime_u32 / 1000.0;
		}

		//std::cout<<"busytime_u32="<<busytime_u32<<std::endl;
	
		// Absolutelu necessary for wine!
		device->run();

		/*
			Viewing range
		*/
		
		updateViewingRange(busytime, &client);
		
		/*
			FPS limiter
		*/

		{
			float fps_max = g_settings.getFloat("fps_max");
			u32 frametime_min = 1000./fps_max;
			
			if(busytime_u32 < frametime_min)
			{
				u32 sleeptime = frametime_min - busytime_u32;
				device->sleep(sleeptime);
			}
		}

		// Absolutelu necessary for wine!
		device->run();

		/*
			Time difference calculation
		*/
		f32 dtime; // in seconds
		
		u32 time = device->getTimer()->getTime();
		if(time > lasttime)
			dtime = (time - lasttime) / 1000.0;
		else
			dtime = 0;
		lasttime = time;

		/*
			Time average and jitter calculation
		*/

		static f32 dtime_avg1 = 0.0;
		dtime_avg1 = dtime_avg1 * 0.98 + dtime * 0.02;
		f32 dtime_jitter1 = dtime - dtime_avg1;

		static f32 dtime_jitter1_max_sample = 0.0;
		static f32 dtime_jitter1_max_fraction = 0.0;
		{
			static f32 jitter1_max = 0.0;
			static f32 counter = 0.0;
			if(dtime_jitter1 > jitter1_max)
				jitter1_max = dtime_jitter1;
			counter += dtime;
			if(counter > 0.0)
			{
				counter -= 3.0;
				dtime_jitter1_max_sample = jitter1_max;
				dtime_jitter1_max_fraction
						= dtime_jitter1_max_sample / (dtime_avg1+0.001);
				jitter1_max = 0.0;
				
				/*
					Control freetime ratio
				*/
				/*if(dtime_jitter1_max_fraction > DTIME_JITTER_MAX_FRACTION)
				{
					if(g_freetime_ratio < FREETIME_RATIO_MAX)
						g_freetime_ratio += 0.01;
				}
				else
				{
					if(g_freetime_ratio > FREETIME_RATIO_MIN)
						g_freetime_ratio -= 0.01;
				}*/
			}
		}
		
		/*
			Busytime average and jitter calculation
		*/

		static f32 busytime_avg1 = 0.0;
		busytime_avg1 = busytime_avg1 * 0.98 + busytime * 0.02;
		f32 busytime_jitter1 = busytime - busytime_avg1;
		
		static f32 busytime_jitter1_max_sample = 0.0;
		static f32 busytime_jitter1_min_sample = 0.0;
		{
			static f32 jitter1_max = 0.0;
			static f32 jitter1_min = 0.0;
			static f32 counter = 0.0;
			if(busytime_jitter1 > jitter1_max)
				jitter1_max = busytime_jitter1;
			if(busytime_jitter1 < jitter1_min)
				jitter1_min = busytime_jitter1;
			counter += dtime;
			if(counter > 0.0){
				counter -= 3.0;
				busytime_jitter1_max_sample = jitter1_max;
				busytime_jitter1_min_sample = jitter1_min;
				jitter1_max = 0.0;
				jitter1_min = 0.0;
			}
		}
		
		/*
			Debug info for client
		*/
		{
			static float counter = 0.0;
			counter -= dtime;
			if(counter < 0)
			{
				counter = 30.0;
				client.printDebugInfo(std::cout);
			}
		}

		/*
			Input handler step()
		*/
		g_input->step(dtime);

		/*
			Player speed control
		*/
		
		{
			/*bool a_up,
			bool a_down,
			bool a_left,
			bool a_right,
			bool a_jump,
			bool a_superspeed,
			float a_pitch,
			float a_yaw*/
			PlayerControl control(
				g_input->isKeyDown(irr::KEY_KEY_W),
				g_input->isKeyDown(irr::KEY_KEY_S),
				g_input->isKeyDown(irr::KEY_KEY_A),
				g_input->isKeyDown(irr::KEY_KEY_D),
				g_input->isKeyDown(irr::KEY_SPACE),
				g_input->isKeyDown(irr::KEY_KEY_2),
				camera_pitch,
				camera_yaw
			);
			client.setPlayerControl(control);
		}

		/*
			Process environment
		*/
		
		{
			//TimeTaker timer("client.step(dtime)");
			client.step(dtime);
			//client.step(dtime_avg1);
		}

		if(server != NULL)
		{
			//TimeTaker timer("server->step(dtime)");
			server->step(dtime);
		}

		v3f player_position = client.getPlayerPosition();
		
		//TimeTaker //timer2("//timer2");

		/*
			Mouse and camera control
		*/
		
		if((device->isWindowActive() && noMenuActive()) || random_input)
		{
			if(!random_input)
				device->getCursorControl()->setVisible(false);

			if(first_loop_after_window_activation){
				//std::cout<<"window active, first loop"<<std::endl;
				first_loop_after_window_activation = false;
			}
			else{
				s32 dx = g_input->getMousePos().X - displaycenter.X;
				s32 dy = g_input->getMousePos().Y - displaycenter.Y;
				//std::cout<<"window active, pos difference "<<dx<<","<<dy<<std::endl;
				camera_yaw -= dx*0.2;
				camera_pitch += dy*0.2;
				if(camera_pitch < -89.5) camera_pitch = -89.5;
				if(camera_pitch > 89.5) camera_pitch = 89.5;
			}
			g_input->setMousePos(displaycenter.X, displaycenter.Y);
		}
		else{
			device->getCursorControl()->setVisible(true);

			//std::cout<<"window inactive"<<std::endl;
			first_loop_after_window_activation = true;
		}

		camera_yaw = wrapDegrees(camera_yaw);
		camera_pitch = wrapDegrees(camera_pitch);
		
		v3f camera_direction = v3f(0,0,1);
		camera_direction.rotateYZBy(camera_pitch);
		camera_direction.rotateXZBy(camera_yaw);

		v3f camera_position =
				player_position + v3f(0, BS+BS/2, 0);

		camera->setPosition(camera_position);
		// *100.0 helps in large map coordinates
		camera->setTarget(camera_position + camera_direction * 100.0);

		if(FIELD_OF_VIEW_TEST){
			//client.m_env.getMap().updateCamera(v3f(0,0,0), v3f(0,0,1));
			client.updateCamera(v3f(0,0,0), v3f(0,0,1));
		}
		else{
			//client.m_env.getMap().updateCamera(camera_position, camera_direction);
			//TimeTaker timer("client.updateCamera");
			client.updateCamera(camera_position, camera_direction);
		}
		
		//timer2.stop();
		//TimeTaker //timer3("//timer3");

		/*
			Calculate what block is the crosshair pointing to
		*/
		
		//u32 t1 = device->getTimer()->getRealTime();
		
		//f32 d = 4; // max. distance
		f32 d = 4; // max. distance
		core::line3d<f32> shootline(camera_position,
				camera_position + camera_direction * BS * (d+1));

		MapBlockObject *selected_object = client.getSelectedObject
				(d*BS, camera_position, shootline);

		if(selected_object != NULL)
		{
			//dstream<<"Client returned selected_object != NULL"<<std::endl;

			core::aabbox3d<f32> box_on_map
					= selected_object->getSelectionBoxOnMap();

			hilightboxes.push_back(box_on_map);

			infotext = narrow_to_wide(selected_object->infoText());

			if(g_input->getLeftClicked())
			{
				std::cout<<DTIME<<"Left-clicked object"<<std::endl;
				client.clickObject(0, selected_object->getBlock()->getPos(),
						selected_object->getId(), g_selected_item);
			}
			else if(g_input->getRightClicked())
			{
				std::cout<<DTIME<<"Right-clicked object"<<std::endl;
				/*
					Check if we want to modify the object ourselves
				*/
				if(selected_object->getTypeId() == MAPBLOCKOBJECT_TYPE_SIGN)
				{
					dstream<<"Sign object right-clicked"<<std::endl;
					
					if(random_input == false)
					{
						// Get a new text for it

						TextDest *dest = new TextDestSign(
								selected_object->getBlock()->getPos(),
								selected_object->getId(),
								&client);

						SignObject *sign_object = (SignObject*)selected_object;

						std::wstring wtext =
								narrow_to_wide(sign_object->getText());

						(new GUITextInputMenu(guienv, guiroot, -1,
								&g_active_menu_count, dest,
								wtext))->drop();
					}
				}
				/*
					Otherwise pass the event to the server as-is
				*/
				else
				{
					client.clickObject(1, selected_object->getBlock()->getPos(),
							selected_object->getId(), g_selected_item);
				}
			}
		}
		else // selected_object == NULL
		{

		/*
			Find out which node we are pointing at
		*/
		
		bool nodefound = false;
		v3s16 nodepos;
		v3s16 neighbourpos;
		core::aabbox3d<f32> nodefacebox;
		f32 mindistance = BS * 1001;
		
		v3s16 pos_i = floatToInt(player_position);

		/*std::cout<<"pos_i=("<<pos_i.X<<","<<pos_i.Y<<","<<pos_i.Z<<")"
				<<std::endl;*/

		s16 a = d;
		s16 ystart = pos_i.Y + 0 - (camera_direction.Y<0 ? a : 1);
		s16 zstart = pos_i.Z - (camera_direction.Z<0 ? a : 1);
		s16 xstart = pos_i.X - (camera_direction.X<0 ? a : 1);
		s16 yend = pos_i.Y + 1 + (camera_direction.Y>0 ? a : 1);
		s16 zend = pos_i.Z + (camera_direction.Z>0 ? a : 1);
		s16 xend = pos_i.X + (camera_direction.X>0 ? a : 1);
		
		for(s16 y = ystart; y <= yend; y++)
		for(s16 z = zstart; z <= zend; z++)
		for(s16 x = xstart; x <= xend; x++)
		{
			MapNode n;
			try
			{
				n = client.getNode(v3s16(x,y,z));
				if(content_pointable(n.d) == false)
					continue;
			}
			catch(InvalidPositionException &e)
			{
				continue;
			}

			v3s16 np(x,y,z);
			v3f npf = intToFloat(np);
			
			f32 d = 0.01;
			
			v3s16 dirs[6] = {
				v3s16(0,0,1), // back
				v3s16(0,1,0), // top
				v3s16(1,0,0), // right
				v3s16(0,0,-1), // front
				v3s16(0,-1,0), // bottom
				v3s16(-1,0,0), // left
			};
			
			/*
				Meta-objects
			*/
			if(n.d == CONTENT_TORCH)
			{
				v3s16 dir = unpackDir(n.dir);
				v3f dir_f = v3f(dir.X, dir.Y, dir.Z);
				dir_f *= BS/2 - BS/6 - BS/20;
				v3f cpf = npf + dir_f;
				f32 distance = (cpf - camera_position).getLength();

				core::aabbox3d<f32> box;
				
				// bottom
				if(dir == v3s16(0,-1,0))
				{
					box = core::aabbox3d<f32>(
						npf - v3f(BS/6, BS/2, BS/6),
						npf + v3f(BS/6, -BS/2+BS/3*2, BS/6)
					);
				}
				// top
				else if(dir == v3s16(0,1,0))
				{
					box = core::aabbox3d<f32>(
						npf - v3f(BS/6, -BS/2+BS/3*2, BS/6),
						npf + v3f(BS/6, BS/2, BS/6)
					);
				}
				// side
				else
				{
					box = core::aabbox3d<f32>(
						cpf - v3f(BS/6, BS/3, BS/6),
						cpf + v3f(BS/6, BS/3, BS/6)
					);
				}

				if(distance < mindistance)
				{
					if(box.intersectsWithLine(shootline))
					{
						nodefound = true;
						nodepos = np;
						neighbourpos = np;
						mindistance = distance;
						nodefacebox = box;
					}
				}
			}
			/*
				Regular blocks
			*/
			else
			{
				for(u16 i=0; i<6; i++)
				{
					v3f dir_f = v3f(dirs[i].X,
							dirs[i].Y, dirs[i].Z);
					v3f centerpoint = npf + dir_f * BS/2;
					f32 distance =
							(centerpoint - camera_position).getLength();
					
					if(distance < mindistance)
					{
						core::CMatrix4<f32> m;
						m.buildRotateFromTo(v3f(0,0,1), dir_f);

						// This is the back face
						v3f corners[2] = {
							v3f(BS/2, BS/2, BS/2),
							v3f(-BS/2, -BS/2, BS/2+d)
						};
						
						for(u16 j=0; j<2; j++)
						{
							m.rotateVect(corners[j]);
							corners[j] += npf;
						}

						core::aabbox3d<f32> facebox(corners[0]);
						facebox.addInternalPoint(corners[1]);

						if(facebox.intersectsWithLine(shootline))
						{
							nodefound = true;
							nodepos = np;
							neighbourpos = np + dirs[i];
							mindistance = distance;
							nodefacebox = facebox;
						}
					} // if distance < mindistance
				} // for dirs
			} // regular block
		} // for coords

		if(nodefound)
		{
			static v3s16 nodepos_old(-32768,-32768,-32768);

			static float dig_time = 0.0;
			static u16 dig_index = 0;

			if(nodepos != nodepos_old)
			{
				std::cout<<DTIME<<"Pointing at ("<<nodepos.X<<","
						<<nodepos.Y<<","<<nodepos.Z<<")"<<std::endl;

				if(nodepos_old != v3s16(-32768,-32768,-32768))
				{
					client.clearTempMod(nodepos_old);
					dig_time = 0.0;
				}
			}

			hilightboxes.push_back(nodefacebox);
			
			if(g_input->getLeftReleased())
			{
				client.clearTempMod(nodepos);
				dig_time = 0.0;
			}
			if(g_input->getLeftClicked() ||
					(g_input->getLeftState() && nodepos != nodepos_old))
			{
				dstream<<DTIME<<"Started digging"<<std::endl;
				client.groundAction(0, nodepos, neighbourpos, g_selected_item);
			}
			if(g_input->getLeftClicked())
			{
				client.setTempMod(nodepos, NodeMod(NODEMOD_CRACK, 0));
			}
			if(g_input->getLeftState())
			{
				MapNode n = client.getNode(nodepos);

				// TODO: Get this from some table that is sent by server
				float dig_time_complete = 0.5;
				if(n.d == CONTENT_STONE)
					dig_time_complete = 1.5;
				
				dig_index = (u16)((float)CRACK_ANIMATION_LENGTH
						* dig_time/dig_time_complete);

				if(dig_index < CRACK_ANIMATION_LENGTH)
				{
					//dstream<<"dig_index="<<dig_index<<std::endl;
					client.setTempMod(nodepos, NodeMod(NODEMOD_CRACK, dig_index));
				}
				else
				{
					dstream<<DTIME<<"Digging completed"<<std::endl;
					client.groundAction(3, nodepos, neighbourpos, g_selected_item);
					client.clearTempMod(nodepos);
					client.removeNode(nodepos);
				}

				dig_time += dtime;
			}
			
			if(g_input->getRightClicked())
			{
				std::cout<<DTIME<<"Ground right-clicked"<<std::endl;
				client.groundAction(1, nodepos, neighbourpos, g_selected_item);
			}
			
			nodepos_old = nodepos;
		}
		else{
		}

		} // selected_object == NULL
		
		g_input->resetLeftClicked();
		g_input->resetRightClicked();
		
		if(g_input->getLeftReleased())
		{
			std::cout<<DTIME<<"Left button released (stopped digging)"
					<<std::endl;
			client.groundAction(2, v3s16(0,0,0), v3s16(0,0,0), 0);
		}
		if(g_input->getRightReleased())
		{
			//std::cout<<DTIME<<"Right released"<<std::endl;
			// Nothing here
		}
		
		g_input->resetLeftReleased();
		g_input->resetRightReleased();
		
		/*
			Calculate stuff for drawing
		*/

		camera->setAspectRatio((f32)screensize.X / (f32)screensize.Y);
		
		u32 daynight_ratio = client.getDayNightRatio();
		video::SColor bgcolor = video::SColor(
				255,
				skycolor.getRed() * daynight_ratio / 1000,
				skycolor.getGreen() * daynight_ratio / 1000,
				skycolor.getBlue() * daynight_ratio / 1000);

		/*
			Fog
		*/
		
		if(g_settings.getBool("enable_fog") == true)
		{
			f32 range = g_viewing_range_nodes * BS;
			if(g_viewing_range_all)
				range = 100000*BS;

			driver->setFog(
				bgcolor,
				video::EFT_FOG_LINEAR,
				range*0.6,
				range,
				0.01,
				false, // pixel fog
				false // range fog
				);
		}


		/*
			Update gui stuff (0ms)
		*/

		//TimeTaker guiupdatetimer("Gui updating");
		
		{
			wchar_t temptext[150];

			static float drawtime_avg = 0;
			drawtime_avg = drawtime_avg * 0.98 + (float)drawtime*0.02;
			static float beginscenetime_avg = 0;
			beginscenetime_avg = beginscenetime_avg * 0.98 + (float)beginscenetime*0.02;
			static float scenetime_avg = 0;
			scenetime_avg = scenetime_avg * 0.98 + (float)scenetime*0.02;
			static float endscenetime_avg = 0;
			endscenetime_avg = endscenetime_avg * 0.98 + (float)endscenetime*0.02;
			
			swprintf(temptext, 150, L"Minetest-c55 ("
					L"F: item=%i"
					L", R: range_all=%i"
					L")"
					L" drawtime=%.0f, beginscenetime=%.0f, scenetime=%.0f, endscenetime=%.0f",
					g_selected_item,
					g_viewing_range_all,
					drawtime_avg,
					beginscenetime_avg,
					scenetime_avg,
					endscenetime_avg
					);
			
			guitext->setText(temptext);
		}
		
		{
			wchar_t temptext[150];
			swprintf(temptext, 150,
					L"(% .1f, % .1f, % .1f)"
					L" (% .3f < btime_jitter < % .3f"
					L", dtime_jitter = % .1f %%)",
					player_position.X/BS,
					player_position.Y/BS,
					player_position.Z/BS,
					busytime_jitter1_min_sample,
					busytime_jitter1_max_sample,
					dtime_jitter1_max_fraction * 100.0
					);

			guitext2->setText(temptext);
		}
		
		{
			guitext_info->setText(infotext.c_str());
		}
		
		/*
			Get chat messages from client
		*/
		{
			// Get messages
			std::wstring message;
			while(client.getChatMessage(message))
			{
				chat_lines.push_back(message);
				if(chat_lines.size() > 5)
				{
					core::list<std::wstring>::Iterator
							i = chat_lines.begin();
					chat_lines.erase(i);
				}
			}
			// Append them to form the whole static text and throw
			// it to the gui element
			std::wstring whole;
			for(core::list<std::wstring>::Iterator
					i = chat_lines.begin();
					i != chat_lines.end(); i++)
			{
				whole += (*i) + L'\n';
			}
			chat_guitext->setText(whole.c_str());
			// Update gui element size and position
			core::rect<s32> rect(
					10,
					screensize.Y - 10 - text_height*chat_lines.size(),
					screensize.X - 10,
					screensize.Y - 10
			);
			chat_guitext->setRelativePosition(rect);
		}

		/*
			Inventory
		*/
		
		static u16 old_selected_item = 65535;
		if(client.getLocalInventoryUpdated()
				|| g_selected_item != old_selected_item)
		{
			old_selected_item = g_selected_item;
			//std::cout<<"Updating local inventory"<<std::endl;
			client.getLocalInventory(local_inventory);
			quick_inventory->setSelection(g_selected_item);
			quick_inventory->update();
		}
		
		/*
			Send actions returned by the inventory menu
		*/
		while(inventory_action_queue.size() != 0)
		{
			InventoryAction *a = inventory_action_queue.pop_front();

			client.sendInventoryAction(a);
			// Eat it
			delete a;
		}

		/*
			Drawing begins
		*/

		TimeTaker drawtimer("Drawing");

		
		{
			TimeTaker timer("beginScene");
			driver->beginScene(true, true, bgcolor);
			//driver->beginScene(false, true, bgcolor);
			beginscenetime = timer.stop(true);
		}

		//timer3.stop();
		
		//std::cout<<DTIME<<"smgr->drawAll()"<<std::endl;
		
		{
			TimeTaker timer("smgr");
			smgr->drawAll();
			scenetime = timer.stop(true);
		}
		
		{
		//TimeTaker timer9("auxiliary drawings");
		// 0ms
		
		//timer9.stop();
		//TimeTaker //timer10("//timer10");
		
		video::SMaterial m;
		m.Thickness = 10;
		m.Lighting = false;
		driver->setMaterial(m);

		driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);

		for(core::list< core::aabbox3d<f32> >::Iterator i=hilightboxes.begin();
				i != hilightboxes.end(); i++)
		{
			/*std::cout<<"hilightbox min="
					<<"("<<i->MinEdge.X<<","<<i->MinEdge.Y<<","<<i->MinEdge.Z<<")"
					<<" max="
					<<"("<<i->MaxEdge.X<<","<<i->MaxEdge.Y<<","<<i->MaxEdge.Z<<")"
					<<std::endl;*/
			driver->draw3DBox(*i, video::SColor(255,0,0,0));
		}

		/*
			Draw crosshair
		*/
		driver->draw2DLine(displaycenter - core::vector2d<s32>(10,0),
				displaycenter + core::vector2d<s32>(10,0),
				video::SColor(255,255,255,255));
		driver->draw2DLine(displaycenter - core::vector2d<s32>(0,10),
				displaycenter + core::vector2d<s32>(0,10),
				video::SColor(255,255,255,255));

		}

		//timer10.stop();
		//TimeTaker //timer11("//timer11");

		/*
			Draw gui
		*/
		// 0-1ms
		guienv->drawAll();
		
		// End drawing
		{
			TimeTaker timer("endScene");
			driver->endScene();
			endscenetime = timer.stop(true);
		}

		drawtime = drawtimer.stop(true);

		/*
			Drawing ends
		*/
		
		static s16 lastFPS = 0;
		//u16 fps = driver->getFPS();
		u16 fps = (1.0/dtime_avg1);

		if (lastFPS != fps)
		{
			core::stringw str = L"Minetest [";
			str += driver->getName();
			str += "] FPS:";
			str += fps;

			device->setWindowCaption(str.c_str());
			lastFPS = fps;
		}
		
		/*}
		else
			device->yield();*/
	}

	delete quick_inventory;

	} // client is deleted at this point
	
	delete g_input;

	/*
		In the end, delete the Irrlicht device.
	*/
	device->drop();
	
	/*
		Update configuration file
	*/
	/*if(configpath != "")
	{
		g_settings.updateConfigFile(configpath.c_str());
	}*/

	} //try
	catch(con::PeerNotFoundException &e)
	{
		dstream<<DTIME<<"Connection timed out."<<std::endl;
	}
#if CATCH_UNHANDLED_EXCEPTIONS
	/*
		This is what has to be done in every thread to get suitable debug info
	*/
	catch(std::exception &e)
	{
		dstream<<std::endl<<DTIME<<"An unhandled exception occurred: "
				<<e.what()<<std::endl;
		assert(0);
	}
#endif

	debugstreams_deinit();
	
	return 0;
}

//END
