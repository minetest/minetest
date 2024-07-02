/*
Minetest
Copyright (C) 2020 DS

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

#include "guiScrollContainer.h"

GUIScrollContainer::GUIScrollContainer(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, const core::rect<s32> &rectangle,
		const std::string &orientation, f32 scrollfactor) :
		gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
		m_scrollbar(nullptr), m_scrollfactor(scrollfactor)
{
	if (orientation == "vertical")
		m_orientation = VERTICAL;
	else if (orientation == "horizontal")
		m_orientation = HORIZONTAL;
	else
		m_orientation = UNDEFINED;
}

bool GUIScrollContainer::OnEvent(const SEvent &event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT &&
			event.MouseInput.Event == EMIE_MOUSE_WHEEL &&
			!event.MouseInput.isLeftPressed() && m_scrollbar) {
		Environment->setFocus(m_scrollbar);
		bool retval = m_scrollbar->OnEvent(event);

		// a hacky fix for updating the hovering and co.
		IGUIElement *hovered_elem = getElementFromPoint(core::position2d<s32>(
				event.MouseInput.X, event.MouseInput.Y));
		SEvent mov_event = event;
		mov_event.MouseInput.Event = EMIE_MOUSE_MOVED;
		Environment->postEventFromUser(mov_event);
		if (hovered_elem)
			hovered_elem->OnEvent(mov_event);

		return retval;
	}

	return IGUIElement::OnEvent(event);
}

void GUIScrollContainer::draw()
{
	if (isVisible()) {
		for (auto child : Children)
			if (child->isNotClipped() ||
					AbsoluteClippingRect.isRectCollided(
							child->getAbsolutePosition()))
				child->draw();
	}
}

void GUIScrollContainer::calculateAutoScrollFactor()
{
	if (!m_scrollbar || !m_auto_scrollfactor)
		return;

	core::rect<s32> size;
	for (gui::IGUIElement *e : Children) {
		core::rect<s32> abs_rect = e->getRelativePosition();
		size.addInternalPoint(abs_rect.UpperLeftCorner);
		size.addInternalPoint(abs_rect.LowerRightCorner);
	}

	// Cannot use RelativeRect because it is moved upwards as we scroll down
	size.LowerRightCorner.X -= AbsoluteClippingRect.getWidth();
	size.LowerRightCorner.Y -= AbsoluteClippingRect.getHeight();

	f32 new_factor = 0;
	if (m_orientation == VERTICAL)
		new_factor = -size.getHeight();
	else if (m_orientation == HORIZONTAL)
		new_factor = -size.getWidth();

	// This factor is always <= 0, or we would be scrolling in opposite direction
	m_scrollfactor = std::min<f32>(new_factor / m_scrollbar->getMax(), 0.0f);
}

void GUIScrollContainer::updateScrolling()
{
	s32 pos = m_scrollbar->getPos();
	core::rect<s32> rect = getRelativePosition();

	if (m_orientation == VERTICAL)
		rect.UpperLeftCorner.Y = pos * m_scrollfactor;
	else if (m_orientation == HORIZONTAL)
		rect.UpperLeftCorner.X = pos * m_scrollfactor;

	setRelativePosition(rect);
}
