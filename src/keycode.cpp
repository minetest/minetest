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

class UnknownKeycode : public BaseException
{
public:
	UnknownKeycode(const char *s) :
		BaseException(s) {};
};

struct table_key {
	const char *Name;
	irr::EKEY_CODE Key;
	wchar_t Char; // may be L'\0' which means no char assigned
	const char *LangName; // NULL means it doesn't have a human description
};

#define DEFINEKEY1(x, lang) { #x, irr::x, L'\0', lang }, // Irrlicht key without char
#define DEFINEKEY2(x, ch, lang) { #x, irr::x, ch, lang }, // Irrlicht key with char
#define DEFINEKEY3(ch) { "KEY_KEY_" TOSTRING(ch), irr::KEY_KEY_ ## ch, (wchar_t) *TOSTRING(ch), TOSTRING(ch) }, // single Irrlicht key (e.g. KEY_KEY_X)

#define N_(text) text

static const struct table_key table[] = {
	DEFINEKEY1(KEY_LBUTTON, N_("Left Button"))
	DEFINEKEY1(KEY_RBUTTON, N_("Right Button"))
	DEFINEKEY1(KEY_CANCEL, N_("Cancel"))
	DEFINEKEY1(KEY_MBUTTON, N_("Middle Button"))
	DEFINEKEY1(KEY_XBUTTON1, N_("X Button 1"))
	DEFINEKEY1(KEY_XBUTTON2, N_("X Button 2"))
	DEFINEKEY1(KEY_BACK, N_("Back"))
	DEFINEKEY1(KEY_TAB, N_("Tab"))
	DEFINEKEY1(KEY_CLEAR, N_("Clear"))
	DEFINEKEY1(KEY_RETURN, N_("Return"))
	DEFINEKEY1(KEY_SHIFT, N_("Shift"))
	DEFINEKEY1(KEY_CONTROL, N_("Control"))
	DEFINEKEY1(KEY_MENU, N_("Menu"))
	DEFINEKEY1(KEY_PAUSE, N_("Pause"))
	DEFINEKEY1(KEY_CAPITAL, N_("Capital"))
	DEFINEKEY1(KEY_KANA, "Kana") // the next few are not worth translating
	DEFINEKEY1(KEY_HANGUEL, "Hangul")
	DEFINEKEY1(KEY_HANGUL, "Hangul")
	DEFINEKEY1(KEY_JUNJA, "Junja")
	DEFINEKEY1(KEY_FINAL, "Final")
	DEFINEKEY1(KEY_KANJI, "Kanji")
	DEFINEKEY1(KEY_HANJA, "Hanja")
	DEFINEKEY1(KEY_ESCAPE, N_("Escape"))
	DEFINEKEY1(KEY_CONVERT, N_("Convert"))
	DEFINEKEY1(KEY_NONCONVERT, N_("Nonconvert"))
	DEFINEKEY1(KEY_ACCEPT, N_("Accept"))
	DEFINEKEY1(KEY_MODECHANGE, N_("Mode Change"))
	DEFINEKEY1(KEY_SPACE, N_("Space"))
	DEFINEKEY1(KEY_PRIOR, N_("Prior"))
	DEFINEKEY1(KEY_NEXT, N_("Next"))
	DEFINEKEY1(KEY_END, N_("End"))
	DEFINEKEY1(KEY_HOME, N_("Home"))
	DEFINEKEY1(KEY_LEFT, N_("Left"))
	DEFINEKEY1(KEY_UP, N_("Up"))
	DEFINEKEY1(KEY_RIGHT, N_("Right"))
	DEFINEKEY1(KEY_DOWN, N_("Down"))
	DEFINEKEY1(KEY_SELECT, N_("Select"))
	DEFINEKEY1(KEY_PRINT, N_("Print"))
	DEFINEKEY1(KEY_EXECUT, N_("Execute"))
	DEFINEKEY1(KEY_SNAPSHOT, N_("Snapshot"))
	DEFINEKEY1(KEY_INSERT, N_("Insert"))
	DEFINEKEY1(KEY_DELETE, N_("Delete"))
	DEFINEKEY1(KEY_HELP, N_("Help"))
	DEFINEKEY3(0)
	DEFINEKEY3(1)
	DEFINEKEY3(2)
	DEFINEKEY3(3)
	DEFINEKEY3(4)
	DEFINEKEY3(5)
	DEFINEKEY3(6)
	DEFINEKEY3(7)
	DEFINEKEY3(8)
	DEFINEKEY3(9)
	DEFINEKEY3(A)
	DEFINEKEY3(B)
	DEFINEKEY3(C)
	DEFINEKEY3(D)
	DEFINEKEY3(E)
	DEFINEKEY3(F)
	DEFINEKEY3(G)
	DEFINEKEY3(H)
	DEFINEKEY3(I)
	DEFINEKEY3(J)
	DEFINEKEY3(K)
	DEFINEKEY3(L)
	DEFINEKEY3(M)
	DEFINEKEY3(N)
	DEFINEKEY3(O)
	DEFINEKEY3(P)
	DEFINEKEY3(Q)
	DEFINEKEY3(R)
	DEFINEKEY3(S)
	DEFINEKEY3(T)
	DEFINEKEY3(U)
	DEFINEKEY3(V)
	DEFINEKEY3(W)
	DEFINEKEY3(X)
	DEFINEKEY3(Y)
	DEFINEKEY3(Z)
	DEFINEKEY1(KEY_LWIN, N_("Left Windows"))
	DEFINEKEY1(KEY_RWIN, N_("Right Windows"))
	DEFINEKEY1(KEY_APPS, N_("Apps"))
	DEFINEKEY1(KEY_SLEEP, N_("Sleep"))
	DEFINEKEY2(KEY_NUMPAD0, L'0', N_("Numpad 0"))
	DEFINEKEY2(KEY_NUMPAD1, L'1', N_("Numpad 1"))
	DEFINEKEY2(KEY_NUMPAD2, L'2', N_("Numpad 2"))
	DEFINEKEY2(KEY_NUMPAD3, L'3', N_("Numpad 3"))
	DEFINEKEY2(KEY_NUMPAD4, L'4', N_("Numpad 4"))
	DEFINEKEY2(KEY_NUMPAD5, L'5', N_("Numpad 5"))
	DEFINEKEY2(KEY_NUMPAD6, L'6', N_("Numpad 6"))
	DEFINEKEY2(KEY_NUMPAD7, L'7', N_("Numpad 7"))
	DEFINEKEY2(KEY_NUMPAD8, L'8', N_("Numpad 8"))
	DEFINEKEY2(KEY_NUMPAD9, L'9', N_("Numpad 9"))
	DEFINEKEY1(KEY_MULTIPLY, N_("Numpad *"))
	DEFINEKEY1(KEY_ADD, N_("Numpad +"))
	DEFINEKEY1(KEY_SEPARATOR, N_("Numpad ."))
	DEFINEKEY1(KEY_SUBTRACT, N_("Numpad -"))
	DEFINEKEY1(KEY_DECIMAL, NULL)
	DEFINEKEY1(KEY_DIVIDE, N_("Numpad /"))
	DEFINEKEY1(KEY_F1, "F1")
	DEFINEKEY1(KEY_F2, "F2")
	DEFINEKEY1(KEY_F3, "F3")
	DEFINEKEY1(KEY_F4, "F4")
	DEFINEKEY1(KEY_F5, "F5")
	DEFINEKEY1(KEY_F6, "F6")
	DEFINEKEY1(KEY_F7, "F7")
	DEFINEKEY1(KEY_F8, "F8")
	DEFINEKEY1(KEY_F9, "F9")
	DEFINEKEY1(KEY_F10, "F10")
	DEFINEKEY1(KEY_F11, "F11")
	DEFINEKEY1(KEY_F12, "F12")
	DEFINEKEY1(KEY_F13, "F13")
	DEFINEKEY1(KEY_F14, "F14")
	DEFINEKEY1(KEY_F15, "F15")
	DEFINEKEY1(KEY_F16, "F16")
	DEFINEKEY1(KEY_F17, "F17")
	DEFINEKEY1(KEY_F18, "F18")
	DEFINEKEY1(KEY_F19, "F19")
	DEFINEKEY1(KEY_F20, "F19")
	DEFINEKEY1(KEY_F21, "F20")
	DEFINEKEY1(KEY_F22, "F21")
	DEFINEKEY1(KEY_F23, "F23")
	DEFINEKEY1(KEY_F24, "F24")
	DEFINEKEY1(KEY_NUMLOCK, N_("Num Lock"))
	DEFINEKEY1(KEY_SCROLL, N_("Scroll Lock"))
	DEFINEKEY1(KEY_LSHIFT, N_("Left Shift"))
	DEFINEKEY1(KEY_RSHIFT, N_("Right Shift"))
	DEFINEKEY1(KEY_LCONTROL, N_("Left Control"))
	DEFINEKEY1(KEY_RCONTROL, N_("Right Control"))
	DEFINEKEY1(KEY_LMENU, N_("Left Menu"))
	DEFINEKEY1(KEY_RMENU, N_("Right Menu"))
	DEFINEKEY2(KEY_OEM_1, L';', N_("Semicolon"))
	DEFINEKEY2(KEY_PLUS, L'+', N_("Plus"))
	DEFINEKEY2(KEY_COMMA, L',', N_("Comma"))
	DEFINEKEY2(KEY_MINUS, L'-', N_("Minus"))
	DEFINEKEY2(KEY_PERIOD, L'.', N_("Period"))
	DEFINEKEY2(KEY_OEM_2, L'/', N_("Slash"))
	DEFINEKEY2(KEY_OEM_3, L'`', N_("Grave accent"))
	DEFINEKEY2(KEY_OEM_4, L'[', "[")
	DEFINEKEY2(KEY_OEM_5, L'\\', N_("Backslash"))
	DEFINEKEY2(KEY_OEM_6, L']', "]")
	DEFINEKEY2(KEY_OEM_7, L'\'', N_("Apostrophe"))
	DEFINEKEY2(KEY_OEM_102, L'<', N_("Less"))
	DEFINEKEY1(KEY_ATTN, N_("Attn"))
	DEFINEKEY1(KEY_CRSEL, "CrSel") // both not worth translating
	DEFINEKEY1(KEY_EXSEL, "ExSel")
	DEFINEKEY1(KEY_EREOF, N_("Erase OEF"))
	DEFINEKEY1(KEY_PLAY, N_("Play"))
	DEFINEKEY1(KEY_ZOOM, N_("Zoom"))
	DEFINEKEY1(KEY_PA1, "PA1") // not worth translating
	DEFINEKEY1(KEY_OEM_CLEAR, N_("OEM Clear"))
};

