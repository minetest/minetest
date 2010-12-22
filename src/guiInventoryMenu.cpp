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


#include "guiInventoryMenu.h"
#include "constants.h"

void drawInventoryItem(gui::IGUIEnvironment* env,
		InventoryItem *item, core::rect<s32> rect,
		const core::rect<s32> *clip)
{
	gui::IGUISkin* skin = env->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = env->getVideoDriver();
	
	video::ITexture *texture = NULL;
	
	if(item != NULL)
	{
		texture = item->getImage();
	}

	if(texture != NULL)
	{
		const video::SColor color(255,255,255,255);
		const video::SColor colors[] = {color,color,color,color};
		driver->draw2DImage(texture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
			core::dimension2di(texture->getOriginalSize())),
			clip, colors, false);
	}
	else
	{
		video::SColor bgcolor(128,128,128,128);
		driver->draw2DRectangle(bgcolor, rect, clip);
	}

	if(item != NULL)
	{
		gui::IGUIFont *font = skin->getFont();
		if(font)
		{
			core::rect<s32> rect2(rect.UpperLeftCorner,
					(core::dimension2d<u32>(rect.getWidth(), 15)));

			video::SColor bgcolor(128,0,0,0);
			driver->draw2DRectangle(bgcolor, rect2, clip);

			font->draw(item->getText().c_str(), rect2,
					video::SColor(255,255,255,255), false, false,
					clip);
		}
	}
}

/*
	GUIInventorySlot
*/

GUIInventorySlot::GUIInventorySlot(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id, core::rect<s32> rect):
	IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rect)
{
	m_item = NULL;
}

void GUIInventorySlot::draw()
{
	if(!IsVisible)
		return;
	
	drawInventoryItem(Environment, m_item, AbsoluteRect,
			&AbsoluteClippingRect);

	gui::IGUIElement::draw();
}

bool GUIInventorySlot::OnEvent(const SEvent& event)
{
	/*if (!IsEnabled)
		return IGUIElement::OnEvent(event);*/

	switch(event.EventType)
	{
	case EET_MOUSE_INPUT_EVENT:
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
		{
			dstream<<"Slot pressed"<<std::endl;
			//return true;
		}
		else
		if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
		{
			dstream<<"Slot released"<<std::endl;
			//return true;
		}
		break;
	default:
		break;
	}

	return Parent ? Parent->OnEvent(event) : false;
}

/*
	GUIInventoryMenu
*/

GUIInventoryMenu::GUIInventoryMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		Inventory *inventory):
	IGUIElement(gui::EGUIET_ELEMENT, env, parent, id,
			core::rect<s32>(0,0,100,100))
{
	m_inventory = inventory;
	m_screensize_old = v2u32(0,0);

	resizeGui();

	setVisible(false);
}

GUIInventoryMenu::~GUIInventoryMenu()
{
}

void GUIInventoryMenu::resizeGui()
{
	video::IVideoDriver* driver = Environment->getVideoDriver();
	v2u32 screensize = driver->getScreenSize();
	if(screensize == m_screensize_old)
		return;
	m_screensize_old = screensize;

	core::rect<s32> rect(
			screensize.X/2 - 560/2,
			screensize.Y/2 - 480/2,
			screensize.X/2 + 560/2,
			screensize.Y/2 + 480/2
	);
	
	DesiredRect = rect;
	recalculateAbsolutePosition(false);
}

void GUIInventoryMenu::update()
{
}

void GUIInventoryMenu::draw()
{
	if(!IsVisible)
		return;
		
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	/*
		Draw items
	*/
	
	{
		InventoryList *ilist = m_inventory->getList("main");
		if(ilist != NULL)
		{
			core::rect<s32> imgsize(0,0,48,48);
			v2s32 basepos(30, 210);
			basepos += AbsoluteRect.UpperLeftCorner;
			for(s32 i=0; i<PLAYER_INVENTORY_SIZE; i++)
			{
				s32 x = (i%8) * 64;
				s32 y = (i/8) * 64;
				v2s32 p(x,y);
				core::rect<s32> rect = imgsize + basepos + p;
				drawInventoryItem(Environment, ilist->getItem(i),
						rect, &AbsoluteClippingRect);
			}
		}
	}

	{
		InventoryList *ilist = m_inventory->getList("craft");
		if(ilist != NULL)
		{
			core::rect<s32> imgsize(0,0,48,48);
			v2s32 basepos(30, 30);
			basepos += AbsoluteRect.UpperLeftCorner;
			for(s32 i=0; i<9; i++)
			{
				s32 x = (i%3) * 64;
				s32 y = (i/3) * 64;
				v2s32 p(x,y);
				core::rect<s32> rect = imgsize + basepos + p;
				drawInventoryItem(Environment, ilist->getItem(i),
						rect, &AbsoluteClippingRect);
			}
		}
	}

	/*
		Call base class
	*/
	gui::IGUIElement::draw();
}

bool GUIInventoryMenu::OnEvent(const SEvent& event)
{
	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
			setVisible(false);
			return true;
		}
		if(event.KeyInput.Key==KEY_KEY_I && event.KeyInput.PressedDown)
		{
			setVisible(false);
			return true;
		}
	}
	if(event.EventType==EET_MOUSE_INPUT_EVENT)
	{
		if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
		{
		}
	}
	if(event.EventType==EET_GUI_EVENT)
	{
		if(event.GUIEvent.EventType==gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible())
		{
			if(!canTakeFocus(event.GUIEvent.Element))
			{
				dstream<<"GUIInventoryMenu: Not allowing focus change."
						<<std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			/*switch(event.GUIEvent.Caller->getID())
			{
			case 256: // continue
				setVisible(false);
				break;
			case 257: // exit
				dev->closeDevice();
				break;
			}*/
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}


