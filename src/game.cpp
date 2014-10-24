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

#include "game.h"
#include "irrlichttypes_extrabloated.h"
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include <IMaterialRendererServices.h>
#include "IMeshCache.h"
#include "client.h"
#include "server.h"
#include "guiPasswordChange.h"
#include "guiVolumeChange.h"
#include "guiKeyChangeMenu.h"
#include "guiFormSpecMenu.h"
#include "tool.h"
#include "guiChatConsole.h"
#include "config.h"
#include "version.h"
#include "clouds.h"
#include "particles.h"
#include "camera.h"
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
#include "shader.h" // For ShaderSource
#include "logoutputbuffer.h"
#include "subgame.h"
#include "quicktune_shortcutter.h"
#include "clientmap.h"
#include "hud.h"
#include "sky.h"
#include "sound.h"
#if USE_SOUND
	#include "sound_openal.h"
#endif
#include "event_manager.h"
#include <iomanip>
#include <list>
#include "util/directiontables.h"
#include "util/pointedthing.h"
#include "drawscene.h"
#include "content_cao.h"

#ifdef HAVE_TOUCHSCREENGUI
#include "touchscreengui.h"
#endif

/*
	Text input system
*/

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
		m_formname = "";
	}
	TextDestPlayerInventory(Client *client, std::string formname)
	{
		m_client = client;
		m_formname = formname;
	}
	void gotText(std::map<std::string, std::string> fields)
	{
		m_client->sendInventoryFields(m_formname, fields);
	}

	Client *m_client;
};

struct LocalFormspecHandler : public TextDest
{
	LocalFormspecHandler();
	LocalFormspecHandler(std::string formname) :
		m_client(0)
	{
		m_formname = formname;
	}

	LocalFormspecHandler(std::string formname, Client *client) :
		m_client(client)
	{
		m_formname = formname;
	}

	void gotText(std::wstring message) {
		errorstream << "LocalFormspecHandler::gotText old style message received" << std::endl;
	}

	void gotText(std::map<std::string, std::string> fields)
	{
		if (m_formname == "MT_PAUSE_MENU") {
			if (fields.find("btn_sound") != fields.end()) {
				g_gamecallback->changeVolume();
				return;
			}

			if (fields.find("btn_key_config") != fields.end()) {
				g_gamecallback->keyConfig();
				return;
			}

			if (fields.find("btn_exit_menu") != fields.end()) {
				g_gamecallback->disconnect();
				return;
			}

			if (fields.find("btn_exit_os") != fields.end()) {
				g_gamecallback->exitToOS();
				return;
			}

			if (fields.find("btn_change_password") != fields.end()) {
				g_gamecallback->changePassword();
				return;
			}

			if (fields.find("quit") != fields.end()) {
				return;
			}

			if (fields.find("btn_continue") != fields.end()) {
				return;
			}
		}
		if (m_formname == "MT_CHAT_MENU") {
			assert(m_client != 0);
			if ((fields.find("btn_send") != fields.end()) ||
					(fields.find("quit") != fields.end())) {
				if (fields.find("f_text") != fields.end()) {
					m_client->typeChatMessage(narrow_to_wide(fields["f_text"]));
				}
				return;
			}
		}

		if (m_formname == "MT_DEATH_SCREEN") {
			assert(m_client != 0);
			if ((fields.find("btn_respawn") != fields.end())) {
				m_client->sendRespawn();
				return;
			}

			if (fields.find("quit") != fields.end()) {
				m_client->sendRespawn();
				return;
			}
		}

		// don't show error message for unhandled cursor keys
		if ( (fields.find("key_up") != fields.end()) ||
			(fields.find("key_down") != fields.end()) ||
			(fields.find("key_left") != fields.end()) ||
			(fields.find("key_right") != fields.end())) {
			return;
		}

		errorstream << "LocalFormspecHandler::gotText unhandled >" << m_formname << "< event" << std::endl;
		int i = 0;
		for (std::map<std::string,std::string>::iterator iter = fields.begin();
				iter != fields.end(); iter++) {
			errorstream << "\t"<< i << ": " << iter->first << "=" << iter->second << std::endl;
			i++;
		}
	}

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
		v3f camera_direction, v3f camera_position, core::line3d<f32> shootline,
		f32 d, bool liquids_pointable, bool look_for_object, v3s16 camera_offset,
		std::vector<aabb3f> &hilightboxes, ClientActiveObject *&selected_object)
{
	PointedThing result;

	hilightboxes.clear();
	selected_object = NULL;

	INodeDefManager *nodedef = client->getNodeDefManager();
	ClientMap &map = client->getEnv().getClientMap();

	f32 mindistance = BS * 1001;

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
						selection_box->MinEdge + pos - intToFloat(camera_offset, BS),
						selection_box->MaxEdge + pos - intToFloat(camera_offset, BS)));
			}

			mindistance = (selected_object->getPosition() - camera_position).getLength();

			result.type = POINTEDTHING_OBJECT;
			result.object_id = selected_object->getId();
		}
	}

	// That didn't work, try to find a pointed at node


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
				if (!g_settings->getBool("enable_node_highlighting")) {
					for(std::vector<aabb3f>::const_iterator
							i2 = boxes.begin();
							i2 != boxes.end(); i2++)
					{
						aabb3f box = *i2;
						box.MinEdge += npf + v3f(-d,-d,-d) - intToFloat(camera_offset, BS);
						box.MaxEdge += npf + v3f(d,d,d) - intToFloat(camera_offset, BS);
						hilightboxes.push_back(box);
					}
				}
			}
		}
	} // for coords

	return result;
}

/* Profiler display */

