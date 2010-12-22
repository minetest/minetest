/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>
Original author Kabak Dmitry <userdima@gmail.com>, contributed under
the minetest contributor agreement.

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

GUIPauseMenu::GUIPauseMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IrrlichtDevice *dev):
	IGUIElement(gui::EGUIET_ELEMENT, env, parent, id,
			core::rect<s32>(0,0,100,100))
{
	m_dev = dev;
	m_screensize_old = v2u32(0,0);
	
	resizeGui();

	setVisible(false);
}

GUIPauseMenu::~GUIPauseMenu()
{
}

void GUIPauseMenu::resizeGui()
{
	video::IVideoDriver* driver = Environment->getVideoDriver();
	v2u32 screensize = driver->getScreenSize();
	if(screensize == m_screensize_old)
		return;
	m_screensize_old = screensize;

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

	core::rect<s32> rect(
			screensize.X/2 - 560/2,
			screensize.Y/2 - 300/2,
			screensize.X/2 + 560/2,
			screensize.Y/2 + 300/2
	);
	
	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 size = rect.getSize();

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
}

void GUIPauseMenu::draw()
{
	if(!IsVisible)
		return;
		
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
			setVisible(false);
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
				setVisible(false);
				break;
			case 257: // exit
				m_dev->closeDevice();
				break;
			}
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

#if 0
GUIPauseMenu::GUIPauseMenu(IrrlichtDevice *device, IEventReceiver *recv):
	dev(device),
	oldRecv(recv)
{
	if(!dev)
		return;
	guienv=dev->getGUIEnvironment();

	if (!loadMenu())
		return;

	device->setEventReceiver(this); // now WE are the input receiver! ahhaha! 
}

GUIPauseMenu::~GUIPauseMenu(void)
{
}

void GUIPauseMenu::scaleGui() // this function scales gui from the size stored in file to screen size
{
	core::dimension2du screen=dev->getVideoDriver()->getScreenSize();
	core::vector2di real=root->getAbsolutePosition().LowerRightCorner; // determine gui size stored in file (which is size of my menu root node)
	float factorX=(float)screen.Width/(float)real.X;
	float factorY=(float)screen.Height/(float)real.Y;
	scaleGui(guienv->getRootGUIElement(),factorX,factorY);
}
void GUIPauseMenu::scaleGui(gui::IGUIElement *node,float factorX,float factorY) // recursive set scale
{
	if((node->getParent() && node->getParent()->getID()==255) || node->getID()==255) // modify only menu's elements
	{
		int lx,rx,ly,ry;
		lx=(float)node->getRelativePosition().UpperLeftCorner.X*factorX;
		ly=(float)node->getRelativePosition().UpperLeftCorner.Y*factorY;
		rx=(float)node->getRelativePosition().LowerRightCorner.X*factorX;
		ry=(float)node->getRelativePosition().LowerRightCorner.Y*factorY;
		node->setRelativePosition(core::recti(lx,ly,rx,ry));
	}

	core::list<gui::IGUIElement*>::ConstIterator it = node->getChildren().begin();
	for(; it != node->getChildren().end(); ++it)
		scaleGui((*it),factorX,factorY);
}

bool GUIPauseMenu::loadMenu()
{
	guienv->loadGUI("../data/pauseMenu.gui");

	root=(gui::IGUIStaticText*)guienv->getRootGUIElement()->getElementFromId(255,true);
	if(!root) // if there is no my root node then menu file not found or corrupted
		return false;

	scaleGui(); // scale gui to our screen size

	root->setVisible(false); // hide our menu
	// make it transparent
	//root->setBackgroundColor(video::SColor(100,128,100,128));
	root->setBackgroundColor(video::SColor(140,0,0,0));

	return true;
}

bool GUIPauseMenu::OnEvent(const SEvent& event)
{
	if(!dev->isWindowFocused())
		setVisible(true);

	bool ret=false;
	if(oldRecv && !isVisible()) // call master if we have it and if we are inactive
		ret=oldRecv->OnEvent(event);

	if(ret==true)
		return true; // if the master receiver does the work

	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
			setVisible(!isVisible());
		}
	}
	if(event.EventType==EET_GUI_EVENT)
	{
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			switch(event.GUIEvent.Caller->getID())
			{
			case 256: // continue
				setVisible(false);
				break;
			case 257: // exit
				dev->closeDevice();
				break;
			}
		}
	}

	return false;
}
#endif

