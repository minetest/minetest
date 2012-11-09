/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "guiCreateWorld.h"
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include <IGUIListBox.h>
#include "gettext.h"
#include "util/string.h"

enum
{
	GUI_ID_NAME_INPUT = 101,
	GUI_ID_GAME_LISTBOX,
	GUI_ID_CREATE,
	GUI_ID_CANCEL
};

GUICreateWorld::GUICreateWorld(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		CreateWorldDest *dest,
		const std::vector<SubgameSpec> &games
):
	GUIModalMenu(env, parent, id, menumgr),
	m_dest(dest),
	m_games(games)
{
	assert(games.size() > 0);
}

GUICreateWorld::~GUICreateWorld()
{
	removeChildren();
	if(m_dest)
		delete m_dest;
}

void GUICreateWorld::removeChildren()
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

void GUICreateWorld::regenerateGui(v2u32 screensize)
{
	std::wstring name = L"";

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_NAME_INPUT);
		if(e != NULL)
			name = e->getText();
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

	v2s32 topleft = v2s32(10+80, 10+70);

	/*
		Add stuff
	*/
	{
		core::rect<s32> rect(0, 0, 100, 20);
		rect += v2s32(0, 5) + topleft;
		Environment->addStaticText(wgettext("World name"),
			rect, false, true, this, -1);
	}
	{
		core::rect<s32> rect(0, 0, 300, 30);
		rect = rect + v2s32(100, 0) + topleft;
		gui::IGUIElement *e = 
		Environment->addEditBox(name.c_str(), rect, true, this, GUI_ID_NAME_INPUT);
		Environment->setFocus(e);

		irr::SEvent evt;
		evt.EventType = EET_KEY_INPUT_EVENT;
		evt.KeyInput.Key = KEY_END;
		evt.KeyInput.PressedDown = true;
		e->OnEvent(evt);
	}
	{
		core::rect<s32> rect(0, 0, 100, 20);
		rect += v2s32(0, 40+5) + topleft;
		Environment->addStaticText(wgettext("Game"),
			rect, false, true, this, -1);
	}
	{
		core::rect<s32> rect(0, 0, 300, 80);
		rect += v2s32(100, 40) + topleft;
		gui::IGUIListBox *e = Environment->addListBox(rect, this,
				GUI_ID_GAME_LISTBOX);
		e->setDrawBackground(true);
		for(u32 i=0; i<m_games.size(); i++){
			std::wostringstream os(std::ios::binary);
			os<<narrow_to_wide(m_games[i].name).c_str();
			os<<L" [";
			os<<narrow_to_wide(m_games[i].id).c_str();
			os<<L"]";
			e->addItem(os.str().c_str());
		}
		e->setSelected(0);
	}
	changeCtype("");
	{
		core::rect<s32> rect(0, 0, 120, 30);
		rect = rect + v2s32(170, 140) + topleft;
		Environment->addButton(rect, this, GUI_ID_CREATE,
			wgettext("Create"));
	}
	{
		core::rect<s32> rect(0, 0, 120, 30);
		rect = rect + v2s32(300, 140) + topleft;
		Environment->addButton(rect, this, GUI_ID_CANCEL,
			wgettext("Cancel"));
	}
	changeCtype("C");
}

void GUICreateWorld::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
}

void GUICreateWorld::acceptInput()
{
	if(m_dest)
	{
		int selected = -1;
		{
			gui::IGUIElement *e = getElementFromId(GUI_ID_GAME_LISTBOX);
			if(e != NULL && e->getType() == gui::EGUIET_LIST_BOX)
				selected = ((gui::IGUIListBox*)e)->getSelected();
		}
		std::wstring name;
		{
			gui::IGUIElement *e = getElementFromId(GUI_ID_NAME_INPUT);
			if(e != NULL)
				name = e->getText();
		}
		if(selected != -1 && name != L"")
			m_dest->accepted(name, m_games[selected].id);
		delete m_dest;
		m_dest = NULL;
	}
}

bool GUICreateWorld::OnEvent(const SEvent& event)
{
	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
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
				dstream<<"GUICreateWorld: Not allowing focus change."
						<<std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		bool accept_input = false;
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED){
			switch(event.GUIEvent.Caller->getID()){
			case GUI_ID_CANCEL:
				quitMenu();
				return true;
				break;
			case GUI_ID_CREATE:
				accept_input = true;
				break;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_EDITBOX_ENTER){
			switch(event.GUIEvent.Caller->getID()){
			case GUI_ID_NAME_INPUT:
				accept_input = true;
				break;
			}
		}
		if(accept_input){
			acceptInput();
			quitMenu();
			// quitMenu deallocates menu
			return true;
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

