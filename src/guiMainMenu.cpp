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
#include "debug.h"
#include "serialization.h"
#include <string>

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

	{
		gui::IGUIElement *e = getElementFromId(258);
		if(e != NULL)
			text_name = e->getText();
		else
			text_name = m_data->name;
	}
	{
		gui::IGUIElement *e = getElementFromId(256);
		if(e != NULL)
			text_address = e->getText();
		else
			text_address = m_data->address;
	}
	{
		gui::IGUIElement *e = getElementFromId(257);
		if(e != NULL)
			text_port = e->getText();
		else
			text_port = m_data->port;
	}
	{
		gui::IGUIElement *e = getElementFromId(259);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			creative_mode = ((gui::IGUICheckBox*)e)->isChecked();
		else
			creative_mode = m_data->creative_mode;
	}

	/*
		Remove stuff
	*/
	removeChildren();
	
	/*
		Calculate new sizes and positions
	*/
	core::rect<s32> rect(
			screensize.X/2 - 580/2,
			screensize.Y/2 - 300/2,
			screensize.X/2 + 580/2,
			screensize.Y/2 + 300/2
	);
	
	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 size = rect.getSize();

	/*
		Add stuff
	*/

	// Nickname
	{
		core::rect<s32> rect(0, 0, 100, 20);
		rect = rect + v2s32(size.X/2 - 250, size.Y/2 - 100 + 6);
		const wchar_t *text = L"Nickname";
		Environment->addStaticText(text, rect, false, true, this, -1);
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect = rect + v2s32(size.X/2 - 130, size.Y/2 - 100);
		gui::IGUIElement *e = 
		Environment->addEditBox(text_name.c_str(), rect, true, this, 258);
		if(text_name == L"")
			Environment->setFocus(e);
	}
	// Address + port
	{
		core::rect<s32> rect(0, 0, 100, 20);
		rect = rect + v2s32(size.X/2 - 250, size.Y/2 - 50 + 6);
		const wchar_t *text = L"Address + Port";
		Environment->addStaticText(text, rect, false, true, this, -1);
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect = rect + v2s32(size.X/2 - 130, size.Y/2 - 50);
		gui::IGUIElement *e = 
		Environment->addEditBox(text_address.c_str(), rect, true, this, 256);
		if(text_name != L"")
			Environment->setFocus(e);
	}
	{
		core::rect<s32> rect(0, 0, 100, 30);
		rect = rect + v2s32(size.X/2 - 130 + 250 + 20, size.Y/2 - 50);
		Environment->addEditBox(text_port.c_str(), rect, true, this, 257);
	}
	{
		core::rect<s32> rect(0, 0, 400, 20);
		rect = rect + v2s32(size.X/2 - 130, size.Y/2 - 50 + 35);
		const wchar_t *text = L"Leave address blank to start a local server.";
		Environment->addStaticText(text, rect, false, true, this, -1);
	}
	// Server parameters
	{
		core::rect<s32> rect(0, 0, 100, 20);
		rect = rect + v2s32(size.X/2 - 250, size.Y/2 + 25 + 6);
		const wchar_t *text = L"Server params";
		Environment->addStaticText(text, rect, false, true, this, -1);
	}
	{
		core::rect<s32> rect(0, 0, 250, 30);
		rect = rect + v2s32(size.X/2 - 130, size.Y/2 + 25);
		Environment->addCheckBox(creative_mode, rect, this, 259, L"Creative Mode");
	}
	// Start game button
	{
		core::rect<s32> rect(0, 0, 180, 30);
		rect = rect + v2s32(size.X/2-180/2, size.Y/2-30/2 + 100);
		Environment->addButton(rect, this, 257, L"Start Game / Connect");
	}
	// Map delete button
	{
		core::rect<s32> rect(0, 0, 130, 30);
		rect = rect + v2s32(size.X/2-130/2+200, size.Y/2-30/2 + 100);
		Environment->addButton(rect, this, 260, L"Delete map");
	}
}

void GUIMainMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
}

void GUIMainMenu::acceptInput()
{
	{
		gui::IGUIElement *e = getElementFromId(258);
		if(e != NULL)
			m_data->name = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(256);
		if(e != NULL)
			m_data->address = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(257);
		if(e != NULL)
			m_data->port = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(259);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data->creative_mode = ((gui::IGUICheckBox*)e)->isChecked();
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
			case 257: // Start game
				acceptInput();
				quitMenu();
				break;
			case 260: // Delete map
				// Don't accept input data, just set deletion request
				m_data->delete_map = true;
				m_accepted = true;
				quitMenu();
				break;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_EDITBOX_ENTER)
		{
			switch(event.GUIEvent.Caller->getID())
			{
			case 256: case 257: case 258:
				acceptInput();
				quitMenu();
				break;
			}
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

