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

#include "guiPauseMenu.h"
#include "debug.h"
#include "serialization.h"

GUIPauseMenu::GUIPauseMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IrrlichtDevice *dev,
		int *active_menu_count):
	GUIModalMenu(env, parent, id, active_menu_count)
{
	m_dev = dev;
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
	{
		core::rect<s32> rect(0, 0, 140, 30);
		rect = rect + v2s32(size.X/2-140/2, size.Y/2-30/2-25);
		Environment->addButton(rect, this, 256, L"Continue");
	}
	{
		core::rect<s32> rect(0, 0, 140, 30);
		rect = rect + v2s32(size.X/2-140/2, size.Y/2-30/2+25);
		Environment->addButton(rect, this, 257, L"Exit");
	}
	{
		core::rect<s32> rect(0, 0, 180, 220);
		rect = rect + v2s32(size.X/2 + 90, size.Y/2-rect.getHeight()/2);
		const wchar_t *text =
		L"Keys:\n"
		L"- WASD: Walk\n"
		L"- Mouse left: dig blocks\n"
		L"- Mouse right: place blocks\n"
		L"- Mouse wheel: select item\n"
		L"- R: Toggle viewing all loaded chunks\n"
		L"- I: Inventory menu\n"
		L"- ESC: This menu\n"
		L"- T: Chat\n"
		L"\n"
		L"To generate a new map, remove the map directory.\n";
		Environment->addStaticText(text, rect, false, true, this, 258);
	}
	{
		core::rect<s32> rect(0, 0, 180, 220);
		rect = rect + v2s32(size.X/2 - 90 - rect.getWidth(), size.Y/2-rect.getHeight()/2);
		wchar_t text[200];
		swprintf(text, 200,
				L"Minetest-c55\n"
				L"SER_FMT_VER_HIGHEST=%i",
				(int)SER_FMT_VER_HIGHEST
		);
	
		Environment->addStaticText(text, rect, false, true, this, 259);
	}
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
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
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
				break;
			case 257: // exit
				m_dev->closeDevice();
				break;
			}
		}
	}
	
	return Parent ? Parent->OnEvent(event) : false;
}

