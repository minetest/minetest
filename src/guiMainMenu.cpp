/*
Minetest-c55
Copyright (C) 2010-12 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "guiMainMenu.h"
#include "guiKeyChangeMenu.h"
#include "guiCreateWorld.h"
#include "guiMessageMenu.h"
#include "guiConfirmMenu.h"
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include <IGUIListBox.h>
#include <IGUITabControl.h>
#include <IGUIImage.h>
// For IGameCallback
#include "guiPauseMenu.h"
#include "gettext.h"
#include "tile.h" // getTexturePath
#include "filesys.h"
#include "util/string.h"
#include "subgame.h"

struct CreateWorldDestMainMenu : public CreateWorldDest
{
	CreateWorldDestMainMenu(GUIMainMenu *menu):
		m_menu(menu)
	{}
	void accepted(std::wstring name, std::string gameid)
	{
		std::string name_narrow = wide_to_narrow(name);
		if(!string_allowed_blacklist(name_narrow, WORLDNAME_BLACKLISTED_CHARS))
		{
			m_menu->displayMessageMenu(wgettext("Cannot create world: Name contains invalid characters"));
			return;
		}
		std::vector<WorldSpec> worlds = getAvailableWorlds();
		for(std::vector<WorldSpec>::iterator i = worlds.begin();
		    i != worlds.end(); i++)
		{
			if((*i).name == name_narrow)
			{
				m_menu->displayMessageMenu(wgettext("Cannot create world: A world by this name already exists"));
				return;
			}
		}
		m_menu->createNewWorld(name, gameid);
	}
	GUIMainMenu *m_menu;
};

struct ConfirmDestDeleteWorld : public ConfirmDest
{
	ConfirmDestDeleteWorld(WorldSpec spec, GUIMainMenu *menu,
			const std::vector<std::string> &paths):
		m_spec(spec),
		m_menu(menu),
		m_paths(paths)
	{}
	void answer(bool answer)
	{
		if(answer == false)
			return;
		m_menu->deleteWorld(m_paths);
	}
	WorldSpec m_spec;
	GUIMainMenu *m_menu;
	std::vector<std::string> m_paths;
};

enum
{
	GUI_ID_QUIT_BUTTON = 101,
	GUI_ID_NAME_INPUT,
	GUI_ID_ADDRESS_INPUT,
	GUI_ID_PORT_INPUT,
	GUI_ID_FANCYTREE_CB,
	GUI_ID_SMOOTH_LIGHTING_CB,
	GUI_ID_3D_CLOUDS_CB,
	GUI_ID_OPAQUE_WATER_CB,
	GUI_ID_MIPMAP_CB,
	GUI_ID_ANISOTROPIC_CB,
	GUI_ID_BILINEAR_CB,
	GUI_ID_TRILINEAR_CB,
	GUI_ID_SHADERS_CB,
	GUI_ID_PRELOAD_ITEM_VISUALS_CB,
	GUI_ID_DAMAGE_CB,
	GUI_ID_CREATIVE_CB,
	GUI_ID_JOIN_GAME_BUTTON,
	GUI_ID_CHANGE_KEYS_BUTTON,
	GUI_ID_DELETE_WORLD_BUTTON,
	GUI_ID_CREATE_WORLD_BUTTON,
	GUI_ID_CONFIGURE_WORLD_BUTTON,
	GUI_ID_WORLD_LISTBOX,
	GUI_ID_TAB_CONTROL,
};

enum
{
	TAB_SINGLEPLAYER=0,
	TAB_MULTIPLAYER,
	TAB_ADVANCED,
	TAB_SETTINGS,
	TAB_CREDITS
};

GUIMainMenu::GUIMainMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		MainMenuData *data,
		IGameCallback *gamecallback
):
	GUIModalMenu(env, parent, id, menumgr),
	m_data(data),
	m_accepted(false),
	m_gamecallback(gamecallback),
	m_is_regenerating(false)
{
	assert(m_data);
	this->env = env;
	this->parent = parent;
	this->id = id;
	this->menumgr = menumgr;
}

GUIMainMenu::~GUIMainMenu()
{
	removeChildren();
}

void GUIMainMenu::removeChildren()
{
	const core::list<gui::IGUIElement*> &children = getChildren();
	core::list<gui::IGUIElement*> children_copy;
	for(core::list<gui::IGUIElement*>::ConstIterator
			i = children.begin(); i != children.end(); i++)
	{
		children_copy.push_back(*i);
	}
	for(core::list<gui::IGUIElement*>::Iterator
			i = children_copy.begin();
			i != children_copy.end(); i++)
	{
		(*i)->remove();
	}
}

void GUIMainMenu::regenerateGui(v2u32 screensize)
{
	m_is_regenerating = true;
	/*
		Read stuff from elements into m_data
	*/
	readInput(m_data);

	/*
		Remove stuff
	*/
	removeChildren();
	
	/*
		Calculate new sizes and positions
	*/
	
	v2s32 size(screensize.X, screensize.Y);

	core::rect<s32> rect(
			screensize.X/2 - size.X/2,
			screensize.Y/2 - size.Y/2,
			screensize.X/2 + size.X/2,
			screensize.Y/2 + size.Y/2
	);

	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	//v2s32 size = rect.getSize();

	/*
		Add stuff
	*/

	changeCtype("");

	// Version
	//if(m_data->selected_tab != TAB_CREDITS)
	{
		core::rect<s32> rect(0, 0, size.X, 40);
		rect += v2s32(4, 0);
		Environment->addStaticText(narrow_to_wide(
				"Minetest-c55 " VERSION_STRING).c_str(),
				rect, false, true, this, -1);
	}

	//v2s32 center(size.X/2, size.Y/2);
	v2s32 c800(size.X/2-400, size.Y/2-300);
	
	m_topleft_client = c800 + v2s32(90, 70+50+30);
	m_size_client = v2s32(620, 270);

	m_size_server = v2s32(620, 140);

	if(m_data->selected_tab == TAB_ADVANCED)
	{
		m_topleft_client = c800 + v2s32(90, 70+50+30);
		m_size_client = v2s32(620, 200);

		m_size_server = v2s32(620, 140);
	}

	m_topleft_server = m_topleft_client + v2s32(0, m_size_client.Y+20);
	
	// Tabs