#undef N_

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

struct table_key lookup_keyname(const char *name)
{
	for (u16 i = 0; i < ARRAYSIZE(table); i++) {
		if (strcmp(table[i].Name, name) == 0)
			return table[i];
	}

	throw UnknownKeycode(name);
}

struct table_key lookup_keykey(irr::EKEY_CODE key)
{
	for (u16 i = 0; i < ARRAYSIZE(table); i++) {
		if (table[i].Key == key)
			return table[i];
	}

	std::ostringstream os;
	os << "<Keycode " << (int) key << ">";
	throw UnknownKeycode(os.str().c_str());
}

struct table_key lookup_keychar(wchar_t Char)
{
	for (u16 i = 0; i < ARRAYSIZE(table); i++) {
		if (table[i].Char == Char)
			return table[i];
	}

	std::ostringstream os;
	os << "<Char " << hex_encode((char*) &Char, sizeof(wchar_t)) << ">";
	throw UnknownKeycode(os.str().c_str());
}

KeyPress::KeyPress() :
	Key(irr::KEY_KEY_CODES_COUNT),
	Char(L'\0')
{}

KeyPress::KeyPress(const char *name)
{
	if (strlen(name) == 0) {
		Key = irr::KEY_KEY_CODES_COUNT;
		Char = L'\0';
		return;
	} else if (strlen(name) <= 4) {
		// Lookup by resulting character
		int chars_read = mbtowc(&Char, name, 1);
		FATAL_ERROR_IF(chars_read != 1, "Unexpected multibyte character");
		try {
			struct table_key k = lookup_keychar(Char);
			m_name = k.Name;
			Key = k.Key;
			return;
		} catch (UnknownKeycode &e) {};
	} else {
		// Lookup by name
		m_name = name;
		try {
			struct table_key k = lookup_keyname(name);
			Key = k.Key;
			Char = k.Char;
			return;
		} catch (UnknownKeycode &e) {};
	}

	// It's not a (known) key, just take the first char and use that
	Key = irr::KEY_KEY_CODES_COUNT;
	int chars_read = mbtowc(&Char, name, 1);
	FATAL_ERROR_IF(chars_read != 1, "Unexpected multibyte character");
	m_name = "UNKNOWN_" + hex_encode((char*) &Char, sizeof(wchar_t));
	infostream << "KeyPress: Unknown key '" << name << "', falling back to first char";
}

