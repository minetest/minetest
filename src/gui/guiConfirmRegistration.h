/*
Minetest
Copyright (C) 2018 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>

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

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"
#include <string>

class Client;

class GUIConfirmRegistration : public GUIModalMenu
{
public:
	GUIConfirmRegistration(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
			s32 id, IMenuManager *menumgr, Client *client,
			const std::string &playername, const std::string &password,
			bool *aborted);
	~GUIConfirmRegistration();

	void removeChildren();
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);

	void drawMenu();

	void closeMenu(bool goNext);

	void acceptInput();

	bool processInput();

	bool OnEvent(const SEvent &event);
#ifdef __ANDROID__
	bool getAndroidUIInput();
#endif

private:
	std::wstring getLabelByID(s32 id) { return L""; }
	std::string getNameByID(s32 id) { return "password"; }

	Client *m_client = nullptr;
	const std::string &m_playername;
	const std::string &m_password;
	bool *m_aborted = nullptr;
	std::wstring m_pass_confirm = L"";
};
