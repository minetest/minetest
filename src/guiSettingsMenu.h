/*
Copyright (C) eh, you figure it out

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

#ifndef GUISETTINGSMENU_HEADER
#define GUISETTINGSMENU_HEADER

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"
#include "client.h"
#include <string>

class IGameCallback;

class GUISettingsMenu : public GUIModalMenu
{
public:
	GUISettingsMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IGameCallback *gamecallback,
			IMenuManager *menumgr);
	~GUISettingsMenu();
	
	void removeChildren();
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);

	void drawMenu();

	void readInput();
	void acceptInput();

	bool OnEvent(const SEvent& event);

private:
	IGameCallback *m_gamecallback;
	v2s32 m_topleft_client;
	v2s32 m_size_client;
	v2s32 m_topleft_server;
	v2s32 m_size_server;
};

#endif
