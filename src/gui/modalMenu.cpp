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
#include <IEventReceiver.h>
#include <IGUIComboBox.h>
#include <IGUIEditBox.h>
#include "client/renderingengine.h"
#include "modalMenu.h"
#include "gettext.h"
#include "porting.h"
#include "settings.h"
#include "touchscreengui.h"

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

	if (g_settings->getBool("enable_touch")) {
		m_gui_scale *= 1.1f - 0.3f * screen_dpi_scale + 0.2f * screen_dpi_scale * screen_dpi_scale;
	} else {
		m_gui_scale *= screen_dpi_scale;
	}

	setVisible(true);
	m_menumgr->createdMenu(this);

	m_last_touch.time = 0;
	m_last_touch.pos = v2s32(0, 0);
}

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
	if (g_touchscreengui)
		g_touchscreengui->show();
}

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

bool GUIModalMenu::remapDoubleClick(const SEvent &event)
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

	if (event.EventType != EET_MOUSE_INPUT_EVENT ||
			event.MouseInput.Event != EMIE_LMOUSE_DOUBLE_CLICK)
		return false;

	// Only exit if the double-click happened outside the menu.
	gui::IGUIElement *hovered =
			Environment->getRootGUIElement()->getElementFromPoint(m_pointer);
	if (isChild(hovered, this))
		return false;

	// Translate double-click to escape.
	SEvent translated{};
	translated.EventType            = EET_KEY_INPUT_EVENT;
	translated.KeyInput.Key         = KEY_ESCAPE;
	translated.KeyInput.Control     = false;
	translated.KeyInput.Shift       = false;
	translated.KeyInput.PressedDown = true;
	translated.KeyInput.Char        = 0;
	OnEvent(translated);

	return true;
}

bool GUIModalMenu::simulateMouseEvent(ETOUCH_INPUT_EVENT touch_event, bool second_try)
{
	IGUIElement *target;
	if (!second_try)
		target = Environment->getFocus();
	else
		target = m_touch_hovered.get();

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
	case ETIE_COUNT:
		// ETIE_COUNT is used for double-tap events.
		mouse_event.MouseInput.Event = EMIE_LMOUSE_DOUBLE_CLICK;
		mouse_event.MouseInput.ButtonStates = EMBSM_LEFT;
		break;
	default:
		return false;
	}

	bool retval;
	m_simulated_mouse = true;
	do {
		if (preprocessEvent(mouse_event)) {
			retval = true;
			break;
		}
		if (!target) {
			retval = false;
			break;
		}
		retval = target->OnEvent(mouse_event);
	} while (false);
	m_simulated_mouse = false;

	if (!retval && !second_try)
		return simulateMouseEvent(touch_event, true);

	return retval;
}

void GUIModalMenu::enter(gui::IGUIElement *hovered)
{
	if (!hovered)
		return;
	sanity_check(!m_touch_hovered);
	m_touch_hovered.grab(hovered);
	SEvent gui_event{};
	gui_event.EventType = EET_GUI_EVENT;
	gui_event.GUIEvent.Caller = m_touch_hovered.get();
	gui_event.GUIEvent.EventType = gui::EGET_ELEMENT_HOVERED;
	gui_event.GUIEvent.Element = gui_event.GUIEvent.Caller;
	m_touch_hovered->OnEvent(gui_event);
}

void GUIModalMenu::leave()
{
	if (!m_touch_hovered)
		return;
	SEvent gui_event{};
	gui_event.EventType = EET_GUI_EVENT;
	gui_event.GUIEvent.Caller = m_touch_hovered.get();
	gui_event.GUIEvent.EventType = gui::EGET_ELEMENT_LEFT;
	m_touch_hovered->OnEvent(gui_event);
	m_touch_hovered.reset();
}

