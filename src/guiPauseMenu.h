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


#ifndef GUIPAUSEMENU_HEADER
#define GUIPAUSEMENU_HEADER

#include "common_irrlicht.h"

class GUIPauseMenu : public gui::IGUIElement
{
public:
	GUIPauseMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IrrlichtDevice *dev);
	~GUIPauseMenu();
	
	/*
		Remove and re-add (or reposition) stuff
	*/
	void resizeGui();

	void draw();

	void launch()
	{
		setVisible(true);
		Environment->setFocus(this);
	}

	bool canTakeFocus(gui::IGUIElement *e)
	{
		return (e && (e == this || isMyChild(e)));
	}

	bool OnEvent(const SEvent& event);
	
private:
	IrrlichtDevice *m_dev;
	v2u32 m_screensize_old;
};

/*class GUIPauseMenu : public IEventReceiver
{
public:
	void scaleGui();

	GUIPauseMenu(IrrlichtDevice *device,IEventReceiver *recv);
	~GUIPauseMenu(void);

	void setVisible(bool visible){root->setVisible(visible);};
	bool isVisible(){return root->isVisible();};

	bool OnEvent(const SEvent& event);

private:
	bool loadMenu();
	void scaleGui(gui::IGUIElement *node,float factorX,float factorY);

	IrrlichtDevice *dev;
	gui::IGUIEnvironment *guienv;
	IEventReceiver *oldRecv;

	gui::IGUIStaticText *root;
};*/

#endif

