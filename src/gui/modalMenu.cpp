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
#include "client/renderingengine.h"
#include "modalMenu.h"
#include "gettext.h"
#include "porting.h"
#include "settings.h"

#ifdef HAVE_TOUCHSCREENGUI
#include "touchscreengui.h"
#endif

// clang-format off
GUIModalMenu::GUIModalMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent,
	s32 id, IMenuManager *menumgr, bool remap_dbl_click) :
		IGUIElement(gui::EGUIET_ELEMENT, env, parent, id,
				core::rect<s32>(0, 0, 100, 100)),
#ifdef __ANDROID__
		m_jni_field_name(""),
#endif
		m_menumgr(menumgr),
		m_remap_dbl_click(remap_dbl_click)
{
	m_gui_scale = std::max(g_settings->getFloat("gui_scaling"), 0.5f);
	const float screen_dpi_scale = RenderingEngine::getDisplayDensity();
#ifdef HAVE_TOUCHSCREENGUI
	m_gui_scale *= 1.1f - 0.3f * screen_dpi_scale + 0.2f * screen_dpi_scale * screen_dpi_scale;
#else
	m_gui_scale *= screen_dpi_scale;
#endif

	setVisible(true);
	Environment->setFocus(this);
	m_menumgr->createdMenu(this);

	m_doubleclickdetect[0].time = 0;
	m_doubleclickdetect[1].time = 0;

	m_doubleclickdetect[0].pos = v2s32(0, 0);
	m_doubleclickdetect[1].pos = v2s32(0, 0);
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

// clang-format off
bool GUIModalMenu::DoubleClickDetection(const SEvent &event)
{
	/* The following code is for capturing double-clicks of the mouse button
	 * and translating the double-click into an EET_KEY_INPUT_EVENT event
	 * -- which closes the form -- under some circumstances.
	 *
	 * There have been many github issues reporting this as a bug even though it
	 * was an intended feature.  For this reason, remapping the double-click as
	 * an ESC must be explicitly set when creating this class via the
	 * /p remap_dbl_click parameter of the constructor.
	 */

	if (!m_remap_dbl_click)
		return false;

	if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
		m_doubleclickdetect[0].pos = m_doubleclickdetect[1].pos;
		m_doubleclickdetect[0].time = m_doubleclickdetect[1].time;

		m_doubleclickdetect[1].pos = m_pointer;
		m_doubleclickdetect[1].time = porting::getTimeMs();
	} else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
		u64 delta = porting::getDeltaMs(
			m_doubleclickdetect[0].time, porting::getTimeMs());
		if (delta > 400)
			return false;

		double squaredistance = m_doubleclickdetect[0].pos.
			getDistanceFromSQ(m_doubleclickdetect[1].pos);

		if (squaredistance > (30 * 30)) {
			return false;
		}

		SEvent translated{};
		// translate doubleclick to escape
		translated.EventType            = EET_KEY_INPUT_EVENT;
		translated.KeyInput.Key         = KEY_ESCAPE;
		translated.KeyInput.Control     = false;
		translated.KeyInput.Shift       = false;
		translated.KeyInput.PressedDown = true;
		translated.KeyInput.Char        = 0;
		OnEvent(translated);

		return true;
	}

	return false;
}
// clang-format on

static bool isChild(gui::IGUIElement *tocheck, gui::IGUIElement *parent)
{
	while (tocheck) {
		if (tocheck == parent) {
			return true;
		}
		tocheck = tocheck->getParent();
	}
	return false;
}

#ifdef HAVE_TOUCHSCREENGUI

bool GUIModalMenu::simulateMouseEvent(
		gui::IGUIElement *target, ETOUCH_INPUT_EVENT touch_event)
{
	SEvent mouse_event{}; // value-initialized, not unitialized
	mouse_event.EventType = EET_MOUSE_INPUT_EVENT;
	mouse_event.MouseInput.X = m_pointer.X;
	mouse_event.MouseInput.Y = m_pointer.Y;
	switch (touch_event) {
	case ETIE_PRESSED_DOWN:
		mouse_event.MouseInput.Event = EMIE_LMOUSE_PRESSED_DOWN;
		mouse_event.MouseInput.ButtonStates = EMBSM_LEFT;
		break;
	case ETIE_MOVED:
		mouse_event.MouseInput.Event = EMIE_MOUSE_MOVED;
		mouse_event.MouseInput.ButtonStates = EMBSM_LEFT;
		break;
	case ETIE_LEFT_UP:
		mouse_event.MouseInput.Event = EMIE_LMOUSE_LEFT_UP;
		mouse_event.MouseInput.ButtonStates = 0;
		break;
	default:
		return false;
	}
	if (preprocessEvent(mouse_event))
		return true;
	if (!target)
		return false;
	return target->OnEvent(mouse_event);
}

