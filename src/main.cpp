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

SUGGESTION: add a second lighting value to the MS nibble of param of
	air to tell how bright the air node is when there is no sunlight.
	When day changes to night, these two values can be interpolated.

TODO: Fix address to be ipv6 compatible

TODO: ESC Pause mode in which the cursor is not kept at the center of window.
TODO: Stop player if focus of window is taken away (go to pause mode)
TODO: Optimize and fix makeFastFace or whatever it's called
      - Face calculation is the source of CPU usage on the client
SUGGESTION: The client will calculate and send lighting changes and
  the server will randomly check some of them and kick the client out
  if it fails to calculate them right.
  - Actually, it could just start ignoring them and calculate them
    itself.
SUGGESTION: Combine MapBlock's face caches to so big pieces that VBO
            gets used
            - That is >500 vertices

TODO: Better dungeons
TODO: There should be very slight natural caves also, starting from
      only a straightened-up cliff

TODO: Changing of block with mouse wheel or something
TODO: Menus

TODO: Mobs
      - Server:
        - One single map container with ids as keys
      - Client:
	    - ?
TODO: - Keep track of the place of the mob in the last few hundreth's
        of a second - then, if a player hits it, take the value that is
		avg_rtt/2 before the moment the packet is received.
TODO: - Scripting

SUGGESTION: Modify client to calculate single changes asynchronously

TODO: Moving players more smoothly. Calculate moving animation
      in a way that doesn't make the player jump to the right place
	  immediately when the server sends a new position

TODO: There are some lighting-related todos and fixmes in
      ServerMap::emergeBlock

