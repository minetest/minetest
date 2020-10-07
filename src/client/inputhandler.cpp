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
#include "gui/mainmenumanager.h"
#include "hud.h"

void KeyCache::populate_nonchanging()
{
	key[KeyType::ESC] = EscapeKey;
}

void KeyCache::populate()
{
	key[KeyType::FORWARD] = getKeySetting("keymap_forward");
	key[KeyType::BACKWARD] = getKeySetting("keymap_backward");
	key[KeyType::LEFT] = getKeySetting("keymap_left");
	key[KeyType::RIGHT] = getKeySetting("keymap_right");
	key[KeyType::JUMP] = getKeySetting("keymap_jump");
	key[KeyType::SPECIAL1] = getKeySetting("keymap_special1");
	key[KeyType::SNEAK] = getKeySetting("keymap_sneak");
	key[KeyType::DIG] = getKeySetting("keymap_dig");
	key[KeyType::PLACE] = getKeySetting("keymap_place");

	key[KeyType::AUTOFORWARD] = getKeySetting("keymap_autoforward");

	key[KeyType::DROP] = getKeySetting("keymap_drop");
	key[KeyType::INVENTORY] = getKeySetting("keymap_inventory");
	key[KeyType::CHAT] = getKeySetting("keymap_chat");
	key[KeyType::CMD] = getKeySetting("keymap_cmd");
	key[KeyType::CMD_LOCAL] = getKeySetting("keymap_cmd_local");
	key[KeyType::CONSOLE] = getKeySetting("keymap_console");
	key[KeyType::MINIMAP] = getKeySetting("keymap_minimap");
	key[KeyType::FREEMOVE] = getKeySetting("keymap_freemove");
	key[KeyType::PITCHMOVE] = getKeySetting("keymap_pitchmove");
	key[KeyType::FASTMOVE] = getKeySetting("keymap_fastmove");
	key[KeyType::NOCLIP] = getKeySetting("keymap_noclip");
	key[KeyType::HOTBAR_PREV] = getKeySetting("keymap_hotbar_previous");
	key[KeyType::HOTBAR_NEXT] = getKeySetting("keymap_hotbar_next");
	key[KeyType::MUTE] = getKeySetting("keymap_mute");
	key[KeyType::INC_VOLUME] = getKeySetting("keymap_increase_volume");
	key[KeyType::DEC_VOLUME] = getKeySetting("keymap_decrease_volume");
	key[KeyType::CINEMATIC] = getKeySetting("keymap_cinematic");
	key[KeyType::SCREENSHOT] = getKeySetting("keymap_screenshot");
	key[KeyType::TOGGLE_HUD] = getKeySetting("keymap_toggle_hud");
	key[KeyType::TOGGLE_CHAT] = getKeySetting("keymap_toggle_chat");
	key[KeyType::TOGGLE_FOG] = getKeySetting("keymap_toggle_fog");
	key[KeyType::TOGGLE_UPDATE_CAMERA] = getKeySetting("keymap_toggle_update_camera");
	key[KeyType::TOGGLE_DEBUG] = getKeySetting("keymap_toggle_debug");
	key[KeyType::TOGGLE_PROFILER] = getKeySetting("keymap_toggle_profiler");
	key[KeyType::CAMERA_MODE] = getKeySetting("keymap_camera_mode");
	key[KeyType::INCREASE_VIEWING_RANGE] =
			getKeySetting("keymap_increase_viewing_range_min");
	key[KeyType::DECREASE_VIEWING_RANGE] =
			getKeySetting("keymap_decrease_viewing_range_min");
	key[KeyType::RANGESELECT] = getKeySetting("keymap_rangeselect");
	key[KeyType::ZOOM] = getKeySetting("keymap_zoom");

	key[KeyType::QUICKTUNE_NEXT] = getKeySetting("keymap_quicktune_next");
	key[KeyType::QUICKTUNE_PREV] = getKeySetting("keymap_quicktune_prev");
	key[KeyType::QUICKTUNE_INC] = getKeySetting("keymap_quicktune_inc");
	key[KeyType::QUICKTUNE_DEC] = getKeySetting("keymap_quicktune_dec");

	for (int i = 0; i < HUD_HOTBAR_ITEMCOUNT_MAX; i++) {
		std::string slot_key_name = "keymap_slot" + std::to_string(i + 1);
		key[KeyType::SLOT_1 + i] = getKeySetting(slot_key_name.c_str());
	}

	if (handler) {
		// First clear all keys, then re-add the ones we listen for
		handler->dontListenForKeys();
		for (const KeyPress &k : key) {
			handler->listenForKey(k);
		}
		handler->listenForKey(EscapeKey);
		handler->listenForKey(CancelKey);
	}
}

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
		if (keysListenedFor.count(keyCode)) {
				// If the key is being held down then the OS may
				// send a continuous stream of keydown events.
				// In this case, we don't want to let this
				// stream reach the application as it will cause
				// certain actions to repeat constantly.
			if (event.KeyInput.PressedDown) {
				if (!IsKeyDown(keyCode)) {
					keyWasDown.insert(keyCode);
					keyWasPressed.insert(keyCode);
				}
				keyIsDown.insert(keyCode);
			} else {
				if (IsKeyDown(keyCode))
					keyWasReleased.insert(keyCode);

				keyIsDown.erase(keyCode);
			}

			return true;
		}

