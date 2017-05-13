/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "util/numeric.h"
#include "inputhandler.h"
#include "mainmenumanager.h"

bool MyEventReceiver::OnEvent(const SEvent &event)
{
	/*
		React to nothing here if a menu is active
	*/
	if (isMenuActive()) {
#ifdef HAVE_TOUCHSCREENGUI
		if (m_touchscreengui) {
			m_touchscreengui->Toggle(false);
		}
#endif
		return g_menumgr.preprocessEvent(event);
	}

	// Remember whether each key is down or up
	if (event.EventType == irr::EET_KEY_INPUT_EVENT) {
		const KeyPress &keyCode = event.KeyInput;
		if (keysListenedFor[keyCode]) {
			if (event.KeyInput.PressedDown) {
				keyIsDown.set(keyCode);
				keyWasDown.set(keyCode);
			} else {
				keyIsDown.unset(keyCode);
			}
			return true;
		}
	}

#ifdef HAVE_TOUCHSCREENGUI
	// case of touchscreengui we have to handle different events
	if (m_touchscreengui && event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
		m_touchscreengui->translateEvent(event);
		return true;
	}
#endif

	if (event.EventType == irr::EET_JOYSTICK_INPUT_EVENT) {
		/* TODO add a check like:
		if (event.JoystickEvent != joystick_we_listen_for)
			return false;
		*/
		return joystick->handleEvent(event.JoystickEvent);
	}
	// handle mouse events
	if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
		if (isMenuActive()) {
			left_active = false;
			middle_active = false;
			right_active = false;
		} else {
			left_active = event.MouseInput.isLeftPressed();
			middle_active = event.MouseInput.isMiddlePressed();
			right_active = event.MouseInput.isRightPressed();

			if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
				leftclicked = true;
			}
			if (event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) {
				rightclicked = true;
			}
			if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
				leftreleased = true;
			}
			if (event.MouseInput.Event == EMIE_RMOUSE_LEFT_UP) {
				rightreleased = true;
			}
			if (event.MouseInput.Event == EMIE_MOUSE_WHEEL) {
				mouse_wheel += event.MouseInput.Wheel;
			}
		}
	} else if (event.EventType == irr::EET_LOG_TEXT_EVENT) {
		static const LogLevel irr_loglev_conv[] = {
				LL_VERBOSE, // ELL_DEBUG
				LL_INFO,    // ELL_INFORMATION
				LL_WARNING, // ELL_WARNING
				LL_ERROR,   // ELL_ERROR
				LL_NONE,    // ELL_NONE
		};
		assert(event.LogEvent.Level < ARRLEN(irr_loglev_conv));
		g_logger.log(irr_loglev_conv[event.LogEvent.Level],
				std::string("Irrlicht: ") +
						(const char *)event.LogEvent.Text);
		return true;
	}
	/* always return false in order to continue processing events */
	return false;
}

/*
 * RandomInputHandler
 */
s32 RandomInputHandler::Rand(s32 min, s32 max)
{
	return (myrand() % (max - min + 1)) + min;
}
