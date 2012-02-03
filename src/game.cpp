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

#include "game.h"
#include "common_irrlicht.h"
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "client.h"
#include "server.h"
#include "guiPauseMenu.h"
#include "guiPasswordChange.h"
#include "guiInventoryMenu.h"
#include "guiTextInputMenu.h"
#include "guiDeathScreen.h"
#include "materials.h"
#include "config.h"
#include "clouds.h"
#include "camera.h"
#include "farmesh.h"
#include "mapblock.h"
#include "settings.h"
#include "profiler.h"
#include "mainmenumanager.h"
#include "gettext.h"
#include "log.h"
#include "filesys.h"
// Needed for determining pointing to nodes
#include "nodedef.h"
#include "nodemetadata.h"
#include "main.h" // For g_settings
#include "itemdef.h"
#include "tile.h" // For TextureSource
#include "logoutputbuffer.h"

/*
	Setting this to 1 enables a special camera mode that forces
	the renderers to think that the camera statically points from
	the starting place to a static direction.

	This allows one to move around with the player and see what
	is actually drawn behind solid things and behind the player.
*/
#define FIELD_OF_VIEW_TEST 0


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
	Text input system
*/

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

		// Send to others
		m_client->sendChatMessage(text);
		// Show locally
		m_client->addChatMessage(text);
	}

	Client *m_client;
};

struct TextDestNodeMetadata : public TextDest
{
	TextDestNodeMetadata(v3s16 p, Client *client)
	{
		m_p = p;
		m_client = client;
	}
	void gotText(std::wstring text)
	{
		std::string ntext = wide_to_narrow(text);
		infostream<<"Changing text of a sign node: "
				<<ntext<<std::endl;
		m_client->sendSignNodeText(m_p, ntext);
	}

	v3s16 m_p;
	Client *m_client;
};

/* Respawn menu callback */

class MainRespawnInitiator: public IRespawnInitiator
{
public:
	MainRespawnInitiator(bool *active, Client *client):
		m_active(active), m_client(client)
	{
		*m_active = true;
	}
	void respawn()
	{
		*m_active = false;
		m_client->sendRespawn();
	}
private:
	bool *m_active;
	Client *m_client;
};

