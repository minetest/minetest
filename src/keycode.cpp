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
#include "main.h" // For g_settings
#include "exceptions.h"
#include "settings.h"
#include "log.h"
#include "hex.h"
#include "debug.h"

class UnknownKeycode : public BaseException
{
public:
	UnknownKeycode(const char *s) :
		BaseException(s) {};
};

#define CHECKKEY(x){if(strcmp(name, #x)==0) return irr::x;}

irr::EKEY_CODE keyname_to_keycode(const char *name)
{
	CHECKKEY(KEY_LBUTTON)
	CHECKKEY(KEY_RBUTTON)
	CHECKKEY(KEY_CANCEL)
	CHECKKEY(KEY_MBUTTON)
	CHECKKEY(KEY_XBUTTON1)
	CHECKKEY(KEY_XBUTTON2)
	CHECKKEY(KEY_BACK)
	CHECKKEY(KEY_TAB)
	CHECKKEY(KEY_CLEAR)
	CHECKKEY(KEY_RETURN)
	CHECKKEY(KEY_SHIFT)
	CHECKKEY(KEY_CONTROL)
	CHECKKEY(KEY_MENU)
	CHECKKEY(KEY_PAUSE)
	CHECKKEY(KEY_CAPITAL)
	CHECKKEY(KEY_KANA)
	CHECKKEY(KEY_HANGUEL)
	CHECKKEY(KEY_HANGUL)
	CHECKKEY(KEY_JUNJA)
	CHECKKEY(KEY_FINAL)
	CHECKKEY(KEY_HANJA)
	CHECKKEY(KEY_KANJI)
	CHECKKEY(KEY_ESCAPE)
	CHECKKEY(KEY_CONVERT)
	CHECKKEY(KEY_NONCONVERT)
	CHECKKEY(KEY_ACCEPT)
	CHECKKEY(KEY_MODECHANGE)
	CHECKKEY(KEY_SPACE)
	CHECKKEY(KEY_PRIOR)
	CHECKKEY(KEY_NEXT)
	CHECKKEY(KEY_END)
	CHECKKEY(KEY_HOME)
	CHECKKEY(KEY_LEFT)
	CHECKKEY(KEY_UP)
	CHECKKEY(KEY_RIGHT)
	CHECKKEY(KEY_DOWN)
	CHECKKEY(KEY_SELECT)
	CHECKKEY(KEY_PRINT)
	CHECKKEY(KEY_EXECUT)
	CHECKKEY(KEY_SNAPSHOT)
	CHECKKEY(KEY_INSERT)
	CHECKKEY(KEY_DELETE)
	CHECKKEY(KEY_HELP)
	CHECKKEY(KEY_KEY_0)
	CHECKKEY(KEY_KEY_1)
	CHECKKEY(KEY_KEY_2)
	CHECKKEY(KEY_KEY_3)
	CHECKKEY(KEY_KEY_4)
	CHECKKEY(KEY_KEY_5)
	CHECKKEY(KEY_KEY_6)
	CHECKKEY(KEY_KEY_7)
	CHECKKEY(KEY_KEY_8)
	CHECKKEY(KEY_KEY_9)
	CHECKKEY(KEY_KEY_A)
	CHECKKEY(KEY_KEY_B)
	CHECKKEY(KEY_KEY_C)
	CHECKKEY(KEY_KEY_D)
	CHECKKEY(KEY_KEY_E)
	CHECKKEY(KEY_KEY_F)
	CHECKKEY(KEY_KEY_G)
	CHECKKEY(KEY_KEY_H)
	CHECKKEY(KEY_KEY_I)
	CHECKKEY(KEY_KEY_J)
	CHECKKEY(KEY_KEY_K)
	CHECKKEY(KEY_KEY_L)
	CHECKKEY(KEY_KEY_M)
	CHECKKEY(KEY_KEY_N)
	CHECKKEY(KEY_KEY_O)
	CHECKKEY(KEY_KEY_P)
	CHECKKEY(KEY_KEY_Q)
	CHECKKEY(KEY_KEY_R)
	CHECKKEY(KEY_KEY_S)
	CHECKKEY(KEY_KEY_T)
	CHECKKEY(KEY_KEY_U)
	CHECKKEY(KEY_KEY_V)
	CHECKKEY(KEY_KEY_W)
	CHECKKEY(KEY_KEY_X)
	CHECKKEY(KEY_KEY_Y)
	CHECKKEY(KEY_KEY_Z)
	CHECKKEY(KEY_LWIN)
	CHECKKEY(KEY_RWIN)
	CHECKKEY(KEY_APPS)
	CHECKKEY(KEY_SLEEP)
	CHECKKEY(KEY_NUMPAD0)
	CHECKKEY(KEY_NUMPAD1)
	CHECKKEY(KEY_NUMPAD2)
	CHECKKEY(KEY_NUMPAD3)
	CHECKKEY(KEY_NUMPAD4)
	CHECKKEY(KEY_NUMPAD5)
	CHECKKEY(KEY_NUMPAD6)
	CHECKKEY(KEY_NUMPAD7)
	CHECKKEY(KEY_NUMPAD8)
	CHECKKEY(KEY_NUMPAD9)
	CHECKKEY(KEY_MULTIPLY)
	CHECKKEY(KEY_ADD)
	CHECKKEY(KEY_SEPARATOR)
	CHECKKEY(KEY_SUBTRACT)
	CHECKKEY(KEY_DECIMAL)
	CHECKKEY(KEY_DIVIDE)
	CHECKKEY(KEY_F1)
	CHECKKEY(KEY_F2)
	CHECKKEY(KEY_F3)
	CHECKKEY(KEY_F4)
	CHECKKEY(KEY_F5)
	CHECKKEY(KEY_F6)
	CHECKKEY(KEY_F7)
	CHECKKEY(KEY_F8)
	CHECKKEY(KEY_F9)
	CHECKKEY(KEY_F10)
	CHECKKEY(KEY_F11)
	CHECKKEY(KEY_F12)
	CHECKKEY(KEY_F13)
	CHECKKEY(KEY_F14)
	CHECKKEY(KEY_F15)
	CHECKKEY(KEY_F16)
	CHECKKEY(KEY_F17)
	CHECKKEY(KEY_F18)
	CHECKKEY(KEY_F19)
	CHECKKEY(KEY_F20)
	CHECKKEY(KEY_F21)
	CHECKKEY(KEY_F22)
	CHECKKEY(KEY_F23)
	CHECKKEY(KEY_F24)
	CHECKKEY(KEY_NUMLOCK)
	CHECKKEY(KEY_SCROLL)
	CHECKKEY(KEY_LSHIFT)
	CHECKKEY(KEY_RSHIFT)
	CHECKKEY(KEY_LCONTROL)
	CHECKKEY(KEY_RCONTROL)
	CHECKKEY(KEY_LMENU)
	CHECKKEY(KEY_RMENU)
	CHECKKEY(KEY_PLUS)
	CHECKKEY(KEY_COMMA)
	CHECKKEY(KEY_MINUS)
	CHECKKEY(KEY_PERIOD)
	CHECKKEY(KEY_ATTN)
	CHECKKEY(KEY_CRSEL)
	CHECKKEY(KEY_EXSEL)
	CHECKKEY(KEY_EREOF)
	CHECKKEY(KEY_PLAY)
	CHECKKEY(KEY_ZOOM)
	CHECKKEY(KEY_PA1)
	CHECKKEY(KEY_OEM_CLEAR)

	throw UnknownKeycode(name);
}