FIXME: When a new sector is generated, it may change the ground level
       of it's and it's neighbors border that two blocks that are
	   above and below each other and that are generated before and
	   after the sector heightmap generation (order doesn't matter),
	   can have a small gap between each other at the border.
SUGGESTION: Use same technique for sector heightmaps as what we're
            using for UnlimitedHeightmap? (getting all neighbors
			when generating)

TODO: Proper handling of spawning place (try to find something that
      is not in the middle of an ocean (some land to stand on at
	  least) and save it in map config.
SUGG: Set server to automatically find a good spawning place in some
      place where there is water and land.
	  - Map to have a getWalkableNear(p)
	  - Is this a good idea? It's part of the game to find a good place.

TODO: Transfer more blocks in a single packet
SUGG: A blockdata combiner class, to which blocks are added and at
      destruction it sends all the stuff in as few packets as possible.

SUGG: If player is on ground, mainly fetch ground-level blocks
SUGG: Fetch stuff mainly from the viewing direction

SUGG: Expose Connection's seqnums and ACKs to server and client.
      - This enables saving many packets and making a faster connection
	  - This also enables server to check if client has received the
	    most recent block sent, for example.
TODO: Add a sane bandwidth throttling system to Connection

SUGG: More fine-grained control of client's dumping of blocks from
      memory
	  - ...What does this mean in the first place?

TODO: Make the amount of blocks sending to client and the total
	  amount of blocks dynamically limited. Transferring blocks is the
	  main network eater of this system, so it is the one that has
	  to be throttled so that RTTs stay low.

TODO: Server to load starting inventory from disk

TODO: PLayers to only be hidden when the client quits.
TODO: - Players to be saved on disk, with inventory
TODO: Players to be saved as text in map/players/<name>

SUGG: A map editing mode (similar to dedicated server mode)

TODO: Make fetching sector's blocks more efficient when rendering
      sectors that have very large amounts of blocks (on client)

TODO: Make the video backend selectable

Block object server side:
      - A "near blocks" buffer, in which some nearby blocks are stored.
	  - For all blocks in the buffer, objects are stepped(). This
	    means they are active.
	  - TODO: A global active buffer is needed for the server
      - TODO: All blocks going in and out of the buffer are recorded.
	    - TODO: For outgoing blocks, a timestamp is written.
	    - TODO: For incoming blocks, the time difference is calculated and
	      objects are stepped according to it.
TODO: A timestamp to blocks

SUGG: Add a time value to the param of footstepped grass and check it
      against a global timer when a block is accessed, to make old
	  steps fade away.

TODO: Add config parameters for server's sending and generating distance

TODO: Copy the text of the last picked sign to inventory in creative
      mode

TODO: Untie client network operations from framerate
      - Needs some input queues or something

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

TODO: Get rid of GotSplitPacketException

SUGG: Implement lighting using VoxelManipulator
      - Would it be significantly faster?

TODO: Check what goes wrong with caching map to disk (Kray)

TODO: Remove LazyMeshUpdater. It is not used as supposed.

FIXME: Rats somehow go underground sometimes (you can see it in water)
       - Does their position get saved to a border value or something?

SUGG: MovingObject::move and Player::move are basically the same.
      combine them.

TODO: Transfer sign texts as metadata of block and not as data of
      object

Doing now:
======================================================================

Water dynamics pseudo-code (block = MapNode):
SUGG: Create separate flag table in VoxelManipulator to allow fast
clearing of "modified" flags

neighborCausedPressure(pos):
	pressure = 0
	dirs = {down, left, right, back, front, up}
	for d in dirs:
		pos2 = pos + d
		p = block_at(pos2).pressure
		if d.Y == 1 and p > min:
			p -= 1
		if d.Y == -1 and p < max:
			p += 1
		if p > pressure:
			pressure = p
	return pressure

# This should somehow update all changed pressure values
# in an unknown body of water
updateWaterPressure(pos):
	TODO

FIXME: This goes in an indefinite loop when there is an underwater
chamber like this:

#111######
#222##22##
#33333333x<- block removed from here
##########

#111######
#222##22##
#3333333x1
##########

#111######
#222##22##
#333333x11
##########

#111######
#222##2x##
#333333333
##########

#111######
#222##x2##
#333333333
##########

Now, consider moving to the last block not allowed.

Consider it a 3D case with a depth of 2. We're now at this situation.
Note the additional blocking ## in the second depth plane.

z=1         z=2
#111######  #111######
#222##x2##  #222##22##
#333333333  #33333##33
##########  ##########

#111######  #111######
#222##22##  #222##x2##
#333333333  #33333##33
##########  ##########

#111######  #111######
#222##22##  #222##2x##
#333333333  #33333##33  
##########  ##########

Now there is nowhere to go, without going to an already visited block,
but the pressure calculated in here from neighboring blocks is >= 2,
so it is not the final ending.

We will back up to a state where there is somewhere to go to.
It is this state:

#111######  #111######
#222##22##  #222##22##
#333333x33  #33333##33
##########  ##########

Then just go on, avoiding already visited blocks:

#111######  #111######
#222##22##  #222##22##
#33333x333  #33333##33
##########  ##########

#111######  #111######
#222##22##  #222##22##
#3333x3333  #33333##33
##########  ##########

#111######  #111######
#222##22##  #222##22##
#333x33333  #33333##33
##########  ##########

#111######  #111######
#222##22##  #222##22##
#33x333333  #33333##33
##########  ##########

#111######  #111######
#22x##22##  #222##22##
#333333333  #33333##33
##########  ##########

#11x######  #111######
#222##22##  #222##22##
#333333333  #33333##33
##########  ##########

"Blob". the air bubble finally got out of the water.
Then return recursively to a state where there is air next to water,
clear the visit flags and feed the neighbor of the water recursively
to the algorithm.

#11 ######  #111######
#222##22##  #222##22##
#333333333x #33333##33
##########  ##########

#11 ######  #111######
#222##22##  #222##22##
#33333333x3 #33333##33
##########  ##########

...and so on.


# removed_pos: a position that has been changed from something to air
flowWater(removed_pos):
	dirs = {top, left, right, back, front, bottom}
	selected_dir = None
	for d in dirs:
		b2 = removed_pos + d

		# Ignore positions that don't have water
		if block_at(b2) != water:
			continue

		# Ignore positions that have already been checked
		if block_at(b2).checked:
			continue

		# If block is at top, select it always.
		if d.Y == 1:
			selected_dir = d
			break

		# If block is at bottom, select it if it has enough pressure.
		# >= 3 needed for stability (and sanity)
		if d.Y == -1:
			if block_at(b2).pressure >= 3:
				selected_dir = d
				break
			continue
		
		# Else block is at some side. select it if it has enough pressure.
		if block_at(b2).pressure >= 2:
			selected_dir = d
			break
	
	# If there is nothing to do anymore, return.
	if selected_dir == None
		return
	
	b2 = removed_pos + selected_dir
	
	# Move block
	set_block(removed_pos, block_at(b2))
	set_block(b2, air_block)
	
	# Update pressure
	updateWaterPressure(removed_pos)
	
	# Flow water to the newly created empty position
	flowWater(b2)

	# Check empty positions around and try flowing water to them
	for d in dirs:
		b3 = removed_pos + d
		# Ignore positions that are not air
		if block_at(b3) is not air:
			continue
		flowWater(b3)


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
// This would get rid of the console window
//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#define sleep_ms(x) Sleep(x)
#else
	#include <unistd.h>
	#define sleep_ms(x) usleep(x*1000)
#endif

#include <iostream>
#include <fstream>
#include <time.h>
#include <jmutexautolock.h>
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
#include <locale.h>

IrrlichtDevice *g_device = NULL;

const char *g_material_filenames[MATERIALS_COUNT] =
{
	"../data/stone.png",
	"../data/grass.png",
	"../data/water.png",
	"../data/light.png",
	"../data/tree.png",
	"../data/leaves.png",
	"../data/grass_footsteps.png",
	"../data/mese.png",
	"../data/mud.png"
};

video::SMaterial g_materials[MATERIALS_COUNT];
//video::SMaterial g_mesh_materials[3];

// All range-related stuff below is locked behind this
JMutex g_range_mutex;

// Blocks are viewed in this range from the player
s16 g_viewing_range_nodes = 60;

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

// Sets default settings
void set_default_settings()
{
	g_settings.set("dedicated_server", "");

	// Client stuff
	g_settings.set("wanted_fps", "30");
	g_settings.set("fps_max", "60");
	g_settings.set("viewing_range_nodes_max", "300");
	g_settings.set("viewing_range_nodes_min", "20");
	g_settings.set("screenW", "");
	g_settings.set("screenH", "");
	g_settings.set("host_game", "");
	g_settings.set("port", "");
	g_settings.set("address", "");
	g_settings.set("name", "");
	g_settings.set("random_input", "false");
	g_settings.set("client_delete_unused_sectors_timeout", "1200");

	// Server stuff
	g_settings.set("creative_mode", "false");
	g_settings.set("heightmap_blocksize", "128");
	g_settings.set("height_randmax", "constant 70.0");
	g_settings.set("height_randfactor", "constant 0.6");
	g_settings.set("height_base", "linear 0 35 0");
	g_settings.set("plants_amount", "1.0");
	g_settings.set("ravines_amount", "1.0");
	g_settings.set("objectdata_interval", "0.2");
	g_settings.set("active_object_range", "2");
	g_settings.set("max_simultaneous_block_sends_per_client", "1");
	g_settings.set("max_simultaneous_block_sends_server_total", "4");
}

/*
	Random stuff
*/

//u16 g_selected_material = 0;
u16 g_selected_item = 0;

bool g_esc_pressed = false;

std::wstring g_text_buffer;
bool g_text_buffer_accepted = false;

// When true, the mouse and keyboard are grabbed
bool g_game_focused = true;

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
	Timestamp stuff
*/

JMutex g_timestamp_mutex;
//std::string g_timestamp;

std::string getTimestamp()
{
	if(g_timestamp_mutex.IsInitialized()==false)
		return "";
	JMutexAutoLock lock(g_timestamp_mutex);
	//return g_timestamp;
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char cs[20];
	strftime(cs, 20, "%H:%M:%S", tm);
	return cs;
}

class MyEventReceiver : public IEventReceiver
{
public:
	// This is the one method that we have to implement
	virtual bool OnEvent(const SEvent& event)
	{
		// Remember whether each key is down or up
		if(event.EventType == irr::EET_KEY_INPUT_EVENT)
		{
			keyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;

			if(event.KeyInput.PressedDown)
			{
				//dstream<<"Pressed key: "<<(char)event.KeyInput.Key<<std::endl;
				if(g_game_focused == false)
				{
					s16 key = event.KeyInput.Key;
					if(key == irr::KEY_RETURN || key == irr::KEY_ESCAPE)
					{
						g_text_buffer_accepted = true;
					}
					else if(key == irr::KEY_BACK)
					{
						if(g_text_buffer.size() > 0)
							g_text_buffer = g_text_buffer.substr
									(0, g_text_buffer.size()-1);
					}
					else
					{
						wchar_t wc = event.KeyInput.Char;
						if(wc != 0)
							g_text_buffer += wc;
					}
				}
				
				if(event.KeyInput.Key == irr::KEY_ESCAPE)
				{
					if(g_game_focused == true)
					{
						dstream<<DTIME<<"ESC pressed"<<std::endl;
						g_esc_pressed = true;
					}
				}

				// Material selection
				if(event.KeyInput.Key == irr::KEY_KEY_F)
				{
					if(g_game_focused == true)
					{
						if(g_selected_item < PLAYER_INVENTORY_SIZE-1)
							g_selected_item++;
						else
							g_selected_item = 0;
						dstream<<DTIME<<"Selected item: "
								<<g_selected_item<<std::endl;
					}
				}

				// Viewing range selection
				if(event.KeyInput.Key == irr::KEY_KEY_R
						&& g_game_focused)
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
				if(event.KeyInput.Key == irr::KEY_KEY_P
						&& g_game_focused)
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
			if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
			{
				leftclicked = true;
			}
			if(event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN)
			{
				rightclicked = true;
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

		return false;
	}

	// This is used to check whether a key is being held down
	virtual bool IsKeyDown(EKEY_CODE keyCode) const
	{
		return keyIsDown[keyCode];
	}

	MyEventReceiver()
	{
		for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
				keyIsDown[i] = false;
		leftclicked = false;
		rightclicked = false;
	}

	bool leftclicked;
	bool rightclicked;
private:
	// We use this array to store the current state of each key
	bool keyIsDown[KEY_KEY_CODES_COUNT];
	//s32 mouseX;
	//s32 mouseY;
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
	virtual bool getLeftClicked() = 0;
	virtual bool getRightClicked() = 0;
	virtual void resetLeftClicked() = 0;
	virtual void resetRightClicked() = 0;
	
	virtual void step(float dtime) {};

	virtual void clear() {};
};

InputHandler *g_input = NULL;

void focusGame()
{
	g_input->clear();
	g_game_focused = true;
}

void unFocusGame()
{
	g_game_focused = false;
}

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

	virtual bool getLeftClicked()
	{
		if(g_game_focused == false)
			return false;
		return m_receiver->leftclicked;
	}
	virtual bool getRightClicked()
	{
		if(g_game_focused == false)
			return false;
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

	virtual void step(float dtime)
	{
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if(counter1 < 0.0)
			{
				counter1 = 0.1*Rand(1,10);
				/*if(g_selected_material < USEFUL_MATERIAL_COUNT-1)
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
	frametime_avg = frametime_avg * 0.9 + frametime * 0.1;

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

	static bool fraction_is_good = false;
	
	float fraction_good_threshold = 0.1;
	float fraction_bad_threshold = 0.25;
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

		for(s32 i=0; i<m_itemcount; i++)
		{
			s32 j = i + start;

			if(j > (s32)m_inventory->getSize() - 1)
				j -= m_inventory->getSize();
			if(j < 0)
				j += m_inventory->getSize();
			
			InventoryItem *item = m_inventory->getItem(j);
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
	
	// Initialize timestamp mutex
	g_timestamp_mutex.Init();

	/*
		Run unit tests
	*/
	if(ENABLE_TESTS)
	{
		run_tests();
	}
	
	/*
		Initialization
	*/

	// Read config file
	
	if(argc >= 2)
	{
		g_settings.readConfigFile(argv[1]);
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
				break;
		}
	}

	// Initialize random seed
	srand(time(0));

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
	<<std::endl
	<<"Now with more waterish water!"
	<<std::endl;

	std::cout<<std::endl;
	char templine[100];
	
	// Dedicated?
	bool dedicated = g_settings.getBoolAsk
			("dedicated_server", "Dedicated server?", false);
	std::cout<<"dedicated = "<<dedicated<<std::endl;
	
	// Port?
	u16 port = g_settings.getU16Ask("port", "Port", 30000);
	std::cout<<"-> "<<port<<std::endl;
	
	if(dedicated)
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

	std::cout<<"Address to connect to [empty = host a game]: ";
	if(g_settings.get("address") != "" && is_yes(g_settings.get("host_game")) == false)
	{
		std::cout<<g_settings.get("address")<<std::endl;
		snprintf(connect_name, 100, "%s", g_settings.get("address").c_str());
	}
	else
	{
		std::cin.getline(connect_name, 100);
	}
	
	if(connect_name[0] == 0){
		snprintf(connect_name, 100, "127.0.0.1");
		hosting = true;
	}
	
	if(hosting)
		std::cout<<"-> hosting"<<std::endl;
	else
		std::cout<<"-> "<<connect_name<<std::endl;
	
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

	u16 screenW;
	u16 screenH;
	bool fullscreen = false;
	
	if(g_settings.get("screenW") != "" && g_settings.get("screenH") != "")
	{
		screenW = atoi(g_settings.get("screenW").c_str());
		screenH = atoi(g_settings.get("screenH").c_str());
	}
	else
	{
		u16 resolutions[][3] = {
			//W, H, fullscreen
			{640,480, 0},
			{800,600, 0},
			{1024,768, 0},
			{1280,1024, 0},
			/*{640,480, 1},
			{800,600, 1},
			{1024,768, 1},
			{1280,1024, 1},*/
		};

		u16 res_count = sizeof(resolutions)/sizeof(resolutions[0]);
		
		for(u16 i=0; i<res_count; i++)
		{
			std::cout<<(i+1)<<": "<<resolutions[i][0]<<"x"
					<<resolutions[i][1];
			if(resolutions[i][2])
				std::cout<<" fullscreen"<<std::endl;
			else
				std::cout<<" windowed"<<std::endl;
		}
		std::cout<<"Select a window resolution number [empty = 2]: ";
		std::cin.getline(templine, 100);

		u16 r0;
		if(templine[0] == 0)
			r0 = 2;
		else
			r0 = atoi(templine);

		if(r0 > res_count || r0 == 0)
			r0 = 2;
		
		{
			u16 i = r0-1;
			std::cout<<"-> ";
			std::cout<<(i+1)<<": "<<resolutions[i][0]<<"x"
					<<resolutions[i][1];
			if(resolutions[i][2])
				std::cout<<" fullscreen"<<std::endl;
			else
				std::cout<<" windowed"<<std::endl;
		}

		screenW = resolutions[r0-1][0];
		screenH = resolutions[r0-1][1];
		fullscreen = resolutions[r0-1][2];
	}

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
	// With vsync
	/*device = createDevice(driverType,
			core::dimension2d<u32>(screenW, screenH),
			16, fullscreen, false, true, &receiver);*/

	if (device == 0)
		return 1; // could not create selected driver.

	g_device = device;
	
	device->setResizable(true);

	if(g_settings.getBool("random_input"))
		g_input = new RandomInputHandler();
	else
		g_input = new RealInputHandler(device, &receiver);
	
	/*
		Continue initialization
	*/

	video::IVideoDriver* driver = device->getVideoDriver();
	// These make the textures not to show at all
	//driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT);
	//driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED );

	//driver->setMinHardwareBufferVertexCount(1);

	scene::ISceneManager* smgr = device->getSceneManager();

	gui::IGUIEnvironment* guienv = device->getGUIEnvironment();
	gui::IGUISkin* skin = guienv->getSkin();
	gui::IGUIFont* font = guienv->getFont("../data/fontlucida.png");
	if(font)
		skin->setFont(font);
	//skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255,0,0,0));
	skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255,255,255,255));
	//skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(0,0,0,0));
	//skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(0,0,0,0));
	skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(255,0,0,0));
	skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(255,0,0,0));
	
	const wchar_t *text = L"Loading and connecting...";
	core::vector2d<s32> center(screenW/2, screenH/2);
	core::dimension2d<u32> textd = font->getDimension(text);
	std::cout<<DTIME<<"Text w="<<textd.Width<<" h="<<textd.Height<<std::endl;
	// Have to add a bit to disable the text from word wrapping
	//core::vector2d<s32> textsize(textd.Width+4, textd.Height);
	core::vector2d<s32> textsize(300, textd.Height);
	core::rect<s32> textrect(center - textsize/2, center + textsize/2);

	gui::IGUIStaticText *gui_loadingtext = guienv->addStaticText(
			text, textrect, false, false);
	gui_loadingtext->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);

	driver->beginScene(true, true, video::SColor(255,0,0,0));
	guienv->drawAll();
	driver->endScene();

	/*
		Initialize material array
	*/

	//video::SMaterial g_materials[MATERIALS_COUNT];
	for(u16 i=0; i<MATERIALS_COUNT; i++)
	{
		g_materials[i].Lighting = false;
		g_materials[i].BackfaceCulling = false;

		const char *filename = g_material_filenames[i];
		if(filename != NULL){
			video::ITexture *t = driver->getTexture(filename);
			if(t == NULL){
				std::cout<<DTIME<<"Texture could not be loaded: \""
						<<filename<<"\""<<std::endl;
				return 1;
			}
			g_materials[i].setTexture(0, driver->getTexture(filename));
		}
		//g_materials[i].setFlag(video::EMF_TEXTURE_WRAP, video::ETC_REPEAT);
		g_materials[i].setFlag(video::EMF_BILINEAR_FILTER, false);
		//g_materials[i].setFlag(video::EMF_ANISOTROPIC_FILTER, false);
		//g_materials[i].setFlag(video::EMF_FOG_ENABLE, true);
		if(i == MATERIAL_WATER)
		{
			g_materials[i].MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
			//g_materials[i].MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
		}
	}

	/*g_mesh_materials[0].setTexture(0, driver->getTexture("../data/water.png"));
	g_mesh_materials[1].setTexture(0, driver->getTexture("../data/grass.png"));
	g_mesh_materials[2].setTexture(0, driver->getTexture("../data/stone.png"));
	for(u32 i=0; i<3; i++)
	{
		g_mesh_materials[i].Lighting = false;
		g_mesh_materials[i].BackfaceCulling = false;
		g_mesh_materials[i].setFlag(video::EMF_BILINEAR_FILTER, false);
		g_mesh_materials[i].setFlag(video::EMF_FOG_ENABLE, true);
	}*/

	// Make a scope here for the client so that it gets removed
	// before the irrlicht device
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

	// TODO: Get rid of the g_materials parameter or it's globalness
	Client client(device, g_materials,
			g_settings.getFloat("client_delete_unused_sectors_timeout"),
			playername);
	
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

	/*//f32 range = BS*HEIGHTMAP_RANGE_NODES*0.9;
	f32 range = BS*HEIGHTMAP_RANGE_NODES*0.9;
	
	camera->setFarValue(range);
	
	driver->setFog(
		skycolor,
		video::EFT_FOG_LINEAR,
		range*0.8,
		range,
		0.01,
		false,
		false
		);*/
	
	f32 camera_yaw = 0; // "right/left"
	f32 camera_pitch = 0; // "up/down"
	
	gui_loadingtext->remove();

	/*
		Add some gui stuff
	*/
	
	// First line of debug text
	gui::IGUIStaticText *guitext = guienv->addStaticText(
			L"Minetest-c55",
			core::rect<s32>(5, 5, 5+600, 5+textsize.Y),
			false, false);
	// Second line of debug text
	gui::IGUIStaticText *guitext2 = guienv->addStaticText(
			L"",
			core::rect<s32>(5, 5+(textsize.Y+5)*1, 5+600, (5+textsize.Y)*2),
			false, false);
	
	// At the middle of the screen
	// Object infos are shown in this
	gui::IGUIStaticText *guitext_info = guienv->addStaticText(
			L"test",
			core::rect<s32>(100, 70, 100+400, 70+(textsize.Y+5)),
			false, false);
	
	// This is a copy of the inventory that the client's environment has
	Inventory local_inventory(PLAYER_INVENTORY_SIZE);
	
	GUIQuickInventory *quick_inventory = new GUIQuickInventory
			(guienv, NULL, v2s32(10, 70), 5, &local_inventory);
	
	/*
		Some statistics are collected in these
	*/
	u32 drawtime = 0;
	u32 scenetime = 0;
	u32 endscenetime = 0;

	/*
		Text input system
	*/
	
	struct TextDest
	{
		virtual void sendText(std::string text) = 0;
	};
	
	struct TextDestSign : public TextDest
	{
		TextDestSign(v3s16 blockpos, s16 id, Client *client)
		{
			m_blockpos = blockpos;
			m_id = id;
			m_client = client;
		}
		void sendText(std::string text)
		{
			dstream<<"Changing text of a sign object: "
					<<text<<std::endl;
			m_client->sendSignText(m_blockpos, m_id, text);
		}

		v3s16 m_blockpos;
		s16 m_id;
		Client *m_client;
	};

	TextDest *textbuf_dest = NULL;
	
	//gui::IGUIWindow* input_window = NULL;
	gui::IGUIStaticText* input_guitext = NULL;

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
		// Hilight boxes collected during the loop and displayed
		core::list< core::aabbox3d<f32> > hilightboxes;
		
		// Info text
		std::wstring infotext;

		//TimeTaker //timer1("//timer1", device);
		
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
		
		//updateViewingRange(dtime, &client);
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
			Special keys
		*/
		if(g_esc_pressed)
		{
			break;
		}

		/*
			Player speed control
		*/
		
		if(g_game_focused)
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
		else
		{
			// Set every key to inactive
			PlayerControl control;
			client.setPlayerControl(control);
		}

		//timer1.stop();
		/*
			Process environment
		*/
		
		{
			//TimeTaker timer("client.step(dtime)", device);
			client.step(dtime);
			//client.step(dtime_avg1);
		}

		if(server != NULL)
		{
			//TimeTaker timer("server->step(dtime)", device);
			server->step(dtime);
		}

		v3f player_position = client.getPlayerPosition();
		
		//TimeTaker //timer2("//timer2", device);

		/*
			Mouse and camera control
		*/
		
		if(device->isWindowActive() && g_game_focused)
		{
			device->getCursorControl()->setVisible(false);

			if(first_loop_after_window_activation){
				//std::cout<<"window active, first loop"<<std::endl;
				first_loop_after_window_activation = false;
			}
			else{
				s32 dx = g_input->getMousePos().X - 320;
				s32 dy = g_input->getMousePos().Y - 240;
				//std::cout<<"window active, pos difference "<<dx<<","<<dy<<std::endl;
				camera_yaw -= dx*0.2;
				camera_pitch += dy*0.2;
				if(camera_pitch < -89.5) camera_pitch = -89.5;
				if(camera_pitch > 89.5) camera_pitch = 89.5;
			}
			g_input->setMousePos(320, 240);
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
			//TimeTaker timer("client.updateCamera", device);
			client.updateCamera(camera_position, camera_direction);
		}
		
		//timer2.stop();
		//TimeTaker //timer3("//timer3", device);

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

					unFocusGame();

					input_guitext = guienv->addStaticText(L"",
							core::rect<s32>(150,100,350,120),
							true, // border?
							false, // wordwrap?
							NULL);

					input_guitext->setDrawBackground(true);

					g_text_buffer = L"";
					g_text_buffer_accepted = false;
					textbuf_dest = new TextDestSign(
							selected_object->getBlock()->getPos(),
							selected_object->getId(),
							&client);
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
		
		for(s16 y = ystart; y <= yend; y++){
		for(s16 z = zstart; z <= zend; z++){
		for(s16 x = xstart; x <= xend; x++)
		{
			try{
				if(material_pointable(client.getNode(v3s16(x,y,z)).d) == false)
					continue;
			}catch(InvalidPositionException &e){
				continue;
			}

			v3s16 np(x,y,z);
			v3f npf = intToFloat(np);
			
			f32 d = 0.01;
			
			v3s16 directions[6] = {
				v3s16(0,0,1), // back
				v3s16(0,1,0), // top
				v3s16(1,0,0), // right
				v3s16(0,0,-1),
				v3s16(0,-1,0),
				v3s16(-1,0,0),
			};

			for(u16 i=0; i<6; i++){
			//{u16 i=3;
				v3f dir_f = v3f(directions[i].X,
						directions[i].Y, directions[i].Z);
				v3f centerpoint = npf + dir_f * BS/2;
				f32 distance =
						(centerpoint - camera_position).getLength();
				
				if(distance < mindistance){
					//std::cout<<DTIME<<"for centerpoint=("<<centerpoint.X<<","<<centerpoint.Y<<","<<centerpoint.Z<<"): distance < mindistance"<<std::endl;
					//std::cout<<DTIME<<"npf=("<<npf.X<<","<<npf.Y<<","<<npf.Z<<")"<<std::endl;
					core::CMatrix4<f32> m;
					m.buildRotateFromTo(v3f(0,0,1), dir_f);

					// This is the back face
					v3f corners[2] = {
						v3f(BS/2, BS/2, BS/2),
						v3f(-BS/2, -BS/2, BS/2+d)
					};
					
					for(u16 j=0; j<2; j++){
						m.rotateVect(corners[j]);
						corners[j] += npf;
						//std::cout<<DTIME<<"box corners["<<j<<"]: ("<<corners[j].X<<","<<corners[j].Y<<","<<corners[j].Z<<")"<<std::endl;
					}

					//core::aabbox3d<f32> facebox(corners[0],corners[1]);
					core::aabbox3d<f32> facebox(corners[0]);
					facebox.addInternalPoint(corners[1]);

					if(facebox.intersectsWithLine(shootline)){
						nodefound = true;
						nodepos = np;
						neighbourpos = np + directions[i];
						mindistance = distance;
						nodefacebox = facebox;
					}
				}
			}
		}}}

		if(nodefound)
		{
			//std::cout<<DTIME<<"nodefound == true"<<std::endl;
			//std::cout<<DTIME<<"nodepos=("<<nodepos.X<<","<<nodepos.Y<<","<<nodepos.Z<<")"<<std::endl;
			//std::cout<<DTIME<<"neighbourpos=("<<neighbourpos.X<<","<<neighbourpos.Y<<","<<neighbourpos.Z<<")"<<std::endl;

			static v3s16 nodepos_old(-1,-1,-1);
			if(nodepos != nodepos_old){
				std::cout<<DTIME<<"Pointing at ("<<nodepos.X<<","
						<<nodepos.Y<<","<<nodepos.Z<<")"<<std::endl;
				nodepos_old = nodepos;

				/*wchar_t positiontext[20];
				swprintf(positiontext, 20, L"(%i,%i,%i)",
						nodepos.X, nodepos.Y, nodepos.Z);
				positiontextgui->setText(positiontext);*/
			}

			hilightboxes.push_back(nodefacebox);
			
			if(g_input->getLeftClicked())
			{
				//std::cout<<DTIME<<"Removing node"<<std::endl;
				//client.removeNode(nodepos);
				std::cout<<DTIME<<"Ground left-clicked"<<std::endl;
				client.clickGround(0, nodepos, neighbourpos, g_selected_item);
			}
			if(g_input->getRightClicked())
			{
				//std::cout<<DTIME<<"Placing node"<<std::endl;
				//client.addNodeFromInventory(neighbourpos, g_selected_item);
				std::cout<<DTIME<<"Ground right-clicked"<<std::endl;
				client.clickGround(1, nodepos, neighbourpos, g_selected_item);
			}
		}
		else{
			//std::cout<<DTIME<<"nodefound == false"<<std::endl;
			//positiontextgui->setText(L"");
		}

		} // selected_object == NULL
		
		g_input->resetLeftClicked();
		g_input->resetRightClicked();
		
		/*
			Calculate stuff for drawing
		*/

		v2u32 screensize = driver->getScreenSize();
		core::vector2d<s32> displaycenter(screensize.X/2,screensize.Y/2);

		camera->setAspectRatio((f32)screensize.X / (f32)screensize.Y);

		/*
			Update gui stuff (0ms)
		*/

		//TimeTaker guiupdatetimer("Gui updating", device);
		
		{
			wchar_t temptext[100];

			static float drawtime_avg = 0;
			drawtime_avg = drawtime_avg * 0.98 + (float)drawtime*0.02;
			static float scenetime_avg = 0;
			scenetime_avg = scenetime_avg * 0.98 + (float)scenetime*0.02;
			static float endscenetime_avg = 0;
			endscenetime_avg = endscenetime_avg * 0.98 + (float)endscenetime*0.02;
			
			swprintf(temptext, 100, L"Minetest-c55 ("
					L"F: item=%i"
					L", R: range_all=%i"
					L")"
					L" drawtime=%.0f, scenetime=%.0f, endscenetime=%.0f",
					g_selected_item,
					g_viewing_range_all,
					drawtime_avg,
					scenetime_avg,
					endscenetime_avg
					);
			
			guitext->setText(temptext);
		}
		
		{
			wchar_t temptext[100];
			/*swprintf(temptext, 100,
					L"("
					L"% .3f < btime_jitter < % .3f"
					L", dtime_jitter = % .1f %%"
					//L", ftime_ratio = % .3f"
					L")",
					busytime_jitter1_min_sample,
					busytime_jitter1_max_sample,
					dtime_jitter1_max_fraction * 100.0
					//g_freetime_ratio
					);*/
			swprintf(temptext, 100,
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
			/*wchar_t temptext[100];
			swprintf(temptext, 100,
					SWPRINTF_CHARSTRING,
					infotext.substr(0,99).c_str()
					);

			guitext_info->setText(temptext);*/

			guitext_info->setText(infotext.c_str());
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

		if(input_guitext != NULL)
		{
			/*wchar_t temptext[100];
			swprintf(temptext, 100,
					SWPRINTF_CHARSTRING,
					g_text_buffer.substr(0,99).c_str()
					);*/
			input_guitext->setText(g_text_buffer.c_str());
		}

		/*
			Text input stuff
		*/
		if(input_guitext != NULL && g_text_buffer_accepted)
		{
			input_guitext->remove();
			input_guitext = NULL;
			
			if(textbuf_dest != NULL)
			{
				std::string text = wide_to_narrow(g_text_buffer);
				dstream<<"Sending text: "<<text<<std::endl;
				textbuf_dest->sendText(text);
				delete textbuf_dest;
				textbuf_dest = NULL;
			}

			focusGame();
		}

		//guiupdatetimer.stop();

		/*
			Drawing begins
		*/

		TimeTaker drawtimer("Drawing", device);

		/*
			Background color is choosen based on whether the player is
			much beyond the initial ground level
		*/
		/*video::SColor bgcolor;
		v3s16 p0 = Map::floatToInt(player_position);
		// Does this make short random delays?
		// NOTE: no need for this, sky doesn't show underground with
		// enough range
		bool is_underground = client.isNodeUnderground(p0);
		//bool is_underground = false;
		if(is_underground == false)
			bgcolor = video::SColor(255,90,140,200);
		else
			bgcolor = video::SColor(255,0,0,0);*/
			
		//video::SColor bgcolor = video::SColor(255,90,140,200);
		video::SColor bgcolor = skycolor;
		
		// 0ms
		driver->beginScene(true, true, bgcolor);

		//timer3.stop();
		
		//std::cout<<DTIME<<"smgr->drawAll()"<<std::endl;
		
		{
		TimeTaker timer("smgr", device);
		smgr->drawAll();
		scenetime = timer.stop(true);
		}
		
		{
		//TimeTaker timer9("auxiliary drawings", device);
		// 0ms

		driver->draw2DLine(displaycenter - core::vector2d<s32>(10,0),
				displaycenter + core::vector2d<s32>(10,0),
				video::SColor(255,255,255,255));
		driver->draw2DLine(displaycenter - core::vector2d<s32>(0,10),
				displaycenter + core::vector2d<s32>(0,10),
				video::SColor(255,255,255,255));

		//timer9.stop();
		//TimeTaker //timer10("//timer10", device);
		
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

		}

		//timer10.stop();
		//TimeTaker //timer11("//timer11", device);

		/*
			Draw gui
		*/
		// 0-1ms
		guienv->drawAll();
		
		// End drawing
		{
		TimeTaker timer("endScene", device);
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

	} // client is deleted at this point
	
	delete g_input;

	/*
		In the end, delete the Irrlicht device.
	*/
	device->drop();

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