#if 1
	{
		core::rect<s32> rect(0, 0, m_size_client.X, 30);
		rect += m_topleft_client + v2s32(0, -30);
		gui::IGUITabControl *e = Environment->addTabControl(
				rect, this, true, true, GUI_ID_TAB_CONTROL);
		e->addTab(wgettext("Singleplayer"));
		e->addTab(wgettext("Multiplayer"));
		e->addTab(wgettext("Advanced"));
		e->addTab(wgettext("Settings"));
		e->addTab(wgettext("Credits"));
		e->setActiveTab(m_data->selected_tab);
	}
#endif
	
	if(m_data->selected_tab == TAB_SINGLEPLAYER)
	{
		// HYBRID
		{
			core::rect<s32> rect(0, 0, 10, m_size_client.Y);
			rect += m_topleft_client + v2s32(15, 0);
			//const wchar_t *text = L"H\nY\nB\nR\nI\nD";
			const wchar_t *text = L"T\nA\nP\nE\n\nA\nN\nD\n\nG\nL\nU\nE";
			gui::IGUIStaticText *t =
			Environment->addStaticText(text, rect, false, false, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
		}
		u32 bs = 5;
		// World selection listbox
		u32 world_sel_h = 160;
		u32 world_sel_w = 365;
		//s32 world_sel_x = 50;
		s32 world_sel_x = m_size_client.X-world_sel_w-30;
		s32 world_sel_y = 30;
		u32 world_button_count = 3;
		u32 world_button_w = (world_sel_w)/world_button_count - bs
				+ bs/(world_button_count-1);
		{
			core::rect<s32> rect(0, 0, world_sel_w-4, 20);
			rect += m_topleft_client + v2s32(world_sel_x+4, world_sel_y-20);
			/*gui::IGUIStaticText *e =*/ Environment->addStaticText(
					wgettext("Select World:"), 
					rect, false, true, this, -1);
			/*e->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);*/
		}
		{
			core::rect<s32> rect(0, 0, world_sel_w, world_sel_h);
			rect += m_topleft_client + v2s32(world_sel_x, world_sel_y);
			gui::IGUIListBox *e = Environment->addListBox(rect, this,
					GUI_ID_WORLD_LISTBOX);
			e->setDrawBackground(true);
			for(std::vector<WorldSpec>::const_iterator i = m_data->worlds.begin();
					i != m_data->worlds.end(); i++){
				e->addItem(narrow_to_wide(i->name+" ["+i->gameid+"]").c_str());
			}
			e->setSelected(m_data->selected_world);
			Environment->setFocus(e);
		}
		// Delete world button
		{
			core::rect<s32> rect(0, 0, world_button_w, 30);
			rect += m_topleft_client + v2s32(world_sel_x, world_sel_y+world_sel_h+0);
			Environment->addButton(rect, this, GUI_ID_DELETE_WORLD_BUTTON,
				  wgettext("Delete"));
		}
		// Create world button
		{
			core::rect<s32> rect(0, 0, world_button_w, 30);
			rect += m_topleft_client + v2s32(world_sel_x+world_button_w+bs, world_sel_y+world_sel_h+0);
			Environment->addButton(rect, this, GUI_ID_CREATE_WORLD_BUTTON,
				  wgettext("New"));
		}
		// Configure world button
		{
			core::rect<s32> rect(0, 0, world_button_w, 30);
			rect += m_topleft_client + v2s32(world_sel_x+(world_button_w+bs)*2,
					world_sel_y+world_sel_h+0);
			Environment->addButton(rect, this, GUI_ID_CONFIGURE_WORLD_BUTTON,
				  wgettext("Configure"));
		}
		// Start game button
		{
			/*core::rect<s32> rect(0, 0, world_button_w, 30);
			rect += m_topleft_client + v2s32(world_sel_x+(world_button_w+bs)*3,
					world_sel_y+world_sel_h+0);*/
			u32 bw = 160;
			/*core::rect<s32> rect(0, 0, bw, 30);
			rect += m_topleft_client + v2s32(m_size_client.X-bw-30,
					m_size_client.Y-30-15);*/
			core::rect<s32> rect(0, 0, bw, 30);
			rect += m_topleft_client + v2s32(world_sel_x+world_sel_w-bw,
					world_sel_y+world_sel_h+30+bs);
			Environment->addButton(rect, this,
					GUI_ID_JOIN_GAME_BUTTON, wgettext("Play"));
		}
		// Options
		s32 option_x = 50;
		//s32 option_x = 50+world_sel_w+20;
		s32 option_y = 30;
		u32 option_w = 150;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += m_topleft_client + v2s32(option_x, option_y+20*0);
			Environment->addCheckBox(m_data->creative_mode, rect, this,
					GUI_ID_CREATIVE_CB, wgettext("Creative Mode"));
		}
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += m_topleft_client + v2s32(option_x, option_y+20*1);
			Environment->addCheckBox(m_data->enable_damage, rect, this,
					GUI_ID_DAMAGE_CB, wgettext("Enable Damage"));
		}
		changeCtype("C");
	}
	else if(m_data->selected_tab == TAB_MULTIPLAYER)
	{
		changeCtype("");
		// CLIENT
		{
			core::rect<s32> rect(0, 0, 10, m_size_client.Y);
			rect += m_topleft_client + v2s32(15, 0);
			const wchar_t *text = L"C\nL\nI\nE\nN\nT";
			gui::IGUIStaticText *t =
			Environment->addStaticText(text, rect, false, false, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
		}
		// Nickname + password
		{
			core::rect<s32> rect(0, 0, 110, 20);
			rect += m_topleft_client + v2s32(35+30, 50+6);
			Environment->addStaticText(wgettext("Name/Password"), 
				rect, false, true, this, -1);
		}
		changeCtype("C");
		{
			core::rect<s32> rect(0, 0, 230, 30);
			rect += m_topleft_client + v2s32(160+30, 50);
			gui::IGUIElement *e = 
			Environment->addEditBox(m_data->name.c_str(), rect, true, this, GUI_ID_NAME_INPUT);
			if(m_data->name == L"")
				Environment->setFocus(e);
		}
		{
			core::rect<s32> rect(0, 0, 120, 30);
			rect += m_topleft_client + v2s32(m_size_client.X-60-100, 50);
			gui::IGUIEditBox *e =
			Environment->addEditBox(L"", rect, true, this, 264);
			e->setPasswordBox(true);
			if(m_data->name != L"" && m_data->address != L"")
				Environment->setFocus(e);

		}
		changeCtype("");
		// Address + port
		{
			core::rect<s32> rect(0, 0, 110, 20);
			rect += m_topleft_client + v2s32(35+30, 100+6);
			Environment->addStaticText(wgettext("Address/Port"),
				rect, false, true, this, -1);
		}
		changeCtype("C");
		{
			core::rect<s32> rect(0, 0, 230, 30);
			rect += m_topleft_client + v2s32(160+30, 100);
			gui::IGUIElement *e = 
			Environment->addEditBox(m_data->address.c_str(), rect, true,
					this, GUI_ID_ADDRESS_INPUT);
			if(m_data->name != L"" && m_data->address == L"")
				Environment->setFocus(e);
		}
		{
			core::rect<s32> rect(0, 0, 120, 30);
			rect += m_topleft_client + v2s32(m_size_client.X-60-100, 100);
			Environment->addEditBox(m_data->port.c_str(), rect, true,
					this, GUI_ID_PORT_INPUT);
		}
		changeCtype("");
		// Start game button
		{
			core::rect<s32> rect(0, 0, 180, 30);
			rect += m_topleft_client + v2s32(m_size_client.X-180-30,
					m_size_client.Y-30-15);
			Environment->addButton(rect, this, GUI_ID_JOIN_GAME_BUTTON,
				wgettext("Start Game / Connect"));
		}
		changeCtype("C");
	}
	else if(m_data->selected_tab == TAB_ADVANCED)
	{
		changeCtype("");
		// CLIENT
		{
			core::rect<s32> rect(0, 0, 10, m_size_client.Y);
			rect += m_topleft_client + v2s32(15, 0);
			const wchar_t *text = L"C\nL\nI\nE\nN\nT";
			gui::IGUIStaticText *t =
			Environment->addStaticText(text, rect, false, false, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
		}
		// Nickname + password
		{
			core::rect<s32> rect(0, 0, 110, 20);
			rect += m_topleft_client + v2s32(35+30, 35+6);
			Environment->addStaticText(wgettext("Name/Password"), 
				rect, false, true, this, -1);
		}
		changeCtype("C");
		{
			core::rect<s32> rect(0, 0, 230, 30);
			rect += m_topleft_client + v2s32(160+30, 35);
			gui::IGUIElement *e = 
			Environment->addEditBox(m_data->name.c_str(), rect, true, this, GUI_ID_NAME_INPUT);
			if(m_data->name == L"")
				Environment->setFocus(e);
		}
		{
			core::rect<s32> rect(0, 0, 120, 30);
			rect += m_topleft_client + v2s32(m_size_client.X-60-100, 35);
			gui::IGUIEditBox *e =
			Environment->addEditBox(L"", rect, true, this, 264);
			e->setPasswordBox(true);
			if(m_data->name != L"" && m_data->address != L"")
				Environment->setFocus(e);

		}
		changeCtype("");
		// Address + port
		{
			core::rect<s32> rect(0, 0, 110, 20);
			rect += m_topleft_client + v2s32(35+30, 75+6);
			Environment->addStaticText(wgettext("Address/Port"),
				rect, false, true, this, -1);
		}
		changeCtype("C");
		{
			core::rect<s32> rect(0, 0, 230, 30);
			rect += m_topleft_client + v2s32(160+30, 75);
			gui::IGUIElement *e = 
			Environment->addEditBox(m_data->address.c_str(), rect, true,
					this, GUI_ID_ADDRESS_INPUT);
			if(m_data->name != L"" && m_data->address == L"")
				Environment->setFocus(e);
		}
		{
			core::rect<s32> rect(0, 0, 120, 30);
			rect += m_topleft_client + v2s32(m_size_client.X-60-100, 75);
			Environment->addEditBox(m_data->port.c_str(), rect, true,
					this, GUI_ID_PORT_INPUT);
		}
		changeCtype("");
		{
			core::rect<s32> rect(0, 0, 400, 20);
			rect += m_topleft_client + v2s32(160+30, 75+35);
			Environment->addStaticText(wgettext("Leave address blank to start a local server."),
				rect, false, true, this, -1);
		}
		// Start game button
		{
			core::rect<s32> rect(0, 0, 180, 30);
			rect += m_topleft_client + v2s32(m_size_client.X-180-30,
					m_size_client.Y-30-20);
			Environment->addButton(rect, this, GUI_ID_JOIN_GAME_BUTTON,
				wgettext("Start Game / Connect"));
		}
		/*
			Server section
		*/
		// SERVER
		{
			core::rect<s32> rect(0, 0, 10, m_size_server.Y);
			rect += m_topleft_server + v2s32(15, 0);
			const wchar_t *text = L"S\nE\nR\nV\nE\nR";
			gui::IGUIStaticText *t =
			Environment->addStaticText(text, rect, false, false, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
		}
		// Server parameters
		{
			core::rect<s32> rect(0, 0, 250, 30);
			rect += m_topleft_server + v2s32(30+20+250+20, 20);
			Environment->addCheckBox(m_data->creative_mode, rect, this, GUI_ID_CREATIVE_CB,
				wgettext("Creative Mode"));
		}
		{
			core::rect<s32> rect(0, 0, 250, 30);
			rect += m_topleft_server + v2s32(30+20+250+20, 40);
			Environment->addCheckBox(m_data->enable_damage, rect, this, GUI_ID_DAMAGE_CB,
				wgettext("Enable Damage"));
		}
		// Delete world button
		{
			core::rect<s32> rect(0, 0, 130, 30);
			rect += m_topleft_server + v2s32(30+20+250+20, 90);
			Environment->addButton(rect, this, GUI_ID_DELETE_WORLD_BUTTON,
				  wgettext("Delete world"));
		}
		// Create world button
		{
			core::rect<s32> rect(0, 0, 130, 30);
			rect += m_topleft_server + v2s32(30+20+250+20+140, 90);
			Environment->addButton(rect, this, GUI_ID_CREATE_WORLD_BUTTON,
				  wgettext("Create world"));
		}
		// World selection listbox
		{
			core::rect<s32> rect(0, 0, 250, 120);
			rect += m_topleft_server + v2s32(30+20, 10);
			gui::IGUIListBox *e = Environment->addListBox(rect, this,
					GUI_ID_WORLD_LISTBOX);
			e->setDrawBackground(true);
			for(std::vector<WorldSpec>::const_iterator i = m_data->worlds.begin();
					i != m_data->worlds.end(); i++){
				e->addItem(narrow_to_wide(i->name+" ["+i->gameid+"]").c_str());
			}
			e->setSelected(m_data->selected_world);
		}
		changeCtype("C");
	}
	else if(m_data->selected_tab == TAB_SETTINGS)
	{
		{
			core::rect<s32> rect(0, 0, 10, m_size_client.Y);
			rect += m_topleft_client + v2s32(15, 0);
			const wchar_t *text = L"S\nE\nT\nT\nI\nN\nG\nS";
			gui::IGUIStaticText *t =
			Environment->addStaticText(text, rect, false, false, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
		}
		s32 option_x = 70;
		s32 option_y = 50;
		u32 option_w = 150;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += m_topleft_client + v2s32(option_x, option_y);
			Environment->addCheckBox(m_data->fancy_trees, rect, this,
					GUI_ID_FANCYTREE_CB, wgettext("Fancy trees")); 
		}
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += m_topleft_client + v2s32(option_x, option_y+20);
			Environment->addCheckBox(m_data->smooth_lighting, rect, this,
					GUI_ID_SMOOTH_LIGHTING_CB, wgettext("Smooth Lighting"));
		}
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += m_topleft_client + v2s32(option_x, option_y+20*2);
			Environment->addCheckBox(m_data->clouds_3d, rect, this,
					GUI_ID_3D_CLOUDS_CB, wgettext("3D Clouds"));
		}
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += m_topleft_client + v2s32(option_x, option_y+20*3);
			Environment->addCheckBox(m_data->opaque_water, rect, this,
					GUI_ID_OPAQUE_WATER_CB, wgettext("Opaque water"));
		}


		// Anisotropic/mipmap/bi-/trilinear settings

		{
			core::rect<s32> rect(0, 0, option_w+20, 30);
			rect += m_topleft_client + v2s32(option_x+175, option_y);
			Environment->addCheckBox(m_data->mip_map, rect, this,
				       GUI_ID_MIPMAP_CB, wgettext("Mip-Mapping"));
		}

		{
			core::rect<s32> rect(0, 0, option_w+20, 30);
			rect += m_topleft_client + v2s32(option_x+175, option_y+20);
			Environment->addCheckBox(m_data->anisotropic_filter, rect, this,
				       GUI_ID_ANISOTROPIC_CB, wgettext("Anisotropic Filtering"));
		}

		{
			core::rect<s32> rect(0, 0, option_w+20, 30);
			rect += m_topleft_client + v2s32(option_x+175, option_y+20*2);
			Environment->addCheckBox(m_data->bilinear_filter, rect, this,
				       GUI_ID_BILINEAR_CB, wgettext("Bi-Linear Filtering"));
		}

		{
			core::rect<s32> rect(0, 0, option_w+20, 30);
			rect += m_topleft_client + v2s32(option_x+175, option_y+20*3);
			Environment->addCheckBox(m_data->trilinear_filter, rect, this,
				       GUI_ID_TRILINEAR_CB, wgettext("Tri-Linear Filtering"));
		}

		// shader/on demand image loading settings
		{
			core::rect<s32> rect(0, 0, option_w+20, 30);
			rect += m_topleft_client + v2s32(option_x+175*2, option_y);
			Environment->addCheckBox(m_data->enable_shaders, rect, this,
					GUI_ID_SHADERS_CB, wgettext("Shaders"));
		}

		{
			core::rect<s32> rect(0, 0, option_w+20+20, 30);
			rect += m_topleft_client + v2s32(option_x+175*2, option_y+20);
			Environment->addCheckBox(m_data->preload_item_visuals, rect, this,
					GUI_ID_PRELOAD_ITEM_VISUALS_CB, wgettext("Preload item visuals"));
		}

		// Key change button
		{
			core::rect<s32> rect(0, 0, 120, 30);
			/*rect += m_topleft_client + v2s32(m_size_client.X-120-30,
					m_size_client.Y-30-20);*/
			rect += m_topleft_client + v2s32(option_x, option_y+120);
			Environment->addButton(rect, this,
					GUI_ID_CHANGE_KEYS_BUTTON, wgettext("Change keys"));
		}
		changeCtype("C");
	}
	else if(m_data->selected_tab == TAB_CREDITS)
	{
		// CREDITS
		{
			core::rect<s32> rect(0, 0, 10, m_size_client.Y);
			rect += m_topleft_client + v2s32(15, 0);
			const wchar_t *text = L"C\nR\nE\nD\nI\nT\nS";
			gui::IGUIStaticText *t =
			Environment->addStaticText(text, rect, false, false, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
		}
		{
			core::rect<s32> rect(0, 0, 454, 250);
			rect += m_topleft_client + v2s32(110, 50+35);
			Environment->addStaticText(narrow_to_wide(
			"Minetest-c55 " VERSION_STRING "\n"
			"http://minetest.net/\n"
			"\n"
			"by Perttu Ahola <celeron55@gmail.com>\n"
			"and contributors: tango_, kahrl (kaaaaaahrl?), erlehmann (the hippie), SpeedProg, JacobF (sqlite worlds), teddydestodes, marktraceur, darkrose, Jonathan Neuschäfer (who the hell?), Felix Krausse (broke liquids, IIRC), sfan5... and >10 more random people."
			).c_str(), rect, false, true, this, -1);
		}
	}

	m_is_regenerating = false;
}

void GUIMainMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	/*video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);*/

	video::SColor bgcolor(140,0,0,0);

	if(getTab() == TAB_SINGLEPLAYER)
	{
		{
			core::rect<s32> rect(0, 0, m_size_client.X, m_size_client.Y);
			rect += AbsoluteRect.UpperLeftCorner + m_topleft_client;
			driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
		}
	}
	else if(getTab() == TAB_MULTIPLAYER)
	{
		{
			core::rect<s32> rect(0, 0, m_size_client.X, m_size_client.Y);
			rect += AbsoluteRect.UpperLeftCorner + m_topleft_client;
			driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
		}
	}
	else if(getTab() == TAB_ADVANCED)
	{
		{
			core::rect<s32> rect(0, 0, m_size_client.X, m_size_client.Y);
			rect += AbsoluteRect.UpperLeftCorner + m_topleft_client;
			driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
		}
		{
			core::rect<s32> rect(0, 0, m_size_server.X, m_size_server.Y);
			rect += AbsoluteRect.UpperLeftCorner + m_topleft_server;
			driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
		}
	}
	else if(getTab() == TAB_SETTINGS)
	{
		{
			core::rect<s32> rect(0, 0, m_size_client.X, m_size_client.Y);
			rect += AbsoluteRect.UpperLeftCorner + m_topleft_client;
			driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
		}
	}
	else if(getTab() == TAB_CREDITS)
	{
		{
			core::rect<s32> rect(0, 0, m_size_client.X, m_size_client.Y);
			rect += AbsoluteRect.UpperLeftCorner + m_topleft_client;
			driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
		}
		video::ITexture *logotexture =
				driver->getTexture(getTexturePath("menulogo.png").c_str());
		if(logotexture)
		{
			v2s32 logosize(logotexture->getOriginalSize().Width,
					logotexture->getOriginalSize().Height);
			logosize *= 2;
			core::rect<s32> rect(0,0,logosize.X,logosize.Y);
			rect += AbsoluteRect.UpperLeftCorner + m_topleft_client;
			rect += v2s32(130, 50);
			driver->draw2DImage(logotexture, rect,
				core::rect<s32>(core::position2d<s32>(0,0),
				core::dimension2di(logotexture->getSize())),
				NULL, NULL, true);
		}
	}

	gui::IGUIElement::draw();
}

