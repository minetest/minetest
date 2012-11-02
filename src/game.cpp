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

#include "game.h"
#include "irrlichttypes_extrabloated.h"
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "client.h"
#include "server.h"
#include "guiPauseMenu.h"
#include "guiPasswordChange.h"
#include "guiFormSpecMenu.h"
#include "guiTextInputMenu.h"
#include "guiDeathScreen.h"
#include "tool.h"
#include "guiChatConsole.h"
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
#include "subgame.h"
#include "quicktune_shortcutter.h"
#include "clientmap.h"
#include "sky.h"
#include "sound.h"
#if USE_SOUND
	#include "sound_openal.h"
#endif
#include "event_manager.h"
#include <list>
#include "util/directiontables.h"

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
		m_client->typeChatMessage(text);
	}
	void gotText(std::map<std::string, std::string> fields)
	{
		m_client->typeChatMessage(narrow_to_wide(fields["text"]));
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
	// This is deprecated I guess? -celeron55
	void gotText(std::wstring text)
	{
		std::string ntext = wide_to_narrow(text);
		infostream<<"Submitting 'text' field of node at ("<<m_p.X<<","
				<<m_p.Y<<","<<m_p.Z<<"): "<<ntext<<std::endl;
		std::map<std::string, std::string> fields;
		fields["text"] = ntext;
		m_client->sendNodemetaFields(m_p, "", fields);
	}
	void gotText(std::map<std::string, std::string> fields)
	{
		m_client->sendNodemetaFields(m_p, "", fields);
	}

	v3s16 m_p;
	Client *m_client;
};

struct TextDestPlayerInventory : public TextDest
{
	TextDestPlayerInventory(Client *client)
	{
		m_client = client;
	}
	void gotText(std::map<std::string, std::string> fields)
	{
		m_client->sendInventoryFields("", fields);
	}

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

/* Form update callback */

class NodeMetadataFormSource: public IFormSource
{
public:
	NodeMetadataFormSource(ClientMap *map, v3s16 p):
		m_map(map),
		m_p(p)
	{
	}
	std::string getForm()
	{
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);
		if(!meta)
			return "";
		return meta->getString("formspec");
	}
	std::string resolveText(std::string str)
	{
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);
		if(!meta)
			return str;
		return meta->resolveString(str);
	}

	ClientMap *m_map;
	v3s16 m_p;
};