/*
	Hotbar draw routine
*/
void draw_hotbar(video::IVideoDriver *driver, gui::IGUIFont *font,
		IGameDef *gamedef,
		v2s32 centerlowerpos, s32 imgsize, s32 itemcount,
		Inventory *inventory, s32 halfheartcount, u16 playeritem)
{
	InventoryList *mainlist = inventory->getList("main");
	if(mainlist == NULL)
	{
		errorstream<<"draw_hotbar(): mainlist == NULL"<<std::endl;
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
		const ItemStack &item = mainlist->getItem(i);
		
		core::rect<s32> rect = imgrect + pos
				+ v2s32(padding+i*(imgsize+padding*2), padding);
		
		if(playeritem == i)
		{
			video::SColor c_outside(255,255,0,0);
			//video::SColor c_outside(255,0,0,0);
			//video::SColor c_inside(255,192,192,192);
			s32 x1 = rect.UpperLeftCorner.X;
			s32 y1 = rect.UpperLeftCorner.Y;
			s32 x2 = rect.LowerRightCorner.X;
			s32 y2 = rect.LowerRightCorner.Y;
			// Black base borders
			driver->draw2DRectangle(c_outside,
					core::rect<s32>(
						v2s32(x1 - padding, y1 - padding),
						v2s32(x2 + padding, y1)
					), NULL);
			driver->draw2DRectangle(c_outside,
					core::rect<s32>(
						v2s32(x1 - padding, y2),
						v2s32(x2 + padding, y2 + padding)
					), NULL);
			driver->draw2DRectangle(c_outside,
					core::rect<s32>(
						v2s32(x1 - padding, y1),
						v2s32(x1, y2)
					), NULL);
			driver->draw2DRectangle(c_outside,
					core::rect<s32>(
						v2s32(x2, y1),
						v2s32(x2 + padding, y2)
					), NULL);
			/*// Light inside borders
			driver->draw2DRectangle(c_inside,
					core::rect<s32>(
						v2s32(x1 - padding/2, y1 - padding/2),
						v2s32(x2 + padding/2, y1)
					), NULL);
			driver->draw2DRectangle(c_inside,
					core::rect<s32>(
						v2s32(x1 - padding/2, y2),
						v2s32(x2 + padding/2, y2 + padding/2)
					), NULL);
			driver->draw2DRectangle(c_inside,
					core::rect<s32>(
						v2s32(x1 - padding/2, y1),
						v2s32(x1, y2)
					), NULL);
			driver->draw2DRectangle(c_inside,
					core::rect<s32>(
						v2s32(x2, y1),
						v2s32(x2 + padding/2, y2)
					), NULL);
			*/
		}

		video::SColor bgcolor2(128,0,0,0);
		driver->draw2DRectangle(bgcolor2, rect, NULL);
		drawItemStack(driver, font, item, rect, NULL, gamedef);
	}
	
	/*
		Draw hearts
	*/
	video::ITexture *heart_texture =
		gamedef->getTextureSource()->getTextureRaw("heart.png");
	if(heart_texture)
	{
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
	Check if a node is pointable
*/
inline bool isPointableNode(const MapNode& n,
		Client *client, bool liquids_pointable)
{
	const ContentFeatures &features = client->getNodeDefManager()->get(n);
	return features.pointable ||
		(liquids_pointable && features.isLiquid());
}

/*
	Find what the player is pointing at
*/
PointedThing getPointedThing(Client *client, v3f player_position,
		v3f camera_direction, v3f camera_position,
		core::line3d<f32> shootline, f32 d,
		bool liquids_pointable,
		bool look_for_object,
		core::aabbox3d<f32> &hilightbox,
		bool &should_show_hilightbox,
		ClientActiveObject *&selected_object)
{
	PointedThing result;

	hilightbox = core::aabbox3d<f32>(0,0,0,0,0,0);
	should_show_hilightbox = false;
	selected_object = NULL;

	INodeDefManager *nodedef = client->getNodeDefManager();

	// First try to find a pointed at active object
	if(look_for_object)
	{
		selected_object = client->getSelectedActiveObject(d*BS,
				camera_position, shootline);
	}
	if(selected_object != NULL)
	{
		core::aabbox3d<f32> *selection_box
			= selected_object->getSelectionBox();
		// Box should exist because object was returned in the
		// first place
		assert(selection_box);

		v3f pos = selected_object->getPosition();

		hilightbox = core::aabbox3d<f32>(
				selection_box->MinEdge + pos,
				selection_box->MaxEdge + pos
		);

		should_show_hilightbox = selected_object->doShowSelectionBox();

		result.type = POINTEDTHING_OBJECT;
		result.object_id = selected_object->getId();
		return result;
	}

	// That didn't work, try to find a pointed at node

	f32 mindistance = BS * 1001;
	
	v3s16 pos_i = floatToInt(player_position, BS);

	/*infostream<<"pos_i=("<<pos_i.X<<","<<pos_i.Y<<","<<pos_i.Z<<")"
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
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}
		if(!isPointableNode(n, client, liquids_pointable))
			continue;

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
		
		const ContentFeatures &f = nodedef->get(n);
		
		if(f.selection_box.type == NODEBOX_FIXED)
		{
			core::aabbox3d<f32> box = f.selection_box.fixed;
			box.MinEdge += npf;
			box.MaxEdge += npf;

			v3s16 facedirs[6] = {
				v3s16(-1,0,0),
				v3s16(1,0,0),
				v3s16(0,-1,0),
				v3s16(0,1,0),
				v3s16(0,0,-1),
				v3s16(0,0,1),
			};

			core::aabbox3d<f32> faceboxes[6] = {
				// X-
				core::aabbox3d<f32>(
					box.MinEdge.X, box.MinEdge.Y, box.MinEdge.Z,
					box.MinEdge.X+d, box.MaxEdge.Y, box.MaxEdge.Z
				),
				// X+
				core::aabbox3d<f32>(
					box.MaxEdge.X-d, box.MinEdge.Y, box.MinEdge.Z,
					box.MaxEdge.X, box.MaxEdge.Y, box.MaxEdge.Z
				),
				// Y-
				core::aabbox3d<f32>(
					box.MinEdge.X, box.MinEdge.Y, box.MinEdge.Z,
					box.MaxEdge.X, box.MinEdge.Y+d, box.MaxEdge.Z
				),
				// Y+
				core::aabbox3d<f32>(
					box.MinEdge.X, box.MaxEdge.Y-d, box.MinEdge.Z,
					box.MaxEdge.X, box.MaxEdge.Y, box.MaxEdge.Z
				),
				// Z-
				core::aabbox3d<f32>(
					box.MinEdge.X, box.MinEdge.Y, box.MinEdge.Z,
					box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z+d
				),
				// Z+
				core::aabbox3d<f32>(
					box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z-d,
					box.MaxEdge.X, box.MaxEdge.Y, box.MaxEdge.Z
				),
			};

			for(u16 i=0; i<6; i++)
			{
				v3f facedir_f(facedirs[i].X, facedirs[i].Y, facedirs[i].Z);
				v3f centerpoint = npf + facedir_f * BS/2;
				f32 distance = (centerpoint - camera_position).getLength();
				if(distance >= mindistance)
					continue;
				if(!faceboxes[i].intersectsWithLine(shootline))
					continue;
				result.type = POINTEDTHING_NODE;
				result.node_undersurface = np;
				result.node_abovesurface = np+facedirs[i];
				mindistance = distance;
				hilightbox = box;
				should_show_hilightbox = true;
			}
		}
		else if(f.selection_box.type == NODEBOX_WALLMOUNTED)
		{
			v3s16 dir = n.getWallMountedDir(nodedef);
			v3f dir_f = v3f(dir.X, dir.Y, dir.Z);
			dir_f *= BS/2 - BS/6 - BS/20;
			v3f cpf = npf + dir_f;
			f32 distance = (cpf - camera_position).getLength();

			core::aabbox3d<f32> box;
			
			// top
			if(dir == v3s16(0,1,0)){
				box = f.selection_box.wall_top;
			}
			// bottom
			else if(dir == v3s16(0,-1,0)){
				box = f.selection_box.wall_bottom;
			}
			// side
			else{
				v3f vertices[2] =
				{
					f.selection_box.wall_side.MinEdge,
					f.selection_box.wall_side.MaxEdge
				};

				for(s32 i=0; i<2; i++)
				{
					if(dir == v3s16(-1,0,0))
						vertices[i].rotateXZBy(0);
					if(dir == v3s16(1,0,0))
						vertices[i].rotateXZBy(180);
					if(dir == v3s16(0,0,-1))
						vertices[i].rotateXZBy(90);
					if(dir == v3s16(0,0,1))
						vertices[i].rotateXZBy(-90);
				}

				box = core::aabbox3d<f32>(vertices[0]);
				box.addInternalPoint(vertices[1]);
			}

			box.MinEdge += npf;
			box.MaxEdge += npf;
			
			if(distance < mindistance)
			{
				if(box.intersectsWithLine(shootline))
				{
					result.type = POINTEDTHING_NODE;
					result.node_undersurface = np;
					result.node_abovesurface = np;
					mindistance = distance;
					hilightbox = box;
					should_show_hilightbox = true;
				}
			}
		}
		else // NODEBOX_REGULAR
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
						result.type = POINTEDTHING_NODE;
						result.node_undersurface = np;
						result.node_abovesurface = np + dirs[i];
						mindistance = distance;

						//hilightbox = facebox;

						const float d = 0.502;
						core::aabbox3d<f32> nodebox
								(-BS*d, -BS*d, -BS*d, BS*d, BS*d, BS*d);
						v3f nodepos_f = intToFloat(np, BS);
						nodebox.MinEdge += nodepos_f;
						nodebox.MaxEdge += nodepos_f;
						hilightbox = nodebox;
						should_show_hilightbox = true;
					}
				} // if distance < mindistance
			} // for dirs
		} // regular block
	} // for coords

	return result;
}

void update_skybox(video::IVideoDriver* driver, ITextureSource *tsrc,
		scene::ISceneManager* smgr, scene::ISceneNode* &skybox,
		float brightness)
{
	if(skybox)
	{
		skybox->remove();
	}
	
	/*// Disable skybox if FarMesh is enabled
	if(g_settings->getBool("enable_farmesh"))
		return;*/
	
	if(brightness >= 0.7)
	{
		skybox = smgr->addSkyBoxSceneNode(
			tsrc->getTextureRaw("skybox2.png"),
			tsrc->getTextureRaw("skybox3.png"),
			tsrc->getTextureRaw("skybox1.png"),
			tsrc->getTextureRaw("skybox1.png"),
			tsrc->getTextureRaw("skybox1.png"),
			tsrc->getTextureRaw("skybox1.png"));
	}
	else if(brightness >= 0.2)
	{
		skybox = smgr->addSkyBoxSceneNode(
			tsrc->getTextureRaw("skybox2_dawn.png"),
			tsrc->getTextureRaw("skybox3_dawn.png"),
			tsrc->getTextureRaw("skybox1_dawn.png"),
			tsrc->getTextureRaw("skybox1_dawn.png"),
			tsrc->getTextureRaw("skybox1_dawn.png"),
			tsrc->getTextureRaw("skybox1_dawn.png"));
	}
	else
	{
		skybox = smgr->addSkyBoxSceneNode(
			tsrc->getTextureRaw("skybox2_night.png"),
			tsrc->getTextureRaw("skybox3_night.png"),
			tsrc->getTextureRaw("skybox1_night.png"),
			tsrc->getTextureRaw("skybox1_night.png"),
			tsrc->getTextureRaw("skybox1_night.png"),
			tsrc->getTextureRaw("skybox1_night.png"));
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

/* Profiler display */

void update_profiler_gui(gui::IGUIStaticText *guitext_profiler,
		gui::IGUIFont *font, u32 text_height,
		u32 show_profiler, u32 show_profiler_max)
{
	if(show_profiler == 0)
	{
		guitext_profiler->setVisible(false);
	}
	else
	{

		std::ostringstream os(std::ios_base::binary);
		g_profiler->printPage(os, show_profiler, show_profiler_max);
		std::wstring text = narrow_to_wide(os.str());
		guitext_profiler->setText(text.c_str());
		guitext_profiler->setVisible(true);

		s32 w = font->getDimension(text.c_str()).Width;
		if(w < 400)
			w = 400;
		core::rect<s32> rect(6, 4+(text_height+5)*2, 12+w,
				8+(text_height+5)*2 +
				font->getDimension(text.c_str()).Height);
		guitext_profiler->setRelativePosition(rect);
		guitext_profiler->setVisible(true);
	}
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
	std::wstring &error_message,
    std::string configpath
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

	draw_load_screen(L"Loading...", driver, font);
	
	// Create texture source
	IWritableTextureSource *tsrc = createTextureSource(device);
	
	// These will be filled by data received from the server
	// Create item definition manager
	IWritableItemDefManager *itemdef = createItemDefManager();
	// Create node definition manager
	IWritableNodeDefManager *nodedef = createNodeDefManager();

	// Add chat log output for errors to be shown in chat
	LogOutputBuffer chat_log_error_buf(LMT_ERROR);

	/*
		Create server.
		SharedPtr will delete it when it goes out of scope.
	*/
	SharedPtr<Server> server;
	if(address == ""){
		draw_load_screen(L"Creating server...", driver, font);
		infostream<<"Creating server"<<std::endl;
		server = new Server(map_dir, configpath);
		server->start(port);
	}

	{ // Client scope
	
	/*
		Create client
	*/

	draw_load_screen(L"Creating client...", driver, font);
	infostream<<"Creating client"<<std::endl;
	
	MapDrawControl draw_control;

	Client client(device, playername.c_str(), password, draw_control,
			tsrc, itemdef, nodedef);
	
	// Client acts as our GameDef
	IGameDef *gamedef = &client;
			
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
		errorstream<<"Couldn't resolve address"<<std::endl;
		//return 0;
		error_message = L"Couldn't resolve address";
		//gui_loadingtext->remove();
		return;
	}

	/*
		Attempt to connect to the server
	*/
	
	infostream<<"Connecting to server at ";
	connect_address.print(&infostream);
	infostream<<std::endl;
	client.connect(connect_address);
	
	/*
		Wait for server to accept connection
	*/
	bool could_connect = false;
	try{
		float frametime = 0.033;
		const float timeout = 10.0;
		float time_counter = 0.0;
		for(;;)
		{
			// Update client and server
			client.step(frametime);
			if(server != NULL)
				server->step(frametime);
			
			// End condition
			if(client.connectedAndInitialized()){
				could_connect = true;
				break;
			}
			// Break conditions
			if(client.accessDenied())
				break;
			if(time_counter >= timeout)
				break;
			
			// Display status
			std::wostringstream ss;
			ss<<L"Connecting to server... (timeout in ";
			ss<<(int)(timeout - time_counter + 1.0);
			ss<<L" seconds)";
			draw_load_screen(ss.str(), driver, font);
			
			// Delay a bit
			sleep_ms(1000*frametime);
			time_counter += frametime;
		}
	}
	catch(con::PeerNotFoundException &e)
	{}
	
	/*
		Handle failure to connect
	*/
	if(could_connect == false)
	{
		if(client.accessDenied())
		{
			error_message = L"Access denied. Reason: "
					+client.accessDeniedReason();
			errorstream<<wide_to_narrow(error_message)<<std::endl;
		}
		else
		{
			error_message = L"Connection timed out.";
			errorstream<<"Timed out."<<std::endl;
		}
		//gui_loadingtext->remove();
		return;
	}
	
	/*
		Wait until content has been received
	*/
	bool got_content = false;
	{
		float frametime = 0.033;
		const float timeout = 30.0;
		float time_counter = 0.0;
		for(;;)
		{
			// Update client and server
			client.step(frametime);
			if(server != NULL)
				server->step(frametime);
			
			// End condition
			if(client.texturesReceived() &&
					client.itemdefReceived() &&
					client.nodedefReceived()){
				got_content = true;
				break;
			}
			// Break conditions
			if(!client.connectedAndInitialized())
				break;
			if(time_counter >= timeout)
				break;
			
			// Display status
			std::wostringstream ss;
			ss<<L"Waiting content... (continuing anyway in ";
			ss<<(int)(timeout - time_counter + 1.0);
			ss<<L" seconds)\n";

			ss<<(client.itemdefReceived()?L"[X]":L"[  ]");
			ss<<L" Item definitions\n";
			ss<<(client.nodedefReceived()?L"[X]":L"[  ]");
			ss<<L" Node definitions\n";
			//ss<<(client.texturesReceived()?L"[X]":L"[  ]");
			ss<<L"["<<(int)(client.textureReceiveProgress()*100+0.5)<<L"%] ";
			ss<<L" Textures\n";

			draw_load_screen(ss.str(), driver, font);
			
			// Delay a bit
			sleep_ms(1000*frametime);
			time_counter += frametime;
		}
	}

	/*
		After all content has been received:
		Update cached textures, meshes and materials
	*/
	client.afterContentReceived();

	/*
		Create skybox
	*/
	float old_brightness = 1.0;
	scene::ISceneNode* skybox = NULL;
	update_skybox(driver, tsrc, smgr, skybox, 1.0);
	
	/*
		Create the camera node
	*/
	Camera camera(smgr, draw_control);
	if (!camera.successfullyCreated(error_message))
		return;

	f32 camera_yaw = 0; // "right/left"
	f32 camera_pitch = 0; // "up/down"

	/*
		Clouds
	*/
	
	float cloud_height = BS*100;
	Clouds *clouds = NULL;
	if(g_settings->getBool("enable_clouds"))
	{
		clouds = new Clouds(smgr->getRootSceneNode(), smgr, -1,
				cloud_height, time(0));
	}
	
	/*
		FarMesh
	*/

	FarMesh *farmesh = NULL;
	if(g_settings->getBool("enable_farmesh"))
	{
		farmesh = new FarMesh(smgr->getRootSceneNode(), smgr, -1, client.getMapSeed(), &client);
	}

	/*
		A copy of the local inventory
	*/
	Inventory local_inventory(itemdef);

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
	
	// Status text (displays info when showing and hiding GUI stuff, etc.)
	gui::IGUIStaticText *guitext_status = guienv->addStaticText(
			L"<Status>",
			core::rect<s32>(0,0,0,0),
			false, false);
	guitext_status->setVisible(false);
	
	std::wstring statustext;
	float statustext_time = 0;
	
	// Chat text
	gui::IGUIStaticText *guitext_chat = guienv->addStaticText(
			L"",
			core::rect<s32>(0,0,0,0),
			//false, false); // Disable word wrap as of now
			false, true);
	//guitext_chat->setBackgroundColor(video::SColor(96,0,0,0));
	core::list<ChatLine> chat_lines;
	
	// Profiler text (size is updated when text is updated)
	gui::IGUIStaticText *guitext_profiler = guienv->addStaticText(
			L"<Profiler>",
			core::rect<s32>(0,0,0,0),
			false, false);
	guitext_profiler->setBackgroundColor(video::SColor(80,0,0,0));
	guitext_profiler->setVisible(false);
	
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

	float brightness = 1.0;

	core::list<float> frametime_log;

	float nodig_delay_timer = 0.0;
	float dig_time = 0.0;
	u16 dig_index = 0;
	PointedThing pointed_old;
	bool digging = false;
	bool ldown_for_dig = false;

	float damage_flash_timer = 0;
	s16 farmesh_range = 20*MAP_BLOCKSIZE;

	const float object_hit_delay = 0.2;
	float object_hit_delay_timer = 0.0;
	
	bool invert_mouse = g_settings->getBool("invert_mouse");

	bool respawn_menu_active = false;
	bool update_wielded_item_trigger = false;

	bool show_hud = true;
	bool show_chat = true;
	bool force_fog_off = false;
	bool disable_camera_update = false;
	bool show_debug = g_settings->getBool("show_debug");
	bool show_debug_frametime = false;
	u32 show_profiler = 0;
	u32 show_profiler_max = 3;  // Number of pages

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

		if(client.accessDenied())
		{
			error_message = L"Access denied. Reason: "
					+client.accessDeniedReason();
			errorstream<<wide_to_narrow(error_message)<<std::endl;
			break;
		}

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
		tsrc->processQueue();

		/*
			Random calculations
		*/
		last_screensize = screensize;
		screensize = driver->getScreenSize();
		v2s32 displaycenter(screensize.X/2,screensize.Y/2);
		//bool screensize_changed = screensize != last_screensize;

		// Resize hotbar
		if(screensize.Y <= 800)
			hotbar_imagesize = 32;
		else if(screensize.Y <= 1280)
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

		//infostream<<"busytime_u32="<<busytime_u32<<std::endl;
	
		// Necessary for device->getTimer()->getTime()
		device->run();

		/*
			FPS limiter
		*/

		{
			float fps_max = g_settings->getFloat("fps_max");
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

		/* Run timers */

		if(nodig_delay_timer >= 0)
			nodig_delay_timer -= dtime;
		if(object_hit_delay_timer >= 0)
			object_hit_delay_timer -= dtime;

		g_profiler->add("Elapsed time", dtime);
		g_profiler->avg("FPS", 1./dtime);

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
			infostream<<"X";
		infostream<<std::endl;*/

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
				client.printDebugInfo(infostream);
			}
		}

		/*
			Profiler
		*/
		float profiler_print_interval =
				g_settings->getFloat("profiler_print_interval");
		bool print_to_log = true;
		if(profiler_print_interval == 0){
			print_to_log = false;
			profiler_print_interval = 5;
		}
		if(m_profiler_interval.step(dtime, profiler_print_interval))
		{
			if(print_to_log){
				infostream<<"Profiler:"<<std::endl;
				g_profiler->print(infostream);
			}

			update_profiler_gui(guitext_profiler, font, text_height,
					show_profiler, show_profiler_max);

			g_profiler->clear();
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
			Launch menus and trigger stuff according to keys
		*/
		if(input->wasKeyDown(getKeySetting("keymap_drop")))
		{
			// drop selected item
			IDropAction *a = new IDropAction();
			a->count = 0;
			a->from_inv.setCurrentPlayer();
			a->from_list = "main";
			a->from_i = client.getPlayerItem();
			client.inventoryAction(a);
		}
		else if(input->wasKeyDown(getKeySetting("keymap_inventory")))
		{
			infostream<<"the_game: "
					<<"Launching inventory"<<std::endl;
			
			GUIInventoryMenu *menu =
				new GUIInventoryMenu(guienv, guiroot, -1,
					&g_menumgr, v2s16(8,7),
					&client, gamedef);

			InventoryLocation inventoryloc;
			inventoryloc.setCurrentPlayer();

			core::array<GUIInventoryMenu::DrawSpec> draw_spec;
			draw_spec.push_back(GUIInventoryMenu::DrawSpec(
					"list", inventoryloc, "main",
					v2s32(0, 3), v2s32(8, 4)));
			draw_spec.push_back(GUIInventoryMenu::DrawSpec(
					"list", inventoryloc, "craft",
					v2s32(3, 0), v2s32(3, 3)));
			draw_spec.push_back(GUIInventoryMenu::DrawSpec(
					"list", inventoryloc, "craftpreview",
					v2s32(7, 1), v2s32(1, 1)));

			menu->setDrawSpec(draw_spec);

			menu->drop();
		}
		else if(input->wasKeyDown(EscapeKey))
		{
			infostream<<"the_game: "
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
		else if(input->wasKeyDown(getKeySetting("keymap_cmd")))
		{
			TextDest *dest = new TextDestChat(&client);

			(new GUITextInputMenu(guienv, guiroot, -1,
					&g_menumgr, dest,
					L"/"))->drop();
		}
		else if(input->wasKeyDown(getKeySetting("keymap_freemove")))
		{
			if(g_settings->getBool("free_move"))
			{
				g_settings->set("free_move","false");
				statustext = L"free_move disabled";
				statustext_time = 0;
			}
			else
			{
				g_settings->set("free_move","true");
				statustext = L"free_move enabled";
				statustext_time = 0;
			}
		}
		else if(input->wasKeyDown(getKeySetting("keymap_fastmove")))
		{
			if(g_settings->getBool("fast_move"))
			{
				g_settings->set("fast_move","false");
				statustext = L"fast_move disabled";
				statustext_time = 0;
			}
			else
			{
				g_settings->set("fast_move","true");
				statustext = L"fast_move enabled";
				statustext_time = 0;
			}
		}
		else if(input->wasKeyDown(getKeySetting("keymap_screenshot")))
		{
			irr::video::IImage* const image = driver->createScreenShot(); 
			if (image) { 
				irr::c8 filename[256]; 
				snprintf(filename, 256, "%s" DIR_DELIM "screenshot_%u.png", 
						 g_settings->get("screenshot_path").c_str(),
						 device->getTimer()->getRealTime()); 
				if (driver->writeImageToFile(image, filename)) {
					std::wstringstream sstr;
					sstr<<"Saved screenshot to '"<<filename<<"'";
					infostream<<"Saved screenshot to '"<<filename<<"'"<<std::endl;
					statustext = sstr.str();
					statustext_time = 0;
				} else{
					infostream<<"Failed to save screenshot '"<<filename<<"'"<<std::endl;
				}
				image->drop(); 
			}			 
		}
		else if(input->wasKeyDown(getKeySetting("keymap_toggle_hud")))
		{
			show_hud = !show_hud;
			if(show_hud)
				statustext = L"HUD shown";
			else
				statustext = L"HUD hidden";
			statustext_time = 0;
		}
		else if(input->wasKeyDown(getKeySetting("keymap_toggle_chat")))
		{
			show_chat = !show_chat;
			if(show_chat)
				statustext = L"Chat shown";
			else
				statustext = L"Chat hidden";
			statustext_time = 0;
		}
		else if(input->wasKeyDown(getKeySetting("keymap_toggle_force_fog_off")))
		{
			force_fog_off = !force_fog_off;
			if(force_fog_off)
				statustext = L"Fog disabled";
			else
				statustext = L"Fog enabled";
			statustext_time = 0;
		}
		else if(input->wasKeyDown(getKeySetting("keymap_toggle_update_camera")))
		{
			disable_camera_update = !disable_camera_update;
			if(disable_camera_update)
				statustext = L"Camera update disabled";
			else
				statustext = L"Camera update enabled";
			statustext_time = 0;
		}
		else if(input->wasKeyDown(getKeySetting("keymap_toggle_debug")))
		{
			// Initial / 3x toggle: Chat only
			// 1x toggle: Debug text with chat
			// 2x toggle: Debug text with frametime
			if(!show_debug)
			{
				show_debug = true;
				show_debug_frametime = false;
				statustext = L"Debug info shown";
				statustext_time = 0;
			}
			else if(show_debug_frametime)
			{
				show_debug = false;
				show_debug_frametime = false;
				statustext = L"Debug info and frametime graph hidden";
				statustext_time = 0;
			}
			else
			{
				show_debug_frametime = true;
				statustext = L"Frametime graph shown";
				statustext_time = 0;
			}
		}
		else if(input->wasKeyDown(getKeySetting("keymap_toggle_profiler")))
		{
			show_profiler = (show_profiler + 1) % (show_profiler_max + 1);

			// FIXME: This updates the profiler with incomplete values
			update_profiler_gui(guitext_profiler, font, text_height,
					show_profiler, show_profiler_max);

			if(show_profiler != 0)
			{
				std::wstringstream sstr;
				sstr<<"Profiler shown (page "<<show_profiler
					<<" of "<<show_profiler_max<<")";
				statustext = sstr.str();
				statustext_time = 0;
			}
			else
			{
				statustext = L"Profiler hidden";
				statustext_time = 0;
			}
		}
		else if(input->wasKeyDown(getKeySetting("keymap_increase_viewing_range_min")))
		{
			s16 range = g_settings->getS16("viewing_range_nodes_min");
			s16 range_new = range + 10;
			g_settings->set("viewing_range_nodes_min", itos(range_new));
			statustext = narrow_to_wide(
					"Minimum viewing range changed to "
					+ itos(range_new));
			statustext_time = 0;
		}
		else if(input->wasKeyDown(getKeySetting("keymap_decrease_viewing_range_min")))
		{
			s16 range = g_settings->getS16("viewing_range_nodes_min");
			s16 range_new = range - 10;
			if(range_new < 0)
				range_new = range;
			g_settings->set("viewing_range_nodes_min",
					itos(range_new));
			statustext = narrow_to_wide(
					"Minimum viewing range changed to "
					+ itos(range_new));
			statustext_time = 0;
		}

		// Item selection with mouse wheel
		u16 new_playeritem = client.getPlayerItem();
		{
			s32 wheel = input->getMouseWheel();
			u16 max_item = MYMIN(PLAYER_INVENTORY_SIZE-1,
					hotbar_itemcount-1);

			if(wheel < 0)
			{
				if(new_playeritem < max_item)
					new_playeritem++;
				else
					new_playeritem = 0;
			}
			else if(wheel > 0)
			{
				if(new_playeritem > 0)
					new_playeritem--;
				else
					new_playeritem = max_item;
			}
		}
		
		// Item selection
		for(u16 i=0; i<10; i++)
		{
			const KeyPress *kp = NumberKey + (i + 1) % 10;
			if(input->wasKeyDown(*kp))
			{
				if(i < PLAYER_INVENTORY_SIZE && i < hotbar_itemcount)
				{
					new_playeritem = i;

					infostream<<"Selected item: "
							<<new_playeritem<<std::endl;
				}
			}
		}

		// Viewing range selection
		if(input->wasKeyDown(getKeySetting("keymap_rangeselect")))
		{
			draw_control.range_all = !draw_control.range_all;
			if(draw_control.range_all)
			{
				infostream<<"Enabled full viewing range"<<std::endl;
				statustext = L"Enabled full viewing range";
				statustext_time = 0;
			}
			else
			{
				infostream<<"Disabled full viewing range"<<std::endl;
				statustext = L"Disabled full viewing range";
				statustext_time = 0;
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
			Mouse and camera control
			NOTE: Do this before client.setPlayerControl() to not cause a camera lag of one frame
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
				//infostream<<"window active, first loop"<<std::endl;
				first_loop_after_window_activation = false;
			}
			else{
				s32 dx = input->getMousePos().X - displaycenter.X;
				s32 dy = input->getMousePos().Y - displaycenter.Y;
				if(invert_mouse)
					dy = -dy;
				//infostream<<"window active, pos difference "<<dx<<","<<dy<<std::endl;
				
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

			//infostream<<"window inactive"<<std::endl;
			first_loop_after_window_activation = true;
		}

		/*
			Player speed control
		*/
		
		if(!noMenuActive() || !device->isWindowActive())
		{
			PlayerControl control(
				false,
				false,
				false,
				false,
				false,
				false,
				false,
				camera_pitch,
				camera_yaw
			);
			client.setPlayerControl(control);
		}
		else
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

		{
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
					//infostream<<"Player damage: "<<damage<<std::endl;
					damage_flash_timer = 0.05;
					if(event.player_damage.amount >= 2){
						damage_flash_timer += 0.05 * event.player_damage.amount;
					}
				}
				else if(event.type == CE_PLAYER_FORCE_MOVE)
				{
					camera_yaw = event.player_force_move.yaw;
					camera_pitch = event.player_force_move.pitch;
				}
				else if(event.type == CE_DEATHSCREEN)
				{
					if(respawn_menu_active)
						continue;

					/*bool set_camera_point_target =
							event.deathscreen.set_camera_point_target;
					v3f camera_point_target;
					camera_point_target.X = event.deathscreen.camera_point_target_x;
					camera_point_target.Y = event.deathscreen.camera_point_target_y;
					camera_point_target.Z = event.deathscreen.camera_point_target_z;*/
					MainRespawnInitiator *respawner =
							new MainRespawnInitiator(
									&respawn_menu_active, &client);
					GUIDeathScreen *menu =
							new GUIDeathScreen(guienv, guiroot, -1, 
								&g_menumgr, respawner);
					menu->drop();
					
					/* Handle visualization */

					damage_flash_timer = 0;

					/*LocalPlayer* player = client.getLocalPlayer();
					player->setPosition(player->getPosition() + v3f(0,-BS,0));
					camera.update(player, busytime, screensize);*/
				}
				else if(event.type == CE_TEXTURES_UPDATED)
				{
					update_skybox(driver, tsrc, smgr, skybox, brightness);
					
					update_wielded_item_trigger = true;
				}
			}
		}
		
		//TimeTaker //timer2("//timer2");

		LocalPlayer* player = client.getLocalPlayer();
		camera.update(player, busytime, screensize);
		camera.step(dtime);

		v3f player_position = player->getPosition();
		v3f camera_position = camera.getPosition();
		v3f camera_direction = camera.getDirection();
		f32 camera_fov = camera.getFovMax();
		
		if(!disable_camera_update){
			client.updateCamera(camera_position,
				camera_direction, camera_fov);
		}

		//timer2.stop();
		//TimeTaker //timer3("//timer3");

		/*
			For interaction purposes, get info about the held item
			- What item is it?
			- Is it a usable item?
			- Can it point to liquids?
		*/
		ItemStack playeritem;
		bool playeritem_usable = false;
		bool playeritem_liquids_pointable = false;
		{
			InventoryList *mlist = local_inventory.getList("main");
			if(mlist != NULL)
			{
				playeritem = mlist->getItem(client.getPlayerItem());
				playeritem_usable = playeritem.getDefinition(itemdef).usable;
				playeritem_liquids_pointable = playeritem.getDefinition(itemdef).liquids_pointable;
			}
		}

		/*
			Calculate what block is the crosshair pointing to
		*/
		
		//u32 t1 = device->getTimer()->getRealTime();
		
		f32 d = 4; // max. distance
		core::line3d<f32> shootline(camera_position,
				camera_position + camera_direction * BS * (d+1));

		core::aabbox3d<f32> hilightbox;
		bool should_show_hilightbox = false;
		ClientActiveObject *selected_object = NULL;

		PointedThing pointed = getPointedThing(
				// input
				&client, player_position, camera_direction,
				camera_position, shootline, d,
				playeritem_liquids_pointable, !ldown_for_dig,
				// output
				hilightbox, should_show_hilightbox,
				selected_object);

		if(pointed != pointed_old)
		{
			infostream<<"Pointing at "<<pointed.dump()<<std::endl;
			//dstream<<"Pointing at "<<pointed.dump()<<std::endl;
		}

		/*
			Visualize selection
		*/
		if(should_show_hilightbox)
			hilightboxes.push_back(hilightbox);

		/*
			Stop digging when
			- releasing left mouse button
			- pointing away from node
		*/
		if(digging)
		{
			if(input->getLeftReleased())
			{
				infostream<<"Left button released"
					<<" (stopped digging)"<<std::endl;
				digging = false;
			}
			else if(pointed != pointed_old)
			{
				if (pointed.type == POINTEDTHING_NODE
					&& pointed_old.type == POINTEDTHING_NODE
					&& pointed.node_undersurface == pointed_old.node_undersurface)
				{
					// Still pointing to the same node,
					// but a different face. Don't reset.
				}
				else
				{
					infostream<<"Pointing away from node"
						<<" (stopped digging)"<<std::endl;
					digging = false;
				}
			}
			if(!digging)
			{
				client.interact(1, pointed_old);
				client.clearTempMod(pointed_old.node_undersurface);
				dig_time = 0.0;
			}
		}
		if(!digging && ldown_for_dig && !input->getLeftState())
		{
			ldown_for_dig = false;
		}

		bool left_punch = false;
		bool left_punch_muted = false;

		if(playeritem_usable && input->getLeftState())
		{
			if(input->getLeftClicked())
				client.interact(4, pointed);
		}
		else if(pointed.type == POINTEDTHING_NODE)
		{
			v3s16 nodepos = pointed.node_undersurface;
			v3s16 neighbourpos = pointed.node_abovesurface;

			/*
				Check information text of node
			*/

			NodeMetadata *meta = client.getNodeMetadata(nodepos);
			if(meta)
			{
				infotext = narrow_to_wide(meta->infoText());
			}
			else
			{
				MapNode n = client.getNode(nodepos);
				if(nodedef->get(n).tname_tiles[0] == "unknown_block.png"){
					infotext = L"Unknown node: ";
					infotext += narrow_to_wide(nodedef->get(n).name);
				}
			}
			
			/*
				Handle digging
			*/
			
			
			if(nodig_delay_timer <= 0.0 && input->getLeftState())
			{
				if(!digging)
				{
					infostream<<"Started digging"<<std::endl;
					client.interact(0, pointed);
					digging = true;
					ldown_for_dig = true;
				}
				MapNode n = client.getNode(nodepos);

				// Get digging properties for material and tool
				MaterialProperties mp = nodedef->get(n.getContent()).material;
				ToolDiggingProperties tp =
						playeritem.getToolDiggingProperties(itemdef);
				DiggingProperties prop = getDiggingProperties(&mp, &tp);

				float dig_time_complete = 0.0;

				if(prop.diggable == false)
				{
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
					//infostream<<"dig_index="<<dig_index<<std::endl;
					client.setTempMod(nodepos, NodeMod(NODEMOD_CRACK, dig_index));
				}
				else
				{
					infostream<<"Digging completed"<<std::endl;
					client.interact(2, pointed);
					client.clearTempMod(nodepos);
					client.removeNode(nodepos);

					dig_time = 0;
					digging = false;

					nodig_delay_timer = dig_time_complete
							/ (float)CRACK_ANIMATION_LENGTH;

					// We don't want a corresponding delay to
					// very time consuming nodes
					if(nodig_delay_timer > 0.5)
					{
						nodig_delay_timer = 0.5;
					}
					// We want a slight delay to very little
					// time consuming nodes
					float mindelay = 0.15;
					if(nodig_delay_timer < mindelay)
					{
						nodig_delay_timer = mindelay;
					}
				}

				dig_time += dtime;

				camera.setDigging(0);  // left click animation
			}

			if(input->getRightClicked())
			{
				infostream<<"Ground right-clicked"<<std::endl;
				
				// If metadata provides an inventory view, activate it
				if(meta && meta->getInventoryDrawSpecString() != "" && !random_input)
				{
					infostream<<"Launching custom inventory view"<<std::endl;

					InventoryLocation inventoryloc;
					inventoryloc.setNodeMeta(nodepos);
					

					/*
						Create menu
					*/

					core::array<GUIInventoryMenu::DrawSpec> draw_spec;
					v2s16 invsize =
						GUIInventoryMenu::makeDrawSpecArrayFromString(
							draw_spec,
							meta->getInventoryDrawSpecString(),
							inventoryloc);

					GUIInventoryMenu *menu =
						new GUIInventoryMenu(guienv, guiroot, -1,
							&g_menumgr, invsize,
							&client, gamedef);
					menu->setDrawSpec(draw_spec);
					menu->drop();
				}
				// If metadata provides text input, activate text input
				else if(meta && meta->allowsTextInput() && !random_input)
				{
					infostream<<"Launching metadata text input"<<std::endl;
					
					// Get a new text for it

					TextDest *dest = new TextDestNodeMetadata(nodepos, &client);

					std::wstring wtext = narrow_to_wide(meta->getText());

					(new GUITextInputMenu(guienv, guiroot, -1,
							&g_menumgr, dest,
							wtext))->drop();
				}
				// Otherwise report right click to server
				else
				{
					client.interact(3, pointed);
					camera.setDigging(1);  // right click animation
				}
			}
		}
		else if(pointed.type == POINTEDTHING_OBJECT)
		{
			infotext = narrow_to_wide(selected_object->infoText());

			//if(input->getLeftClicked())
			if(input->getLeftState())
			{
				bool do_punch = false;
				bool do_punch_damage = false;
				if(object_hit_delay_timer <= 0.0){
					do_punch = true;
					do_punch_damage = true;
					object_hit_delay_timer = object_hit_delay;
				}
				if(input->getLeftClicked()){
					do_punch = true;
				}
				if(do_punch){
					infostream<<"Left-clicked object"<<std::endl;
					left_punch = true;
				}
				if(do_punch_damage){
					// Report direct punch
                		        v3f objpos = selected_object->getPosition();
		                        v3f dir = (objpos - player_position).normalize();

					bool disable_send = selected_object->directReportPunch(playeritem.name, dir);
					if(!disable_send)
						client.interact(0, pointed);
				}
			}
			else if(input->getRightClicked())
			{
				infostream<<"Right-clicked object"<<std::endl;
				client.interact(3, pointed);  // place
			}
		}

		pointed_old = pointed;
		
		if(left_punch || (input->getLeftClicked() && !left_punch_muted))
		{
			camera.setDigging(0); // left click animation
		}

		input->resetLeftClicked();
		input->resetRightClicked();

		input->resetLeftReleased();
		input->resetRightReleased();
		
		/*
			Calculate stuff for drawing
		*/
		
		/*
			Calculate general brightness
		*/
		u32 daynight_ratio = client.getDayNightRatio();
		u8 light8 = decode_light((daynight_ratio * LIGHT_SUN) / 1000);
		brightness = (float)light8/255.0;
		// Make night look good
		brightness = brightness * 1.15 - 0.15;
		video::SColor bgcolor;
		if(brightness >= 0.2 && brightness < 0.7)
			bgcolor = video::SColor(
					255,
					bgcolor_bright.getRed() * brightness,
					bgcolor_bright.getGreen() * brightness*0.7,
					bgcolor_bright.getBlue() * brightness*0.5);
		else
			bgcolor = video::SColor(
					255,
					bgcolor_bright.getRed() * brightness,
					bgcolor_bright.getGreen() * brightness,
					bgcolor_bright.getBlue() * brightness);

		/*
			Update skybox
		*/
		if(fabs(brightness - old_brightness) > 0.01)
			update_skybox(driver, tsrc, smgr, skybox, brightness);

		/*
			Update clouds
		*/
		if(clouds)
		{
			clouds->step(dtime);
			clouds->update(v2f(player_position.X, player_position.Z),
					brightness);
		}
		
		/*
			Update farmesh
		*/
		if(farmesh)
		{
			farmesh_range = draw_control.wanted_range * 10;
			if(draw_control.range_all && farmesh_range < 500)
				farmesh_range = 500;
			if(farmesh_range > 1000)
				farmesh_range = 1000;

			farmesh->step(dtime);
			farmesh->update(v2f(player_position.X, player_position.Z),
					brightness, farmesh_range);
		}
		
		// Store brightness value
		old_brightness = brightness;

		/*
			Fog
		*/
		
		if(g_settings->getBool("enable_fog") == true && !force_fog_off)
		{
			f32 range;
			if(farmesh)
			{
				range = BS*farmesh_range;
			}
			else
			{
				range = draw_control.wanted_range*BS + 0.0*MAP_BLOCKSIZE*BS;
				range *= 0.9;
				if(draw_control.range_all)
					range = 100000*BS;
				/*if(range < 50*BS)
					range = range * 0.5 + 25*BS;*/
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
		
		const char program_name_and_version[] =
			"Minetest-c55 " VERSION_STRING;

		if(show_debug)
		{
			static float drawtime_avg = 0;
			drawtime_avg = drawtime_avg * 0.95 + (float)drawtime*0.05;
			/*static float beginscenetime_avg = 0;
			beginscenetime_avg = beginscenetime_avg * 0.95 + (float)beginscenetime*0.05;
			static float scenetime_avg = 0;
			scenetime_avg = scenetime_avg * 0.95 + (float)scenetime*0.05;
			static float endscenetime_avg = 0;
			endscenetime_avg = endscenetime_avg * 0.95 + (float)endscenetime*0.05;*/
			
			char temptext[300];
			snprintf(temptext, 300, "%s ("
					"R: range_all=%i"
					")"
					" drawtime=%.0f, dtime_jitter = % .1f %%"
					", v_range = %.1f, RTT = %.3f",
					program_name_and_version,
					draw_control.range_all,
					drawtime_avg,
					dtime_jitter1_max_fraction * 100.0,
					draw_control.wanted_range,
					client.getRTT()
					);
			
			guitext->setText(narrow_to_wide(temptext).c_str());
			guitext->setVisible(true);
		}
		else if(show_hud || show_chat)
		{
			guitext->setText(narrow_to_wide(program_name_and_version).c_str());
			guitext->setVisible(true);
		}
		else
		{
			guitext->setVisible(false);
		}
		
		if(show_debug)
		{
			char temptext[300];
			snprintf(temptext, 300,
					"(% .1f, % .1f, % .1f)"
					" (yaw = %.1f)",
					player_position.X/BS,
					player_position.Y/BS,
					player_position.Z/BS,
					wrapDegrees_0_360(camera_yaw));

			guitext2->setText(narrow_to_wide(temptext).c_str());
			guitext2->setVisible(true);
		}
		else
		{
			guitext2->setVisible(false);
		}
		
		{
			guitext_info->setText(infotext.c_str());
			guitext_info->setVisible(show_hud);
		}

		{
			float statustext_time_max = 3.0;
			if(!statustext.empty())
			{
				statustext_time += dtime;
				if(statustext_time >= statustext_time_max)
				{
					statustext = L"";
					statustext_time = 0;
				}
			}
			guitext_status->setText(statustext.c_str());
			guitext_status->setVisible(!statustext.empty());

			if(!statustext.empty())
			{
				s32 status_y = screensize.Y - 130;
				core::rect<s32> rect(
						10,
						status_y - guitext_status->getTextHeight(),
						screensize.X - 10,
						status_y
				);
				guitext_status->setRelativePosition(rect);

				// Fade out
				video::SColor initial_color(255,0,0,0);
				if(guienv->getSkin())
					initial_color = guienv->getSkin()->getColor(gui::EGDC_BUTTON_TEXT);
				video::SColor final_color = initial_color;
				final_color.setAlpha(0);
				video::SColor fade_color =
					initial_color.getInterpolated_quadratic(
						initial_color,
						final_color,
						statustext_time / (float) statustext_time_max);
				guitext_status->setOverrideColor(fade_color);
				guitext_status->enableOverrideColor(true);
			}
		}
		
		/*
			Get chat messages from client
		*/
		{
			// Get new messages from error log buffer
			while(!chat_log_error_buf.empty())
			{
				chat_lines.push_back(ChatLine(narrow_to_wide(
						chat_log_error_buf.get())));
			}
			// Get new messages from client
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

			s32 chat_y = 5+(text_height+5);
			if(show_debug)
				chat_y += (text_height+5);
			core::rect<s32> rect(
					10,
					chat_y,
					screensize.X - 10,
					chat_y + guitext_chat->getTextHeight()
			);

			guitext_chat->setRelativePosition(rect);

			// Don't show chat if empty or profiler or debug is enabled
			guitext_chat->setVisible(chat_lines.size() != 0
					&& show_chat && show_profiler == 0);
		}

		/*
			Inventory
		*/
		
		if(client.getPlayerItem() != new_playeritem)
		{
			client.selectPlayerItem(new_playeritem);
		}
		if(client.getLocalInventoryUpdated())
		{
			//infostream<<"Updating local inventory"<<std::endl;
			client.getLocalInventory(local_inventory);
			
			update_wielded_item_trigger = true;
		}
		if(update_wielded_item_trigger)
		{
			update_wielded_item_trigger = false;
			// Update wielded tool
			InventoryList *mlist = local_inventory.getList("main");
			ItemStack item;
			if(mlist != NULL)
				item = mlist->getItem(client.getPlayerItem());
			camera.wield(item, gamedef);
		}
		
		/*
			Drawing begins
		*/

		TimeTaker drawtimer("Drawing");

		
		{
			TimeTaker timer("beginScene");
			driver->beginScene(false, true, bgcolor);
			//driver->beginScene(false, true, bgcolor);
			beginscenetime = timer.stop(true);
		}
		
		//timer3.stop();
		
		//infostream<<"smgr->drawAll()"<<std::endl;
		
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

		if(show_hud)
		{
			for(core::list<aabb3f>::Iterator i=hilightboxes.begin();
					i != hilightboxes.end(); i++)
			{
				/*infostream<<"hilightbox min="
						<<"("<<i->MinEdge.X<<","<<i->MinEdge.Y<<","<<i->MinEdge.Z<<")"
						<<" max="
						<<"("<<i->MaxEdge.X<<","<<i->MaxEdge.Y<<","<<i->MaxEdge.Z<<")"
						<<std::endl;*/
				driver->draw3DBox(*i, video::SColor(255,0,0,0));
			}
		}

		/*
			Wielded tool
		*/
		if(show_hud)
		{
			// Warning: This clears the Z buffer.
			camera.drawWieldedTool();
		}

		/*
			Post effects
		*/
		{
			client.renderPostFx();
		}

		/*
			Frametime log
		*/
		if(show_debug_frametime)
		{
			s32 x = 10;
			s32 y = screensize.Y - 10;
			for(core::list<float>::Iterator
					i = frametime_log.begin();
					i != frametime_log.end();
					i++)
			{
				driver->draw2DLine(v2s32(x,y),
						v2s32(x,y-(*i)*1000),
						video::SColor(255,255,255,255));
				x++;
			}
		}

		/*
			Draw crosshair
		*/
		if(show_hud)
		{
			driver->draw2DLine(displaycenter - core::vector2d<s32>(10,0),
					displaycenter + core::vector2d<s32>(10,0),
					video::SColor(255,255,255,255));
			driver->draw2DLine(displaycenter - core::vector2d<s32>(0,10),
					displaycenter + core::vector2d<s32>(0,10),
					video::SColor(255,255,255,255));
		}

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
		if(show_hud)
		{
			draw_hotbar(driver, font, gamedef,
					v2s32(displaycenter.X, screensize.Y),
					hotbar_imagesize, hotbar_itemcount, &local_inventory,
					client.getHP(), client.getPlayerItem());
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

	} // Client scope (must be destructed before destructing *def and tsrc

	delete tsrc;
	delete nodedef;
	delete itemdef;
}