KeyPress::KeyPress(const irr::SEvent::SKeyInput &in, bool prefer_character)
{
	if (prefer_character)
		Key = irr::KEY_KEY_CODES_COUNT;
	else
		Key = in.Key;
	Char = in.Char;

	if (valid_kcode(Key))
		m_name = lookup_keykey(Key).Name;
	else
		m_name = "UNKNOWN_" + hex_encode((char*) &Char, sizeof(wchar_t));
}

const char *KeyPress::sym() const
{
	return m_name.c_str();
}

const char *KeyPress::name() const
{
	if (!valid_kcode(Key))
		return "";
	const char *ret = lookup_keykey(Key).LangName;
	return ret ? ret : "<Unnamed key>";
}

const KeyPress EscapeKey("KEY_ESCAPE");
const KeyPress CancelKey("KEY_CANCEL");
const KeyPress NumberKey[] = {
	KeyPress("KEY_KEY_0"), KeyPress("KEY_KEY_1"), KeyPress("KEY_KEY_2"),
	KeyPress("KEY_KEY_3"), KeyPress("KEY_KEY_4"), KeyPress("KEY_KEY_5"),
	KeyPress("KEY_KEY_6"), KeyPress("KEY_KEY_7"), KeyPress("KEY_KEY_8"),
	KeyPress("KEY_KEY_9")
};

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
