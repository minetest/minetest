/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "common_irrlicht.h"
#include "game.h"
#include "client.h"
#include "server.h"
#include "guiPauseMenu.h"
#include "guiPasswordChange.h"
#include "guiInventoryMenu.h"
#include "guiTextInputMenu.h"
#include "materials.h"
#include "config.h"
#include "clouds.h"
#include "keycode.h"
#include "farmesh.h"
#include "mapblock.h"

/*
	TODO: Move content-aware stuff to separate file by adding properties
	      and virtual interfaces
*/
#include "content_mapnode.h"
#include "content_nodemeta.h"

/*
	Setting this to 1 enables a special camera mode that forces
	the renderers to think that the camera statically points from
	the starting place to a static direction.

	This allows one to move around with the player and see what
	is actually drawn behind solid things and behind the player.
*/
#define FIELD_OF_VIEW_TEST 0


MapDrawControl draw_control;

// Chat data
struct ChatLine
{
	ChatLine():
		age(0.0)
	{
	}
	ChatLine(const std::wstring &a_text):
		age(0.0),
		text(a_text)
	{
	}
	float age;
	std::wstring text;
};

/*
	Inventory stuff
*/

// Inventory actions from the menu are buffered here before sending
Queue<InventoryAction*> inventory_action_queue;
// This is a copy of the inventory that the client's environment has
Inventory local_inventory;

u16 g_selected_item = 0;

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
		// Discard empty line
		if(text == L"")
			return;
		
		// Parse command (server command starts with "/#")
		if(text[0] == L'/' && text[1] != L'#')
		{
			std::wstring reply = L"Local: ";

			reply += L"Local commands not yet supported. "
					L"Server prefix is \"/#\".";
			
			m_client->addChatMessage(reply);
			return;
		}

		// Send to others
		m_client->sendChatMessage(text);
		// Show locally
		m_client->addChatMessage(text);
	}

	Client *m_client;
};

struct TextDestSignNode : public TextDest
{
	TextDestSignNode(v3s16 p, Client *client)
	{
		m_p = p;
		m_client = client;
	}
	void gotText(std::wstring text)
	{
		std::string ntext = wide_to_narrow(text);
		dstream<<"Changing text of a sign node: "
				<<ntext<<std::endl;
		m_client->sendSignNodeText(m_p, ntext);
	}

	v3s16 m_p;
	Client *m_client;
};

/*
	Render distance feedback loop
*/
void updateViewingRange(f32 frametime_in, Client *client)
{
	if(draw_control.range_all == true)
		return;
	
	static f32 added_frametime = 0;
	static s16 added_frames = 0;

	added_frametime += frametime_in;
	added_frames += 1;

	// Actually this counter kind of sucks because frametime is busytime
	static f32 counter = 0;
	counter -= frametime_in;
	if(counter > 0)
		return;
	//counter = 0.1;
	counter = 0.2;

	/*dstream<<__FUNCTION_NAME
			<<": Collected "<<added_frames<<" frames, total of "
			<<added_frametime<<"s."<<std::endl;*/
	
	/*dstream<<"draw_control.blocks_drawn="
			<<draw_control.blocks_drawn
			<<", draw_control.blocks_would_have_drawn="
			<<draw_control.blocks_would_have_drawn
			<<std::endl;*/
	
	float range_min = g_settings.getS16("viewing_range_nodes_min");
	float range_max = g_settings.getS16("viewing_range_nodes_max");
	
	// Limit minimum to keep the feedback loop stable
	if(range_min < 5)
		range_min = 5;
	
	draw_control.wanted_min_range = range_min;
	//draw_control.wanted_max_blocks = (1.5*draw_control.blocks_drawn)+1;
	draw_control.wanted_max_blocks = (1.5*draw_control.blocks_would_have_drawn)+1;
	if(draw_control.wanted_max_blocks < 10)
		draw_control.wanted_max_blocks = 10;
	
	float block_draw_ratio = 1.0;
	if(draw_control.blocks_would_have_drawn != 0)
	{
		block_draw_ratio = (float)draw_control.blocks_drawn
			/ (float)draw_control.blocks_would_have_drawn;
	}

	// Calculate the average frametime in the case that all wanted
	// blocks had been drawn
	f32 frametime = added_frametime / added_frames / block_draw_ratio;
	
	added_frametime = 0.0;
	added_frames = 0;
	
	float wanted_fps = g_settings.getFloat("wanted_fps");
	float wanted_frametime = 1.0 / wanted_fps;
	
	f32 wanted_frametime_change = wanted_frametime - frametime;
	//dstream<<"wanted_frametime_change="<<wanted_frametime_change<<std::endl;
	
	// If needed frametime change is small, just return
	if(fabs(wanted_frametime_change) < wanted_frametime*0.4)
	{
		//dstream<<"ignoring small wanted_frametime_change"<<std::endl;
		return;
	}

	float range = draw_control.wanted_range;
	float new_range = range;

	static s16 range_old = 0;
	static f32 frametime_old = 0;
	
	float d_range = range - range_old;
	f32 d_frametime = frametime - frametime_old;
	// A sane default of 30ms per 50 nodes of range
	static f32 time_per_range = 30. / 50;
	if(d_range != 0)
	{
		time_per_range = d_frametime / d_range;
	}
	
	// The minimum allowed calculated frametime-range derivative:
	// Practically this sets the maximum speed of changing the range.
	// The lower this value, the higher the maximum changing speed.
	// A low value here results in wobbly range (0.001)
	// A high value here results in slow changing range (0.0025)
	// SUGG: This could be dynamically adjusted so that when
	//       the camera is turning, this is lower
	//float min_time_per_range = 0.0015;
	float min_time_per_range = 0.0010;
	//float min_time_per_range = 0.05 / range;
	if(time_per_range < min_time_per_range)
	{
		time_per_range = min_time_per_range;
		//dstream<<"time_per_range="<<time_per_range<<" (min)"<<std::endl;
	}
	else
	{
		//dstream<<"time_per_range="<<time_per_range<<std::endl;
	}

	f32 wanted_range_change = wanted_frametime_change / time_per_range;
	// Dampen the change a bit to kill oscillations
	//wanted_range_change *= 0.9;
	//wanted_range_change *= 0.75;
	wanted_range_change *= 0.5;
	//dstream<<"wanted_range_change="<<wanted_range_change<<std::endl;

	// If needed range change is very small, just return
	if(fabs(wanted_range_change) < 0.001)
	{
		//dstream<<"ignoring small wanted_range_change"<<std::endl;
		return;
	}

	new_range += wanted_range_change;
	
	//float new_range_unclamped = new_range;
	if(new_range < range_min)
		new_range = range_min;
	if(new_range > range_max)
		new_range = range_max;
	
	/*dstream<<"new_range="<<new_range_unclamped
			<<", clamped to "<<new_range<<std::endl;*/

	draw_control.wanted_range = new_range;

	range_old = new_range;
	frametime_old = frametime;
}