class PlayerInventoryFormSource: public IFormSource
{
public:
	PlayerInventoryFormSource(Client *client):
		m_client(client)
	{
	}
	std::string getForm()
	{
		LocalPlayer* player = m_client->getEnv().getLocalPlayer();
		return player->inventory_formspec;
	}

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
		std::vector<aabb3f> &hilightboxes,
		ClientActiveObject *&selected_object)
{
	PointedThing result;

	hilightboxes.clear();
	selected_object = NULL;

	INodeDefManager *nodedef = client->getNodeDefManager();
	ClientMap &map = client->getEnv().getClientMap();

	// First try to find a pointed at active object
	if(look_for_object)
	{
		selected_object = client->getSelectedActiveObject(d*BS,
				camera_position, shootline);

		if(selected_object != NULL)
		{
			if(selected_object->doShowSelectionBox())
			{
				aabb3f *selection_box = selected_object->getSelectionBox();
				// Box should exist because object was
				// returned in the first place
				assert(selection_box);

				v3f pos = selected_object->getPosition();
				hilightboxes.push_back(aabb3f(
						selection_box->MinEdge + pos,
						selection_box->MaxEdge + pos));
			}


			result.type = POINTEDTHING_OBJECT;
			result.object_id = selected_object->getId();
			return result;
		}
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
	
	// Prevent signed number overflow
	if(yend==32767)
		yend=32766;
	if(zend==32767)
		zend=32766;
	if(xend==32767)
		xend=32766;

	for(s16 y = ystart; y <= yend; y++)
	for(s16 z = zstart; z <= zend; z++)
	for(s16 x = xstart; x <= xend; x++)
	{
		MapNode n;
		try
		{
			n = map.getNode(v3s16(x,y,z));
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}
		if(!isPointableNode(n, client, liquids_pointable))
			continue;

		std::vector<aabb3f> boxes = n.getSelectionBoxes(nodedef);

		v3s16 np(x,y,z);
		v3f npf = intToFloat(np, BS);

		for(std::vector<aabb3f>::const_iterator
				i = boxes.begin();
				i != boxes.end(); i++)
		{
			aabb3f box = *i;
			box.MinEdge += npf;
			box.MaxEdge += npf;

			for(u16 j=0; j<6; j++)
			{
				v3s16 facedir = g_6dirs[j];
				aabb3f facebox = box;

				f32 d = 0.001*BS;
				if(facedir.X > 0)
					facebox.MinEdge.X = facebox.MaxEdge.X-d;
				else if(facedir.X < 0)
					facebox.MaxEdge.X = facebox.MinEdge.X+d;
				else if(facedir.Y > 0)
					facebox.MinEdge.Y = facebox.MaxEdge.Y-d;
				else if(facedir.Y < 0)
					facebox.MaxEdge.Y = facebox.MinEdge.Y+d;
				else if(facedir.Z > 0)
					facebox.MinEdge.Z = facebox.MaxEdge.Z-d;
				else if(facedir.Z < 0)
					facebox.MaxEdge.Z = facebox.MinEdge.Z+d;

				v3f centerpoint = facebox.getCenter();
				f32 distance = (centerpoint - camera_position).getLength();
				if(distance >= mindistance)
					continue;
				if(!facebox.intersectsWithLine(shootline))
					continue;

				v3s16 np_above = np + facedir;

				result.type = POINTEDTHING_NODE;
				result.node_undersurface = np;
				result.node_abovesurface = np_above;
				mindistance = distance;

				hilightboxes.clear();
				for(std::vector<aabb3f>::const_iterator
						i2 = boxes.begin();
						i2 != boxes.end(); i2++)
				{
					aabb3f box = *i2;
					box.MinEdge += npf + v3f(-d,-d,-d);
					box.MaxEdge += npf + v3f(d,d,d);
					hilightboxes.push_back(box);
				}
			}
		}
	} // for coords

	return result;
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

class ProfilerGraph
{
private:
	struct Piece{
		Profiler::GraphValues values;
	};
	struct Meta{
		float min;
		float max;
		video::SColor color;
		Meta(float initial=0, video::SColor color=
				video::SColor(255,255,255,255)):
			min(initial),
			max(initial),
			color(color)
		{}
	};
	std::list<Piece> m_log;
public:
	u32 m_log_max_size;

	ProfilerGraph():
		m_log_max_size(200)
	{}

	void put(const Profiler::GraphValues &values)
	{
		Piece piece;
		piece.values = values;
		m_log.push_back(piece);
		while(m_log.size() > m_log_max_size)
			m_log.erase(m_log.begin());
	}
	
	void draw(s32 x_left, s32 y_bottom, video::IVideoDriver *driver,
			gui::IGUIFont* font) const
	{
		std::map<std::string, Meta> m_meta;
		for(std::list<Piece>::const_iterator k = m_log.begin();
				k != m_log.end(); k++)
		{
			const Piece &piece = *k;
			for(Profiler::GraphValues::const_iterator i = piece.values.begin();
					i != piece.values.end(); i++){
				const std::string &id = i->first;
				const float &value = i->second;
				std::map<std::string, Meta>::iterator j =
						m_meta.find(id);
				if(j == m_meta.end()){
					m_meta[id] = Meta(value);
					continue;
				}
				if(value < j->second.min)
					j->second.min = value;
				if(value > j->second.max)
					j->second.max = value;
			}
		}

		// Assign colors
		static const video::SColor usable_colors[] = {
			video::SColor(255,255,100,100),
			video::SColor(255,90,225,90),
			video::SColor(255,100,100,255),
			video::SColor(255,255,150,50),
			video::SColor(255,220,220,100)
		};
		static const u32 usable_colors_count =
				sizeof(usable_colors) / sizeof(*usable_colors);
		u32 next_color_i = 0;
		for(std::map<std::string, Meta>::iterator i = m_meta.begin();
				i != m_meta.end(); i++){
			Meta &meta = i->second;
			video::SColor color(255,200,200,200);
			if(next_color_i < usable_colors_count)
				color = usable_colors[next_color_i++];
			meta.color = color;
		}

		s32 graphh = 50;
		s32 textx = x_left + m_log_max_size + 15;
		s32 textx2 = textx + 200 - 15;
		
		// Draw background
		/*{
			u32 num_graphs = m_meta.size();
			core::rect<s32> rect(x_left, y_bottom - num_graphs*graphh,
					textx2, y_bottom);
			video::SColor bgcolor(120,0,0,0);
			driver->draw2DRectangle(bgcolor, rect, NULL);
		}*/
		
		s32 meta_i = 0;
		for(std::map<std::string, Meta>::const_iterator i = m_meta.begin();
				i != m_meta.end(); i++){
			const std::string &id = i->first;
			const Meta &meta = i->second;
			s32 x = x_left;
			s32 y = y_bottom - meta_i * 50;
			float show_min = meta.min;
			float show_max = meta.max;
			if(show_min >= -0.0001 && show_max >= -0.0001){
				if(show_min <= show_max * 0.5)
					show_min = 0;
			}
			s32 texth = 15;
			char buf[10];
			snprintf(buf, 10, "%.3g", show_max);
			font->draw(narrow_to_wide(buf).c_str(),
					core::rect<s32>(textx, y - graphh,
					textx2, y - graphh + texth),
					meta.color);
			snprintf(buf, 10, "%.3g", show_min);
			font->draw(narrow_to_wide(buf).c_str(),
					core::rect<s32>(textx, y - texth,
					textx2, y),
					meta.color);
			font->draw(narrow_to_wide(id).c_str(),
					core::rect<s32>(textx, y - graphh/2 - texth/2,
					textx2, y - graphh/2 + texth/2),
					meta.color);
			s32 graph1y = y;
			s32 graph1h = graphh;
			bool relativegraph = (show_min != 0 && show_min != show_max);
			float lastscaledvalue = 0.0;
			bool lastscaledvalue_exists = false;
			for(std::list<Piece>::const_iterator j = m_log.begin();
					j != m_log.end(); j++)
			{
				const Piece &piece = *j;
				float value = 0;
				bool value_exists = false;
				Profiler::GraphValues::const_iterator k =
						piece.values.find(id);
				if(k != piece.values.end()){
					value = k->second;
					value_exists = true;
				}
				if(!value_exists){
					x++;
					lastscaledvalue_exists = false;
					continue;
				}
				float scaledvalue = 1.0;
				if(show_max != show_min)
					scaledvalue = (value - show_min) / (show_max - show_min);
				if(scaledvalue == 1.0 && value == 0){
					x++;
					lastscaledvalue_exists = false;
					continue;
				}
				if(relativegraph){
					if(lastscaledvalue_exists){
						s32 ivalue1 = lastscaledvalue * graph1h;
						s32 ivalue2 = scaledvalue * graph1h;
						driver->draw2DLine(v2s32(x-1, graph1y - ivalue1),
								v2s32(x, graph1y - ivalue2), meta.color);
					}
					lastscaledvalue = scaledvalue;
					lastscaledvalue_exists = true;
				} else{
					s32 ivalue = scaledvalue * graph1h;
					driver->draw2DLine(v2s32(x, graph1y),
							v2s32(x, graph1y - ivalue), meta.color);
				}
				x++;
			}
			meta_i++;
		}
	}
};

class NodeDugEvent: public MtEvent
{
public:
	v3s16 p;
	MapNode n;
	
	NodeDugEvent(v3s16 p, MapNode n):
		p(p),
		n(n)
	{}
	const char* getType() const
	{return "NodeDug";}
};

class SoundMaker
{
	ISoundManager *m_sound;
	INodeDefManager *m_ndef;
public:
	float m_player_step_timer;

	SimpleSoundSpec m_player_step_sound;
	SimpleSoundSpec m_player_leftpunch_sound;
	SimpleSoundSpec m_player_rightpunch_sound;

	SoundMaker(ISoundManager *sound, INodeDefManager *ndef):
		m_sound(sound),
		m_ndef(ndef),
		m_player_step_timer(0)
	{
	}

	void playPlayerStep()
	{
		if(m_player_step_timer <= 0 && m_player_step_sound.exists()){
			m_player_step_timer = 0.03;
			m_sound->playSound(m_player_step_sound, false);
		}
	}

	static void viewBobbingStep(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker*)data;
		sm->playPlayerStep();
	}

	static void playerRegainGround(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker*)data;
		sm->playPlayerStep();
	}

	static void playerJump(MtEvent *e, void *data)
	{
		//SoundMaker *sm = (SoundMaker*)data;
	}

	static void cameraPunchLeft(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker*)data;
		sm->m_sound->playSound(sm->m_player_leftpunch_sound, false);
	}

	static void cameraPunchRight(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker*)data;
		sm->m_sound->playSound(sm->m_player_rightpunch_sound, false);
	}

	static void nodeDug(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker*)data;
		NodeDugEvent *nde = (NodeDugEvent*)e;
		sm->m_sound->playSound(sm->m_ndef->get(nde->n).sound_dug, false);
	}

	void registerReceiver(MtEventManager *mgr)
	{
		mgr->reg("ViewBobbingStep", SoundMaker::viewBobbingStep, this);
		mgr->reg("PlayerRegainGround", SoundMaker::playerRegainGround, this);
		mgr->reg("PlayerJump", SoundMaker::playerJump, this);
		mgr->reg("CameraPunchLeft", SoundMaker::cameraPunchLeft, this);
		mgr->reg("CameraPunchRight", SoundMaker::cameraPunchRight, this);
		mgr->reg("NodeDug", SoundMaker::nodeDug, this);
	}

	void step(float dtime)
	{
		m_player_step_timer -= dtime;
	}
};

