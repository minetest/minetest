/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2018 stujones11, Stuart Jones <stujones111@gmail.com>

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

#include <cstdlib>
#include "modalMenu.h"
#include "gettext.h"
#include "porting.h"
#include "settings.h"

#ifdef HAVE_TOUCHSCREENGUI
#include "touchscreengui.h"
#endif

// clang-format off
GUIModalMenu::GUIModalMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr) :
		IGUIElement(gui::EGUIET_ELEMENT, env, parent, id,
				core::rect<s32>(0, 0, 100, 100)),
#ifdef __ANDROID__
		m_jni_field_name(""),
#endif
		m_menumgr(menumgr)
{
	m_gui_scale = g_settings->getFloat("gui_scaling");
#ifdef __ANDROID__
	float d = porting::getDisplayDensity();
	m_gui_scale *= 1.1 - 0.3 * d + 0.2 * d * d;
#endif
	setVisible(true);
	Environment->setFocus(this);
	m_menumgr->createdMenu(this);
}
// clang-format on

GUIModalMenu::~GUIModalMenu()
{
	m_menumgr->deletingMenu(this);
}

void GUIModalMenu::allowFocusRemoval(bool allow)
{
	m_allow_focus_removal = allow;
}

bool GUIModalMenu::canTakeFocus(gui::IGUIElement *e)
{
	return (e && (e == this || isMyChild(e))) || m_allow_focus_removal;
}

void GUIModalMenu::draw()
{
	if (!IsVisible)
		return;

	video::IVideoDriver *driver = Environment->getVideoDriver();
	v2u32 screensize = driver->getScreenSize();
	if (screensize != m_screensize_old) {
		m_screensize_old = screensize;
		regenerateGui(screensize);
	}

	drawMenu();
}

/*
	This should be called when the menu wants to quit.

	WARNING: THIS DEALLOCATES THE MENU FROM MEMORY. Return
	immediately if you call this from the menu itself.

	(More precisely, this decrements the reference count.)
*/
void GUIModalMenu::quitMenu()
{
	allowFocusRemoval(true);
	// This removes Environment's grab on us
	Environment->removeFocus(this);
	m_menumgr->deletingMenu(this);
	this->remove();
#ifdef HAVE_TOUCHSCREENGUI
	if (g_touchscreengui && m_touchscreen_visible)
		g_touchscreengui->show();
#endif
}

void GUIModalMenu::removeChildren()
{
	const core::list<gui::IGUIElement *> &children = getChildren();
	core::list<gui::IGUIElement *> children_copy;
	for (gui::IGUIElement *i : children) {
		children_copy.push_back(i);
	}

	for (gui::IGUIElement *i : children_copy) {
		i->remove();
	}
}

