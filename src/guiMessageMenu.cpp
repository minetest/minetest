/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "guiMessageMenu.h"
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>

#include "gettext.h"

GUIMessageMenu::GUIMessageMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		std::wstring message_text
):
	GUIModalMenu(env, parent, id, menumgr),
	m_message_text(message_text),
	m_status(false)
{
}

GUIMessageMenu::~GUIMessageMenu()
{
	removeChildren();
}

void GUIMessageMenu::removeChildren()
{
	{
		gui::IGUIElement *e = getElementFromId(256);
		if(e != NULL)
			e->remove();
	}
	{
		gui::IGUIElement *e = getElementFromId(257);
		if(e != NULL)
			e->remove();
	}
}

void GUIMessageMenu::regenerateGui(v2u32 screensize)
{
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

	gui::IGUISkin *skin = Environment->getSkin();
	gui::IGUIFont *font = skin->getFont();
	s32 msg_h = font->getDimension(m_message_text.c_str()).Height;
	s32 msg_w = font->getDimension(m_message_text.c_str()).Width;
	if(msg_h > 200)
		msg_h = 200;
	if(msg_w > 540)
		msg_w = 540;

	/*
		Add stuff
	*/
	{
		core::rect<s32> rect(0, 0, msg_w, msg_h);
		rect += v2s32(size.X/2-msg_w/2, size.Y/2-30/2 - msg_h/2);
		Environment->addStaticText(m_message_text.c_str(),
			rect, false, true, this, -1);
	}
	changeCtype("");
	int bw = 140;
	{
		core::rect<s32> rect(0, 0, bw, 30);
		rect = rect + v2s32(size.X/2-bw/2, size.Y/2-30/2+5 + msg_h/2);
		gui::IGUIElement *e = 
		Environment->addButton(rect, this, 257,
			wgettext("Proceed"));
		Environment->setFocus(e);
	}
	changeCtype("C");
}

void GUIMessageMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
}

bool GUIMessageMenu::OnEvent(const SEvent& event)
{
	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
			m_status = true;
			quitMenu();
			return true;
		}
		if(event.KeyInput.Key==KEY_RETURN && event.KeyInput.PressedDown)
		{
			m_status = true;
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
				dstream<<"GUIMessageMenu: Not allowing focus change."
						<<std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			switch(event.GUIEvent.Caller->getID())
			{
			case 257:
				m_status = true;
				quitMenu();
				return true;
			}
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