static const char *KeyNames[] =
{ "-", "KEY_LBUTTON", "KEY_RBUTTON", "KEY_CANCEL", "KEY_MBUTTON", "KEY_XBUTTON1",
		"KEY_XBUTTON2", "-", "KEY_BACK", "KEY_TAB", "-", "-", "KEY_CLEAR", "KEY_RETURN", "-",
		"-", "KEY_SHIFT", "KEY_CONTROL", "KEY_MENU", "KEY_PAUSE", "KEY_CAPITAL", "KEY_KANA", "-",
		"KEY_JUNJA", "KEY_FINAL", "KEY_KANJI", "-", "KEY_ESCAPE", "KEY_CONVERT", "KEY_NONCONVERT",
		"KEY_ACCEPT", "KEY_MODECHANGE", "KEY_SPACE", "KEY_PRIOR", "KEY_NEXT", "KEY_END",
		"KEY_HOME", "KEY_LEFT", "KEY_UP", "KEY_RIGHT", "KEY_DOWN", "KEY_SELECT", "KEY_PRINT",
		"KEY_EXECUTE", "KEY_SNAPSHOT", "KEY_INSERT", "KEY_DELETE", "KEY_HELP", "KEY_KEY_0",
		"KEY_KEY_1", "KEY_KEY_2", "KEY_KEY_3", "KEY_KEY_4", "KEY_KEY_5",
		"KEY_KEY_6", "KEY_KEY_7", "KEY_KEY_8", "KEY_KEY_9", "-", "-", "-", "-",
		"-", "-", "-", "KEY_KEY_A", "KEY_KEY_B", "KEY_KEY_C", "KEY_KEY_D",
		"KEY_KEY_E", "KEY_KEY_F", "KEY_KEY_G", "KEY_KEY_H", "KEY_KEY_I",
		"KEY_KEY_J", "KEY_KEY_K", "KEY_KEY_L", "KEY_KEY_M", "KEY_KEY_N",
		"KEY_KEY_O", "KEY_KEY_P", "KEY_KEY_Q", "KEY_KEY_R", "KEY_KEY_S",
		"KEY_KEY_T", "KEY_KEY_U", "KEY_KEY_V", "KEY_KEY_W", "KEY_KEY_X",
		"KEY_KEY_Y", "KEY_KEY_Z", "KEY_LWIN", "KEY_RWIN", "KEY_APPS", "-",
		"KEY_SLEEP", "KEY_NUMPAD0", "KEY_NUMPAD1", "KEY_NUMPAD2", "KEY_NUMPAD3",
		"KEY_NUMPAD4", "KEY_NUMPAD5", "KEY_NUMPAD6", "KEY_NUMPAD7",
		"KEY_NUMPAD8", "KEY_NUMPAD9", "KEY_MULTIPLY", "KEY_ADD", "KEY_SEPERATOR",
		"KEY_SUBTRACT", "KEY_DECIMAL", "KEY_DIVIDE", "KEY_F1", "KEY_F2", "KEY_F3",
		"KEY_F4", "KEY_F5", "KEY_F6", "KEY_F7", "KEY_F8", "KEY_F9", "KEY_F10",
		"KEY_F11", "KEY_F12", "KEY_F13", "KEY_F14", "KEY_F15", "KEY_F16",
		"KEY_F17", "KEY_F18", "KEY_F19", "KEY_F20", "KEY_F21", "KEY_F22",
		"KEY_F23", "KEY_F24", "-", "-", "-", "-", "-", "-", "-", "-",
		"KEY_NUMLOCK", "KEY_SCROLL", "-", "-", "-", "-", "-", "-", "-", "-", "-",
		"-", "-", "-", "-", "-", "KEY_LSHIFT", "KEY_RSHIFT", "KEY_LCONTROL",
		"KEY_RCONTROL", "KEY_LMENU", "KEY_RMENU", "-", "-", "-", "-", "-",
		"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
		"-", "-", "KEY_PLUS", "KEY_COMMA", "KEY_MINUS", "KEY_PERIOD", "-", "-", "-", "-", "-",
		"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
		"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
		"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
		"-", "-", "-", "-", "-", "-", "-", "-", "KEY_ATTN", "KEY_CRSEL", "KEY_EXSEL",
		"KEY_EREOF", "KEY_PLAY", "KEY_ZOOM", "KEY_PA1", "KEY_OEM_CLEAR", "-" };

