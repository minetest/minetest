/*
Minetest-c55
Copyright (C) 2010-12 celeron55, Perttu Ahola <celeron55@gmail.com>

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
// For IGameCallback
#include "guiPauseMenu.h"
#include "gettext.h"
#include "utility.h"

struct CreateWorldDestMainMenu : public CreateWorldDest
{
	CreateWorldDestMainMenu(GUIMainMenu *menu):
		m_menu(menu)
	{}
	void accepted(std::wstring name, std::string gameid)
	{
		m_menu->createNewWorld(name, gameid);
	}
	GUIMainMenu *m_menu;
};

struct ConfirmDestDeleteWorld : public ConfirmDest
{
	ConfirmDestDeleteWorld(WorldSpec spec, GUIMainMenu *menu):
		m_spec(spec),
		m_menu(menu)
	{}
	void answer(bool answer)
	{
		if(answer == false)
			return;
		m_menu->deleteWorld(m_spec);
	}
	WorldSpec m_spec;
	GUIMainMenu *m_menu;
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
	GUI_ID_DAMAGE_CB,
	GUI_ID_CREATIVE_CB,
	GUI_ID_JOIN_GAME_BUTTON,
	GUI_ID_CHANGE_KEYS_BUTTON,
	GUI_ID_DELETE_WORLD_BUTTON,
	GUI_ID_CREATE_WORLD_BUTTON,
	GUI_ID_WORLD_LISTBOX,
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
	m_gamecallback(gamecallback)
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
	
	v2s32 size(620, 430);

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

	/*
		Client section
	*/

	v2s32 topleft_client(40, 0);
	v2s32 size_client = size - v2s32(40, 0);
	
	changeCtype("");
	
	// Version
	{
		core::rect<s32> rect(0, 0, 300, 30);
		rect += topleft_client + v2s32(-36, 0);
		Environment->addStaticText(narrow_to_wide(VERSION_STRING).c_str(), 
			rect, false, true, this, -1);
	}
	// CLIENT
	{
		core::rect<s32> rect(0, 0, 20, 125);
		rect += topleft_client + v2s32(-15, 80);
		const wchar_t *text = L"C\nL\nI\nE\nN\nT";
		//gui::IGUIStaticText *t =
		Environment->addStaticText(text, rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}
	// Nickname + password
	{
		core::rect<s32> rect(0, 0, 110, 20);
		rect += topleft_client + v2s32(35, 50+6);
		Environment->addStaticText(wgettext("Name/Password"), 
			rect, false, true, this, -1);
	}
	changeCtype("C");
	{
		core::rect<s32> rect(0, 0, 230, 30);
		rect += topleft_client + v2s32(160, 50);
		gui::IGUIElement *e = 
		Environment->addEditBox(m_data->name.c_str(), rect, true, this, GUI_ID_NAME_INPUT);
		if(m_data->name == L"")
			Environment->setFocus(e);
	}
	{
		core::rect<s32> rect(0, 0, 120, 30);
		rect += topleft_client + v2s32(size_client.X-60-100, 50);
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
		rect += topleft_client + v2s32(35, 100+6);
		Environment->addStaticText(wgettext("Address/Port"),
			rect, false, true, this, -1);
	}
	changeCtype("C");
	{
		core::rect<s32> rect(0, 0, 230, 30);
		rect += topleft_client + v2s32(160, 100);
		gui::IGUIElement *e = 
		Environment->addEditBox(m_data->address.c_str(), rect, true, this, GUI_ID_ADDRESS_INPUT);
		if(m_data->name != L"" && m_data->address == L"")
			Environment->setFocus(e);
	}
	{
		core::rect<s32> rect(0, 0, 120, 30);
		//rect += topleft_client + v2s32(160+250+20, 125);
		rect += topleft_client + v2s32(size_client.X-60-100, 100);
		Environment->addEditBox(m_data->port.c_str(), rect, true, this, GUI_ID_PORT_INPUT);
	}
	changeCtype("");
	{
		core::rect<s32> rect(0, 0, 400, 20);
		rect += topleft_client + v2s32(160, 100+35);
		Environment->addStaticText(wgettext("Leave address blank to start a local server."),
			rect, false, true, this, -1);
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_client + v2s32(35, 150);
		Environment->addCheckBox(m_data->fancy_trees, rect, this, GUI_ID_FANCYTREE_CB,
			wgettext("Fancy trees")); 
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_client + v2s32(35, 150+20);
		Environment->addCheckBox(m_data->smooth_lighting, rect, this, GUI_ID_SMOOTH_LIGHTING_CB,
				wgettext("Smooth Lighting"));
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_client + v2s32(35, 150+40);
		Environment->addCheckBox(m_data->clouds_3d, rect, this, GUI_ID_3D_CLOUDS_CB,
				wgettext("3D Clouds"));
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_client + v2s32(35, 150+60);
		Environment->addCheckBox(m_data->opaque_water, rect, this, GUI_ID_OPAQUE_WATER_CB,
				wgettext("Opaque water"));
	}
	// Start game button
	{
		core::rect<s32> rect(0, 0, 180, 30);
		//rect += topleft_client + v2s32(size_client.X/2-180/2, 225-30/2);
		rect += topleft_client + v2s32(size_client.X-180-40, 150+25);
		Environment->addButton(rect, this, GUI_ID_JOIN_GAME_BUTTON,
			wgettext("Start Game / Connect"));
	}

	// Key change button
	{
		core::rect<s32> rect(0, 0, 100, 30);
		//rect += topleft_client + v2s32(size_client.X/2-180/2, 225-30/2);
		rect += topleft_client + v2s32(size_client.X-180-40-100-20, 150+25);
		Environment->addButton(rect, this, GUI_ID_CHANGE_KEYS_BUTTON,
			wgettext("Change keys"));
	}
	/*
		Server section
	*/

	v2s32 topleft_server(40, 290);
	v2s32 size_server = size - v2s32(40, 0);
	
	// SERVER
	{
		core::rect<s32> rect(0, 0, 20, 125);
		rect += topleft_server + v2s32(-15, 15);
		const wchar_t *text = L"S\nE\nR\nV\nE\nR";
		//gui::IGUIStaticText *t =
		Environment->addStaticText(text, rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}
	// Server parameters
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_server + v2s32(20+250+20, 20);
		Environment->addCheckBox(m_data->creative_mode, rect, this, GUI_ID_CREATIVE_CB,
			wgettext("Creative Mode"));
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_server + v2s32(20+250+20, 40);
		Environment->addCheckBox(m_data->enable_damage, rect, this, GUI_ID_DAMAGE_CB,
			wgettext("Enable Damage"));
	}
	// Delete world button
	{
		core::rect<s32> rect(0, 0, 130, 30);
		rect += topleft_server + v2s32(20+250+20, 90);
		Environment->addButton(rect, this, GUI_ID_DELETE_WORLD_BUTTON,
			  wgettext("Delete world"));
	}
	// Create world button
	{
		core::rect<s32> rect(0, 0, 130, 30);
		rect += topleft_server + v2s32(20+250+20+140, 90);
		Environment->addButton(rect, this, GUI_ID_CREATE_WORLD_BUTTON,
			  wgettext("Create world"));
	}
	// World selection listbox
	{
		core::rect<s32> rect(0, 0, 250, 120);
		rect += topleft_server + v2s32(20, 10);
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

void GUIMainMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	/*video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);*/

	video::SColor bgcolor(140,0,0,0);

	{
		core::rect<s32> rect(0, 0, 620, 270);
		rect += AbsoluteRect.UpperLeftCorner;
		driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
	}

	{
		core::rect<s32> rect(0, 290, 620, 430);
		rect += AbsoluteRect.UpperLeftCorner;
		driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
	}

	gui::IGUIElement::draw();
}

void GUIMainMenu::readInput(MainMenuData *dst)
{
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
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			switch(event.GUIEvent.Caller->getID())
			{
			case GUI_ID_JOIN_GAME_BUTTON:
				acceptInput();
				quitMenu();
				return true;
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
					ConfirmDestDeleteWorld *dest = new
							ConfirmDestDeleteWorld(spec, this);
					(new GUIConfirmMenu(env, parent, -1, menumgr, dest,
							(std::wstring(wgettext("Delete world "))
							+L"\""+narrow_to_wide(spec.name)+L"\"?").c_str()
							))->drop();
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

void GUIMainMenu::deleteWorld(WorldSpec spec)
{
	if(!spec.isValid())
		return;
	acceptInput();
	m_data->delete_world_spec = spec;
	quitMenu();
}