bool GUIModalMenu::preprocessEvent(const SEvent &event)
{
#ifdef __ANDROID__
	// display software keyboard when clicking edit boxes
	if (event.EventType == EET_MOUSE_INPUT_EVENT &&
			event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN &&
			!porting::hasPhysicalKeyboardAndroid()) {
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

			porting::showTextInputDialog("",
					wide_to_utf8(((gui::IGUIEditBox *) hovered)->getText()), type);
			return retval;
		}
	}

	if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == gui::EGET_LISTBOX_OPENED) {
			gui::IGUIComboBox *dropdown = (gui::IGUIComboBox *) event.GUIEvent.Caller;

			std::string field_name = getNameByID(dropdown->getID());
			if (field_name.empty())
				return false;

			m_jni_field_name = field_name;

			s32 selected_idx = dropdown->getSelected();
			s32 option_size = dropdown->getItemCount();
			std::string list_of_options[option_size];

			for (s32 i = 0; i < option_size; i++) {
				list_of_options[i] = wide_to_utf8(dropdown->getItem(i));
			}

			porting::showComboBoxDialog(list_of_options, option_size, selected_idx);
			return true; // Prevent the Irrlicht dropdown from opening.
		}
	}
#endif

	// Convert touch events into mouse events.
	if (event.EventType == EET_TOUCH_INPUT_EVENT) {
		irr_ptr<GUIModalMenu> holder;
		holder.grab(this); // keep this alive until return (it might be dropped downstream [?])

		if (event.TouchInput.touchedCount == 1) {
			m_pointer_type = PointerType::Touch;
			m_pointer = v2s32(event.TouchInput.X, event.TouchInput.Y);

			gui::IGUIElement *hovered = Environment->getRootGUIElement()->getElementFromPoint(core::position2d<s32>(m_pointer));
			if (event.TouchInput.Event == ETIE_PRESSED_DOWN)
				Environment->setFocus(hovered);
			if (m_touch_hovered != hovered) {
				leave();
				enter(hovered);
			}
			bool ret = simulateMouseEvent(event.TouchInput.Event);
			if (event.TouchInput.Event == ETIE_LEFT_UP)
				leave();

			// Detect double-taps and convert them into double-click events.
			if (event.TouchInput.Event == ETIE_PRESSED_DOWN) {
				u64 time_now = porting::getTimeMs();
				u64 time_delta = porting::getDeltaMs(m_last_touch.time, time_now);

				v2s32 pos_delta = m_pointer - m_last_touch.pos;
				f32 distance_sq = (f32)pos_delta.X * pos_delta.X +
						(f32)pos_delta.Y * pos_delta.Y;

				if (time_delta < 400 && distance_sq < (30 * 30)) {
					// ETIE_COUNT is used for double-tap events.
					simulateMouseEvent(ETIE_COUNT);
				}

				m_last_touch.time = time_now;
				m_last_touch.pos = m_pointer;
			}

			return ret;
		} else if (event.TouchInput.touchedCount == 2) {
			if (event.TouchInput.Event != ETIE_LEFT_UP)
				return true; // ignore
			auto focused = Environment->getFocus();
			if (!focused)
				return true;
			// The second-touch event is propagated as is (not converted).
			m_second_touch = true;
			focused->OnEvent(event);
			m_second_touch = false;
			return true;
		} else {
			// Any other touch after the second touch is ignored.
			return true;
		}
	}

	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		if (!m_simulated_mouse) {
			// Only set the pointer type to mouse if this is a real mouse event.
			m_pointer_type = PointerType::Mouse;
			m_pointer = v2s32(event.MouseInput.X, event.MouseInput.Y);
			m_touch_hovered.reset();
		}

		if (remapDoubleClick(event))
			return true;
	}

	return false;
}

#ifdef __ANDROID__
porting::AndroidDialogState GUIModalMenu::getAndroidUIInputState()
{
	// No dialog is shown
	if (m_jni_field_name.empty())
		return porting::DIALOG_CANCELED;

	return porting::getInputDialogState();
}
#endif
