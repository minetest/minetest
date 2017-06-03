/*
 Minetest
 Copyright (C) 2013 sapier

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

#ifndef GUIFILESELECTMENU_H_
#define GUIFILESELECTMENU_H_

#include <string>

#include "modalMenu.h"
#include "IGUIFileOpenDialog.h"
#include "guiFormSpecMenu.h" //required because of TextDest only !!!

class GUIFileSelectMenu : public GUIModalMenu
{
public:
	GUIFileSelectMenu(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
			IMenuManager *menumgr, const std::string &title,
			const std::string &formid);
	~GUIFileSelectMenu();

	void removeChildren();

	/*
	 Remove and re-add (or reposition) stuff
	 */
	void regenerateGui(v2u32 screensize);

	void drawMenu();

	bool OnEvent(const SEvent &event);

	void setTextDest(TextDest *dest) { m_text_dst = dest; }

private:
	void acceptInput();

	std::wstring m_title;
	bool m_accepted;

	gui::IGUIFileOpenDialog *m_fileOpenDialog;

	TextDest *m_text_dst;

	std::string m_formname;
};

#endif /* GUIFILESELECTMENU_H_ */