void GUIMainMenu::readInput(MainMenuData *dst)
{
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_TAB_CONTROL);
		if(e != NULL && e->getType() == gui::EGUIET_TAB_CONTROL)
			dst->selected_tab = ((gui::IGUITabControl*)e)->getActiveTab();
	}
	if(dst->selected_tab == TAB_SINGLEPLAYER)
	{
		dst->simple_singleplayer_mode = true;
	}
	else
	{
		dst->simple_singleplayer_mode = false;
		{
			gui::IGUIElement *e = getElementFromId(GUI_ID_NAME_INPUT);
			if(e != NULL)
				dst->name = e->getText();
		}
		{
			gui::IGUIElement *e = getElementFromId(264);
			if(e != NULL)
				dst->password = e->getText();
		}
		{
			gui::IGUIElement *e = getElementFromId(GUI_ID_ADDRESS_INPUT);
			if(e != NULL)
				dst->address = e->getText();
		}
		{
			gui::IGUIElement *e = getElementFromId(GUI_ID_PORT_INPUT);
			if(e != NULL)
				dst->port = e->getText();
		}
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CREATIVE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->creative_mode = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_DAMAGE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->enable_damage = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_FANCYTREE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->fancy_trees = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_SMOOTH_LIGHTING_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->smooth_lighting = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_3D_CLOUDS_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->clouds_3d = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_OPAQUE_WATER_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->opaque_water = ((gui::IGUICheckBox*)e)->isChecked();
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_MIPMAP_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->mip_map = ((gui::IGUICheckBox*)e)->isChecked();
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_ANISOTROPIC_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->anisotropic_filter = ((gui::IGUICheckBox*)e)->isChecked();
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_BILINEAR_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->bilinear_filter = ((gui::IGUICheckBox*)e)->isChecked();
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_TRILINEAR_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			dst->trilinear_filter = ((gui::IGUICheckBox*)e)->isChecked();
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_SHADERS_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
		        dst->enable_shaders = ((gui::IGUICheckBox*)e)->isChecked() ? 2 : 0;
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_PRELOAD_ITEM_VISUALS_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
		        dst->preload_item_visuals = ((gui::IGUICheckBox*)e)->isChecked();
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_WORLD_LISTBOX);
		if(e != NULL && e->getType() == gui::EGUIET_LIST_BOX)
			dst->selected_world = ((gui::IGUIListBox*)e)->getSelected();
	}
}

