/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

/*
	All kinds of stuff that needs to be exposed from main.cpp
*/
#include "modalMenu.h"
#include <cassert>
#include <list>

class IGameCallback
{
public:
	virtual void exitToOS() = 0;
	virtual void keyConfig() = 0;
	virtual void disconnect() = 0;
	virtual void changePassword() = 0;
	virtual void changeVolume() = 0;
	virtual void showOpenURLDialog(const std::string &url) = 0;
	virtual void signalKeyConfigChange() = 0;
};

extern gui::IGUIEnvironment *guienv;
extern gui::IGUIStaticText *guiroot;

// Handler for the modal menus

class MainMenuManager : public IMenuManager
{
public:
	virtual void createdMenu(gui::IGUIElement *menu)
	{
		for (gui::IGUIElement *e : m_stack) {
			if (e == menu)
				return;
		}

		if(!m_stack.empty())
			m_stack.back()->setVisible(false);

		m_stack.push_back(menu);
		guienv->setFocus(m_stack.back());
	}

	virtual void deletingMenu(gui::IGUIElement *menu)
	{
		// Remove all entries if there are duplicates
		m_stack.remove(menu);

		if(!m_stack.empty()) {
			m_stack.back()->setVisible(true);
			guienv->setFocus(m_stack.back());
		}
	}

	// Returns true to prevent further processing
	virtual bool preprocessEvent(const SEvent& event)
	{
		if (m_stack.empty())
			return false;
		GUIModalMenu *mm = dynamic_cast<GUIModalMenu*>(m_stack.back());
		return mm && mm->preprocessEvent(event);
	}

	size_t menuCount() const
	{
		return m_stack.size();
	}

	bool pausesGame()
	{
		for (gui::IGUIElement *i : m_stack) {
			GUIModalMenu *mm = dynamic_cast<GUIModalMenu*>(i);
			if (mm && mm->pausesGame())
				return true;
		}
		return false;
	}

	// FIXME: why isn't this private?
	std::list<gui::IGUIElement*> m_stack;
};

extern MainMenuManager g_menumgr;

static inline bool isMenuActive()
{
	return g_menumgr.menuCount() != 0;
}

class MainGameCallback : public IGameCallback
{
public:
	MainGameCallback() = default;
	virtual ~MainGameCallback() = default;

	void exitToOS() override
	{
		shutdown_requested = true;
	}

	void disconnect() override
	{
		disconnect_requested = true;
	}

	void changePassword() override
	{
		changepassword_requested = true;
	}

	void changeVolume() override
	{
		changevolume_requested = true;
	}

	void keyConfig() override
	{
		keyconfig_requested = true;
	}

	void signalKeyConfigChange() override
	{
		keyconfig_changed = true;
	}

	void showOpenURLDialog(const std::string &url) override
	{
		show_open_url_dialog = url;
	}

	bool disconnect_requested = false;
	bool changepassword_requested = false;
	bool changevolume_requested = false;
	bool keyconfig_requested = false;
	bool shutdown_requested = false;
	bool keyconfig_changed = false;
	std::string show_open_url_dialog = "";
};

extern MainGameCallback *g_gamecallback;
