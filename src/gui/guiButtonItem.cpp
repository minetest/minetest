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

#include "guiButtonItem.h"

#include "client/client.h"
#include "client/hud.h" // drawItemStack
#include "IGUIEnvironment.h"
#include "itemdef.h"

using namespace irr;
using namespace gui;

GUIButtonItem::GUIButtonItem(gui::IGUIEnvironment *environment, gui::IGUIElement *parent,
		s32 id, core::rect<s32> rectangle, std::string item, Client *client, bool noclip)
		: GUIButton (environment, parent, id, rectangle, noclip)
{
	m_item_name = item;
	m_client = client;
}

void GUIButtonItem::drawContent()
{
	GUISkin *skin = dynamic_cast<GUISkin *>(Environment->getSkin());
	video::IVideoDriver *driver = Environment->getVideoDriver();

	IGUIFont *font = getActiveFont();

	IItemDefManager *idef = m_client->idef();
	ItemStack item;
	item.deSerialize(m_item_name, idef);

	core::rect<s32> rect = AbsoluteRect;
	if (isPressed()) {
		rect += core::dimension2d<s32>(
				skin->getSize(irr::gui::EGDS_BUTTON_PRESSED_IMAGE_OFFSET_X),
				skin->getSize(irr::gui::EGDS_BUTTON_PRESSED_IMAGE_OFFSET_Y));
	}
	drawItemStack(driver, font, item, rect, &AbsoluteClippingRect, m_client, IT_ROT_NONE);
}

GUIButtonItem *GUIButtonItem::addButton(IGUIEnvironment *environment,
		const core::rect<s32> &rectangle, IGUIElement *parent, s32 id,
		const wchar_t *text, std::string item, Client *client)
{
	GUIButtonItem *button = new GUIButtonItem(environment,
			parent ? parent : environment->getRootGUIElement(),
			id, rectangle, item, client);

	if (text)
		button->setText(text);

	button->drop();
	return button;
}
