// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "irrlichttypes.h"
#include <Keycodes.h>
#include <IEventReceiver.h>
#include <string>
#include <unordered_map>

/* A key press, consisting of a scancode or a keycode */
class KeyPress
{
public:
	KeyPress() {};

	KeyPress(const std::string_view &name);

	KeyPress(const irr::SEvent::SKeyInput &in) :
		scancode(in.SystemKeyCode) {};

	std::string sym() const;
	std::string name() const;

	bool operator==(const KeyPress &o) const {
		return scancode == o.scancode;
	}

	operator bool() const {
		return scancode != 0;
	}

	static const KeyPress &getSpecialKey(const std::string &name);

private:
	u32 scancode = 0;

	static std::unordered_map<std::string, KeyPress> specialKeyCache;
};

// Global defines for convenience
// This implementation defers creation of the objects to make sure that the
// IrrlichtDevice is initialized.
#define EscapeKey KeyPress::getSpecialKey("KEY_ESCAPE")
#define LMBKey KeyPress::getSpecialKey("KEY_LBUTTON")
#define MMBKey KeyPress::getSpecialKey("KEY_MBUTTON") // Middle Mouse Button
#define RMBKey KeyPress::getSpecialKey("KEY_RBUTTON")

// Key configuration getter
const KeyPress &getKeySetting(const char *settingname);

// Clear fast lookup cache
void clearKeyCache();

irr::EKEY_CODE keyname_to_keycode(const char *name);
