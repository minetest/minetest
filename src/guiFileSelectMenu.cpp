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

#include "guiFileSelectMenu.h"
#include "util/string.h"
#include <locale.h>

GUIFileSelectMenu::GUIFileSelectMenu(gui::IGUIEnvironment* env,
				gui::IGUIElement* parent, s32 id, IMenuManager *menumgr,
				std::string title, std::string formname) :
GUIModalMenu(env, parent, id, menumgr)
{
	m_title = narrow_to_wide(title);
	m_parent = parent;
	m_formname = formname;
	m_text_dst = 0;
	m_accepted = false;
}

GUIFileSelectMenu::~GUIFileSelectMenu()
{
	removeChildren();
}

void GUIFileSelectMenu::removeChildren()
{
	const core::list<gui::IGUIElement*> &children = getChildren();
	core::list<gui::IGUIElement*> children_copy;
	for (core::list<gui::IGUIElement*>::ConstIterator i = children.begin(); i
		 != children.end(); i++)
	{
		children_copy.push_back(*i);
	}
	for (core::list<gui::IGUIElement*>::Iterator i = children_copy.begin(); i
		 != children_copy.end(); i++)
	{
		(*i)->remove();
	}
}

void GUIFileSelectMenu::regenerateGui(v2u32 screensize)
{
	removeChildren();
	m_fileOpenDialog = 0;

	core::dimension2du size(600,400);
	core::rect < s32 > rect(0,0,screensize.X,screensize.Y);

	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	m_fileOpenDialog =
			Environment->addFileOpenDialog(m_title.c_str(),false,this,-1);

	core::position2di pos = core::position2di(screensize.X/2 - size.Width/2,screensize.Y/2 -size.Height/2);
	m_fileOpenDialog->setRelativePosition(pos);
	m_fileOpenDialog->setMinSize(size);
}

void GUIFileSelectMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;

	gui::IGUIElement::draw();
}

void GUIFileSelectMenu::acceptInput() {
	if ((m_text_dst != 0) && (this->m_formname != "")){
		std::map<std::string, std::string> fields;

		if (m_accepted)
			fields[m_formname + "_accepted"] = wide_to_narrow(m_fileOpenDialog->getFileName());
		else
			fields[m_formname + "_canceled"] = m_formname;

		this->m_text_dst->gotText(fields);
	}
}

bool GUIFileSelectMenu::OnEvent(const SEvent& event)
{

	if (event.EventType == irr::EET_GUI_EVENT) {
		switch (event.GUIEvent.EventType) {
			case gui::EGET_ELEMENT_CLOSED:
			case gui::EGET_FILE_CHOOSE_DIALOG_CANCELLED:
				m_accepted=false;
				acceptInput();
				quitMenu();
				return true;
				break;

			case gui::EGET_DIRECTORY_SELECTED:
			case gui::EGET_FILE_SELECTED:
				m_accepted=true;
				acceptInput();
				quitMenu();
				return true;
				break;

			default:
				//ignore this event
				break;
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}
