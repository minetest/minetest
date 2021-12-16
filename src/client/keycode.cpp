/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "keycode.h"
#include "exceptions.h"
#include "settings.h"
#include "log.h"
#include "debug.h"
#include "util/hex.h"
#include "util/string.h"
#include "util/basic_macros.h"
#include <unordered_map>

std::unordered_map<irr::EKEY_CODE, std::string> code_to_str;
std::unordered_map<std::string, irr::EKEY_CODE> str_to_code;

struct KeyInfo
{
	KeyInfo()
	{
#define K(code, str) \
		code_to_str[irr::KEY_ ## code] = str; \
		str_to_code[std::string(str)] = irr::KEY_ ## code;

		K(LBUTTON,		"Left Mouse")
		K(MBUTTON,		"Middle Mouse")
		K(RBUTTON,		"Right Mouse")

		K(KEY_0,		"0")
		K(KEY_1,		"1")
		K(KEY_2,		"2")
		K(KEY_3,		"3")
		K(KEY_4,		"4")
		K(KEY_5,		"5")
		K(KEY_6,		"6")
		K(KEY_7,		"7")
		K(KEY_8,		"8")
		K(KEY_9,		"9")
		K(KEY_A,		"A")
		K(OEM_7,		"'")
		K(APPS,			"Application")
		K(KEY_B,		"B")
		K(OEM_5,		"\\")
		K(BACK,			"Backspace")
		K(KEY_C,		"C")
		K(CANCEL,		"Cancel")
		K(CAPITAL,		"Caps Lock")
		K(CLEAR,		"Clear")
		K(COMMA,		",")
		K(CRSEL,		"CrSel")
		K(KEY_D,		"D")
		K(DELETE,		"Delete")
		K(DOWN,			"Down")
		K(KEY_E,		"E")
		K(END,			"End")
		K(PLUS,			"=")
		K(ESCAPE,		"Escape")
		K(EXECUT,		"Execute")
		K(EXSEL,		"ExSel")
		K(KEY_F,		"F")
		K(F1,			"F1")
		K(F10,			"F10")
		K(F11,			"F11")
		K(F12,			"F12")
		K(F13,			"F13")
		K(F14,			"F14")
		K(F15,			"F15")
		K(F16,			"F16")
		K(F17,			"F17")
		K(F18,			"F18")
		K(F19,			"F19")
		K(F2,			"F2")
		K(F20,			"F20")
		K(F21,			"F21")
		K(F22,			"F22")
		K(F23,			"F23")
		K(F24,			"F24")
		K(F3,			"F3")
		K(F4,			"F4")
		K(F5,			"F5")
		K(F6,			"F6")
		K(F7,			"F7")
		K(F8,			"F8")
		K(F9,			"F9")
		K(KEY_G,		"G")
		K(OEM_3,		"`")
		K(KEY_H,		"H")
		K(HELP,			"Help")
		K(HOME,			"Home")
		K(KEY_I,		"I")
		K(INSERT,		"Insert")
		K(KEY_J,		"J")
		K(KEY_K,		"K")
		K(NUMPAD0,		"Keypad 0")
		K(NUMPAD1,		"Keypad 1")
		K(NUMPAD2,		"Keypad 2")
		K(NUMPAD3,		"Keypad 3")
		K(NUMPAD4,		"Keypad 4")
		K(NUMPAD5,		"Keypad 5")
		K(NUMPAD6,		"Keypad 6")
		K(NUMPAD7,		"Keypad 7")
		K(NUMPAD8,		"Keypad 8")
		K(NUMPAD9,		"Keypad 9")
		K(SUBTRACT,		"Keypad -")
		K(MULTIPLY,		"Keypad *")
		K(DIVIDE,		"Keypad /")
		K(DECIMAL,		"Keypad .")
		K(ADD,			"Keypad +")
		K(KEY_L,		"L")
		K(LMENU,		"Left Alt")
		K(LCONTROL,		"Left Control")
		K(LEFT,			"Left")
		K(OEM_4,		"[")
		K(LWIN,			"Left Super")
		K(LSHIFT,		"Left Shift")
		K(KEY_M,		"M")
		K(MENU,			"Menu")
		K(MINUS,		"-")
		K(MODECHANGE,	"Mode Switch")
		K(KEY_N,		"N")
		K(NUMLOCK,		"Numlock")
		K(KEY_O,		"O")
		K(KEY_P,		"P")
		K(NEXT,			"Page Down")
		K(PRIOR,		"Page Up")
		K(PAUSE,		"Pause")
		K(PERIOD,		".")
		K(SNAPSHOT,		"Print Screen")
		K(KEY_Q,		"Q")
		K(KEY_R,		"R")
		K(RMENU,		"Right Alt")
		K(RCONTROL,		"Right Control")
		K(RETURN,		"Return")
		K(RWIN,			"Right Super")
		K(RIGHT,		"Right")
		K(OEM_6,		"]")
		K(RSHIFT,		"Right Shift")
		K(KEY_S,		"S")
		K(SCROLL,		"Scroll Lock")
		K(SELECT,		"Select")
		K(OEM_1,		";")
		K(SEPARATOR,	"Separator")
		K(OEM_2,		"/")
		K(SLEEP,		"Sleep")
		K(SPACE,		"Space")
		K(KEY_T,		"T")
		K(TAB,			"Tab")
		K(KEY_U,		"U")
		K(UP,			"Up")
		K(KEY_V,		"V")
		K(KEY_W,		"W")
		K(KEY_X,		"X")
		K(KEY_Y,		"Y")
		K(KEY_Z,		"Z")
		K(KEY_CODES_COUNT, "")
	}
};
KeyInfo static_constructor{};

void KeyPress::fromName(const std::string &name)
{
	const auto &it = str_to_code.find(name);
	if (it == str_to_code.end())
		m_key = irr::KEY_KEY_CODES_COUNT;
	m_key = it->second;
}

std::string KeyPress::getName() const
{
	const auto &it = code_to_str.find(m_key);
	if (it == code_to_str.end())
		return "Unknown Key";
	return it->second;
}

const KeyPress EscapeKey(irr::KEY_ESCAPE);
const KeyPress CancelKey(irr::KEY_CANCEL);

// A simple cache for quicker lookup
std::unordered_map<std::string, KeyPress> g_key_setting_cache;

KeyPress getKeySetting(const char *settingname)
{
	std::unordered_map<std::string, KeyPress>::iterator n;
	n = g_key_setting_cache.find(settingname);
	if (n != g_key_setting_cache.end())
		return n->second;

	KeyPress k(g_settings->get(settingname).c_str());
	g_key_setting_cache[settingname] = k;
	return k;
}

void clearKeyCache()
{
	g_key_setting_cache.clear();
}