bool GUIModalMenu::preprocessEvent(const SEvent &event)
{
#ifdef __ANDROID__
	// clang-format off
	// display software keyboard when clicking edit boxes
	if (event.EventType == EET_MOUSE_INPUT_EVENT &&
			event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
		gui::IGUIElement *hovered =
			Environment->getRootGUIElement()->getElementFromPoint(
				core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y));
		if ((hovered) && (hovered->getType() == irr::gui::EGUIET_EDIT_BOX)) {
			bool retval = hovered->OnEvent(event);
			if (retval)
				Environment->setFocus(hovered);

			std::string field_name = getNameByID(hovered->getID());
			// read-only field
			if (field_name.empty())
				return retval;

			m_jni_field_name = field_name;
			/*~ Imperative, as in "Enter/type in text".
			Don't forget the space. */
			std::string message = gettext("Enter ");
			std::string label = wide_to_utf8(getLabelByID(hovered->getID()));
			if (label.empty())
				label = "text";
			message += gettext(label) + ":";

			// single line text input
			int type = 2;

			// multi line text input
			if (((gui::IGUIEditBox *)hovered)->isMultiLineEnabled())
				type = 1;

			// passwords are always single line
			if (((gui::IGUIEditBox *)hovered)->isPasswordBox())
				type = 3;

			porting::showInputDialog(gettext("ok"), "",
				wide_to_utf8(((gui::IGUIEditBox *)hovered)->getText()),	type);
			return retval;
		}
	}

	if (event.EventType == EET_TOUCH_INPUT_EVENT) {
		SEvent translated;
		memset(&translated, 0, sizeof(SEvent));
		translated.EventType = EET_MOUSE_INPUT_EVENT;
		gui::IGUIElement *root = Environment->getRootGUIElement();

		if (!root) {
			errorstream << "GUIModalMenu::preprocessEvent"
			            << " unable to get root element" << std::endl;
			return false;
		}
		gui::IGUIElement *hovered = root->getElementFromPoint(
			core::position2d<s32>(event.TouchInput.X, event.TouchInput.Y));

		translated.MouseInput.X = event.TouchInput.X;
		translated.MouseInput.Y = event.TouchInput.Y;
		translated.MouseInput.Control = false;

		bool dont_send_event = false;

		if (event.TouchInput.touchedCount == 1) {
			switch (event.TouchInput.Event) {
			case ETIE_PRESSED_DOWN:
				m_pointer = v2s32(event.TouchInput.X, event.TouchInput.Y);
				translated.MouseInput.Event = EMIE_LMOUSE_PRESSED_DOWN;
				translated.MouseInput.ButtonStates = EMBSM_LEFT;
				m_down_pos = m_pointer;
				break;
			case ETIE_MOVED:
				m_pointer = v2s32(event.TouchInput.X, event.TouchInput.Y);
				translated.MouseInput.Event = EMIE_MOUSE_MOVED;
				translated.MouseInput.ButtonStates = EMBSM_LEFT;
				break;
			case ETIE_LEFT_UP:
				translated.MouseInput.Event = EMIE_LMOUSE_LEFT_UP;
				translated.MouseInput.ButtonStates = 0;
				hovered = root->getElementFromPoint(m_down_pos);
				// we don't have a valid pointer element use last
				// known pointer pos
				translated.MouseInput.X = m_pointer.X;
				translated.MouseInput.Y = m_pointer.Y;

				// reset down pos
				m_down_pos = v2s32(0, 0);
				break;
			default:
				dont_send_event = true;
				// this is not supposed to happen
				errorstream << "GUIModalMenu::preprocessEvent"
				            << " unexpected usecase Event="
				            << event.TouchInput.Event << std::endl;
			}
		} else if ((event.TouchInput.touchedCount == 2) &&
				(event.TouchInput.Event == ETIE_PRESSED_DOWN)) {
			hovered = root->getElementFromPoint(m_down_pos);

			translated.MouseInput.Event = EMIE_RMOUSE_PRESSED_DOWN;
			translated.MouseInput.ButtonStates = EMBSM_LEFT | EMBSM_RIGHT;
			translated.MouseInput.X = m_pointer.X;
			translated.MouseInput.Y = m_pointer.Y;
			if (hovered) {
				hovered->OnEvent(translated);
			}

			translated.MouseInput.Event = EMIE_RMOUSE_LEFT_UP;
			translated.MouseInput.ButtonStates = EMBSM_LEFT;

			if (hovered) {
				hovered->OnEvent(translated);
			}
			dont_send_event = true;
		}
		// ignore unhandled 2 touch events ... accidental moving for example
		else if (event.TouchInput.touchedCount == 2) {
			dont_send_event = true;
		}
		else if (event.TouchInput.touchedCount > 2) {
			errorstream << "GUIModalMenu::preprocessEvent"
			            << " to many multitouch events "
			            << event.TouchInput.touchedCount << " ignoring them"
			            << std::endl;
		}

		if (dont_send_event) {
			return true;
		}

		// check if translated event needs to be preprocessed again
		if (preprocessEvent(translated)) {
			return true;
		}
		if (hovered) {
			grab();
			bool retval = hovered->OnEvent(translated);

			if (event.TouchInput.Event == ETIE_LEFT_UP) {
				// reset pointer
				m_pointer = v2s32(0, 0);
			}
			drop();
			return retval;
		}
	}
	// clang-format on
#endif
	return false;
}

#ifdef __ANDROID__
bool GUIModalMenu::hasAndroidUIInput()
{
	// no dialog shown
	if (m_jni_field_name.empty()) {
		return false;
	}

	// still waiting
	if (porting::getInputDialogState() == -1) {
		return true;
	}

	// no value abort dialog processing
	if (porting::getInputDialogState() != 0) {
		m_jni_field_name.clear();
		return false;
	}

	return true;
}
#endif
