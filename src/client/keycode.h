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

#pragma once

#include "irrlichttypes.h"
#include "Keycodes.h"
#include <IEventReceiver.h>
#include <string>

class KeyPress;
namespace std
{
	template <> struct hash<KeyPress>;
}

/* A key press, consisting of either an Irrlicht keycode
   or an actual char */

class KeyPress
{
public:
	friend struct std::hash<KeyPress>;

	KeyPress() = default;

	KeyPress(const char *name);

	KeyPress(const irr::SEvent::SKeyInput &in, bool prefer_character = false);

	bool operator==(const KeyPress &o) const
	{
		return (Char > 0 && Char == o.Char) || (valid_kcode(Key) && Key == o.Key);
	}

	const char *sym() const;
	const char *name() const;

protected:
	static bool valid_kcode(irr::EKEY_CODE k)
	{
		return k > 0 && k < irr::KEY_KEY_CODES_COUNT;
	}

	irr::EKEY_CODE Key = irr::KEY_KEY_CODES_COUNT;
	wchar_t Char = L'\0';
	std::string m_name = "";
};

namespace std
{
	template <> struct hash<KeyPress>
	{
		size_t operator()(const KeyPress &key) const
		{
			return key.Key;
		}
	};
}

extern const KeyPress EscapeKey;
extern const KeyPress CancelKey;

// Key configuration getter
KeyPress getKeySetting(const char *settingname);

// Clear fast lookup cache
void clearKeyCache();

irr::EKEY_CODE keyname_to_keycode(const char *name);
