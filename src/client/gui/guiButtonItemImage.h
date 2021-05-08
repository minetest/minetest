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

#pragma once

#include "guiButton.h"
#include "IGUIButton.h"

using namespace irr;

class Client;
class GUIItemImage;

class GUIButtonItemImage : public GUIButton
{
public:
	//! constructor
	GUIButtonItemImage(gui::IGUIEnvironment *environment, gui::IGUIElement *parent,
			s32 id, core::rect<s32> rectangle, ISimpleTextureSource *tsrc,
			const std::string &item, Client *client, bool noclip = false);

	//! Do not drop returned handle
	static GUIButtonItemImage *addButton(gui::IGUIEnvironment *environment,
			const core::rect<s32> &rectangle, ISimpleTextureSource *tsrc,
			IGUIElement *parent, s32 id, const wchar_t *text,
			const std::string &item, Client *client);

private:
	Client *m_client;
	GUIItemImage *m_image;
};