#ifdef HAVE_TOUCHSCREENGUI
	} else if (m_touchscreengui && event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
		// In case of touchscreengui, we have to handle different events
		m_touchscreengui->translateEvent(event);
		return true;
#endif

	} else if (event.EventType == irr::EET_JOYSTICK_INPUT_EVENT) {
		/* TODO add a check like:
		if (event.JoystickEvent != joystick_we_listen_for)
			return false;
		*/
		return joystick->handleEvent(event.JoystickEvent);
	} else if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
		// Handle mouse events
		KeyPress key;
		switch (event.MouseInput.Event) {
		case EMIE_LMOUSE_PRESSED_DOWN:
			key = "KEY_LBUTTON";
			keyIsDown.insert(key);
			keyWasDown.insert(key);
			keyWasPressed.insert(key);
			break;
		case EMIE_MMOUSE_PRESSED_DOWN:
			key = "KEY_MBUTTON";
			keyIsDown.insert(key);
			keyWasDown.insert(key);
			keyWasPressed.insert(key);
			break;
		case EMIE_RMOUSE_PRESSED_DOWN:
			key = "KEY_RBUTTON";
			keyIsDown.insert(key);
			keyWasDown.insert(key);
			keyWasPressed.insert(key);
			break;
		case EMIE_LMOUSE_LEFT_UP:
			key = "KEY_LBUTTON";
			keyIsDown.erase(key);
			keyWasReleased.insert(key);
			break;
		case EMIE_MMOUSE_LEFT_UP:
			key = "KEY_MBUTTON";
			keyIsDown.erase(key);
			keyWasReleased.insert(key);
			break;
		case EMIE_RMOUSE_LEFT_UP:
			key = "KEY_RBUTTON";
			keyIsDown.erase(key);
			keyWasReleased.insert(key);
			break;
		case EMIE_MOUSE_WHEEL:
			mouse_wheel += event.MouseInput.Wheel;
			break;
		default: break;
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
				std::string("Irrlicht: ") + event.LogEvent.Text);
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

struct RandomInputHandlerSimData {
	std::string key;
	float counter;
	int time_max;
};

void RandomInputHandler::step(float dtime)
{
	static RandomInputHandlerSimData rnd_data[] = {
		{ "keymap_jump", 0.0f, 40 },
		{ "keymap_special1", 0.0f, 40 },
		{ "keymap_forward", 0.0f, 40 },
		{ "keymap_left", 0.0f, 40 },
		{ "keymap_dig", 0.0f, 30 },
		{ "keymap_place", 0.0f, 15 }
	};

	for (auto &i : rnd_data) {
		i.counter -= dtime;
		if (i.counter < 0.0) {
			i.counter = 0.1 * Rand(1, i.time_max);
			KeyPress k = getKeySetting(i.key.c_str());
			if (keydown.count(k))
				keydown.erase(k);
			else
				keydown.insert(k);
		}
	}
	{
		static float counter1 = 0;
		counter1 -= dtime;
		if (counter1 < 0.0) {
			counter1 = 0.1 * Rand(1, 20);
			mousespeed = v2s32(Rand(-20, 20), Rand(-15, 20));
		}
	}
	mousepos += mousespeed;
}