void GUIModalMenu::enter(gui::IGUIElement *hovered)
{
	if (!hovered)
		return;
	sanity_check(!m_hovered);
	m_hovered.grab(hovered);
	SEvent gui_event{};
	gui_event.EventType = EET_GUI_EVENT;
	gui_event.GUIEvent.Caller = m_hovered.get();
	gui_event.GUIEvent.EventType = EGET_ELEMENT_HOVERED;
	gui_event.GUIEvent.Element = gui_event.GUIEvent.Caller;
	m_hovered->OnEvent(gui_event);
}

void GUIModalMenu::leave()
{
	if (!m_hovered)
		return;
	SEvent gui_event{};
	gui_event.EventType = EET_GUI_EVENT;
	gui_event.GUIEvent.Caller = m_hovered.get();
	gui_event.GUIEvent.EventType = EGET_ELEMENT_LEFT;
	m_hovered->OnEvent(gui_event);
	m_hovered.reset();
}

#endif

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

			// single line text input
			int type = 2;

			// multi line text input
			if (((gui::IGUIEditBox *)hovered)->isMultiLineEnabled())
				type = 1;

			// passwords are always single line
			if (((gui::IGUIEditBox *)hovered)->isPasswordBox())
				type = 3;

			porting::showInputDialog(gettext("OK"), "",
				wide_to_utf8(((gui::IGUIEditBox *)hovered)->getText()), type);
			return retval;
		}
	}
#endif

#ifdef HAVE_TOUCHSCREENGUI
	if (event.EventType == EET_TOUCH_INPUT_EVENT) {
		irr_ptr<GUIModalMenu> holder;
		holder.grab(this); // keep this alive until return (it might be dropped downstream [?])

		switch ((int)event.TouchInput.touchedCount) {
		case 1: {
			if (event.TouchInput.Event == ETIE_PRESSED_DOWN || event.TouchInput.Event == ETIE_MOVED)
				m_pointer = v2s32(event.TouchInput.X, event.TouchInput.Y);
			if (event.TouchInput.Event == ETIE_PRESSED_DOWN)
				m_down_pos = m_pointer;
			gui::IGUIElement *hovered = Environment->getRootGUIElement()->getElementFromPoint(core::position2d<s32>(m_pointer));
			if (event.TouchInput.Event == ETIE_PRESSED_DOWN)
				Environment->setFocus(hovered);
			if (m_hovered != hovered) {
				leave();
				enter(hovered);
			}
			gui::IGUIElement *focused = Environment->getFocus();
			bool ret = simulateMouseEvent(focused, event.TouchInput.Event);
			if (!ret && m_hovered != focused)
				ret = simulateMouseEvent(m_hovered.get(), event.TouchInput.Event);
			if (event.TouchInput.Event == ETIE_LEFT_UP)
				leave();
			return ret;
		}
		case 2: {
			if (event.TouchInput.Event != ETIE_PRESSED_DOWN)
				return true; // ignore
			auto focused = Environment->getFocus();
			if (!focused)
				return true;
			SEvent rclick_event{};
			rclick_event.EventType = EET_MOUSE_INPUT_EVENT;
			rclick_event.MouseInput.Event = EMIE_RMOUSE_PRESSED_DOWN;
			rclick_event.MouseInput.ButtonStates = EMBSM_LEFT | EMBSM_RIGHT;
			rclick_event.MouseInput.X = m_pointer.X;
			rclick_event.MouseInput.Y = m_pointer.Y;
			focused->OnEvent(rclick_event);
			rclick_event.MouseInput.Event = EMIE_RMOUSE_LEFT_UP;
			rclick_event.MouseInput.ButtonStates = EMBSM_LEFT;
			focused->OnEvent(rclick_event);
			return true;
		}
		default: // ignored
			return true;
		}
	}
#endif

	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		s32 x = event.MouseInput.X;
		s32 y = event.MouseInput.Y;
		gui::IGUIElement *hovered =
				Environment->getRootGUIElement()->getElementFromPoint(
						core::position2d<s32>(x, y));
		if (!isChild(hovered, this)) {
			if (DoubleClickDetection(event)) {
				return true;
			}
		}
	}
	return false;
}

#ifdef __ANDROID__
bool GUIModalMenu::hasAndroidUIInput()
{
	// no dialog shown
	if (m_jni_field_name.empty())
		return false;

	// still waiting
	if (porting::getInputDialogState() == -1)
		return true;

	// no value abort dialog processing
	if (porting::getInputDialogState() != 0) {
		m_jni_field_name.clear();
		return false;
	}

	return true;
}
#endif