/*
	Hotbar draw routine
*/
void draw_hotbar(video::IVideoDriver *driver, gui::IGUIFont *font,
		v2s32 centerlowerpos, s32 imgsize, s32 itemcount,
		Inventory *inventory, s32 halfheartcount)
{
	InventoryList *mainlist = inventory->getList("main");
	if(mainlist == NULL)
	{
		dstream<<"WARNING: draw_hotbar(): mainlist == NULL"<<std::endl;
		return;
	}
	
	s32 padding = imgsize/12;
	//s32 height = imgsize + padding*2;
	s32 width = itemcount*(imgsize+padding*2);
	
	// Position of upper left corner of bar
	v2s32 pos = centerlowerpos - v2s32(width/2, imgsize+padding*2);
	
	// Draw background color
	/*core::rect<s32> barrect(0,0,width,height);
	barrect += pos;
	video::SColor bgcolor(255,128,128,128);
	driver->draw2DRectangle(bgcolor, barrect, NULL);*/

	core::rect<s32> imgrect(0,0,imgsize,imgsize);

	for(s32 i=0; i<itemcount; i++)
	{
		InventoryItem *item = mainlist->getItem(i);
		
		core::rect<s32> rect = imgrect + pos
				+ v2s32(padding+i*(imgsize+padding*2), padding);
		
		if(g_selected_item == i)
		{
			driver->draw2DRectangle(video::SColor(255,255,0,0),
					core::rect<s32>(rect.UpperLeftCorner - v2s32(1,1)*padding,
							rect.LowerRightCorner + v2s32(1,1)*padding),
					NULL);
		}
		else
		{
			video::SColor bgcolor2(128,0,0,0);
			driver->draw2DRectangle(bgcolor2, rect, NULL);
		}

		if(item != NULL)
		{
			drawInventoryItem(driver, font, item, rect, NULL);
		}
	}
	
	/*
		Draw hearts
	*/
	{
		video::ITexture *heart_texture =
				driver->getTexture(getTexturePath("heart.png").c_str());
		v2s32 p = pos + v2s32(0, -20);
		for(s32 i=0; i<halfheartcount/2; i++)
		{
			const video::SColor color(255,255,255,255);
			const video::SColor colors[] = {color,color,color,color};
			core::rect<s32> rect(0,0,16,16);
			rect += p;
			driver->draw2DImage(heart_texture, rect,
				core::rect<s32>(core::position2d<s32>(0,0),
				core::dimension2di(heart_texture->getOriginalSize())),
				NULL, colors, true);
			p += v2s32(16,0);
		}
		if(halfheartcount % 2 == 1)
		{
			const video::SColor color(255,255,255,255);
			const video::SColor colors[] = {color,color,color,color};
			core::rect<s32> rect(0,0,16/2,16);
			rect += p;
			core::dimension2di srcd(heart_texture->getOriginalSize());
			srcd.Width /= 2;
			driver->draw2DImage(heart_texture, rect,
				core::rect<s32>(core::position2d<s32>(0,0), srcd),
				NULL, colors, true);
			p += v2s32(16,0);
		}
	}
}

/*
	Find what the player is pointing at
*/
void getPointedNode(Client *client, v3f player_position,
		v3f camera_direction, v3f camera_position,
		bool &nodefound, core::line3d<f32> shootline,
		v3s16 &nodepos, v3s16 &neighbourpos,
		core::aabbox3d<f32> &nodehilightbox,
		f32 d)
{
	f32 mindistance = BS * 1001;
	
	v3s16 pos_i = floatToInt(player_position, BS);

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
			n = client->getNode(v3s16(x,y,z));
			if(content_pointable(n.d) == false)
				continue;
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}

		v3s16 np(x,y,z);
		v3f npf = intToFloat(np, BS);
		
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
					nodehilightbox = box;
				}
			}
		}
		else if(n.d == CONTENT_SIGN_WALL)
		{
			v3s16 dir = unpackDir(n.dir);
			v3f dir_f = v3f(dir.X, dir.Y, dir.Z);
			dir_f *= BS/2 - BS/6 - BS/20;
			v3f cpf = npf + dir_f;
			f32 distance = (cpf - camera_position).getLength();

			v3f vertices[4] =
			{
				v3f(BS*0.42,-BS*0.35,-BS*0.4),
				v3f(BS*0.49, BS*0.35, BS*0.4),
			};

			for(s32 i=0; i<2; i++)
			{
				if(dir == v3s16(1,0,0))
					vertices[i].rotateXZBy(0);
				if(dir == v3s16(-1,0,0))
					vertices[i].rotateXZBy(180);
				if(dir == v3s16(0,0,1))
					vertices[i].rotateXZBy(90);
				if(dir == v3s16(0,0,-1))
					vertices[i].rotateXZBy(-90);
				if(dir == v3s16(0,-1,0))
					vertices[i].rotateXYBy(-90);
				if(dir == v3s16(0,1,0))
					vertices[i].rotateXYBy(90);

				vertices[i] += npf;
			}

			core::aabbox3d<f32> box;

			box = core::aabbox3d<f32>(vertices[0]);
			box.addInternalPoint(vertices[1]);

			if(distance < mindistance)
			{
				if(box.intersectsWithLine(shootline))
				{
					nodefound = true;
					nodepos = np;
					neighbourpos = np;
					mindistance = distance;
					nodehilightbox = box;
				}
			}
		}
		else if(n.d == CONTENT_RAIL)
		{
			v3s16 dir = unpackDir(n.dir);
			v3f dir_f = v3f(dir.X, dir.Y, dir.Z);
			dir_f *= BS/2 - BS/6 - BS/20;
			v3f cpf = npf + dir_f;
			f32 distance = (cpf - camera_position).getLength();

			float d = (float)BS/16;
			v3f vertices[4] =
			{
				v3f(BS/2, -BS/2+d, -BS/2),
				v3f(-BS/2, -BS/2, BS/2),
			};

			for(s32 i=0; i<2; i++)
			{
				vertices[i] += npf;
			}

			core::aabbox3d<f32> box;

			box = core::aabbox3d<f32>(vertices[0]);
			box.addInternalPoint(vertices[1]);

			if(distance < mindistance)
			{
				if(box.intersectsWithLine(shootline))
				{
					nodefound = true;
					nodepos = np;
					neighbourpos = np;
					mindistance = distance;
					nodehilightbox = box;
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

						//nodehilightbox = facebox;

						const float d = 0.502;
						core::aabbox3d<f32> nodebox
								(-BS*d, -BS*d, -BS*d, BS*d, BS*d, BS*d);
						v3f nodepos_f = intToFloat(nodepos, BS);
						nodebox.MinEdge += nodepos_f;
						nodebox.MaxEdge += nodepos_f;
						nodehilightbox = nodebox;
					}
				} // if distance < mindistance
			} // for dirs
		} // regular block
	} // for coords
}