#define N_(text) text

static const char *KeyNamesLang[] =
	{ "-", N_("Left Button"), N_("Right Button"), N_("Cancel"), N_("Middle Button"), N_("X Button 1"),
			N_("X Button 2"), "-", N_("Back"), N_("Tab"), "-", "-", N_("Clear"), N_("Return"), "-",
			"-", N_("Shift"), N_("Control"), N_("Menu"), N_("Pause"), N_("Capital"), N_("Kana"), "-",
			N_("Junja"), N_("Final"), N_("Kanji"), "-", N_("Escape"), N_("Convert"), N_("Nonconvert"),
			N_("Accept"), N_("Mode Change"), N_("Space"), N_("Prior"), N_("Next"), N_("End"), N_("Home"),
			N_("Left"), N_("Up"), N_("Right"), N_("Down"), N_("Select"), N_("Print"), N_("Execute"),
			N_("Snapshot"), N_("Insert"), N_("Delete"), N_("Help"), "0", "1", "2", "3", "4", "5",
			"6", "7", "8", "9", "-", "-", "-", "-", "-", "-", "-", "A", "B", "C",
			"D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q",
			"R", "S", "T", "U", "V", "W", "X", "Y", "Z", N_("Left Windows"),
			N_("Right Windows"), N_("Apps"), "-", N_("Sleep"), N_("Numpad 0"), N_("Numpad 1"),
			N_("Numpad 2"), N_("Numpad 3"), N_("Numpad 4"), N_("Numpad 5"), N_("Numpad 6"), N_("Numpad 7"),
			N_("Numpad 8"), N_("Numpad 9"), N_("Numpad *"), N_("Numpad +"), N_("Numpad /"), N_("Numpad -"),
			"Numpad .", "Numpad /", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
			"F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18",
			"F19", "F20", "F21", "F22", "F23", "F24", "-", "-", "-", "-", "-", "-",
			"-", "-", N_("Num Lock"), N_("Scroll Lock"), "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", N_("Left Shift"), N_("Right Shift"),
			N_("Left Control"), N_("Right Control"), N_("Left Menu"), N_("Right Menu"), "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", N_("Plus"), N_("Comma"), N_("Minus"), N_("Period"), "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", N_("Attn"), N_("CrSel"),
			N_("ExSel"), N_("Erase OEF"), N_("Play"), N_("Zoom"), N_("PA1"), N_("OEM Clear"), "-" };