// Locally stored sounds don't need to be preloaded because of this
class GameOnDemandSoundFetcher: public OnDemandSoundFetcher
{
	std::set<std::string> m_fetched;
public:

	void fetchSounds(const std::string &name,
			std::set<std::string> &dst_paths,
			std::set<std::string> &dst_datas)
	{
		if(m_fetched.count(name))
			return;
		m_fetched.insert(name);
		std::string base = porting::path_share + DIR_DELIM + "testsounds";
		dst_paths.insert(base + DIR_DELIM + name + ".ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".0.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".1.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".2.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".3.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".4.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".5.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".6.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".7.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".8.ogg");
		dst_paths.insert(base + DIR_DELIM + name + ".9.ogg");
	}
};

void the_game(
	bool &kill,
	bool random_input,
	InputHandler *input,
	IrrlichtDevice *device,
	gui::IGUIFont* font,
	std::string map_dir,
	std::string playername,
	std::string password,
	std::string address, // If "", local server is used
	u16 port,
	std::wstring &error_message,
	std::string configpath,
	ChatBackend &chat_backend,
	const SubgameSpec &gamespec, // Used for local game,
	bool simple_singleplayer_mode
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
	
	// Sound fetcher (useful when testing)
	GameOnDemandSoundFetcher soundfetcher;

	// Sound manager
	ISoundManager *sound = NULL;
	bool sound_is_dummy = false;
#if USE_SOUND
	if(g_settings->getBool("enable_sound")){
		infostream<<"Attempting to use OpenAL audio"<<std::endl;
		sound = createOpenALSoundManager(&soundfetcher);
		if(!sound)
			infostream<<"Failed to initialize OpenAL audio"<<std::endl;
	} else {
		infostream<<"Sound disabled."<<std::endl;
	}
#endif
	if(!sound){
		infostream<<"Using dummy audio."<<std::endl;
		sound = &dummySoundManager;
		sound_is_dummy = true;
	}

	// Event manager
	EventManager eventmgr;

	// Sound maker
	SoundMaker soundmaker(sound, nodedef);
	soundmaker.registerReceiver(&eventmgr);
	
	// Add chat log output for errors to be shown in chat
	LogOutputBuffer chat_log_error_buf(LMT_ERROR);

	// Create UI for modifying quicktune values
	QuicktuneShortcutter quicktune;

	/*
		Create server.
		SharedPtr will delete it when it goes out of scope.
	*/
	SharedPtr<Server> server;
	if(address == ""){
		draw_load_screen(L"Creating server...", driver, font);
		infostream<<"Creating server"<<std::endl;
		server = new Server(map_dir, configpath, gamespec,
				simple_singleplayer_mode);
		server->start(port);
	}

	try{
	do{ // Client scope (breakable do-while(0))
	
	/*
		Create client
	*/

	draw_load_screen(L"Creating client...", driver, font);
	infostream<<"Creating client"<<std::endl;
	
	MapDrawControl draw_control;

	Client client(device, playername.c_str(), password, draw_control,
			tsrc, itemdef, nodedef, sound, &eventmgr);
	
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
		error_message = L"Couldn't resolve address";
		errorstream<<wide_to_narrow(error_message)<<std::endl;
		// Break out of client scope
		break;
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
	bool connect_aborted = false;
	try{
		float frametime = 0.033;
		float time_counter = 0.0;
		input->clear();
		while(device->run())
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
			if(client.accessDenied()){
				error_message = L"Access denied. Reason: "
						+client.accessDeniedReason();
				errorstream<<wide_to_narrow(error_message)<<std::endl;
				break;
			}
			if(input->wasKeyDown(EscapeKey)){
				connect_aborted = true;
				infostream<<"Connect aborted [Escape]"<<std::endl;
				break;
			}
			
			// Display status
			std::wostringstream ss;
			ss<<L"Connecting to server... (press Escape to cancel)\n";
			std::wstring animation = L"/-\\|";
			ss<<animation[(int)(time_counter/0.2)%4];
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
	if(!could_connect){
		if(error_message == L"" && !connect_aborted){
			error_message = L"Connection failed";
			errorstream<<wide_to_narrow(error_message)<<std::endl;
		}
		// Break out of client scope
		break;
	}
	
	/*
		Wait until content has been received
	*/
	bool got_content = false;
	bool content_aborted = false;
	{
		float frametime = 0.033;
		float time_counter = 0.0;
		input->clear();
		while(device->run())
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
			if(!client.connectedAndInitialized()){
				error_message = L"Client disconnected";
				errorstream<<wide_to_narrow(error_message)<<std::endl;
				break;
			}
			if(input->wasKeyDown(EscapeKey)){
				content_aborted = true;
				infostream<<"Connect aborted [Escape]"<<std::endl;
				break;
			}
			
			// Display status
			std::wostringstream ss;
			ss<<L"Waiting content... (press Escape to cancel)\n";

			ss<<(client.itemdefReceived()?L"[X]":L"[  ]");
			ss<<L" Item definitions\n";
			ss<<(client.nodedefReceived()?L"[X]":L"[  ]");
			ss<<L" Node definitions\n";
			ss<<L"["<<(int)(client.mediaReceiveProgress()*100+0.5)<<L"%] ";
			ss<<L" Media\n";

			draw_load_screen(ss.str(), driver, font);
			
			// Delay a bit
			sleep_ms(1000*frametime);
			time_counter += frametime;
		}
	}

	if(!got_content){
		if(error_message == L"" && !content_aborted){
			error_message = L"Something failed";
			errorstream<<wide_to_narrow(error_message)<<std::endl;
		}
		// Break out of client scope
		break;
	}

	/*
		After all content has been received:
		Update cached textures, meshes and materials
	*/
	client.afterContentReceived();

	/*
		Create the camera node
	*/
	Camera camera(smgr, draw_control, gamedef);
	if (!camera.successfullyCreated(error_message))
		return;

	f32 camera_yaw = 0; // "right/left"
	f32 camera_pitch = 0; // "up/down"

	/*
		Clouds
	*/
	
	Clouds *clouds = NULL;
	if(g_settings->getBool("enable_clouds"))
	{
		clouds = new Clouds(smgr->getRootSceneNode(), smgr, -1, time(0));
	}

	/*
		Skybox thingy
	*/

	Sky *sky = NULL;
	sky = new Sky(smgr->getRootSceneNode(), smgr, -1);
	
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
		Find out size of crack animation
	*/
	int crack_animation_length = 5;
	{
		video::ITexture *t = tsrc->getTextureRaw("crack_anylength.png");
		v2u32 size = t->getOriginalSize();
		crack_animation_length = size.Y / size.X;
	}

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
			core::rect<s32>(0,0,400,text_height*5+5) + v2s32(100,200),
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
	// Remove stale "recent" chat messages from previous connections
	chat_backend.clearRecentChat();
	// Chat backend and console
	GUIChatConsole *gui_chat_console = new GUIChatConsole(guienv, guienv->getRootGUIElement(), -1, &chat_backend, &client);
	
	// Profiler text (size is updated when text is updated)
	gui::IGUIStaticText *guitext_profiler = guienv->addStaticText(
			L"<Profiler>",
			core::rect<s32>(0,0,0,0),
			false, false);
	guitext_profiler->setBackgroundColor(video::SColor(120,0,0,0));
	guitext_profiler->setVisible(false);
	
	/*
		Some statistics are collected in these
	*/
	u32 drawtime = 0;
	u32 beginscenetime = 0;
	u32 scenetime = 0;
	u32 endscenetime = 0;
	
	float recent_turn_speed = 0.0;
	
	ProfilerGraph graph;
	// Initially clear the profiler
	Profiler::GraphValues dummyvalues;
	g_profiler->graphGet(dummyvalues);

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
	float time_from_last_punch = 10;

	float update_draw_list_timer = 0.0;
	v3f update_draw_list_last_cam_dir;

	bool invert_mouse = g_settings->getBool("invert_mouse");

	bool respawn_menu_active = false;
	bool update_wielded_item_trigger = false;

	bool show_hud = true;
	bool show_chat = true;
	bool force_fog_off = false;
	bool disable_camera_update = false;
	bool show_debug = g_settings->getBool("show_debug");
	bool show_profiler_graph = false;
	u32 show_profiler = 0;
	u32 show_profiler_max = 3;  // Number of pages

	float time_of_day = 0;
	float time_of_day_smooth = 0;

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

	for(;;)
	{
		if(device->run() == false || kill == true)
			break;

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
		
		g_profiler->graphAdd("mainloop_other", busytime - (float)drawtime/1000.0f);

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
				g_profiler->graphAdd("mainloop_sleep", (float)sleeptime/1000.0f);
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

		g_profiler->graphAdd("mainloop_dtime", dtime);

		/* Run timers */

		if(nodig_delay_timer >= 0)
			nodig_delay_timer -= dtime;
		if(object_hit_delay_timer >= 0)
			object_hit_delay_timer -= dtime;
		time_from_last_punch += dtime;
		
		g_profiler->add("Elapsed time", dtime);
		g_profiler->avg("FPS", 1./dtime);

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
			Handle miscellaneous stuff
		*/
		
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
		std::vector<aabb3f> hilightboxes;
		
		// Info text
		std::wstring infotext;

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
		if(device->isWindowActive() == false
				|| noMenuActive() == false
				|| guienv->hasFocus(gui_chat_console))
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
			
			GUIFormSpecMenu *menu =
				new GUIFormSpecMenu(guienv, guiroot, -1,
					&g_menumgr,
					&client, gamedef);

			InventoryLocation inventoryloc;
			inventoryloc.setCurrentPlayer();

			PlayerInventoryFormSource *src = new PlayerInventoryFormSource(&client);
			assert(src);
			menu->setFormSpec(src->getForm(), inventoryloc);
			menu->setFormSource(src);
			menu->setTextDest(new TextDestPlayerInventory(&client));
			menu->drop();
		}
		else if(input->wasKeyDown(EscapeKey))
		{
			infostream<<"the_game: "
					<<"Launching pause menu"<<std::endl;
			// It will delete itself by itself
			(new GUIPauseMenu(guienv, guiroot, -1, g_gamecallback,
					&g_menumgr, simple_singleplayer_mode))->drop();

			// Move mouse cursor on top of the disconnect button
			if(simple_singleplayer_mode)
				input->setMousePos(displaycenter.X, displaycenter.Y+0);
			else
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
		else if(input->wasKeyDown(getKeySetting("keymap_console")))
		{
			if (!gui_chat_console->isOpenInhibited())
			{
				// Open up to over half of the screen
				gui_chat_console->openConsole(0.6);
				guienv->setFocus(gui_chat_console);
			}
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
				if(!client.checkPrivilege("fly"))
					statustext += L" (note: no 'fly' privilege)";
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
				if(!client.checkPrivilege("fast"))
					statustext += L" (note: no 'fast' privilege)";
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
			// 2x toggle: Debug text with profiler graph
			if(!show_debug)
			{
				show_debug = true;
				show_profiler_graph = false;
				statustext = L"Debug info shown";
				statustext_time = 0;
			}
			else if(show_profiler_graph)
			{
				show_debug = false;
				show_profiler_graph = false;
				statustext = L"Debug info and profiler graph hidden";
				statustext_time = 0;
			}
			else
			{
				show_profiler_graph = true;
				statustext = L"Profiler graph shown";
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
		
		// Handle QuicktuneShortcutter
		if(input->wasKeyDown(getKeySetting("keymap_quicktune_next")))
			quicktune.next();
		if(input->wasKeyDown(getKeySetting("keymap_quicktune_prev")))
			quicktune.prev();
		if(input->wasKeyDown(getKeySetting("keymap_quicktune_inc")))
			quicktune.inc();
		if(input->wasKeyDown(getKeySetting("keymap_quicktune_dec")))
			quicktune.dec();
		{
			std::string msg = quicktune.getMessage();
			if(msg != ""){
				statustext = narrow_to_wide(msg);
				statustext_time = 0;
			}
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
		
		float turn_amount = 0;
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
				
				float d = 0.2;
				camera_yaw -= dx*d;
				camera_pitch += dy*d;
				if(camera_pitch < -89.5) camera_pitch = -89.5;
				if(camera_pitch > 89.5) camera_pitch = 89.5;
				
				turn_amount = v2f(dx, dy).getLength() * d;
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
		recent_turn_speed = recent_turn_speed * 0.9 + turn_amount * 0.1;
		//std::cerr<<"recent_turn_speed = "<<recent_turn_speed<<std::endl;

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
					
					chat_backend.addMessage(L"", L"You died.");

					/* Handle visualization */

					damage_flash_timer = 0;

					/*LocalPlayer* player = client.getLocalPlayer();
					player->setPosition(player->getPosition() + v3f(0,-BS,0));
					camera.update(player, busytime, screensize);*/
				}
				else if(event.type == CE_TEXTURES_UPDATED)
				{
					update_wielded_item_trigger = true;
				}
			}
		}
		
		//TimeTaker //timer2("//timer2");

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
		ToolCapabilities playeritem_toolcap =
				playeritem.getToolCapabilities(itemdef);
		
		/*
			Update camera
		*/

		LocalPlayer* player = client.getEnv().getLocalPlayer();
		float full_punch_interval = playeritem_toolcap.full_punch_interval;
		float tool_reload_ratio = time_from_last_punch / full_punch_interval;
		tool_reload_ratio = MYMIN(tool_reload_ratio, 1.0);
		camera.update(player, busytime, screensize, tool_reload_ratio);
		camera.step(dtime);

		v3f player_position = player->getPosition();
		v3f camera_position = camera.getPosition();
		v3f camera_direction = camera.getDirection();
		f32 camera_fov = camera.getFovMax();
		
		if(!disable_camera_update){
			client.getEnv().getClientMap().updateCamera(camera_position,
				camera_direction, camera_fov);
		}
		
		// Update sound listener
		sound->updateListener(camera.getCameraNode()->getPosition(),
				v3f(0,0,0), // velocity
				camera.getDirection(),
				camera.getCameraNode()->getUpVector());
		sound->setListenerGain(g_settings->getFloat("sound_volume"));

		/*
			Update sound maker
		*/
		{
			soundmaker.step(dtime);
			
			ClientMap &map = client.getEnv().getClientMap();
			MapNode n = map.getNodeNoEx(player->getStandingNodePos());
			soundmaker.m_player_step_sound = nodedef->get(n).sound_footstep;
		}

		/*
			Calculate what block is the crosshair pointing to
		*/
		
		//u32 t1 = device->getTimer()->getRealTime();
		
		f32 d = 4; // max. distance
		core::line3d<f32> shootline(camera_position,
				camera_position + camera_direction * BS * (d+1));

		ClientActiveObject *selected_object = NULL;

		PointedThing pointed = getPointedThing(
				// input
				&client, player_position, camera_direction,
				camera_position, shootline, d,
				playeritem_liquids_pointable, !ldown_for_dig,
				// output
				hilightboxes,
				selected_object);

		if(pointed != pointed_old)
		{
			infostream<<"Pointing at "<<pointed.dump()<<std::endl;
			//dstream<<"Pointing at "<<pointed.dump()<<std::endl;
		}

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
				client.setCrack(-1, v3s16(0,0,0));
				dig_time = 0.0;
			}
		}
		if(!digging && ldown_for_dig && !input->getLeftState())
		{
			ldown_for_dig = false;
		}

		bool left_punch = false;
		soundmaker.m_player_leftpunch_sound.name = "";

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
			
			ClientMap &map = client.getEnv().getClientMap();
			NodeMetadata *meta = map.getNodeMetadata(nodepos);
			if(meta){
				infotext = narrow_to_wide(meta->getString("infotext"));
			} else {
				MapNode n = map.getNode(nodepos);
				if(nodedef->get(n).tiledef[0].name == "unknown_block.png"){
					infotext = L"Unknown node: ";
					infotext += narrow_to_wide(nodedef->get(n).name);
				}
			}
			
			// We can't actually know, but assume the sound of right-clicking
			// to be the sound of placing a node
			soundmaker.m_player_rightpunch_sound.gain = 0.5;
			soundmaker.m_player_rightpunch_sound.name = "default_place_node";
			
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
				MapNode n = client.getEnv().getClientMap().getNode(nodepos);
				
				// NOTE: Similar piece of code exists on the server side for
				// cheat detection.
				// Get digging parameters
				DigParams params = getDigParams(nodedef->get(n).groups,
						&playeritem_toolcap);
				// If can't dig, try hand
				if(!params.diggable){
					const ItemDefinition &hand = itemdef->get("");
					const ToolCapabilities *tp = hand.tool_capabilities;
					if(tp)
						params = getDigParams(nodedef->get(n).groups, tp);
				}
				
				SimpleSoundSpec sound_dig = nodedef->get(n).sound_dig;
				if(sound_dig.exists()){
					if(sound_dig.name == "__group"){
						if(params.main_group != ""){
							soundmaker.m_player_leftpunch_sound.gain = 0.5;
							soundmaker.m_player_leftpunch_sound.name =
									std::string("default_dig_") +
											params.main_group;
						}
					} else{
						soundmaker.m_player_leftpunch_sound = sound_dig;
					}
				}

				float dig_time_complete = 0.0;

				if(params.diggable == false)
				{
					// I guess nobody will wait for this long
					dig_time_complete = 10000000.0;
				}
				else
				{
					dig_time_complete = params.time;
				}

				if(dig_time_complete >= 0.001)
				{
					dig_index = (u16)((float)crack_animation_length
							* dig_time/dig_time_complete);
				}
				// This is for torches
				else
				{
					dig_index = crack_animation_length;
				}

				// Don't show cracks if not diggable
				if(dig_time_complete >= 100000.0)
				{
				}
				else if(dig_index < crack_animation_length)
				{
					//TimeTaker timer("client.setTempMod");
					//infostream<<"dig_index="<<dig_index<<std::endl;
					client.setCrack(dig_index, nodepos);
				}
				else
				{
					infostream<<"Digging completed"<<std::endl;
					client.interact(2, pointed);
					client.setCrack(-1, v3s16(0,0,0));
					MapNode wasnode = map.getNode(nodepos);
					client.removeNode(nodepos);

					dig_time = 0;
					digging = false;

					nodig_delay_timer = dig_time_complete
							/ (float)crack_animation_length;

					// We don't want a corresponding delay to
					// very time consuming nodes
					if(nodig_delay_timer > 0.3)
						nodig_delay_timer = 0.3;
					// We want a slight delay to very little
					// time consuming nodes
					float mindelay = 0.15;
					if(nodig_delay_timer < mindelay)
						nodig_delay_timer = mindelay;
					
					// Send event to trigger sound
					MtEvent *e = new NodeDugEvent(nodepos, wasnode);
					gamedef->event()->put(e);
				}

				dig_time += dtime;

				camera.setDigging(0);  // left click animation
			}

			if(input->getRightClicked())
			{
				infostream<<"Ground right-clicked"<<std::endl;
				
				// Sign special case, at least until formspec is properly implemented.
				// Deprecated?
				if(meta && meta->getString("formspec") == "hack:sign_text_input" && !random_input)
				{
					infostream<<"Launching metadata text input"<<std::endl;
					
					// Get a new text for it

					TextDest *dest = new TextDestNodeMetadata(nodepos, &client);

					std::wstring wtext = narrow_to_wide(meta->getString("text"));

					(new GUITextInputMenu(guienv, guiroot, -1,
							&g_menumgr, dest,
							wtext))->drop();
				}
				// If metadata provides an inventory view, activate it
				else if(meta && meta->getString("formspec") != "" && !random_input)
				{
					infostream<<"Launching custom inventory view"<<std::endl;

					InventoryLocation inventoryloc;
					inventoryloc.setNodeMeta(nodepos);
					
					/* Create menu */

					GUIFormSpecMenu *menu =
						new GUIFormSpecMenu(guienv, guiroot, -1,
							&g_menumgr,
							&client, gamedef);
					menu->setFormSpec(meta->getString("formspec"),
							inventoryloc);
					menu->setFormSource(new NodeMetadataFormSource(
							&client.getEnv().getClientMap(), nodepos));
					menu->setTextDest(new TextDestNodeMetadata(nodepos, &client));
					menu->drop();
				}
				// Otherwise report right click to server
				else
				{
					// Report to server
					client.interact(3, pointed);
					camera.setDigging(1);  // right click animation
					
					// If the wielded item has node placement prediction,
					// make that happen
					const ItemDefinition &def =
							playeritem.getDefinition(itemdef);
					if(def.node_placement_prediction != "")
					do{ // breakable
						verbosestream<<"Node placement prediction for "
								<<playeritem.name<<" is "
								<<def.node_placement_prediction<<std::endl;
						v3s16 p = neighbourpos;
						// Place inside node itself if buildable_to
						try{
							MapNode n_under = map.getNode(nodepos);
							if(nodedef->get(n_under).buildable_to)
								p = nodepos;
						}catch(InvalidPositionException &e){}
						// Find id of predicted node
						content_t id;
						bool found =
							nodedef->getId(def.node_placement_prediction, id);
						if(!found){
							errorstream<<"Node placement prediction failed for "
									<<playeritem.name<<" (places "
									<<def.node_placement_prediction
									<<") - Name not known"<<std::endl;
							break;
						}
						MapNode n(id);
						try{
							// This triggers the required mesh update too
							client.addNode(p, n);
						}catch(InvalidPositionException &e){
							errorstream<<"Node placement prediction failed for "
									<<playeritem.name<<" (places "
									<<def.node_placement_prediction
									<<") - Position not loaded"<<std::endl;
						}
					}while(0);
				}
			}
		}
		else if(pointed.type == POINTEDTHING_OBJECT)
		{
			infotext = narrow_to_wide(selected_object->infoText());

			if(infotext == L"" && show_debug){
				infotext = narrow_to_wide(selected_object->debugInfoText());
			}

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
					
					bool disable_send = selected_object->directReportPunch(
							dir, &playeritem, time_from_last_punch);
					time_from_last_punch = 0;
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
		else if(input->getLeftState())
		{
			// When button is held down in air, show continuous animation
			left_punch = true;
		}

		pointed_old = pointed;
		
		if(left_punch || input->getLeftClicked())
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
			Fog range
		*/
	
		f32 fog_range;
		if(farmesh)
		{
			fog_range = BS*farmesh_range;
		}
		else
		{
			fog_range = draw_control.wanted_range*BS + 0.0*MAP_BLOCKSIZE*BS;
			fog_range *= 0.9;
			if(draw_control.range_all)
				fog_range = 100000*BS;
		}

		/*
			Calculate general brightness
		*/
		u32 daynight_ratio = client.getEnv().getDayNightRatio();
		float time_brightness = (float)decode_light(
				(daynight_ratio * LIGHT_SUN) / 1000) / 255.0;
		float direct_brightness = 0;
		bool sunlight_seen = false;
		if(g_settings->getBool("free_move")){
			direct_brightness = time_brightness;
			sunlight_seen = true;
		} else {
			ScopeProfiler sp(g_profiler, "Detecting background light", SPT_AVG);
			float old_brightness = sky->getBrightness();
			direct_brightness = (float)client.getEnv().getClientMap()
					.getBackgroundBrightness(MYMIN(fog_range*1.2, 60*BS),
					daynight_ratio, (int)(old_brightness*255.5), &sunlight_seen)
					/ 255.0;
		}
		
		time_of_day = client.getEnv().getTimeOfDayF();
		float maxsm = 0.05;
		if(fabs(time_of_day - time_of_day_smooth) > maxsm &&
				fabs(time_of_day - time_of_day_smooth + 1.0) > maxsm &&
				fabs(time_of_day - time_of_day_smooth - 1.0) > maxsm)
			time_of_day_smooth = time_of_day;
		float todsm = 0.05;
		if(time_of_day_smooth > 0.8 && time_of_day < 0.2)
			time_of_day_smooth = time_of_day_smooth * (1.0-todsm)
					+ (time_of_day+1.0) * todsm;
		else
			time_of_day_smooth = time_of_day_smooth * (1.0-todsm)
					+ time_of_day * todsm;
			
		sky->update(time_of_day_smooth, time_brightness, direct_brightness,
				sunlight_seen);
		
		float brightness = sky->getBrightness();
		video::SColor bgcolor = sky->getBgColor();
		video::SColor skycolor = sky->getSkyColor();

		/*
			Update clouds
		*/
		if(clouds){
			if(sky->getCloudsVisible()){
				clouds->setVisible(true);
				clouds->step(dtime);
				clouds->update(v2f(player_position.X, player_position.Z),
						sky->getCloudColor());
			} else{
				clouds->setVisible(false);
			}
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
		
		/*
			Fog
		*/
		
		if(g_settings->getBool("enable_fog") == true && !force_fog_off)
		{
			driver->setFog(
				bgcolor,
				video::EFT_FOG_LINEAR,
				fog_range*0.4,
				fog_range*1.0,
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
					" (yaw = %.1f) (seed = %lli)",
					player_position.X/BS,
					player_position.Y/BS,
					player_position.Z/BS,
					wrapDegrees_0_360(camera_yaw),
					client.getMapSeed());

			guitext2->setText(narrow_to_wide(temptext).c_str());
			guitext2->setVisible(true);
		}
		else
		{
			guitext2->setVisible(false);
		}
		
		{
			guitext_info->setText(infotext.c_str());
			guitext_info->setVisible(show_hud && g_menumgr.menuCount() == 0);
		}

		{
			float statustext_time_max = 1.5;
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
						pow(statustext_time / (float)statustext_time_max, 2.0f));
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
				chat_backend.addMessage(L"", narrow_to_wide(
						chat_log_error_buf.get()));
			}
			// Get new messages from client
			std::wstring message;
			while(client.getChatMessage(message))
			{
				chat_backend.addUnparsedMessage(message);
			}
			// Remove old messages
			chat_backend.step(dtime);

			// Display all messages in a static text element
			u32 recent_chat_count = chat_backend.getRecentBuffer().getLineCount();
			std::wstring recent_chat = chat_backend.getRecentChat();
			guitext_chat->setText(recent_chat.c_str());

			// Update gui element size and position
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

			// Don't show chat if disabled or empty or profiler is enabled
			guitext_chat->setVisible(show_chat && recent_chat_count != 0
					&& !show_profiler);
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
			camera.wield(item);
		}