void update_skybox(video::IVideoDriver* driver,
		scene::ISceneManager* smgr, scene::ISceneNode* &skybox,
		float brightness)
{
	if(skybox)
	{
		skybox->remove();
	}
	
	/*// Disable skybox if FarMesh is enabled
	if(g_settings.getBool("enable_farmesh"))
		return;*/
	
	if(brightness >= 0.5)
	{
		skybox = smgr->addSkyBoxSceneNode(
			driver->getTexture(getTexturePath("skybox2.png").c_str()),
			driver->getTexture(getTexturePath("skybox3.png").c_str()),
			driver->getTexture(getTexturePath("skybox1.png").c_str()),
			driver->getTexture(getTexturePath("skybox1.png").c_str()),
			driver->getTexture(getTexturePath("skybox1.png").c_str()),
			driver->getTexture(getTexturePath("skybox1.png").c_str()));
	}
	else if(brightness >= 0.2)
	{
		skybox = smgr->addSkyBoxSceneNode(
			driver->getTexture(getTexturePath("skybox2_dawn.png").c_str()),
			driver->getTexture(getTexturePath("skybox3_dawn.png").c_str()),
			driver->getTexture(getTexturePath("skybox1_dawn.png").c_str()),
			driver->getTexture(getTexturePath("skybox1_dawn.png").c_str()),
			driver->getTexture(getTexturePath("skybox1_dawn.png").c_str()),
			driver->getTexture(getTexturePath("skybox1_dawn.png").c_str()));
	}
	else
	{
		skybox = smgr->addSkyBoxSceneNode(
			driver->getTexture(getTexturePath("skybox2_night.png").c_str()),
			driver->getTexture(getTexturePath("skybox3_night.png").c_str()),
			driver->getTexture(getTexturePath("skybox1_night.png").c_str()),
			driver->getTexture(getTexturePath("skybox1_night.png").c_str()),
			driver->getTexture(getTexturePath("skybox1_night.png").c_str()),
			driver->getTexture(getTexturePath("skybox1_night.png").c_str()));
	}
}

/*
	Draws a screen with a single text on it.
	Text will be removed when the screen is drawn the next time.
*/
/*gui::IGUIStaticText **/
void draw_load_screen(const std::wstring &text,
		video::IVideoDriver* driver, gui::IGUIFont* font)
{
	v2u32 screensize = driver->getScreenSize();
	const wchar_t *loadingtext = text.c_str();
	core::vector2d<u32> textsize_u = font->getDimension(loadingtext);
	core::vector2d<s32> textsize(textsize_u.X,textsize_u.Y);
	core::vector2d<s32> center(screensize.X/2, screensize.Y/2);
	core::rect<s32> textrect(center - textsize/2, center + textsize/2);

	gui::IGUIStaticText *guitext = guienv->addStaticText(
			loadingtext, textrect, false, false);
	guitext->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);

	driver->beginScene(true, true, video::SColor(255,0,0,0));
	guienv->drawAll();
	driver->endScene();
	
	guitext->remove();
	
	//return guitext;
}