void GUIMainMenu::acceptInput()
{
	readInput(m_data);
	m_accepted = true;
}

bool GUIMainMenu::OnEvent(const SEvent& event)
{
	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
			m_gamecallback->exitToOS();
			quitMenu();
			return true;
		}
		if(event.KeyInput.Key==KEY_RETURN && event.KeyInput.PressedDown)
		{
			acceptInput();
			quitMenu();
			return true;
		}
	}
	if(event.EventType==EET_GUI_EVENT)
	{
		if(event.GUIEvent.EventType==gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible())
		{
			if(!canTakeFocus(event.GUIEvent.Element))
			{
				dstream<<"GUIMainMenu: Not allowing focus change."
						<<std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_TAB_CHANGED)
		{
			if(!m_is_regenerating)
				regenerateGui(m_screensize_old);
			return true;
		}
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			switch(event.GUIEvent.Caller->getID())
			{
			case GUI_ID_JOIN_GAME_BUTTON: {
				MainMenuData cur;
				readInput(&cur);
				if(cur.address == L"" && getTab() == TAB_MULTIPLAYER){
					(new GUIMessageMenu(env, parent, -1, menumgr,
							wgettext("Address required."))
							)->drop();
					return true;
				}
				acceptInput();
				quitMenu();
				return true;
			}
			case GUI_ID_CHANGE_KEYS_BUTTON: {
				GUIKeyChangeMenu *kmenu = new GUIKeyChangeMenu(env, parent, -1,menumgr);
				kmenu->drop();
				return true;
			}
			case GUI_ID_DELETE_WORLD_BUTTON: {
				MainMenuData cur;
				readInput(&cur);
				if(cur.selected_world == -1){
					(new GUIMessageMenu(env, parent, -1, menumgr,
							wgettext("Cannot delete world: Nothing selected"))
							)->drop();
				} else {
					WorldSpec spec = m_data->worlds[cur.selected_world];
					// Get files and directories involved
					std::vector<std::string> paths;
					paths.push_back(spec.path);
					fs::GetRecursiveSubPaths(spec.path, paths);
					// Launch confirmation dialog
					ConfirmDestDeleteWorld *dest = new
							ConfirmDestDeleteWorld(spec, this, paths);
					std::wstring text = wgettext("Delete world");
					text += L" \"";
					text += narrow_to_wide(spec.name);
					text += L"\"?\n\n";
					text += wgettext("Files to be deleted");
					text += L":\n";
					for(u32 i=0; i<paths.size(); i++){
						if(i == 3){ text += L"..."; break; }
						text += narrow_to_wide(paths[i]) + L"\n";
					}
					(new GUIConfirmMenu(env, parent, -1, menumgr, dest,
							text.c_str()))->drop();
				}
				return true;
			}
			case GUI_ID_CREATE_WORLD_BUTTON: {
				std::vector<SubgameSpec> games = getAvailableGames();
				if(games.size() == 0){
					GUIMessageMenu *menu = new GUIMessageMenu(env, parent,
							-1, menumgr,
							wgettext("Cannot create world: No games found"));
					menu->drop();
				} else {
					CreateWorldDest *dest = new CreateWorldDestMainMenu(this);
					GUICreateWorld *menu = new GUICreateWorld(env, parent, -1,
							menumgr, dest, games);
					menu->drop();
				}
				return true;
			}
			case GUI_ID_CONFIGURE_WORLD_BUTTON: {
				GUIMessageMenu *menu = new GUIMessageMenu(env, parent,
						-1, menumgr,
						wgettext("Nothing here"));
				menu->drop();
				return true;
			}
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_EDITBOX_ENTER)
		{
			switch(event.GUIEvent.Caller->getID())
			{
				case GUI_ID_ADDRESS_INPUT: case GUI_ID_PORT_INPUT: case GUI_ID_NAME_INPUT: case 264:
				acceptInput();
				quitMenu();
				return true;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_LISTBOX_SELECTED_AGAIN)
		{
			switch(event.GUIEvent.Caller->getID())
			{
			case GUI_ID_WORLD_LISTBOX:
				acceptInput();
				if(getTab() != TAB_SINGLEPLAYER)
					m_data->address = L""; // Force local game
				quitMenu();
				return true;
			}
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

void GUIMainMenu::createNewWorld(std::wstring name, std::string gameid)
{
	if(name == L"")
		return;
	acceptInput();
	m_data->create_world_name = name;
	m_data->create_world_gameid = gameid;
	quitMenu();
}

void GUIMainMenu::deleteWorld(const std::vector<std::string> &paths)
{
	// Delete files
	bool did = fs::DeletePaths(paths);
	if(!did){
		GUIMessageMenu *menu = new GUIMessageMenu(env, parent,
				-1, menumgr, wgettext("Failed to delete all world files"));
		menu->drop();
	}
	// Quit menu to refresh it
	acceptInput();
	m_data->only_refresh = true;
	quitMenu();
}
	
int GUIMainMenu::getTab()
{
	gui::IGUIElement *e = getElementFromId(GUI_ID_TAB_CONTROL);
	if(e != NULL && e->getType() == gui::EGUIET_TAB_CONTROL)
		return ((gui::IGUITabControl*)e)->getActiveTab();
	return TAB_SINGLEPLAYER; // Default
}

void GUIMainMenu::displayMessageMenu(std::wstring msg)
{
	(new GUIMessageMenu(env, parent, -1, menumgr, msg))->drop();
}
