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

#include <irrlicht.h>
using namespace irr;

class guiPauseMenu : public IEventReceiver
{
private:
	IrrlichtDevice *dev;
	gui::IGUIEnvironment *guienv;
	IEventReceiver *oldRecv;

	gui::IGUIStaticText *root;

	bool loadMenu();
	void scaleGui();
	void scaleGui(gui::IGUIElement *node,float factorX,float factorY);
public:
	guiPauseMenu(IrrlichtDevice *device,IEventReceiver *recv);

	void setVisible(bool visible){root->setVisible(visible);};
	bool isVisible(){return root->isVisible();};

	bool OnEvent(const SEvent& event);

	~guiPauseMenu(void);
};

#endif