void the_game(
	bool &kill,
	bool random_input,
	InputHandler *input,
	IrrlichtDevice *device,
	gui::IGUIFont* font,
	std::string map_dir,
	std::string playername,
	std::string password,
	std::string address,
	u16 port,
	std::wstring &error_message
)
{
	video::IVideoDriver* driver = device->getVideoDriver();
	scene::ISceneManager* smgr = device->getSceneManager();
	
	// Calculate text height using the font
	u32 text_height = font->getDimension(L"Random test string").Height;

	v2u32 screensize(0,0);
	v2u32 last_screensize(0,0);
	screensize = driver->getScreenSize();

	const s32 hotbar_itemcount = 8;
	//const s32 hotbar_imagesize = 36;
	//const s32 hotbar_imagesize = 64;
	s32 hotbar_imagesize = 48;
	
	// The color of the sky

	//video::SColor skycolor = video::SColor(255,140,186,250);

	video::SColor bgcolor_bright = video::SColor(255,170,200,230);

	/*
		Draw "Loading" screen
	*/
	/*gui::IGUIStaticText *gui_loadingtext = */
	//draw_load_screen(L"Loading and connecting...", driver, font);

	draw_load_screen(L"Loading...", driver, font);
	
	/*
		Create server.
		SharedPtr will delete it when it goes out of scope.
	*/
	SharedPtr<Server> server;
	if(address == ""){
		draw_load_screen(L"Creating server...", driver, font);
		std::cout<<DTIME<<"Creating server"<<std::endl;
		server = new Server(map_dir);
		server->start(port);
	}
	
	/*
		Create client
	*/

	draw_load_screen(L"Creating client...", driver, font);
	std::cout<<DTIME<<"Creating client"<<std::endl;
	Client client(device, playername.c_str(), password, draw_control);
			
	draw_load_screen(L"Resolving address...", driver, font);
	Address connect_address(0,0,0,0, port);
	try{
		if(address == "")
			//connect_address.Resolve("localhost");
			connect_address.setAddress(127,0,0,1);
		else
			connect_address.Resolve(address.c_str());
	}
	catch(ResolveError &e)
	{
		std::cout<<DTIME<<"Couldn't resolve address"<<std::endl;
		//return 0;
		error_message = L"Couldn't resolve address";
		//gui_loadingtext->remove();
		return;
	}

	/*
		Attempt to connect to the server
	*/
	
	dstream<<DTIME<<"Connecting to server at ";
	connect_address.print(&dstream);
	dstream<<std::endl;
	client.connect(connect_address);

	bool could_connect = false;
	
	try{
		float time_counter = 0.0;
		for(;;)
		{
			if(client.connectedAndInitialized())
			{
				could_connect = true;
				break;
			}
			if(client.accessDenied())
			{
				break;
			}
			// Wait for 10 seconds
			if(time_counter >= 10.0)
			{
				break;
			}
			
			std::wostringstream ss;
			ss<<L"Connecting to server... (timeout in ";
			ss<<(int)(10.0 - time_counter + 1.0);
			ss<<L" seconds)";
			draw_load_screen(ss.str(), driver, font);

			/*// Update screen
			driver->beginScene(true, true, video::SColor(255,0,0,0));
			guienv->drawAll();
			driver->endScene();*/

			// Update client and server

			client.step(0.1);

			if(server != NULL)
				server->step(0.1);
			
			// Delay a bit
			sleep_ms(100);
			time_counter += 0.1;
		}
	}
	catch(con::PeerNotFoundException &e)
	{}

	if(could_connect == false)
	{
		if(client.accessDenied())
		{
			error_message = L"Access denied. Reason: "
					+client.accessDeniedReason();
			std::cout<<DTIME<<wide_to_narrow(error_message)<<std::endl;
		}
		else
		{
			error_message = L"Connection timed out.";
			std::cout<<DTIME<<"Timed out."<<std::endl;
		}
		//gui_loadingtext->remove();
		return;
	}

	/*
		Create skybox
	*/
	float old_brightness = 1.0;
	scene::ISceneNode* skybox = NULL;
	update_skybox(driver, smgr, skybox, 1.0);
	
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
	{
		error_message = L"Failed to create the camera node";
		return;
	}

	camera->setFOV(FOV_ANGLE);

	// Just so big a value that everything rendered is visible
	camera->setFarValue(100000*BS);
	
	f32 camera_yaw = 0; // "right/left"
	f32 camera_pitch = 0; // "up/down"

	/*
		Clouds
	*/
	
	float cloud_height = BS*100;
	Clouds *clouds = NULL;
	if(g_settings.getBool("enable_clouds"))
	{
		clouds = new Clouds(smgr->getRootSceneNode(), smgr, -1,
				cloud_height, time(0));
	}
	
	/*
		FarMesh
	*/

	FarMesh *farmesh = NULL;
	if(g_settings.getBool("enable_farmesh"))
	{
		farmesh = new FarMesh(smgr->getRootSceneNode(), smgr, -1, client.getMapSeed(), &client);
	}

	/*
		Move into game
	*/
	
	//gui_loadingtext->remove();

	/*
		Add some gui stuff
	*/

	// First line of debug text
	gui::IGUIStaticText *guitext = guienv->addStaticText(
			L"Minetest-c55",
			core::rect<s32>(5, 5, 795, 5+text_height),
			false, false);
	// Second line of debug text
	gui::IGUIStaticText *guitext2 = guienv->addStaticText(
			L"",
			core::rect<s32>(5, 5+(text_height+5)*1, 795, (5+text_height)*2),
			false, false);
	
	// At the middle of the screen
	// Object infos are shown in this
	gui::IGUIStaticText *guitext_info = guienv->addStaticText(
			L"",
			core::rect<s32>(0,0,400,text_height+5) + v2s32(100,200),
			false, false);
	
	// Chat text
	gui::IGUIStaticText *guitext_chat = guienv->addStaticText(
			L"",
			core::rect<s32>(0,0,0,0),
			//false, false); // Disable word wrap as of now
			false, true);
	//guitext_chat->setBackgroundColor(video::SColor(96,0,0,0));
	core::list<ChatLine> chat_lines;
	
	/*GUIQuickInventory *quick_inventory = new GUIQuickInventory
			(guienv, NULL, v2s32(10, 70), 5, &local_inventory);*/
	/*GUIQuickInventory *quick_inventory = new GUIQuickInventory
			(guienv, NULL, v2s32(0, 0), quickinv_itemcount, &local_inventory);*/
	
	// Test the text input system
	/*(new GUITextInputMenu(guienv, guiroot, -1, &g_menumgr,
			NULL))->drop();*/
	/*GUIMessageMenu *menu =
			new GUIMessageMenu(guienv, guiroot, -1, 
				&g_menumgr,
				L"Asd");
	menu->drop();*/
	
	// Launch pause menu
	(new GUIPauseMenu(guienv, guiroot, -1, g_gamecallback,
			&g_menumgr))->drop();
	
	// Enable texts
	/*guitext2->setVisible(true);
	guitext_info->setVisible(true);
	guitext_chat->setVisible(true);*/

	//s32 guitext_chat_pad_bottom = 70;

	/*
		Some statistics are collected in these
	*/
	u32 drawtime = 0;
	u32 beginscenetime = 0;
	u32 scenetime = 0;
	u32 endscenetime = 0;
	
	// A test
	//throw con::PeerNotFoundException("lol");

	core::list<float> frametime_log;

	float damage_flash_timer = 0;
	s16 farmesh_range = 20*MAP_BLOCKSIZE;
	
	bool invert_mouse = g_settings.getBool("invert_mouse");

	/*
		Main loop
	*/

	bool first_loop_after_window_activation = true;

	// TODO: Convert the static interval timers to these
	// Interval limiter for profiler
	IntervalLimiter m_profiler_interval;

	// Time is in milliseconds
	// NOTE: getRealTime() causes strange problems in wine (imprecision?)
	// NOTE: So we have to use getTime() and call run()s between them
	u32 lasttime = device->getTimer()->getTime();

	while(device->run() && kill == false)
	{
		//std::cerr<<"frame"<<std::endl;

		if(g_gamecallback->disconnect_requested)
		{
			g_gamecallback->disconnect_requested = false;
			break;
		}

		if(g_gamecallback->changepassword_requested)
		{
			(new GUIPasswordChange(guienv, guiroot, -1,
				&g_menumgr, &client))->drop();
			g_gamecallback->changepassword_requested = false;
		}

		/*
			Process TextureSource's queue
		*/
		((TextureSource*)g_texturesource)->processQueue();

		/*
			Random calculations
		*/
		last_screensize = screensize;
		screensize = driver->getScreenSize();
		v2s32 displaycenter(screensize.X/2,screensize.Y/2);
		//bool screensize_changed = screensize != last_screensize;

		// Resize hotbar
		if(screensize.Y <= 600)
			hotbar_imagesize = 32;
		else if(screensize.Y <= 1024)
			hotbar_imagesize = 48;
		else
			hotbar_imagesize = 64;
		
		// Hilight boxes collected during the loop and displayed
		core::list< core::aabbox3d<f32> > hilightboxes;
		
		// Info text
		std::wstring infotext;

		// When screen size changes, update positions and sizes of stuff
		/*if(screensize_changed)
		{
			v2s32 pos(displaycenter.X-((quickinv_itemcount-1)*quickinv_spacing+quickinv_size)/2, screensize.Y-quickinv_spacing);
			quick_inventory->updatePosition(pos);
		}*/

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
	
		// Necessary for device->getTimer()->getTime()
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

		// Necessary for device->getTimer()->getTime()
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
			Log frametime for visualization
		*/
		frametime_log.push_back(dtime);
		if(frametime_log.size() > 100)
		{
			core::list<float>::Iterator i = frametime_log.begin();
			frametime_log.erase(i);
		}

		/*
			Visualize frametime in terminal
		*/
		/*for(u32 i=0; i<dtime*400; i++)
			std::cout<<"X";
		std::cout<<std::endl;*/

		/*
			Time average and jitter calculation
		*/

		static f32 dtime_avg1 = 0.0;
		dtime_avg1 = dtime_avg1 * 0.96 + dtime * 0.04;
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
			Profiler
		*/
		float profiler_print_interval =
				g_settings.getFloat("profiler_print_interval");
		if(profiler_print_interval != 0)
		{
			if(m_profiler_interval.step(0.030, profiler_print_interval))
			{
				dstream<<"Profiler:"<<std::endl;
				g_profiler.print(dstream);
				g_profiler.clear();
			}
		}

		/*
			Direct handling of user input
		*/
		
		// Reset input if window not active or some menu is active
		if(device->isWindowActive() == false || noMenuActive() == false)
		{
			input->clear();
		}

		// Input handler step() (used by the random input generator)
		input->step(dtime);

		/*
			Launch menus according to keys
		*/
		if(input->wasKeyDown(getKeySetting("keymap_inventory")))
		{
			dstream<<DTIME<<"the_game: "
					<<"Launching inventory"<<std::endl;
			
			GUIInventoryMenu *menu =
				new GUIInventoryMenu(guienv, guiroot, -1,
					&g_menumgr, v2s16(8,7),
					client.getInventoryContext(),
					&client);

			core::array<GUIInventoryMenu::DrawSpec> draw_spec;
			draw_spec.push_back(GUIInventoryMenu::DrawSpec(
					"list", "current_player", "main",
					v2s32(0, 3), v2s32(8, 4)));
			draw_spec.push_back(GUIInventoryMenu::DrawSpec(
					"list", "current_player", "craft",
					v2s32(3, 0), v2s32(3, 3)));
			draw_spec.push_back(GUIInventoryMenu::DrawSpec(
					"list", "current_player", "craftresult",
					v2s32(7, 1), v2s32(1, 1)));

			menu->setDrawSpec(draw_spec);

			menu->drop();
		}
		else if(input->wasKeyDown(KEY_ESCAPE))
		{
			dstream<<DTIME<<"the_game: "
					<<"Launching pause menu"<<std::endl;
			// It will delete itself by itself
			(new GUIPauseMenu(guienv, guiroot, -1, g_gamecallback,
					&g_menumgr))->drop();

			// Move mouse cursor on top of the disconnect button
			input->setMousePos(displaycenter.X, displaycenter.Y+25);
		}
		else if(input->wasKeyDown(getKeySetting("keymap_chat")))
		{
			TextDest *dest = new TextDestChat(&client);

			(new GUITextInputMenu(guienv, guiroot, -1,
					&g_menumgr, dest,
					L""))->drop();
		}
		else if(input->wasKeyDown(getKeySetting("keymap_freemove")))
		{
			if(g_settings.getBool("free_move"))
			{
				g_settings.set("free_move","false");
				chat_lines.push_back(ChatLine(L"free_move disabled"));
			}
			else
			{
				g_settings.set("free_move","true");
				chat_lines.push_back(ChatLine(L"free_move enabled"));
			}
		}
		else if(input->wasKeyDown(getKeySetting("keymap_fastmove")))
		{
			if(g_settings.getBool("fast_move"))
			{
				g_settings.set("fast_move","false");
				chat_lines.push_back(ChatLine(L"fast_move disabled"));
			}
			else
			{
				g_settings.set("fast_move","true");
				chat_lines.push_back(ChatLine(L"fast_move enabled"));
			}
		}
		else if(input->wasKeyDown(getKeySetting("keymap_frametime_graph")))
		{
			if(g_settings.getBool("frametime_graph"))
			{
				g_settings.set("frametime_graph","false");
				chat_lines.push_back(ChatLine(L"frametime_graph disabled"));
			}
			else
			{
				g_settings.set("frametime_graph","true");
				chat_lines.push_back(ChatLine(L"frametime_graph enabled"));
			}
		}
		else if(input->wasKeyDown(getKeySetting("keymap_screenshot")))
		{
			irr::video::IImage* const image = driver->createScreenShot(); 
			if (image) { 
				irr::c8 filename[256]; 
				snprintf(filename, 256, "%s/screenshot_%u.png", 
						 g_settings.get("screenshot_path").c_str(),
						 device->getTimer()->getRealTime()); 
				if (driver->writeImageToFile(image, filename)) {
					std::wstringstream sstr;
					sstr<<"Saved screenshot to '"<<filename<<"'";
					dstream<<"Saved screenshot to '"<<filename<<"'"<<std::endl;
					chat_lines.push_back(ChatLine(sstr.str()));
				} else{
					dstream<<"Failed to save screenshot '"<<filename<<"'"<<std::endl;
				}
				image->drop(); 
			}			 
		}

		// Item selection with mouse wheel
		{
			s32 wheel = input->getMouseWheel();
			u16 max_item = MYMIN(PLAYER_INVENTORY_SIZE-1,
					hotbar_itemcount-1);

			if(wheel < 0)
			{
				if(g_selected_item < max_item)
					g_selected_item++;
				else
					g_selected_item = 0;
			}
			else if(wheel > 0)
			{
				if(g_selected_item > 0)
					g_selected_item--;
				else
					g_selected_item = max_item;
			}
		}
		
		// Item selection
		for(u16 i=0; i<10; i++)
		{
			s32 keycode = irr::KEY_KEY_1 + i;
			if(i == 9)
				keycode = irr::KEY_KEY_0;
			if(input->wasKeyDown((irr::EKEY_CODE)keycode))
			{
				if(i < PLAYER_INVENTORY_SIZE && i < hotbar_itemcount)
				{
					g_selected_item = i;

					dstream<<DTIME<<"Selected item: "
							<<g_selected_item<<std::endl;
				}
			}
		}

		// Viewing range selection
		if(input->wasKeyDown(getKeySetting("keymap_rangeselect")))
		{
			if(draw_control.range_all)
			{
				draw_control.range_all = false;
				dstream<<DTIME<<"Disabled full viewing range"<<std::endl;
			}
			else
			{
				draw_control.range_all = true;
				dstream<<DTIME<<"Enabled full viewing range"<<std::endl;
			}
		}

		// Print debug stacks
		if(input->wasKeyDown(getKeySetting("keymap_print_debug_stacks")))
		{
			dstream<<"-----------------------------------------"
					<<std::endl;
			dstream<<DTIME<<"Printing debug stacks:"<<std::endl;
			dstream<<"-----------------------------------------"
					<<std::endl;
			debug_stacks_print();
		}

		/*
			Player speed control
			TODO: Cache the keycodes from getKeySetting
		*/
		
		{
			/*bool a_up,
			bool a_down,
			bool a_left,
			bool a_right,
			bool a_jump,
			bool a_superspeed,
			bool a_sneak,
			float a_pitch,
			float a_yaw*/
			PlayerControl control(
				input->isKeyDown(getKeySetting("keymap_forward")),
				input->isKeyDown(getKeySetting("keymap_backward")),
				input->isKeyDown(getKeySetting("keymap_left")),
				input->isKeyDown(getKeySetting("keymap_right")),
				input->isKeyDown(getKeySetting("keymap_jump")),
				input->isKeyDown(getKeySetting("keymap_special1")),
				input->isKeyDown(getKeySetting("keymap_sneak")),
				camera_pitch,
				camera_yaw
			);
			client.setPlayerControl(control);
		}
		
		/*
			Run server
		*/

		if(server != NULL)
		{
			//TimeTaker timer("server->step(dtime)");
			server->step(dtime);
		}

		/*
			Process environment
		*/
		
		{
			//TimeTaker timer("client.step(dtime)");
			client.step(dtime);
			//client.step(dtime_avg1);
		}

		// Read client events
		for(;;)
		{
			ClientEvent event = client.getClientEvent();
			if(event.type == CE_NONE)
			{
				break;
			}
			else if(event.type == CE_PLAYER_DAMAGE)
			{
				//u16 damage = event.player_damage.amount;
				//dstream<<"Player damage: "<<damage<<std::endl;
				damage_flash_timer = 0.05;
			}
			else if(event.type == CE_PLAYER_FORCE_MOVE)
			{
				camera_yaw = event.player_force_move.yaw;
				camera_pitch = event.player_force_move.pitch;
			}
		}
		
		// Get player position
		v3f player_position = client.getPlayerPosition();

		//TimeTaker //timer2("//timer2");

		/*
			Mouse and camera control
		*/
		
		if((device->isWindowActive() && noMenuActive()) || random_input)
		{
			if(!random_input)
			{
				// Mac OSX gets upset if this is set every frame
				if(device->getCursorControl()->isVisible())
					device->getCursorControl()->setVisible(false);
			}

			if(first_loop_after_window_activation){
				//std::cout<<"window active, first loop"<<std::endl;
				first_loop_after_window_activation = false;
			}
			else{
				s32 dx = input->getMousePos().X - displaycenter.X;
				s32 dy = input->getMousePos().Y - displaycenter.Y;
				if(invert_mouse)
					dy = -dy;
				//std::cout<<"window active, pos difference "<<dx<<","<<dy<<std::endl;
				
				/*const float keyspeed = 500;
				if(input->isKeyDown(irr::KEY_UP))
					dy -= dtime * keyspeed;
				if(input->isKeyDown(irr::KEY_DOWN))
					dy += dtime * keyspeed;
				if(input->isKeyDown(irr::KEY_LEFT))
					dx -= dtime * keyspeed;
				if(input->isKeyDown(irr::KEY_RIGHT))
					dx += dtime * keyspeed;*/

				camera_yaw -= dx*0.2;
				camera_pitch += dy*0.2;
				if(camera_pitch < -89.5) camera_pitch = -89.5;
				if(camera_pitch > 89.5) camera_pitch = 89.5;
			}
			input->setMousePos(displaycenter.X, displaycenter.Y);
		}
		else{
			// Mac OSX gets upset if this is set every frame
			if(device->getCursorControl()->isVisible() == false)
				device->getCursorControl()->setVisible(true);

			//std::cout<<"window inactive"<<std::endl;
			first_loop_after_window_activation = true;
		}

		camera_yaw = wrapDegrees(camera_yaw);
		camera_pitch = wrapDegrees(camera_pitch);
		
		v3f camera_direction = v3f(0,0,1);
		camera_direction.rotateYZBy(camera_pitch);
		camera_direction.rotateXZBy(camera_yaw);
		
		// This is at the height of the eyes of the current figure
		//v3f camera_position = player_position + v3f(0, BS+BS/2, 0);
		// This is more like in minecraft
		v3f camera_position = player_position + v3f(0, BS+BS*0.625, 0);

		camera->setPosition(camera_position);
		// *100.0 helps in large map coordinates
		camera->setTarget(camera_position + camera_direction * 100.0);

		if(FIELD_OF_VIEW_TEST){
			client.updateCamera(v3f(0,0,0), v3f(0,0,1));
		}
		else{
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

		ClientActiveObject *selected_active_object
				= client.getSelectedActiveObject
					(d*BS, camera_position, shootline);

		if(selected_object != NULL)
		{
			//dstream<<"Client returned selected_object != NULL"<<std::endl;

			core::aabbox3d<f32> box_on_map
					= selected_object->getSelectionBoxOnMap();

			hilightboxes.push_back(box_on_map);

			infotext = narrow_to_wide(selected_object->infoText());

			if(input->getLeftClicked())
			{
				std::cout<<DTIME<<"Left-clicked object"<<std::endl;
				client.clickObject(0, selected_object->getBlock()->getPos(),
						selected_object->getId(), g_selected_item);
			}
			else if(input->getRightClicked())
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
								&g_menumgr, dest,
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
		else if(selected_active_object != NULL)
		{
			//dstream<<"Client returned selected_active_object != NULL"<<std::endl;
			
			core::aabbox3d<f32> *selection_box
					= selected_active_object->getSelectionBox();
			// Box should exist because object was returned in the
			// first place
			assert(selection_box);

			v3f pos = selected_active_object->getPosition();

			core::aabbox3d<f32> box_on_map(
					selection_box->MinEdge + pos,
					selection_box->MaxEdge + pos
			);

			hilightboxes.push_back(box_on_map);

			//infotext = narrow_to_wide("A ClientActiveObject");
			infotext = narrow_to_wide(selected_active_object->infoText());

			if(input->getLeftClicked())
			{
				std::cout<<DTIME<<"Left-clicked object"<<std::endl;
				client.clickActiveObject(0,
						selected_active_object->getId(), g_selected_item);
			}
			else if(input->getRightClicked())
			{
				std::cout<<DTIME<<"Right-clicked object"<<std::endl;
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
		core::aabbox3d<f32> nodehilightbox;

		getPointedNode(&client, player_position,
				camera_direction, camera_position,
				nodefound, shootline,
				nodepos, neighbourpos,
				nodehilightbox, d);
	
		static float nodig_delay_counter = 0.0;

		if(nodefound)
		{
			static v3s16 nodepos_old(-32768,-32768,-32768);

			static float dig_time = 0.0;
			static u16 dig_index = 0;
			
			/*
				Visualize selection
			*/

			hilightboxes.push_back(nodehilightbox);

			/*
				Check information text of node
			*/

			NodeMetadata *meta = client.getNodeMetadata(nodepos);
			if(meta)
			{
				infotext = narrow_to_wide(meta->infoText());
			}
			
			//MapNode node = client.getNode(nodepos);

			/*
				Handle digging
			*/
			
			if(input->getLeftReleased())
			{
				client.clearTempMod(nodepos);
				dig_time = 0.0;
			}
			
			if(nodig_delay_counter > 0.0)
			{
				nodig_delay_counter -= dtime;
			}
			else
			{
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

				if(input->getLeftClicked() ||
						(input->getLeftState() && nodepos != nodepos_old))
				{
					dstream<<DTIME<<"Started digging"<<std::endl;
					client.groundAction(0, nodepos, neighbourpos, g_selected_item);
				}
				if(input->getLeftClicked())
				{
					client.setTempMod(nodepos, NodeMod(NODEMOD_CRACK, 0));
				}
				if(input->getLeftState())
				{
					MapNode n = client.getNode(nodepos);
				
					// Get tool name. Default is "" = bare hands
					std::string toolname = "";
					InventoryList *mlist = local_inventory.getList("main");
					if(mlist != NULL)
					{
						InventoryItem *item = mlist->getItem(g_selected_item);
						if(item && (std::string)item->getName() == "ToolItem")
						{
							ToolItem *titem = (ToolItem*)item;
							toolname = titem->getToolName();
						}
					}

					// Get digging properties for material and tool
					u8 material = n.d;
					DiggingProperties prop =
							getDiggingProperties(material, toolname);
					
					float dig_time_complete = 0.0;

					if(prop.diggable == false)
					{
						/*dstream<<"Material "<<(int)material
								<<" not diggable with \""
								<<toolname<<"\""<<std::endl;*/
						// I guess nobody will wait for this long
						dig_time_complete = 10000000.0;
					}
					else
					{
						dig_time_complete = prop.time;
					}
					
					if(dig_time_complete >= 0.001)
					{
						dig_index = (u16)((float)CRACK_ANIMATION_LENGTH
								* dig_time/dig_time_complete);
					}
					// This is for torches
					else
					{
						dig_index = CRACK_ANIMATION_LENGTH;
					}

					if(dig_index < CRACK_ANIMATION_LENGTH)
					{
						//TimeTaker timer("client.setTempMod");
						//dstream<<"dig_index="<<dig_index<<std::endl;
						client.setTempMod(nodepos, NodeMod(NODEMOD_CRACK, dig_index));
					}
					else
					{
						dstream<<DTIME<<"Digging completed"<<std::endl;
						client.groundAction(3, nodepos, neighbourpos, g_selected_item);
						client.clearTempMod(nodepos);
						client.removeNode(nodepos);

						dig_time = 0;

						nodig_delay_counter = dig_time_complete
								/ (float)CRACK_ANIMATION_LENGTH;

						// We don't want a corresponding delay to
						// very time consuming nodes
						if(nodig_delay_counter > 0.5)
						{
							nodig_delay_counter = 0.5;
						}
						// We want a slight delay to very little
						// time consuming nodes
						float mindelay = 0.15;
						if(nodig_delay_counter < mindelay)
						{
							nodig_delay_counter = mindelay;
						}
					}

					dig_time += dtime;
				}
			}
			
			if(input->getRightClicked())
			{
				std::cout<<DTIME<<"Ground right-clicked"<<std::endl;
				
				// If metadata provides an inventory view, activate it
				if(meta && meta->getInventoryDrawSpecString() != "" && !random_input)
				{
					dstream<<DTIME<<"Launching custom inventory view"<<std::endl;
					/*
						Construct the unique identification string of the node
					*/
					std::string current_name;
					current_name += "nodemeta:";
					current_name += itos(nodepos.X);
					current_name += ",";
					current_name += itos(nodepos.Y);
					current_name += ",";
					current_name += itos(nodepos.Z);
					
					/*
						Create menu
					*/

					core::array<GUIInventoryMenu::DrawSpec> draw_spec;
					v2s16 invsize =
						GUIInventoryMenu::makeDrawSpecArrayFromString(
							draw_spec,
							meta->getInventoryDrawSpecString(),
							current_name);

					GUIInventoryMenu *menu =
						new GUIInventoryMenu(guienv, guiroot, -1,
							&g_menumgr, invsize,
							client.getInventoryContext(),
							&client);
					menu->setDrawSpec(draw_spec);
					menu->drop();
				}
				else if(meta && meta->typeId() == CONTENT_SIGN_WALL && !random_input)
				{
					dstream<<"Sign node right-clicked"<<std::endl;
					
					SignNodeMetadata *signmeta = (SignNodeMetadata*)meta;
					
					// Get a new text for it

					TextDest *dest = new TextDestSignNode(nodepos, &client);

					std::wstring wtext =
							narrow_to_wide(signmeta->getText());

					(new GUITextInputMenu(guienv, guiroot, -1,
							&g_menumgr, dest,
							wtext))->drop();
				}
				else
				{
					client.groundAction(1, nodepos, neighbourpos, g_selected_item);
				}
			}
			
			nodepos_old = nodepos;
		}
		else{
		}

		} // selected_object == NULL
		
		input->resetLeftClicked();
		input->resetRightClicked();
		
		if(input->getLeftReleased())
		{
			std::cout<<DTIME<<"Left button released (stopped digging)"
					<<std::endl;
			client.groundAction(2, v3s16(0,0,0), v3s16(0,0,0), 0);
		}
		if(input->getRightReleased())
		{
			//std::cout<<DTIME<<"Right released"<<std::endl;
			// Nothing here
		}
		
		input->resetLeftReleased();
		input->resetRightReleased();
		
		/*
			Calculate stuff for drawing
		*/

		camera->setAspectRatio((f32)screensize.X / (f32)screensize.Y);
		
		u32 daynight_ratio = client.getDayNightRatio();
		u8 l = decode_light((daynight_ratio * LIGHT_SUN) / 1000);
		video::SColor bgcolor = video::SColor(
				255,
				bgcolor_bright.getRed() * l / 255,
				bgcolor_bright.getGreen() * l / 255,
				bgcolor_bright.getBlue() * l / 255);
				/*skycolor.getRed() * l / 255,
				skycolor.getGreen() * l / 255,
				skycolor.getBlue() * l / 255);*/

		float brightness = (float)l/255.0;

		/*
			Update skybox
		*/
		if(fabs(brightness - old_brightness) > 0.01)
			update_skybox(driver, smgr, skybox, brightness);

		/*
			Update coulds
		*/
		if(clouds)
		{
			clouds->step(dtime);
			clouds->update(v2f(player_position.X, player_position.Z),
					0.05+brightness*0.95);
		}
		
		/*
			Update farmesh
		*/
		if(farmesh)
		{
			farmesh->step(dtime);
			farmesh->update(v2f(player_position.X, player_position.Z),
					0.05+brightness*0.95);
		}
		
		// Store brightness value
		old_brightness = brightness;

		/*
			Fog
		*/
		
		if(g_settings.getBool("enable_fog") == true)
		{
			f32 range;
			if(farmesh)
			{
				range = BS*farmesh_range;
			}
			else
			{
				range = draw_control.wanted_range*BS + MAP_BLOCKSIZE*BS*1.5;
				if(draw_control.range_all)
					range = 100000*BS;
				if(range < 50*BS)
					range = range * 0.5 + 25*BS;
			}

			driver->setFog(
				bgcolor,
				video::EFT_FOG_LINEAR,
				range*0.4,
				range*1.0,
				0.01,
				false, // pixel fog
				false // range fog
			);
		}
		else
		{
			driver->setFog(
				bgcolor,
				video::EFT_FOG_LINEAR,
				100000*BS,
				110000*BS,
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
			static float drawtime_avg = 0;
			drawtime_avg = drawtime_avg * 0.95 + (float)drawtime*0.05;
			static float beginscenetime_avg = 0;
			beginscenetime_avg = beginscenetime_avg * 0.95 + (float)beginscenetime*0.05;
			static float scenetime_avg = 0;
			scenetime_avg = scenetime_avg * 0.95 + (float)scenetime*0.05;
			static float endscenetime_avg = 0;
			endscenetime_avg = endscenetime_avg * 0.95 + (float)endscenetime*0.05;
			
			char temptext[300];
			snprintf(temptext, 300, "Minetest-delta %s ("
					"R: range_all=%i"
					")"
					" drawtime=%.0f, beginscenetime=%.0f"
					", scenetime=%.0f, endscenetime=%.0f",
					VERSION_STRING,
					draw_control.range_all,
					drawtime_avg,
					beginscenetime_avg,
					scenetime_avg,
					endscenetime_avg
					);
			
			guitext->setText(narrow_to_wide(temptext).c_str());
		}
		
		{
			char temptext[300];
			snprintf(temptext, 300,
					"(% .1f, % .1f, % .1f)"
					" (% .3f < btime_jitter < % .3f"
					", dtime_jitter = % .1f %%"
					", v_range = %.1f)",
					player_position.X/BS,
					player_position.Y/BS,
					player_position.Z/BS,
					busytime_jitter1_min_sample,
					busytime_jitter1_max_sample,
					dtime_jitter1_max_fraction * 100.0,
					draw_control.wanted_range
					);

			guitext2->setText(narrow_to_wide(temptext).c_str());
		}
		
		{
			guitext_info->setText(infotext.c_str());
		}
		
		/*
			Get chat messages from client
		*/
		{
			// Get new messages
			std::wstring message;
			while(client.getChatMessage(message))
			{
				chat_lines.push_back(ChatLine(message));
				/*if(chat_lines.size() > 6)
				{
					core::list<ChatLine>::Iterator
							i = chat_lines.begin();
					chat_lines.erase(i);
				}*/
			}
			// Append them to form the whole static text and throw
			// it to the gui element
			std::wstring whole;
			// This will correspond to the line number counted from
			// top to bottom, from size-1 to 0
			s16 line_number = chat_lines.size();
			// Count of messages to be removed from the top
			u16 to_be_removed_count = 0;
			for(core::list<ChatLine>::Iterator
					i = chat_lines.begin();
					i != chat_lines.end(); i++)
			{
				// After this, line number is valid for this loop
				line_number--;
				// Increment age
				(*i).age += dtime;
				/*
					This results in a maximum age of 60*6 to the
					lowermost line and a maximum of 6 lines
				*/
				float allowed_age = (6-line_number) * 60.0;

				if((*i).age > allowed_age)
				{
					to_be_removed_count++;
					continue;
				}
				whole += (*i).text + L'\n';
			}
			for(u16 i=0; i<to_be_removed_count; i++)
			{
				core::list<ChatLine>::Iterator
						it = chat_lines.begin();
				chat_lines.erase(it);
			}
			guitext_chat->setText(whole.c_str());

			// Update gui element size and position

			/*core::rect<s32> rect(
					10,
					screensize.Y - guitext_chat_pad_bottom
							- text_height*chat_lines.size(),
					screensize.X - 10,
					screensize.Y - guitext_chat_pad_bottom
			);*/
			core::rect<s32> rect(
					10,
					50,
					screensize.X - 10,
					50 + guitext_chat->getTextHeight()
			);

			guitext_chat->setRelativePosition(rect);

			if(chat_lines.size() == 0)
				guitext_chat->setVisible(false);
			else
				guitext_chat->setVisible(true);
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
		//m.Thickness = 10;
		m.Thickness = 3;
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
			Frametime log
		*/
		if(g_settings.getBool("frametime_graph") == true)
		{
			s32 x = 10;
			for(core::list<float>::Iterator
					i = frametime_log.begin();
					i != frametime_log.end();
					i++)
			{
				driver->draw2DLine(v2s32(x,50),
						v2s32(x,50+(*i)*1000),
						video::SColor(255,255,255,255));
				x++;
			}
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

		} // timer

		//timer10.stop();
		//TimeTaker //timer11("//timer11");

		/*
			Draw gui
		*/
		// 0-1ms
		guienv->drawAll();

		/*
			Draw hotbar
		*/
		{
			draw_hotbar(driver, font, v2s32(displaycenter.X, screensize.Y),
					hotbar_imagesize, hotbar_itemcount, &local_inventory,
					client.getHP());
		}

		/*
			Damage flash
		*/
		if(damage_flash_timer > 0.0)
		{
			damage_flash_timer -= dtime;
			
			video::SColor color(128,255,0,0);
			driver->draw2DRectangle(color,
					core::rect<s32>(0,0,screensize.X,screensize.Y),
					NULL);
		}

		/*
			Environment post fx
		*/
		{
			client.getEnv()->drawPostFx(driver, camera_position);
		}
		
		/*
			End scene
		*/
		{
			TimeTaker timer("endScene");
			endSceneX(driver);
			endscenetime = timer.stop(true);
		}

		drawtime = drawtimer.stop(true);

		/*
			End of drawing
		*/

		static s16 lastFPS = 0;
		//u16 fps = driver->getFPS();
		u16 fps = (1.0/dtime_avg1);

		if (lastFPS != fps)
		{
			core::stringw str = L"Minetest [";
			str += driver->getName();
			str += "] FPS=";
			str += fps;

			device->setWindowCaption(str.c_str());
			lastFPS = fps;
		}
	}

	/*
		Drop stuff
	*/
	if(clouds)
		clouds->drop();
	
	/*
		Draw a "shutting down" screen, which will be shown while the map
		generator and other stuff quits
	*/
	{
		/*gui::IGUIStaticText *gui_shuttingdowntext = */
		draw_load_screen(L"Shutting down stuff...", driver, font);
		/*driver->beginScene(true, true, video::SColor(255,0,0,0));
		guienv->drawAll();
		driver->endScene();
		gui_shuttingdowntext->remove();*/
	}
}


