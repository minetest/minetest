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

#include "guiItemImage.h"
#include "client/client.h"

GUIItemImage::GUIItemImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
	s32 id, const core::rect<s32> &rectangle, const std::string &item_name,
	gui::IGUIFont *font, Client *client) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
	m_item_name(item_name), m_font(font), m_client(client), m_label(core::stringw())
{
}

void GUIItemImage::draw()
{
	if (!IsVisible)
		return;

	if (!m_client) {
		IGUIElement::draw();
		return;
	}

	IItemDefManager *idef = m_client->idef();
	ItemStack item;
	item.deSerialize(m_item_name, idef);
	// Viewport rectangle on screen
	core::rect<s32> rect = core::rect<s32>(AbsoluteRect);
	drawItemStack(Environment->getVideoDriver(), m_font, item, rect,
			&AbsoluteClippingRect, m_client, IT_ROT_NONE);
	video::SColor color(255, 255, 255, 255);
	m_font->draw(m_label, rect, color, true, true, &AbsoluteClippingRect);

	IGUIElement::draw();
}
