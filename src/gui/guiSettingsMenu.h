/*
Part of Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>
Copyright (C) 2013 RealBadAngel, Maciej Kasatkin <mk@realbadangel.pl>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"
#include "client/gameui.h"
#include <string>

class ISimpleTextureSource;
class ISoundManager;
class RenderingEngine;
class Client;
struct GuiFormspecHandler;

struct SettingsPageContent
{
	std::string name = "";
	bool is_heading = false;
};


struct SettingsPage
{
	std::string id;
	std::string title;
	std::string section;
	std::vector<SettingsPageContent> content;	
};

class GUISettingsMenu : public GUIModalMenu
{
public:
	GUISettingsMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IMenuManager *menumgr, ISimpleTextureSource *tsrc, InputHandler *input,
			RenderingEngine *engine, Client *client, ISoundManager *sound_manager);
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);

	void drawMenu();
	
	void addPage(const SettingsPage& page);

	bool OnEvent(const SEvent& event);

	bool pausesGame() { return true; }

protected:
	std::wstring getLabelByID(s32 id) { return L""; }
	std::string getNameByID(s32 id) { return ""; }

private:
	std::string build_page_components(SettingsPage *curr_page);
	
	ISimpleTextureSource *m_tsrc;
	std::vector<SettingsPage> m_filtered_pages;
	SettingsPage *m_curr_page;
	GameUI *m_game_ui;
	GuiFormspecHandler *m_fs_handler;
	InputHandler *m_input;
	RenderingEngine *m_rendering_engine;
	Client *m_client;
	ISoundManager *m_sound_manager;
};
