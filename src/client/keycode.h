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
#include <Keycodes.h>
#include <IEventReceiver.h>
#include <string>

/** Represents a scancode on the keyboard. It only contains characters common to
  * Irrlicht keycodes and SDL scancodes, and NOT characters like ~ or ? that have
  * no physical key, but only exist when combined with shift.
  */
class KeyPress
{
private:
	irr::EKEY_CODE m_key;

	void fromName(const std::string &name);

public:
	KeyPress() : m_key(irr::KEY_KEY_CODES_COUNT) {}
	KeyPress(irr::EKEY_CODE key) : m_key(key) {}
	KeyPress(const irr::SEvent::SKeyInput &input) : m_key(input.Key) {}

	KeyPress(const std::string &name) { fromName(name); }
	KeyPress(const char *name) { fromName(name); }

	bool operator==(const KeyPress &o) const { return m_key == o.m_key; }

	irr::EKEY_CODE getCode() const { return m_key; }
	std::string getName() const;
};

extern const KeyPress EscapeKey;
extern const KeyPress CancelKey;

// Key configuration getter
KeyPress getKeySetting(const char *settingname);

// Clear fast lookup cache
void clearKeyCache();