#undef N_

KeyPress::KeyPress() :
	Key(irr::KEY_KEY_CODES_COUNT),
	Char(L'\0')
{}

KeyPress::KeyPress(const char *name)
{
	if (strlen(name) > 4) {
		try {
			Key = keyname_to_keycode(name);
			m_name = name;
			if (strlen(name) > 8 && strncmp(name, "KEY_KEY_", 8) == 0) {
				int chars_read = mbtowc(&Char, name + 8, 1);
				assert (chars_read == 1 && "unexpected multibyte character");
			} else
				Char = L'\0';
			return;
		} catch (UnknownKeycode &e) {};
	} else {
		// see if we can set it up as a KEY_KEY_something
		m_name = "KEY_KEY_";
		m_name += name;
		try {
			Key = keyname_to_keycode(m_name.c_str());
			int chars_read = mbtowc(&Char, name, 1);
			assert (chars_read == 1 && "unexpected multibyte character");
			return;
		} catch (UnknownKeycode &e) {};
	}

	// it's not a (known) key, just take the first char and use that

	Key = irr::KEY_KEY_CODES_COUNT;

	int mbtowc_ret = mbtowc(&Char, name, 1);
	assert (mbtowc_ret == 1 && "unexpected multibyte character");
	m_name = name[0];
}

KeyPress::KeyPress(const irr::SEvent::SKeyInput &in, bool prefer_character)
{
	Key = in.Key;
	Char = in.Char;

	if(prefer_character){
		m_name.resize(MB_CUR_MAX+1, '\0');
		int written = wctomb(&m_name[0], Char);
		if(written > 0){
			infostream<<"KeyPress: Preferring character for "<<m_name<<std::endl;
			Key = irr::KEY_KEY_CODES_COUNT;
			return;
		}
	}

	if (valid_kcode(Key)) {
		m_name = KeyNames[Key];
	} else {
		m_name.resize(MB_CUR_MAX+1, '\0');
		int written = wctomb(&m_name[0], Char);
		if(written < 0){
			std::string hexstr = hex_encode((const char*)&Char, sizeof(Char));
			errorstream<<"KeyPress: Unexpected multibyte character "<<hexstr<<std::endl;
		}
	}
}

const char *KeyPress::sym() const
{
	if (Key && Key < irr::KEY_KEY_CODES_COUNT)
		return KeyNames[Key];
	else {
		return m_name.c_str();
	}
}

const char *KeyPress::name() const
{
	if (Key && Key < irr::KEY_KEY_CODES_COUNT)
		return KeyNamesLang[Key];
	else {
		return m_name.c_str();
	}
}

const KeyPress EscapeKey("KEY_ESCAPE");
const KeyPress CancelKey("KEY_CANCEL");
const KeyPress NumberKey[] = {
	KeyPress("KEY_KEY_0"), KeyPress("KEY_KEY_1"), KeyPress("KEY_KEY_2"),
	KeyPress("KEY_KEY_3"), KeyPress("KEY_KEY_4"), KeyPress("KEY_KEY_5"),
	KeyPress("KEY_KEY_6"), KeyPress("KEY_KEY_7"), KeyPress("KEY_KEY_8"),
	KeyPress("KEY_KEY_9")};

/*
	Key config
*/

// A simple cache for quicker lookup
std::map<std::string, KeyPress> g_key_setting_cache;

KeyPress getKeySetting(const char *settingname)
{
	std::map<std::string, KeyPress>::iterator n;
	n = g_key_setting_cache.find(settingname);
	if(n != g_key_setting_cache.end())
		return n->second;
	g_key_setting_cache[settingname] = g_settings->get(settingname).c_str();
	return g_key_setting_cache.find(settingname)->second;
}

void clearKeyCache()
{
	g_key_setting_cache.clear();
}