		/*
			Update block draw list every 200ms or when camera direction has
			changed much
		*/
		update_draw_list_timer += dtime;
		if(update_draw_list_timer >= 0.2 ||
				update_draw_list_last_cam_dir.getDistanceFrom(camera_direction) > 0.2){
			update_draw_list_timer = 0;
			client.getEnv().getClientMap().updateDrawList(driver);
			update_draw_list_last_cam_dir = camera_direction;
		}

		/*
			Drawing begins
		*/

		TimeTaker tt_draw("mainloop: draw");

		
		{
			TimeTaker timer("beginScene");
			//driver->beginScene(false, true, bgcolor);
			//driver->beginScene(true, true, bgcolor);
			driver->beginScene(true, true, skycolor);
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
			for(std::vector<aabb3f>::const_iterator
					i = hilightboxes.begin();
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
			client.getEnv().getClientMap().renderPostFx();
		}

		/*
			Profiler graph
		*/
		if(show_profiler_graph)
		{
			graph.draw(10, screensize.Y - 10, driver, font);
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

		drawtime = tt_draw.stop(true);
		g_profiler->graphAdd("mainloop_draw", (float)drawtime/1000.0f);

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

		/*
			Log times and stuff for visualization
		*/
		Profiler::GraphValues values;
		g_profiler->graphGet(values);
		graph.put(values);
	}

	/*
		Drop stuff
	*/
	if(clouds)
		clouds->drop();
	if(gui_chat_console)
		gui_chat_console->drop();
	
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

	chat_backend.addMessage(L"", L"# Disconnected.");
	chat_backend.addMessage(L"", L"");

	// Client scope (client is destructed before destructing *def and tsrc)
	}while(0);
	} // try-catch
	catch(SerializationError &e)
	{
		error_message = L"A serialization error occurred:\n"
				+ narrow_to_wide(e.what()) + L"\n\nThe server is probably "
				L" running a different version of Minetest.";
		errorstream<<wide_to_narrow(error_message)<<std::endl;
	}
	
	if(!sound_is_dummy)
		delete sound;
	delete nodedef;
	delete itemdef;
	delete tsrc;
}


