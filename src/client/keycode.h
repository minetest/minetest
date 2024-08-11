// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "exceptions.h"
#include "irrlichttypes.h"
#include <Keycodes.h>
#include <IEventReceiver.h>
#include <string>

class UnknownKeycode : public BaseException
{
public:
	UnknownKeycode(const std::string &s) :
		BaseException(s) {};
};

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

private:
	u32 scancode = 0;
};

// Global defines for convenience

extern const KeyPress EscapeKey;

extern const KeyPress LMBKey;
extern const KeyPress MMBKey; // Middle Mouse Button
extern const KeyPress RMBKey;

// Key configuration getter
const KeyPress &getKeySetting(const char *settingname);

// Clear fast lookup cache
void clearKeyCache();

irr::EKEY_CODE keyname_to_keycode(const char *name);
