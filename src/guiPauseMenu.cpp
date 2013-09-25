/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "guiPauseMenu.h"
#include "debug.h"
#include "serialization.h"
#include "porting.h"
#include "config.h"
#include "version.h"
#include "main.h"
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "gettext.h"
#include "util/string.h"

GUIPauseMenu::GUIPauseMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IGameCallback *gamecallback,
		IMenuManager *menumgr,
		bool simple_singleplayer_mode):
	GUIModalMenu(env, parent, id, menumgr),
	m_gamecallback(gamecallback),
	m_simple_singleplayer_mode(simple_singleplayer_mode)
{
}

GUIPauseMenu::~GUIPauseMenu()
{
	removeChildren();
}

void GUIPauseMenu::removeChildren()
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
	{
		gui::IGUIElement *e = getElementFromId(258);
		if(e != NULL)
			e->remove();
	}
	{
		gui::IGUIElement *e = getElementFromId(259);
		if(e != NULL)
			e->remove();
	}
	{
		gui::IGUIElement *e = getElementFromId(260);
		if(e != NULL)
			e->remove();
	}
	{
		gui::IGUIElement *e = getElementFromId(261);
		if(e != NULL)
			e->remove();
	}
	{
		gui::IGUIElement *e = getElementFromId(262);
		if(e != NULL)
			e->remove();
	}
}

void GUIPauseMenu::regenerateGui(v2u32 screensize)
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

	/*
		Add stuff
	*/
	const s32 btn_height = 30;
	const s32 btn_gap = 20;
	const s32 btn_num = m_simple_singleplayer_mode ? 4 : 5;
	s32 btn_y = size.Y/2-((btn_num*btn_height+(btn_num-1)*btn_gap))/2;
	changeCtype("");
	{
		core::rect<s32> rect(0, 0, 140, btn_height);
		rect = rect + v2s32(size.X/2-140/2, btn_y);
		wchar_t* text = wgettext("Continue");
		Environment->addButton(rect, this, 256,
			text);
		delete[] text;
	}
	btn_y += btn_height + btn_gap;
	if(!m_simple_singleplayer_mode)
	{
		{
			core::rect<s32> rect(0, 0, 140, btn_height);
			rect = rect + v2s32(size.X/2-140/2, btn_y);
			wchar_t* text = wgettext("Change Password");
			Environment->addButton(rect, this, 261,
				text);
			delete[] text;
		}
		btn_y += btn_height + btn_gap;
	}
	{
		core::rect<s32> rect(0, 0, 140, btn_height);
		rect = rect + v2s32(size.X/2-140/2, btn_y);
		wchar_t* text = wgettext("Sound Volume");
		Environment->addButton(rect, this, 262,
			text);
		delete[] text;
	}
	btn_y += btn_height + btn_gap;
	{
		core::rect<s32> rect(0, 0, 140, btn_height);
		rect = rect + v2s32(size.X/2-140/2, btn_y);
		wchar_t* text = wgettext("Exit to Menu");
		Environment->addButton(rect, this, 260,
			text);
		delete[] text;
	}
	btn_y += btn_height + btn_gap;
	{
		core::rect<s32> rect(0, 0, 140, btn_height);
		rect = rect + v2s32(size.X/2-140/2, btn_y);
		wchar_t* text = wgettext("Exit to OS");
		Environment->addButton(rect, this, 257,
			text);
		delete[] text;
	}

	{
		core::rect<s32> rect(0, 0, 180, 240);
		rect = rect + v2s32(size.X/2 + 90, size.Y/2-rect.getHeight()/2);
		wchar_t* text = wgettext("Default Controls:\n"
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
				);
		Environment->addStaticText(text, rect, false, true, this, 258);
		delete[] text;

	}
	{
		core::rect<s32> rect(0, 0, 180, 220);
		rect = rect + v2s32(size.X/2 - 90 - rect.getWidth(), size.Y/2-rect.getHeight()/2);
	
		v2u32 max_texture_size;
		{
			video::IVideoDriver* driver = Environment->getVideoDriver();
			max_texture_size = driver->getMaxTextureSize();
		}

		std::ostringstream os;
		os<<"Minetest\n";
		os<<minetest_build_info<<"\n";
		os<<"path_user = "<<wrap_rows(porting::path_user, 20)<<"\n";
	
		Environment->addStaticText(narrow_to_wide(os.str()).c_str(), rect, false, true, this, 259);
	}
	changeCtype("C");
}

void GUIPauseMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
}

bool GUIPauseMenu::OnEvent(const SEvent& event)
{

	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.PressedDown)
		{
			if(event.KeyInput.Key==KEY_ESCAPE)
			{
				quitMenu();
				return true;
			}
			else if(event.KeyInput.Key==KEY_RETURN)
			{
				quitMenu();
				return true;
			}
		}
	}
	if(event.EventType==EET_GUI_EVENT)
	{
		if(event.GUIEvent.EventType==gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible())
		{
			if(!canTakeFocus(event.GUIEvent.Element))
			{
				dstream<<"GUIPauseMenu: Not allowing focus change."
						<<std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			switch(event.GUIEvent.Caller->getID())
			{
			case 256: // continue
				quitMenu();
				// ALWAYS return immediately after quitMenu()
				return true;
			case 261:
				quitMenu();
				m_gamecallback->changePassword();
				return true;
			case 262:
				quitMenu();
				m_gamecallback->changeVolume();
				return true;
			case 260: // disconnect
				m_gamecallback->disconnect();
				quitMenu();
				return true;
			case 257: // exit
				m_gamecallback->exitToOS();
				quitMenu();
				return true;
			}
		}
	}
	
	return Parent ? Parent->OnEvent(event) : false;
}