void update_profiler_gui(gui::IGUIStaticText *guitext_profiler,
		gui::IGUIFont *font, u32 text_height, u32 show_profiler,
		u32 show_profiler_max)
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

	static void playerDamage(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker*)data;
		sm->m_sound->playSound(SimpleSoundSpec("player_damage", 0.5), false);
	}

	static void playerFallingDamage(MtEvent *e, void *data)
	{
		SoundMaker *sm = (SoundMaker*)data;
		sm->m_sound->playSound(SimpleSoundSpec("player_falling_damage", 0.5), false);
	}

	void registerReceiver(MtEventManager *mgr)
	{
		mgr->reg("ViewBobbingStep", SoundMaker::viewBobbingStep, this);
		mgr->reg("PlayerRegainGround", SoundMaker::playerRegainGround, this);
		mgr->reg("PlayerJump", SoundMaker::playerJump, this);
		mgr->reg("CameraPunchLeft", SoundMaker::cameraPunchLeft, this);
		mgr->reg("CameraPunchRight", SoundMaker::cameraPunchRight, this);
		mgr->reg("NodeDug", SoundMaker::nodeDug, this);
		mgr->reg("PlayerDamage", SoundMaker::playerDamage, this);
		mgr->reg("PlayerFallingDamage", SoundMaker::playerFallingDamage, this);
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

class GameGlobalShaderConstantSetter : public IShaderConstantSetter
{
	Sky *m_sky;
	bool *m_force_fog_off;
	f32 *m_fog_range;
	Client *m_client;

public:
	GameGlobalShaderConstantSetter(Sky *sky, bool *force_fog_off,
			f32 *fog_range, Client *client):
		m_sky(sky),
		m_force_fog_off(force_fog_off),
		m_fog_range(fog_range),
		m_client(client)
	{}
	~GameGlobalShaderConstantSetter() {}

	virtual void onSetConstants(video::IMaterialRendererServices *services,
			bool is_highlevel)
	{
		if(!is_highlevel)
			return;

		// Background color
		video::SColor bgcolor = m_sky->getBgColor();
		video::SColorf bgcolorf(bgcolor);
		float bgcolorfa[4] = {
			bgcolorf.r,
			bgcolorf.g,
			bgcolorf.b,
			bgcolorf.a,
		};
		services->setPixelShaderConstant("skyBgColor", bgcolorfa, 4);

		// Fog distance
		float fog_distance = 10000*BS;
		if(g_settings->getBool("enable_fog") && !*m_force_fog_off)
			fog_distance = *m_fog_range;
		services->setPixelShaderConstant("fogDistance", &fog_distance, 1);

		// Day-night ratio
		u32 daynight_ratio = m_client->getEnv().getDayNightRatio();
		float daynight_ratio_f = (float)daynight_ratio / 1000.0;
		services->setPixelShaderConstant("dayNightRatio", &daynight_ratio_f, 1);

		u32 animation_timer = porting::getTimeMs() % 100000;
		float animation_timer_f = (float)animation_timer / 100000.0;
		services->setPixelShaderConstant("animationTimer", &animation_timer_f, 1);
		services->setVertexShaderConstant("animationTimer", &animation_timer_f, 1);

		LocalPlayer* player = m_client->getEnv().getLocalPlayer();
		v3f eye_position = player->getEyePosition();
		services->setPixelShaderConstant("eyePosition", (irr::f32*)&eye_position, 3);
		services->setVertexShaderConstant("eyePosition", (irr::f32*)&eye_position, 3);

		// Uniform sampler layers
		int layer0 = 0;
		int layer1 = 1;
		int layer2 = 2;
		// before 1.8 there isn't a "integer interface", only float
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
		services->setPixelShaderConstant("baseTexture" , (irr::f32*)&layer0, 1);
		services->setPixelShaderConstant("normalTexture" , (irr::f32*)&layer1, 1);
		services->setPixelShaderConstant("useNormalmap" , (irr::f32*)&layer2, 1);
#else
		services->setPixelShaderConstant("baseTexture" , (irr::s32*)&layer0, 1);
		services->setPixelShaderConstant("normalTexture" , (irr::s32*)&layer1, 1);
		services->setPixelShaderConstant("useNormalmap" , (irr::s32*)&layer2, 1);
#endif
	}
};

bool nodePlacementPrediction(Client &client,
		const ItemDefinition &playeritem_def, v3s16 nodepos, v3s16 neighbourpos)
{
	std::string prediction = playeritem_def.node_placement_prediction;
	INodeDefManager *nodedef = client.ndef();
	ClientMap &map = client.getEnv().getClientMap();

	if(prediction != "" && !nodedef->get(map.getNode(nodepos)).rightclickable)
	{
		verbosestream<<"Node placement prediction for "
				<<playeritem_def.name<<" is "
				<<prediction<<std::endl;
		v3s16 p = neighbourpos;
		// Place inside node itself if buildable_to
		try{
			MapNode n_under = map.getNode(nodepos);
			if(nodedef->get(n_under).buildable_to)
				p = nodepos;
			else if (!nodedef->get(map.getNode(p)).buildable_to)
				return false;
		}catch(InvalidPositionException &e){}
		// Find id of predicted node
		content_t id;
		bool found = nodedef->getId(prediction, id);
		if(!found){
			errorstream<<"Node placement prediction failed for "
					<<playeritem_def.name<<" (places "
					<<prediction
					<<") - Name not known"<<std::endl;
			return false;
		}
		// Predict param2 for facedir and wallmounted nodes
		u8 param2 = 0;
		if(nodedef->get(id).param_type_2 == CPT2_WALLMOUNTED){
			v3s16 dir = nodepos - neighbourpos;
			if(abs(dir.Y) > MYMAX(abs(dir.X), abs(dir.Z))){
				param2 = dir.Y < 0 ? 1 : 0;
			} else if(abs(dir.X) > abs(dir.Z)){
				param2 = dir.X < 0 ? 3 : 2;
			} else {
				param2 = dir.Z < 0 ? 5 : 4;
			}
		}
		if(nodedef->get(id).param_type_2 == CPT2_FACEDIR){
			v3s16 dir = nodepos - floatToInt(client.getEnv().getLocalPlayer()->getPosition(), BS);
			if(abs(dir.X) > abs(dir.Z)){
				param2 = dir.X < 0 ? 3 : 1;
			} else {
				param2 = dir.Z < 0 ? 2 : 0;
			}
		}
		assert(param2 >= 0 && param2 <= 5);
		//Check attachment if node is in group attached_node
		if(((ItemGroupList) nodedef->get(id).groups)["attached_node"] != 0){
			static v3s16 wallmounted_dirs[8] = {
				v3s16(0,1,0),
				v3s16(0,-1,0),
				v3s16(1,0,0),
				v3s16(-1,0,0),
				v3s16(0,0,1),
				v3s16(0,0,-1),
			};
			v3s16 pp;
			if(nodedef->get(id).param_type_2 == CPT2_WALLMOUNTED)
				pp = p + wallmounted_dirs[param2];
			else
				pp = p + v3s16(0,-1,0);
			if(!nodedef->get(map.getNode(pp)).walkable)
				return false;
		}
		// Add node to client map
		MapNode n(id, 0, param2);
		try{
			LocalPlayer* player = client.getEnv().getLocalPlayer();

			// Dont place node when player would be inside new node
			// NOTE: This is to be eventually implemented by a mod as client-side Lua
			if (!nodedef->get(n).walkable ||
				(client.checkPrivilege("noclip") && g_settings->getBool("noclip")) ||
				(nodedef->get(n).walkable &&
				neighbourpos != player->getStandingNodePos() + v3s16(0,1,0) &&
				neighbourpos != player->getStandingNodePos() + v3s16(0,2,0))) {

					// This triggers the required mesh update too
					client.addNode(p, n);
					return true;
				}
		}catch(InvalidPositionException &e){
			errorstream<<"Node placement prediction failed for "
					<<playeritem_def.name<<" (places "
					<<prediction
					<<") - Position not loaded"<<std::endl;
		}
	}
	return false;
}

static inline void create_formspec_menu(GUIFormSpecMenu** cur_formspec,
		InventoryManager *invmgr, IGameDef *gamedef,
		IWritableTextureSource* tsrc, IrrlichtDevice * device,
		IFormSource* fs_src, TextDest* txt_dest, Client* client
		) {

	if (*cur_formspec == 0) {
		*cur_formspec = new GUIFormSpecMenu(device, guiroot, -1, &g_menumgr,
				invmgr, gamedef, tsrc, fs_src, txt_dest, client);
		(*cur_formspec)->doPause = false;

		/*
			Caution: do not call (*cur_formspec)->drop() here --
			the reference might outlive the menu, so we will
			periodically check if *cur_formspec is the only
			remaining reference (i.e. the menu was removed)
			and delete it in that case.
		*/
	}
	else {
		(*cur_formspec)->setFormSource(fs_src);
		(*cur_formspec)->setTextDest(txt_dest);
	}
}

#ifdef __ANDROID__
#define SIZE_TAG "size[11,5.5]"
#else
#define SIZE_TAG "size[11,5.5,true]"
#endif

static void show_chat_menu(GUIFormSpecMenu** cur_formspec,
		InventoryManager *invmgr, IGameDef *gamedef,
		IWritableTextureSource* tsrc, IrrlichtDevice * device,
		Client* client, std::string text)
{
	std::string formspec =
		FORMSPEC_VERSION_STRING
		SIZE_TAG
		"field[3,2.35;6,0.5;f_text;;" + text + "]"
		"button_exit[4,3;3,0.5;btn_send;" + wide_to_narrow(wstrgettext("Proceed")) + "]"
		;

	/* Create menu */
	/* Note: FormspecFormSource and LocalFormspecHandler
	 * are deleted by guiFormSpecMenu                     */
	FormspecFormSource* fs_src = new FormspecFormSource(formspec);
	LocalFormspecHandler* txt_dst = new LocalFormspecHandler("MT_CHAT_MENU", client);

	create_formspec_menu(cur_formspec, invmgr, gamedef, tsrc, device, fs_src, txt_dst, NULL);
}

static void show_deathscreen(GUIFormSpecMenu** cur_formspec,
		InventoryManager *invmgr, IGameDef *gamedef,
		IWritableTextureSource* tsrc, IrrlichtDevice * device, Client* client)
{
	std::string formspec =
		std::string(FORMSPEC_VERSION_STRING) +
		SIZE_TAG
		"bgcolor[#320000b4;true]"
		"label[4.85,1.35;You died.]"
		"button_exit[4,3;3,0.5;btn_respawn;" + gettext("Respawn") + "]"
		;

	/* Create menu */
	/* Note: FormspecFormSource and LocalFormspecHandler
	 * are deleted by guiFormSpecMenu                     */
	FormspecFormSource* fs_src = new FormspecFormSource(formspec);
	LocalFormspecHandler* txt_dst = new LocalFormspecHandler("MT_DEATH_SCREEN", client);

	create_formspec_menu(cur_formspec, invmgr, gamedef, tsrc, device,  fs_src, txt_dst, NULL);
}

/******************************************************************************/
static void show_pause_menu(GUIFormSpecMenu** cur_formspec,
		InventoryManager *invmgr, IGameDef *gamedef,
		IWritableTextureSource* tsrc, IrrlichtDevice * device,
		bool singleplayermode)
{
#ifdef __ANDROID__
	std::string control_text = wide_to_narrow(wstrgettext("Default Controls:\n"
			"No menu visible:\n"
			"- single tap: button activate\n"
			"- double tap: place/use\n"
			"- slide finger: look around\n"
			"Menu/Inventory visible:\n"
			"- double tap (outside):\n"
			" -->close\n"
			"- touch stack, touch slot:\n"
			" --> move stack\n"
			"- touch&drag, tap 2nd finger\n"
			" --> place single item to slot\n"
			));
#else
	std::string control_text = wide_to_narrow(wstrgettext("Default Controls:\n"
			"- WASD: move\n"
			"- Space: jump/climb\n"
			"- Shift: sneak/go down\n"
			"- Q: drop item\n"
			"- I: inventory\n"
			"- Mouse: turn/look\n"
			"- Mouse left: dig/punch\n"
			"- Mouse right: place/use\n"
			"- Mouse wheel: select item\n"
			"- T: chat\n"
			));
#endif

	float ypos = singleplayermode ? 0.5 : 0.1;
	std::ostringstream os;

	os << FORMSPEC_VERSION_STRING  << SIZE_TAG
			<< "button_exit[4," << (ypos++) << ";3,0.5;btn_continue;"
					<< wide_to_narrow(wstrgettext("Continue"))     << "]";

	if (!singleplayermode) {
		os << "button_exit[4," << (ypos++) << ";3,0.5;btn_change_password;"
					<< wide_to_narrow(wstrgettext("Change Password")) << "]";
		}

	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_sound;"
					<< wide_to_narrow(wstrgettext("Sound Volume")) << "]";
	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_key_config;"
					<< wide_to_narrow(wstrgettext("Change Keys"))  << "]";
	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_exit_menu;"
					<< wide_to_narrow(wstrgettext("Exit to Menu")) << "]";
	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_exit_os;"
					<< wide_to_narrow(wstrgettext("Exit to OS"))   << "]"
			<< "textarea[7.5,0.25;3.9,6.25;;" << control_text << ";]"
			<< "textarea[0.4,0.25;3.5,6;;" << "Minetest\n"
			<< minetest_build_info << "\n"
			<< "path_user = " << wrap_rows(porting::path_user, 20)
			<< "\n;]";

	/* Create menu */
	/* Note: FormspecFormSource and LocalFormspecHandler  *
	 * are deleted by guiFormSpecMenu                     */
	FormspecFormSource* fs_src = new FormspecFormSource(os.str());
	LocalFormspecHandler* txt_dst = new LocalFormspecHandler("MT_PAUSE_MENU");

	create_formspec_menu(cur_formspec, invmgr, gamedef, tsrc, device,  fs_src, txt_dst, NULL);

	(*cur_formspec)->doPause = true;
}

/******************************************************************************/
static void updateChat(Client& client, f32 dtime, bool show_debug,
		const v2u32& screensize, bool show_chat, u32 show_profiler,
		ChatBackend& chat_backend, gui::IGUIStaticText* guitext_chat,
		gui::IGUIFont* font)
{
	// Add chat log output for errors to be shown in chat
	static LogOutputBuffer chat_log_error_buf(LMT_ERROR);

	// Get new messages from error log buffer
	while(!chat_log_error_buf.empty()) {
		chat_backend.addMessage(L"", narrow_to_wide(chat_log_error_buf.get()));
	}

	// Get new messages from client
	std::wstring message;
	while (client.getChatMessage(message)) {
		chat_backend.addUnparsedMessage(message);
	}

	// Remove old messages
	chat_backend.step(dtime);

	// Display all messages in a static text element
	unsigned int recent_chat_count = chat_backend.getRecentBuffer().getLineCount();
	std::wstring recent_chat       = chat_backend.getRecentChat();

	// TODO replace by fontengine fcts
	unsigned int line_height       = font->getDimension(L"Ay").Height + font->getKerningHeight();

	guitext_chat->setText(recent_chat.c_str());

	// Update gui element size and position
	s32 chat_y = 5 + line_height;
	if (show_debug)
		chat_y += line_height;

	// first pass to calculate height of text to be set
	s32 width = std::min(font->getDimension(recent_chat.c_str()).Width + 10,
			porting::getWindowSize().X - 20);
	core::rect<s32> rect(10, chat_y, width, chat_y + porting::getWindowSize().Y);
	guitext_chat->setRelativePosition(rect);

	//now use real height of text and adjust rect according to this size	
	rect = core::rect<s32>(10, chat_y, width,
			chat_y + guitext_chat->getTextHeight());


	guitext_chat->setRelativePosition(rect);
	// Don't show chat if disabled or empty or profiler is enabled
	guitext_chat->setVisible(
			show_chat && recent_chat_count != 0 && !show_profiler);
}

/******************************************************************************/
void the_game(bool &kill, bool random_input, InputHandler *input,
	IrrlichtDevice *device, gui::IGUIFont* font, std::string map_dir,
	std::string playername, std::string password,
	std::string address /* If "", local server is used */,
	u16 port, std::wstring &error_message, ChatBackend &chat_backend,
	const SubgameSpec &gamespec /* Used for local game */,
	bool simple_singleplayer_mode)
{
	GUIFormSpecMenu* current_formspec = 0;
	video::IVideoDriver* driver = device->getVideoDriver();
	scene::ISceneManager* smgr = device->getSceneManager();

	// Calculate text height using the font
	u32 text_height = font->getDimension(L"Random test string").Height;

	/*
		Draw "Loading" screen
	*/

	{
		wchar_t* text = wgettext("Loading...");
		draw_load_screen(text, device, guienv, font, 0, 0);
		delete[] text;
	}

	// Create texture source
	IWritableTextureSource *tsrc = createTextureSource(device);

	// Create shader source
	IWritableShaderSource *shsrc = createShaderSource(device);

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

	Server *server = NULL;

	try{
	// Event manager
	EventManager eventmgr;

	// Sound maker
	SoundMaker soundmaker(sound, nodedef);
	soundmaker.registerReceiver(&eventmgr);

	// Create UI for modifying quicktune values
	QuicktuneShortcutter quicktune;

	/*
		Create server.
	*/

	if(address == ""){
		wchar_t* text = wgettext("Creating server....");
		draw_load_screen(text, device, guienv, font, 0, 25);
		delete[] text;
		infostream<<"Creating server"<<std::endl;

		std::string bind_str = g_settings->get("bind_address");
		Address bind_addr(0,0,0,0, port);

		if (g_settings->getBool("ipv6_server")) {
			bind_addr.setAddress((IPv6AddressBytes*) NULL);
		}
		try {
			bind_addr.Resolve(bind_str.c_str());
			address = bind_str;
		} catch (ResolveError &e) {
			infostream << "Resolving bind address \"" << bind_str
					   << "\" failed: " << e.what()
					   << " -- Listening on all addresses." << std::endl;
		}

		if(bind_addr.isIPv6() && !g_settings->getBool("enable_ipv6")) {
			error_message = L"Unable to listen on " +
				narrow_to_wide(bind_addr.serializeString()) +
				L" because IPv6 is disabled";
			errorstream<<wide_to_narrow(error_message)<<std::endl;
			// Break out of client scope
			return;
		}

		server = new Server(map_dir, gamespec,
				simple_singleplayer_mode,
				bind_addr.isIPv6());

		server->start(bind_addr);
	}

	do{ // Client scope (breakable do-while(0))

	/*
		Create client
	*/

	{
		wchar_t* text = wgettext("Creating client...");
		draw_load_screen(text, device, guienv, font, 0, 50);
		delete[] text;
	}
	infostream<<"Creating client"<<std::endl;

	MapDrawControl draw_control;

	{
		wchar_t* text = wgettext("Resolving address...");
		draw_load_screen(text, device, guienv, font, 0, 75);
		delete[] text;
	}
	Address connect_address(0,0,0,0, port);
	try {
		connect_address.Resolve(address.c_str());
		if (connect_address.isZero()) { // i.e. INADDR_ANY, IN6ADDR_ANY
			//connect_address.Resolve("localhost");
			if (connect_address.isIPv6()) {
				IPv6AddressBytes addr_bytes;
				addr_bytes.bytes[15] = 1;
				connect_address.setAddress(&addr_bytes);
			} else {
				connect_address.setAddress(127,0,0,1);
			}
		}
	}
	catch(ResolveError &e) {
		error_message = L"Couldn't resolve address: " + narrow_to_wide(e.what());
		errorstream<<wide_to_narrow(error_message)<<std::endl;
		// Break out of client scope
		break;
	}
	if(connect_address.isIPv6() && !g_settings->getBool("enable_ipv6")) {
		error_message = L"Unable to connect to " +
			narrow_to_wide(connect_address.serializeString()) +
			L" because IPv6 is disabled";
		errorstream<<wide_to_narrow(error_message)<<std::endl;
		// Break out of client scope
		break;
	}

	/*
		Create client
	*/
	Client client(device, playername.c_str(), password, draw_control,
		tsrc, shsrc, itemdef, nodedef, sound, &eventmgr,
		connect_address.isIPv6());

	// Client acts as our GameDef
	IGameDef *gamedef = &client;

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
		float time_counter = 0.0;
		input->clear();
		float fps_max = g_settings->getFloat("fps_max");
		bool cloud_menu_background = g_settings->getBool("menu_clouds");
		u32 lasttime = device->getTimer()->getTime();
		while(device->run())
		{
			f32 dtime = 0.033; // in seconds
			if (cloud_menu_background) {
				u32 time = device->getTimer()->getTime();
				if(time > lasttime)
					dtime = (time - lasttime) / 1000.0;
				else
					dtime = 0;
				lasttime = time;
			}
			// Update client and server
			client.step(dtime);
			if(server != NULL)
				server->step(dtime);

			// End condition
			if(client.getState() == LC_Init) {
				could_connect = true;
				break;
			}
			// Break conditions
			if(client.accessDenied()) {
				error_message = L"Access denied. Reason: "
						+client.accessDeniedReason();
				errorstream<<wide_to_narrow(error_message)<<std::endl;
				break;
			}
			if(input->wasKeyDown(EscapeKey) || input->wasKeyDown(CancelKey)) {
				connect_aborted = true;
				infostream<<"Connect aborted [Escape]"<<std::endl;
				break;
			}

			// Display status
			{
				wchar_t* text = wgettext("Connecting to server...");
				draw_load_screen(text, device, guienv, font, dtime, 100);
				delete[] text;
			}

			// On some computers framerate doesn't seem to be
			// automatically limited
			if (cloud_menu_background) {
				// Time of frame without fps limit
				float busytime;
				u32 busytime_u32;
				// not using getRealTime is necessary for wine
				u32 time = device->getTimer()->getTime();
				if(time > lasttime)
					busytime_u32 = time - lasttime;
				else
					busytime_u32 = 0;
				busytime = busytime_u32 / 1000.0;

				// FPS limiter
				u32 frametime_min = 1000./fps_max;

				if(busytime_u32 < frametime_min) {
					u32 sleeptime = frametime_min - busytime_u32;
					device->sleep(sleeptime);
				}
			} else {
				sleep_ms(25);
			}
			time_counter += dtime;
		}
	}
	catch(con::PeerNotFoundException &e)
	{}

	/*
		Handle failure to connect
	*/
	if(!could_connect) {
		if(error_message == L"" && !connect_aborted) {
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
		float time_counter = 0.0;
		input->clear();
		float fps_max = g_settings->getFloat("fps_max");
		bool cloud_menu_background = g_settings->getBool("menu_clouds");
		u32 lasttime = device->getTimer()->getTime();
		while (device->run()) {
			f32 dtime = 0.033; // in seconds
			if (cloud_menu_background) {
				u32 time = device->getTimer()->getTime();
				if(time > lasttime)
					dtime = (time - lasttime) / 1000.0;
				else
					dtime = 0;
				lasttime = time;
			}
			// Update client and server
			client.step(dtime);
			if (server != NULL)
				server->step(dtime);

			// End condition
			if (client.mediaReceived() &&
					client.itemdefReceived() &&
					client.nodedefReceived()) {
				got_content = true;
				break;
			}
			// Break conditions
			if (client.accessDenied()) {
				error_message = L"Access denied. Reason: "
						+client.accessDeniedReason();
				errorstream<<wide_to_narrow(error_message)<<std::endl;
				break;
			}
			if (client.getState() < LC_Init) {
				error_message = L"Client disconnected";
				errorstream<<wide_to_narrow(error_message)<<std::endl;
				break;
			}
			if (input->wasKeyDown(EscapeKey) || input->wasKeyDown(CancelKey)) {
				content_aborted = true;
				infostream<<"Connect aborted [Escape]"<<std::endl;
				break;
			}

			// Display status
			int progress=0;
			if (!client.itemdefReceived())
			{
				wchar_t* text = wgettext("Item definitions...");
				progress = 0;
				draw_load_screen(text, device, guienv, font, dtime, progress);
				delete[] text;
			}
			else if (!client.nodedefReceived())
			{
				wchar_t* text = wgettext("Node definitions...");
				progress = 25;
				draw_load_screen(text, device, guienv, font, dtime, progress);
				delete[] text;
			}
			else
			{

				std::stringstream message;
				message.precision(3);
				message << gettext("Media...");

				if ( ( USE_CURL == 0) ||
						(!g_settings->getBool("enable_remote_media_server"))) {
					float cur = client.getCurRate();
					std::string cur_unit = gettext(" KB/s");

					if (cur > 900) {
						cur /= 1024.0;
						cur_unit = gettext(" MB/s");
					}
					message << " ( " << cur << cur_unit << " )";
				}
				progress = 50+client.mediaReceiveProgress()*50+0.5;
				draw_load_screen(narrow_to_wide(message.str().c_str()), device,
						guienv, font, dtime, progress);
			}

			// On some computers framerate doesn't seem to be
			// automatically limited
			if (cloud_menu_background) {
				// Time of frame without fps limit
				float busytime;
				u32 busytime_u32;
				// not using getRealTime is necessary for wine
				u32 time = device->getTimer()->getTime();
				if(time > lasttime)
					busytime_u32 = time - lasttime;
				else
					busytime_u32 = 0;
				busytime = busytime_u32 / 1000.0;

				// FPS limiter
				u32 frametime_min = 1000./fps_max;

				if(busytime_u32 < frametime_min) {
					u32 sleeptime = frametime_min - busytime_u32;
					device->sleep(sleeptime);
				}
			} else {
				sleep_ms(25);
			}
			time_counter += dtime;
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
	client.afterContentReceived(device,font);

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

	scene::ISceneNode* skybox = NULL;

	/*
		A copy of the local inventory
	*/
	Inventory local_inventory(itemdef);

	/*
		Find out size of crack animation
	*/
	int crack_animation_length = 5;
	{
		video::ITexture *t = tsrc->getTexture("crack_anylength.png");
		v2u32 size = t->getOriginalSize();
		crack_animation_length = size.Y / size.X;
	}

	/*
		Add some gui stuff
	*/

	// First line of debug text
	gui::IGUIStaticText *guitext = guienv->addStaticText(
			L"Minetest",
			core::rect<s32>(0, 0, 0, 0),
			false, false, guiroot);
	// Second line of debug text
	gui::IGUIStaticText *guitext2 = guienv->addStaticText(
			L"",
			core::rect<s32>(0, 0, 0, 0),
			false, false, guiroot);
	// At the middle of the screen
	// Object infos are shown in this
	gui::IGUIStaticText *guitext_info = guienv->addStaticText(
			L"",
			core::rect<s32>(0,0,400,text_height*5+5) + v2s32(100,200),
			false, true, guiroot);

	// Status text (displays info when showing and hiding GUI stuff, etc.)
	gui::IGUIStaticText *guitext_status = guienv->addStaticText(
			L"<Status>",
			core::rect<s32>(0,0,0,0),
			false, false, guiroot);
	guitext_status->setVisible(false);

	std::wstring statustext;
	float statustext_time = 0;

	// Chat text
	gui::IGUIStaticText *guitext_chat = guienv->addStaticText(
			L"",
			core::rect<s32>(0,0,0,0),
			//false, false); // Disable word wrap as of now
			false, true, guiroot);
	// Remove stale "recent" chat messages from previous connections
	chat_backend.clearRecentChat();
	// Chat backend and console
	GUIChatConsole *gui_chat_console = new GUIChatConsole(guienv, guienv->getRootGUIElement(), -1, &chat_backend, &client);

	// Profiler text (size is updated when text is updated)
	gui::IGUIStaticText *guitext_profiler = guienv->addStaticText(
			L"<Profiler>",
			core::rect<s32>(0,0,0,0),
			false, false, guiroot);
	guitext_profiler->setBackgroundColor(video::SColor(120,0,0,0));
	guitext_profiler->setVisible(false);
	guitext_profiler->setWordWrap(true);

#ifdef HAVE_TOUCHSCREENGUI
	if (g_touchscreengui)
		g_touchscreengui->init(tsrc,porting::getDisplayDensity());
#endif

	/*
		Some statistics are collected in these
	*/
	u32 drawtime = 0;
	u32 beginscenetime = 0;
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

	float damage_flash = 0;

	float jump_timer = 0;
	bool reset_jump_timer = false;

	const float object_hit_delay = 0.2;
	float object_hit_delay_timer = 0.0;
	float time_from_last_punch = 10;

	float update_draw_list_timer = 0.0;
	v3f update_draw_list_last_cam_dir;

	bool invert_mouse = g_settings->getBool("invert_mouse");

	bool update_wielded_item_trigger = true;

	bool show_hud = true;
	bool show_chat = true;
	bool force_fog_off = false;
	f32 fog_range = 100*BS;
	bool disable_camera_update = false;
	bool show_debug = g_settings->getBool("show_debug");
	bool show_profiler_graph = false;
	u32 show_profiler = 0;
	u32 show_profiler_max = 3;  // Number of pages

	float time_of_day = 0;
	float time_of_day_smooth = 0;

	float repeat_rightclick_timer = 0;

	/*
		Shader constants
	*/
	shsrc->addGlobalConstantSetter(new GameGlobalShaderConstantSetter(
			sky, &force_fog_off, &fog_range, &client));

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

	LocalPlayer* player = client.getEnv().getLocalPlayer();
	player->hurt_tilt_timer = 0;
	player->hurt_tilt_strength = 0;

	/*
		HUD object
	*/
	Hud hud(driver, smgr, guienv, font, text_height,
			gamedef, player, &local_inventory);

	core::stringw str = L"Minetest [";
	str += driver->getName();
	str += "]";
	device->setWindowCaption(str.c_str());

	// Info text
	std::wstring infotext;

	for(;;)
	{
		if(device->run() == false || kill == true ||
				g_gamecallback->shutdown_requested)
			break;

		v2u32 screensize = driver->getScreenSize();

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
			float fps_max = g_menumgr.pausesGame() ?
					g_settings->getFloat("pause_fps_max") :
					g_settings->getFloat("fps_max");
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

		if(g_gamecallback->changevolume_requested)
		{
			(new GUIVolumeChange(guienv, guiroot, -1,
				&g_menumgr, &client))->drop();
			g_gamecallback->changevolume_requested = false;
		}

		if(g_gamecallback->keyconfig_requested)
		{
			(new GUIKeyChangeMenu(guienv, guiroot, -1,
				&g_menumgr))->drop();
			g_gamecallback->keyconfig_requested = false;
		}


		/* Process TextureSource's queue */
		tsrc->processQueue();

		/* Process ItemDefManager's queue */
		itemdef->processQueue(gamedef);

		/*
			Process ShaderSource's queue
		*/
		shsrc->processQueue();

		/*
			Random calculations
		*/
		hud.resizeHotbar();

		// Hilight boxes collected during the loop and displayed
		std::vector<aabb3f> hilightboxes;

		/* reset infotext */
		infotext = L"";
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
		if (!guienv->hasFocus(gui_chat_console) && gui_chat_console->isOpen())
		{
			gui_chat_console->closeConsoleAtOnce();
		}

		// Input handler step() (used by the random input generator)
		input->step(dtime);
#ifdef HAVE_TOUCHSCREENGUI
		if (g_touchscreengui) {
			g_touchscreengui->step(dtime);
		}
#endif
#ifdef __ANDROID__
		if (current_formspec != 0)
			current_formspec->getAndroidUIInput();
#endif

		// Increase timer for doubleclick of "jump"
		if(g_settings->getBool("doubletap_jump") && jump_timer <= 0.2)
			jump_timer += dtime;

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

			PlayerInventoryFormSource* fs_src = new PlayerInventoryFormSource(&client);
			TextDest* txt_dst = new TextDestPlayerInventory(&client);

			create_formspec_menu(&current_formspec, &client, gamedef, tsrc, device, fs_src, txt_dst, &client);

			InventoryLocation inventoryloc;
			inventoryloc.setCurrentPlayer();
			current_formspec->setFormSpec(fs_src->getForm(), inventoryloc);
		}
		else if(input->wasKeyDown(EscapeKey) || input->wasKeyDown(CancelKey))
		{
			show_pause_menu(&current_formspec, &client, gamedef, tsrc, device,
					simple_singleplayer_mode);
		}
		else if(input->wasKeyDown(getKeySetting("keymap_chat")))
		{
			show_chat_menu(&current_formspec, &client, gamedef, tsrc, device,
					&client,"");
		}
		else if(input->wasKeyDown(getKeySetting("keymap_cmd")))
		{
			show_chat_menu(&current_formspec, &client, gamedef, tsrc, device,
					&client,"/");
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
		else if(input->wasKeyDown(getKeySetting("keymap_jump")))
		{
			if(g_settings->getBool("doubletap_jump") && jump_timer < 0.2)
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
			reset_jump_timer = true;
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
		else if(input->wasKeyDown(getKeySetting("keymap_noclip")))
		{
			if(g_settings->getBool("noclip"))
			{
				g_settings->set("noclip","false");
				statustext = L"noclip disabled";
				statustext_time = 0;
			}
			else
			{
				g_settings->set("noclip","true");
				statustext = L"noclip enabled";
				statustext_time = 0;
				if(!client.checkPrivilege("noclip"))
					statustext += L" (note: no 'noclip' privilege)";
			}
		}
		else if(input->wasKeyDown(getKeySetting("keymap_screenshot")))
		{
			client.makeScreenshot(device);
		}
		else if(input->wasKeyDown(getKeySetting("keymap_toggle_hud")))
		{
			show_hud = !show_hud;
			if(show_hud) {
				statustext = L"HUD shown";
				client.setHighlighted(client.getHighlighted(), true);
			} else {
				statustext = L"HUD hidden";
				client.setHighlighted(client.getHighlighted(), false);
			}
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

		// Reset jump_timer
		if(!input->isKeyDown(getKeySetting("keymap_jump")) && reset_jump_timer)
		{
			reset_jump_timer = false;
			jump_timer = 0.0;
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
					player->hud_hotbar_itemcount-1);

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
				if(i < PLAYER_INVENTORY_SIZE && i < player->hud_hotbar_itemcount)
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
#ifndef __ANDROID__
			if(!random_input)
			{
				// Mac OSX gets upset if this is set every frame
				if(device->getCursorControl()->isVisible())
					device->getCursorControl()->setVisible(false);
			}
#endif

			if(first_loop_after_window_activation){
				//infostream<<"window active, first loop"<<std::endl;
				first_loop_after_window_activation = false;
			} else {
#ifdef HAVE_TOUCHSCREENGUI
				if (g_touchscreengui) {
					camera_yaw   = g_touchscreengui->getYaw();
					camera_pitch = g_touchscreengui->getPitch();
				} else {
#endif
					s32 dx = input->getMousePos().X - (driver->getScreenSize().Width/2);
					s32 dy = input->getMousePos().Y - (driver->getScreenSize().Height/2);
				if ((invert_mouse)
						|| (camera.getCameraMode() == CAMERA_MODE_THIRD_FRONT)) {
					dy = -dy;
				}
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

				float d = g_settings->getFloat("mouse_sensitivity");
				d = rangelim(d, 0.01, 100.0);
				camera_yaw -= dx*d;
				camera_pitch += dy*d;
				turn_amount = v2f(dx, dy).getLength() * d;

#ifdef HAVE_TOUCHSCREENGUI
				}
#endif
				if(camera_pitch < -89.5) camera_pitch = -89.5;
				if(camera_pitch > 89.5) camera_pitch = 89.5;
			}
			input->setMousePos((driver->getScreenSize().Width/2),
					(driver->getScreenSize().Height/2));
		}
		else{
#ifndef ANDROID
			// Mac OSX gets upset if this is set every frame
			if(device->getCursorControl()->isVisible() == false)
				device->getCursorControl()->setVisible(true);
#endif

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
			bool a_LMB,
			bool a_RMB,
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
				input->getLeftState(),
				input->getRightState(),
				camera_pitch,
				camera_yaw
			);
			client.setPlayerControl(control);
			LocalPlayer* player = client.getEnv().getLocalPlayer();
			player->keyPressed=
			(((int)input->isKeyDown(getKeySetting("keymap_forward"))  & 0x1) << 0) |
			(((int)input->isKeyDown(getKeySetting("keymap_backward")) & 0x1) << 1) |
			(((int)input->isKeyDown(getKeySetting("keymap_left"))     & 0x1) << 2) |
			(((int)input->isKeyDown(getKeySetting("keymap_right"))    & 0x1) << 3) |
			(((int)input->isKeyDown(getKeySetting("keymap_jump"))     & 0x1) << 4) |
			(((int)input->isKeyDown(getKeySetting("keymap_special1")) & 0x1) << 5) |
			(((int)input->isKeyDown(getKeySetting("keymap_sneak"))    & 0x1) << 6) |
			(((int)input->getLeftState()  & 0x1) << 7) |
			(((int)input->getRightState() & 0x1) << 8);
		}

		/*
			Run server, client (and process environments)
		*/
		bool can_be_and_is_paused =
				(simple_singleplayer_mode && g_menumgr.pausesGame());
		if(can_be_and_is_paused)
		{
			// No time passes
			dtime = 0;
		}
		else
		{
			if(server != NULL)
			{
				//TimeTaker timer("server->step(dtime)");
				server->step(dtime);
			}
			{
				//TimeTaker timer("client.step(dtime)");
				client.step(dtime);
			}
		}

		{
			// Read client events
			for(;;) {
				ClientEvent event = client.getClientEvent();
				if(event.type == CE_NONE) {
					break;
				}
				else if(event.type == CE_PLAYER_DAMAGE &&
						client.getHP() != 0) {
					//u16 damage = event.player_damage.amount;
					//infostream<<"Player damage: "<<damage<<std::endl;

					damage_flash += 100.0;
					damage_flash += 8.0 * event.player_damage.amount;

					player->hurt_tilt_timer = 1.5;
					player->hurt_tilt_strength = event.player_damage.amount/4;
					player->hurt_tilt_strength = rangelim(player->hurt_tilt_strength, 1.0, 4.0);

					MtEvent *e = new SimpleTriggerEvent("PlayerDamage");
					gamedef->event()->put(e);
				}
				else if(event.type == CE_PLAYER_FORCE_MOVE) {
					camera_yaw = event.player_force_move.yaw;
					camera_pitch = event.player_force_move.pitch;
				}
				else if(event.type == CE_DEATHSCREEN) {
					show_deathscreen(&current_formspec, &client, gamedef, tsrc,
							device, &client);

					chat_backend.addMessage(L"", L"You died.");

					/* Handle visualization */
					damage_flash = 0;

					LocalPlayer* player = client.getEnv().getLocalPlayer();
					player->hurt_tilt_timer = 0;
					player->hurt_tilt_strength = 0;

				}
				else if (event.type == CE_SHOW_FORMSPEC) {
					FormspecFormSource* fs_src =
							new FormspecFormSource(*(event.show_formspec.formspec));
					TextDestPlayerInventory* txt_dst =
							new TextDestPlayerInventory(&client,*(event.show_formspec.formname));

					create_formspec_menu(&current_formspec, &client, gamedef,
							tsrc, device, fs_src, txt_dst, &client);

					delete(event.show_formspec.formspec);
					delete(event.show_formspec.formname);
				}
				else if(event.type == CE_SPAWN_PARTICLE) {
					LocalPlayer* player = client.getEnv().getLocalPlayer();
					video::ITexture *texture =
						gamedef->tsrc()->getTexture(*(event.spawn_particle.texture));

					new Particle(gamedef, smgr, player, client.getEnv(),
						*event.spawn_particle.pos,
						*event.spawn_particle.vel,
						*event.spawn_particle.acc,
						 event.spawn_particle.expirationtime,
						 event.spawn_particle.size,
						 event.spawn_particle.collisiondetection,
						 event.spawn_particle.vertical,
						 texture,
						 v2f(0.0, 0.0),
						 v2f(1.0, 1.0));
				}
				else if(event.type == CE_ADD_PARTICLESPAWNER) {
					LocalPlayer* player = client.getEnv().getLocalPlayer();
					video::ITexture *texture =
						gamedef->tsrc()->getTexture(*(event.add_particlespawner.texture));

					new ParticleSpawner(gamedef, smgr, player,
						 event.add_particlespawner.amount,
						 event.add_particlespawner.spawntime,
						*event.add_particlespawner.minpos,
						*event.add_particlespawner.maxpos,
						*event.add_particlespawner.minvel,
						*event.add_particlespawner.maxvel,
						*event.add_particlespawner.minacc,
						*event.add_particlespawner.maxacc,
						 event.add_particlespawner.minexptime,
						 event.add_particlespawner.maxexptime,
						 event.add_particlespawner.minsize,
						 event.add_particlespawner.maxsize,
						 event.add_particlespawner.collisiondetection,
						 event.add_particlespawner.vertical,
						 texture,
						 event.add_particlespawner.id);
				}
				else if(event.type == CE_DELETE_PARTICLESPAWNER) {
					delete_particlespawner (event.delete_particlespawner.id);
				}
				else if (event.type == CE_HUDADD) {
					u32 id = event.hudadd.id;

					HudElement *e = player->getHud(id);

					if (e != NULL) {
						delete event.hudadd.pos;
						delete event.hudadd.name;
						delete event.hudadd.scale;
						delete event.hudadd.text;
						delete event.hudadd.align;
						delete event.hudadd.offset;
						delete event.hudadd.world_pos;
						delete event.hudadd.size;
						continue;
					}

					e = new HudElement;
					e->type   = (HudElementType)event.hudadd.type;
					e->pos    = *event.hudadd.pos;
					e->name   = *event.hudadd.name;
					e->scale  = *event.hudadd.scale;
					e->text   = *event.hudadd.text;
					e->number = event.hudadd.number;
					e->item   = event.hudadd.item;
					e->dir    = event.hudadd.dir;
					e->align  = *event.hudadd.align;
					e->offset = *event.hudadd.offset;
					e->world_pos = *event.hudadd.world_pos;
					e->size = *event.hudadd.size;

					u32 new_id = player->addHud(e);
					//if this isn't true our huds aren't consistent
					assert(new_id == id);

					delete event.hudadd.pos;
					delete event.hudadd.name;
					delete event.hudadd.scale;
					delete event.hudadd.text;
					delete event.hudadd.align;
					delete event.hudadd.offset;
					delete event.hudadd.world_pos;
					delete event.hudadd.size;
				}
				else if (event.type == CE_HUDRM) {
					HudElement* e = player->removeHud(event.hudrm.id);

					if (e != NULL)
						delete (e);
				}
				else if (event.type == CE_HUDCHANGE) {
					u32 id = event.hudchange.id;
					HudElement* e = player->getHud(id);
					if (e == NULL)
					{
						delete event.hudchange.v3fdata;
						delete event.hudchange.v2fdata;
						delete event.hudchange.sdata;
						delete event.hudchange.v2s32data;
						continue;
					}

					switch (event.hudchange.stat) {
						case HUD_STAT_POS:
							e->pos = *event.hudchange.v2fdata;
							break;
						case HUD_STAT_NAME:
							e->name = *event.hudchange.sdata;
							break;
						case HUD_STAT_SCALE:
							e->scale = *event.hudchange.v2fdata;
							break;
						case HUD_STAT_TEXT:
							e->text = *event.hudchange.sdata;
							break;
						case HUD_STAT_NUMBER:
							e->number = event.hudchange.data;
							break;
						case HUD_STAT_ITEM:
							e->item = event.hudchange.data;
							break;
						case HUD_STAT_DIR:
							e->dir = event.hudchange.data;
							break;
						case HUD_STAT_ALIGN:
							e->align = *event.hudchange.v2fdata;
							break;
						case HUD_STAT_OFFSET:
							e->offset = *event.hudchange.v2fdata;
							break;
						case HUD_STAT_WORLD_POS:
							e->world_pos = *event.hudchange.v3fdata;
							break;
						case HUD_STAT_SIZE:
							e->size = *event.hudchange.v2s32data;
							break;
					}

					delete event.hudchange.v3fdata;
					delete event.hudchange.v2fdata;
					delete event.hudchange.sdata;
					delete event.hudchange.v2s32data;
				}
				else if (event.type == CE_SET_SKY) {
					sky->setVisible(false);
					if(skybox){
						skybox->remove();
						skybox = NULL;
					}
					// Handle according to type
					if(*event.set_sky.type == "regular") {
						sky->setVisible(true);
					}
					else if(*event.set_sky.type == "skybox" &&
							event.set_sky.params->size() == 6) {
						sky->setFallbackBgColor(*event.set_sky.bgcolor);
						skybox = smgr->addSkyBoxSceneNode(
								tsrc->getTexture((*event.set_sky.params)[0]),
								tsrc->getTexture((*event.set_sky.params)[1]),
								tsrc->getTexture((*event.set_sky.params)[2]),
								tsrc->getTexture((*event.set_sky.params)[3]),
								tsrc->getTexture((*event.set_sky.params)[4]),
								tsrc->getTexture((*event.set_sky.params)[5]));
					}
					// Handle everything else as plain color
					else {
						if(*event.set_sky.type != "plain")
							infostream<<"Unknown sky type: "
									<<(*event.set_sky.type)<<std::endl;
						sky->setFallbackBgColor(*event.set_sky.bgcolor);
					}

					delete event.set_sky.bgcolor;
					delete event.set_sky.type;
					delete event.set_sky.params;
				}
				else if (event.type == CE_OVERRIDE_DAY_NIGHT_RATIO) {
					bool enable = event.override_day_night_ratio.do_override;
					u32 value = event.override_day_night_ratio.ratio_f * 1000;
					client.getEnv().setDayNightRatioOverride(enable, value);
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
		{
			InventoryList *mlist = local_inventory.getList("main");
			if((mlist != NULL) && (client.getPlayerItem() < mlist->getSize()))
				playeritem = mlist->getItem(client.getPlayerItem());
		}
		const ItemDefinition &playeritem_def =
				playeritem.getDefinition(itemdef);
		ToolCapabilities playeritem_toolcap =
				playeritem.getToolCapabilities(itemdef);

		/*
			Update camera
		*/

		v3s16 old_camera_offset = camera.getOffset();

		LocalPlayer* player = client.getEnv().getLocalPlayer();
		float full_punch_interval = playeritem_toolcap.full_punch_interval;
		float tool_reload_ratio = time_from_last_punch / full_punch_interval;

		if(input->wasKeyDown(getKeySetting("keymap_camera_mode"))) {
			camera.toggleCameraMode();
			GenericCAO* playercao = player->getCAO();

			assert( playercao != NULL );
			if (camera.getCameraMode() > CAMERA_MODE_FIRST) {
				playercao->setVisible(true);
			}
			else {
				playercao->setVisible(false);
			}
		}
		tool_reload_ratio = MYMIN(tool_reload_ratio, 1.0);
		camera.update(player, dtime, busytime, tool_reload_ratio,
				client.getEnv());
		camera.step(dtime);

		v3f player_position = player->getPosition();
		v3f camera_position = camera.getPosition();
		v3f camera_direction = camera.getDirection();
		f32 camera_fov = camera.getFovMax();
		v3s16 camera_offset = camera.getOffset();

		bool camera_offset_changed = (camera_offset != old_camera_offset);

		if(!disable_camera_update){
			client.getEnv().getClientMap().updateCamera(camera_position,
				camera_direction, camera_fov, camera_offset);
			if (camera_offset_changed){
				client.updateCameraOffset(camera_offset);
				client.getEnv().updateCameraOffset(camera_offset);
				if (clouds)
					clouds->updateCameraOffset(camera_offset);
			}
		}

		// Update sound listener
		sound->updateListener(camera.getCameraNode()->getPosition()+intToFloat(camera_offset, BS),
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

		f32 d = playeritem_def.range; // max. distance
		f32 d_hand = itemdef->get("").range;
		if(d < 0 && d_hand >= 0)
			d = d_hand;
		else if(d < 0)
			d = 4.0;
		core::line3d<f32> shootline(camera_position,
				camera_position + camera_direction * BS * (d+1));


		// prevent player pointing anything in front-view
		if (camera.getCameraMode() == CAMERA_MODE_THIRD_FRONT)
			shootline = core::line3d<f32>(0,0,0,0,0,0);

#ifdef HAVE_TOUCHSCREENGUI
		if ((g_settings->getBool("touchtarget")) && (g_touchscreengui)) {
			shootline = g_touchscreengui->getShootline();
			shootline.start += intToFloat(camera_offset,BS);
			shootline.end += intToFloat(camera_offset,BS);
		}
#endif

		ClientActiveObject *selected_object = NULL;

		PointedThing pointed = getPointedThing(
				// input
				&client, player_position, camera_direction,
				camera_position, shootline, d,
				playeritem_def.liquids_pointable, !ldown_for_dig,
				camera_offset,
				// output
				hilightboxes,
				selected_object);

		if(pointed != pointed_old)
		{
			infostream<<"Pointing at "<<pointed.dump()<<std::endl;
			if (g_settings->getBool("enable_node_highlighting")) {
				if (pointed.type == POINTEDTHING_NODE) {
					client.setHighlighted(pointed.node_undersurface, show_hud);
				} else {
					client.setHighlighted(pointed.node_undersurface, false);
				}
			}
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

		if(input->getRightState())
			repeat_rightclick_timer += dtime;
		else
			repeat_rightclick_timer = 0;

		if(playeritem_def.usable && input->getLeftState())
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
				if(nodedef->get(n).tiledef[0].name == "unknown_node.png"){
					infotext = L"Unknown node: ";
					infotext += narrow_to_wide(nodedef->get(n).name);
				}
			}

			/*
				Handle digging
			*/

			if(nodig_delay_timer <= 0.0 && input->getLeftState()
					&& client.checkPrivilege("interact"))
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

				float dig_time_complete = 0.0;

				if(params.diggable == false)
				{
					// I guess nobody will wait for this long
					dig_time_complete = 10000000.0;
				}
				else
				{
					dig_time_complete = params.time;
					if (g_settings->getBool("enable_particles"))
					{
						const ContentFeatures &features =
							client.getNodeDefManager()->get(n);
						addPunchingParticles
							(gamedef, smgr, player, client.getEnv(),
							 nodepos, features.tiles);
					}
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

				SimpleSoundSpec sound_dig = nodedef->get(n).sound_dig;
				if(sound_dig.exists() && params.diggable){
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

					if (g_settings->getBool("enable_particles"))
					{
						const ContentFeatures &features =
							client.getNodeDefManager()->get(wasnode);
						addDiggingParticles
							(gamedef, smgr, player, client.getEnv(),
							 nodepos, features.tiles);
					}

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

				if(dig_time_complete < 100000.0)
					dig_time += dtime;
				else {
					dig_time = 0;
					client.setCrack(-1, nodepos);
				}

				camera.setDigging(0);  // left click animation
			}

			if((input->getRightClicked() ||
					repeat_rightclick_timer >=
						g_settings->getFloat("repeat_rightclick_time")) &&
					client.checkPrivilege("interact"))
			{
				repeat_rightclick_timer = 0;
				infostream<<"Ground right-clicked"<<std::endl;

				if(meta && meta->getString("formspec") != "" && !random_input
						&& !input->isKeyDown(getKeySetting("keymap_sneak")))
				{
					infostream<<"Launching custom inventory view"<<std::endl;

					InventoryLocation inventoryloc;
					inventoryloc.setNodeMeta(nodepos);

					NodeMetadataFormSource* fs_src = new NodeMetadataFormSource(
							&client.getEnv().getClientMap(), nodepos);
					TextDest* txt_dst = new TextDestNodeMetadata(nodepos, &client);

					create_formspec_menu(&current_formspec, &client, gamedef,
							tsrc, device, fs_src, txt_dst, &client);

					current_formspec->setFormSpec(meta->getString("formspec"), inventoryloc);
				}
				// Otherwise report right click to server
				else
				{
					camera.setDigging(1);  // right click animation (always shown for feedback)

					// If the wielded item has node placement prediction,
					// make that happen
					bool placed = nodePlacementPrediction(client,
						playeritem_def,
						nodepos, neighbourpos);

					if(placed) {
						// Report to server
						client.interact(3, pointed);
						// Read the sound
						soundmaker.m_player_rightpunch_sound =
							playeritem_def.sound_place;
					} else {
						soundmaker.m_player_rightpunch_sound =
							SimpleSoundSpec();
					}

					if (playeritem_def.node_placement_prediction == "" ||
						nodedef->get(map.getNode(nodepos)).rightclickable)
						client.interact(3, pointed); // Report to server
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

		if(draw_control.range_all)
			fog_range = 100000*BS;
		else {
			fog_range = draw_control.wanted_range*BS + 0.0*MAP_BLOCKSIZE*BS;
			fog_range = MYMIN(fog_range, (draw_control.farthest_drawn+20)*BS);
			fog_range *= 0.9;
		}

		/*
			Calculate general brightness
		*/
		u32 daynight_ratio = client.getEnv().getDayNightRatio();
		float time_brightness = decode_light_f((float)daynight_ratio/1000.0);
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
				sunlight_seen,camera.getCameraMode(), player->getYaw(),
				player->getPitch());

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
			Update particles
		*/

		allparticles_step(dtime);
		allparticlespawners_step(dtime, client.getEnv());

		/*
			Fog
		*/

		if(g_settings->getBool("enable_fog") && !force_fog_off)
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

			u16 fps = (1.0/dtime_avg1);

			std::ostringstream os(std::ios_base::binary);
			os<<std::fixed
				<<"Minetest "<<minetest_version_hash
				<<" FPS = "<<fps
				<<" (R: range_all="<<draw_control.range_all<<")"
				<<std::setprecision(0)
				<<" drawtime = "<<drawtime_avg
				<<std::setprecision(1)
				<<", dtime_jitter = "
				<<(dtime_jitter1_max_fraction * 100.0)<<" %"
				<<std::setprecision(1)
				<<", v_range = "<<draw_control.wanted_range
				<<std::setprecision(3)
				<<", RTT = "<<client.getRTT();
			guitext->setText(narrow_to_wide(os.str()).c_str());
			guitext->setVisible(true);
		}
		else if(show_hud || show_chat)
		{
			std::ostringstream os(std::ios_base::binary);
			os<<"Minetest "<<minetest_version_hash;
			guitext->setText(narrow_to_wide(os.str()).c_str());
			guitext->setVisible(true);
		}
		else
		{
			guitext->setVisible(false);
		}

		if (guitext->isVisible())
		{
			core::rect<s32> rect(
				5,
				5,
				screensize.X,
				5 + text_height
			);
			guitext->setRelativePosition(rect);
		}

		if(show_debug)
		{
			std::ostringstream os(std::ios_base::binary);
			os<<std::setprecision(1)<<std::fixed
				<<"(" <<(player_position.X/BS)
				<<", "<<(player_position.Y/BS)
				<<", "<<(player_position.Z/BS)
				<<") (yaw="<<(wrapDegrees_0_360(camera_yaw))
				<<") (seed = "<<((u64)client.getMapSeed())
				<<")";
			guitext2->setText(narrow_to_wide(os.str()).c_str());
			guitext2->setVisible(true);

			core::rect<s32> rect(
				5,
				5 + text_height,
				screensize.X,
				5 + (text_height * 2)
			);
			guitext2->setRelativePosition(rect);
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
						10 + guitext_status->getTextWidth(),
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
		updateChat(client, dtime, show_debug, screensize, show_chat,
				show_profiler, chat_backend, guitext_chat, font);

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
			if((mlist != NULL) && (client.getPlayerItem() < mlist->getSize()))
				item = mlist->getItem(client.getPlayerItem());
			camera.wield(item, client.getPlayerItem());
		}

		/*
			Update block draw list every 200ms or when camera direction has
			changed much
		*/
		update_draw_list_timer += dtime;
		if(update_draw_list_timer >= 0.2 ||
				update_draw_list_last_cam_dir.getDistanceFrom(camera_direction) > 0.2 ||
				camera_offset_changed){
			update_draw_list_timer = 0;
			client.getEnv().getClientMap().updateDrawList(driver);
			update_draw_list_last_cam_dir = camera_direction;
		}

		/*
			1. Delete formspec menu reference if menu was removed
			2. Else, make sure formspec menu is on top
		*/
		if (current_formspec) {
			if (current_formspec->getReferenceCount() == 1) {
				current_formspec->drop();
				current_formspec = NULL;
			} else if (!noMenuActive()) {
				guiroot->bringToFront(current_formspec);
			}
		}

		/*
			Drawing begins
		*/
		TimeTaker tt_draw("mainloop: draw");
		{
			TimeTaker timer("beginScene");
			driver->beginScene(true, true, skycolor);
			beginscenetime = timer.stop(true);
		}


		draw_scene(driver, smgr, camera, client, player, hud, guienv,
				hilightboxes, screensize, skycolor, show_hud);

		/*
			Profiler graph
		*/
		if(show_profiler_graph)
		{
			graph.draw(10, screensize.Y - 10, driver, font);
		}

		/*
			Damage flash
		*/
		if(damage_flash > 0.0)
		{
			video::SColor color(std::min(damage_flash, 180.0f),180,0,0);
			driver->draw2DRectangle(color,
					core::rect<s32>(0,0,screensize.X,screensize.Y),
					NULL);

			damage_flash -= 100.0*dtime;
		}

		/*
			Damage camera tilt
		*/
		if(player->hurt_tilt_timer > 0.0)
		{
			player->hurt_tilt_timer -= dtime*5;
			if(player->hurt_tilt_timer < 0)
				player->hurt_tilt_strength = 0;
		}

		/*
			End scene
		*/
		{
			TimeTaker timer("endScene");
			driver->endScene();
			endscenetime = timer.stop(true);
		}

		drawtime = tt_draw.stop(true);
		g_profiler->graphAdd("mainloop_draw", (float)drawtime/1000.0f);

		/*
			End of drawing
		*/

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
	if (clouds)
		clouds->drop();
	if (gui_chat_console)
		gui_chat_console->drop();
	if (sky)
		sky->drop();
	clear_particles();

	/* cleanup menus */
	while (g_menumgr.menuCount() > 0)
	{
		g_menumgr.m_stack.front()->setVisible(false);
		g_menumgr.deletingMenu(g_menumgr.m_stack.front());
	}
	if (current_formspec) {
		current_formspec->drop();
		current_formspec = NULL;
	}

	/*
		Draw a "shutting down" screen, which will be shown while the map
		generator and other stuff quits
	*/
	{
		wchar_t* text = wgettext("Shutting down stuff...");
		draw_load_screen(text, device, guienv, font, 0, -1, false);
		delete[] text;
	}

	chat_backend.addMessage(L"", L"# Disconnected.");
	chat_backend.addMessage(L"", L"");

	client.Stop();

	//force answer all texture and shader jobs (TODO return empty values)

	while(!client.isShutdown()) {
		tsrc->processQueue();
		shsrc->processQueue();
		sleep_ms(100);
	}

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
	catch(ServerError &e) {
		error_message = narrow_to_wide(e.what());
		errorstream << "ServerError: " << e.what() << std::endl;
	}
	catch(ModError &e) {
		errorstream << "ModError: " << e.what() << std::endl;
		error_message = narrow_to_wide(e.what()) + wgettext("\nCheck debug.txt for details.");
	}



	if(!sound_is_dummy)
		delete sound;

	//has to be deleted first to stop all server threads
	delete server;

	delete tsrc;
	delete shsrc;
	delete nodedef;
	delete itemdef;

	//extended resource accounting
	infostream << "Irrlicht resources after cleanup:" << std::endl;
	infostream << "\tRemaining meshes   : "
		<< device->getSceneManager()->getMeshCache()->getMeshCount() << std::endl;
	infostream << "\tRemaining textures : "
		<< driver->getTextureCount() << std::endl;
	for (unsigned int i = 0; i < driver->getTextureCount(); i++ ) {
		irr::video::ITexture* texture = driver->getTextureByIndex(i);
		infostream << "\t\t" << i << ":" << texture->getName().getPath().c_str()
				<< std::endl;
	}
	clearTextureNameCache();
	infostream << "\tRemaining materials: "
		<< driver-> getMaterialRendererCount ()
		<< " (note: irrlicht doesn't support removing renderers)"<< std::endl;
}
