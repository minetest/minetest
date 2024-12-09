// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>
// Copyright (C) 2013 teddydestodes <derkomtur@schattengang.net>

#pragma once

#include "guiFormSpecMenu.h"
#include "mainmenumanager.h"
#include "client/client.h"
#include "client/keycode.h"
#include <string>
#include <unordered_map>

class GUIKeyChangeMenu : public GUIFormSpecMenu
{
	using super = GUIFormSpecMenu;

public:
	// We can't use Client* in the mainmenu
	GUIKeyChangeMenu(Client * client, gui::IGUIEnvironment *guienv, JoystickController *joystick,
			ISimpleTextureSource *tsrc, ISoundManager *sound_manager,
			const std::string &formspec_prepend = "") :
		super(joystick, guiroot, -1, &g_menumgr, client, guienv, tsrc,
				sound_manager, nullptr, nullptr, formspec_prepend),
		has_client(client != nullptr), guienv(guienv)
		{
			updateFormSource();
			setFormspecHandler();
		}

	/*
	 Remove and re-add (or reposition) stuff
	 */
	void regenerateGui(v2u32 screensize) {
		super::regenerateGui(screensize);
		if (!active_key.empty())
			guienv->setFocus(this);
	}

	void saveSettings();

	bool OnEvent(const SEvent &event);

	bool pausesGame() { return true; }

private:
	class KeyChangeFormspecHandler : public TextDest
	{
	public:
		const std::string formname = "MT_KEY_CHANGE_MENU";
		KeyChangeFormspecHandler(GUIKeyChangeMenu *form) :
			form(form) {};
		void gotText(const StringMap &fields);
	private:
		GUIKeyChangeMenu *form;
	};

	void updateFormSource(const std::string &message = "");
	void setFormspecHandler()
	{
		// The formspec handler is deleted by guiFormSpecMenu
		setTextDest(new KeyChangeFormspecHandler(this));
	}

	const KeyPress &getKeySetting(const std::string &name) const
	{
		const auto &found = keymap.find(name);
		if (found != keymap.end())
			return found->second;
		return ::getKeySetting(name.c_str());
	}

	bool getControlOption(const std::string &name) const
	{
		const auto &found = control_options.find(name);
		if (found != control_options.end())
			return found->second;
		return g_settings->getBool(name);
	}

	std::string getTexture(const std::string &name) const;

	std::unordered_map<std::string, KeyPress> keymap;
	std::unordered_map<std::string, bool> control_options;
	std::string active_key;
	bool has_client;
	gui::IGUIEnvironment *guienv;
	float scroll_position = 0;
};
