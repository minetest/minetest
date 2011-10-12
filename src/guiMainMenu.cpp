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

#include "guiMainMenu.h"
#include "guiKeyChangeMenu.h"
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>


#include "gettext.h"

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
	std::wstring text_name;
	std::wstring text_address;
	std::wstring text_port;
	bool creative_mode;
	bool enable_damage;
	bool fancy_trees;
	bool smooth_lighting;
	
	// Client options
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_NAME_INPUT);
		if(e != NULL)
			text_name = e->getText();
		else
			text_name = m_data->name;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_ADDRESS_INPUT);
		if(e != NULL)
			text_address = e->getText();
		else
			text_address = m_data->address;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_PORT_INPUT);
		if(e != NULL)
			text_port = e->getText();
		else
			text_port = m_data->port;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_FANCYTREE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			fancy_trees = ((gui::IGUICheckBox*)e)->isChecked();
		else
			fancy_trees = m_data->fancy_trees;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_SMOOTH_LIGHTING_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			smooth_lighting = ((gui::IGUICheckBox*)e)->isChecked();
		else
			smooth_lighting = m_data->smooth_lighting;
	}
	
	// Server options
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CREATIVE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			creative_mode = ((gui::IGUICheckBox*)e)->isChecked();
		else
			creative_mode = m_data->creative_mode;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_DAMAGE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			enable_damage = ((gui::IGUICheckBox*)e)->isChecked();
		else
			enable_damage = m_data->enable_damage;
	}

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
	{
		core::rect<s32> rect(0, 0, 20, 125);
		rect += topleft_client + v2s32(-15, 60);
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
		Environment->addEditBox(text_name.c_str(), rect, true, this, GUI_ID_NAME_INPUT);
		if(text_name == L"")
			Environment->setFocus(e);
	}
	{
		core::rect<s32> rect(0, 0, 120, 30);
		rect += topleft_client + v2s32(size_client.X-60-100, 50);
		gui::IGUIEditBox *e =
		Environment->addEditBox(L"", rect, true, this, 264);
		e->setPasswordBox(true);
		if(text_name != L"" && text_address != L"")
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
		Environment->addEditBox(text_address.c_str(), rect, true, this, GUI_ID_ADDRESS_INPUT);
		if(text_name != L"" && text_address == L"")
			Environment->setFocus(e);
	}
	{
		core::rect<s32> rect(0, 0, 120, 30);
		//rect += topleft_client + v2s32(160+250+20, 125);
		rect += topleft_client + v2s32(size_client.X-60-100, 100);
		Environment->addEditBox(text_port.c_str(), rect, true, this, GUI_ID_PORT_INPUT);
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
		Environment->addCheckBox(fancy_trees, rect, this, GUI_ID_FANCYTREE_CB,
			wgettext("Fancy trees")); 
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_client + v2s32(35, 150+30);
		Environment->addCheckBox(smooth_lighting, rect, this, GUI_ID_SMOOTH_LIGHTING_CB,
				wgettext("Smooth Lighting"));
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

	v2s32 topleft_server(40, 250);
	v2s32 size_server = size - v2s32(40, 0);
	
	{
		core::rect<s32> rect(0, 0, 20, 125);
		rect += topleft_server + v2s32(-15, 40);
		const wchar_t *text = L"S\nE\nR\nV\nE\nR";
		//gui::IGUIStaticText *t =
		Environment->addStaticText(text, rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	// Server parameters
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_server + v2s32(35, 30);
		Environment->addCheckBox(creative_mode, rect, this, GUI_ID_CREATIVE_CB,
			wgettext("Creative Mode"));
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect += topleft_server + v2s32(35, 60);
		Environment->addCheckBox(enable_damage, rect, this, GUI_ID_DAMAGE_CB,
			wgettext("Enable Damage"));
	}
	// Map delete button
	{
		core::rect<s32> rect(0, 0, 130, 30);
		//rect += topleft_server + v2s32(size_server.X-40-130, 100+25);
		rect += topleft_server + v2s32(40, 100+25);
		Environment->addButton(rect, this, GUI_ID_DELETE_MAP_BUTTON,
			  wgettext("Delete map"));
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
		core::rect<s32> rect(0, 0, 620, 230);
		rect += AbsoluteRect.UpperLeftCorner;
		driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
	}

	{
		core::rect<s32> rect(0, 250, 620, 430);
		rect += AbsoluteRect.UpperLeftCorner;
		driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
	}

	gui::IGUIElement::draw();
}

void GUIMainMenu::acceptInput()
{
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_NAME_INPUT);
		if(e != NULL)
			m_data->name = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(264);
		if(e != NULL)
			m_data->password = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_ADDRESS_INPUT);
		if(e != NULL)
			m_data->address = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_PORT_INPUT);
		if(e != NULL)
			m_data->port = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CREATIVE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data->creative_mode = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_DAMAGE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data->enable_damage = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_SMOOTH_LIGHTING_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data->smooth_lighting = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_FANCYTREE_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data->fancy_trees = ((gui::IGUICheckBox*)e)->isChecked();
	}
	
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
			case GUI_ID_JOIN_GAME_BUTTON: // Start game
				acceptInput();
				quitMenu();
				return true;
			case GUI_ID_CHANGE_KEYS_BUTTON: {
				GUIKeyChangeMenu *kmenu = new GUIKeyChangeMenu(env, parent, -1,menumgr);
				kmenu->drop();
				return true;
			}
			case GUI_ID_DELETE_MAP_BUTTON: // Delete map
				// Don't accept input data, just set deletion request
				m_data->delete_map = true;
				m_accepted = true;
				quitMenu();
				return true;
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
	}

	return Parent ? Parent->OnEvent(event) : false;
}

