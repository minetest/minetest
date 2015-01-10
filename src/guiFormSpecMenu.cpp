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

#include "guiFormSpecMenu.h"
#include "guiFormSpecMenu_v1.h"
#include "guiFormSpecMenu_v2.h"

/******************************************************************************/
std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> tokens;

	std::string current = "";
	bool last_was_escape = false;
	for(unsigned int i=0; i < s.size(); i++) {
		if (last_was_escape) {
			current += '\\';
			current += s.c_str()[i];
			last_was_escape = false;
		}
		else {
			if (s.c_str()[i] == delim) {
				tokens.push_back(current);
				current = "";
				last_was_escape = false;
			}
			else if (s.c_str()[i] == '\\'){
				last_was_escape = true;
			}
			else {
				current += s.c_str()[i];
				last_was_escape = false;
			}
		}
	}
	//push last element
	tokens.push_back(current);

	return tokens;
}

/******************************************************************************/
bool isChild(gui::IGUIElement * tocheck, gui::IGUIElement * parent)
{
	while(tocheck != NULL) {
		if (tocheck == parent) {
			return true;
		}
		tocheck = tocheck->getParent();
	}
	return false;
}

/******************************************************************************/
guiFormSpecMenuGeneric::guiFormSpecMenuGeneric(irr::IrrlichtDevice* dev,
	gui::IGUIElement* parent, s32 id, IMenuManager *menumgr,
	InventoryManager *invmgr, IGameDef *gamedef,
	ISimpleTextureSource *tsrc, IFormSource* fsrc, TextDest* tdst,
	Client* client) :
	m_current_interpreter(FS_INTERPRETER_V1),
	m_Interpreter_v1(new guiFormSpecMenu_v1(dev, parent, id, menumgr,invmgr, gamedef, tsrc, fsrc, tdst, client))
	//m_Interpreter_v2(new guiFormSpecMenu_v2(dev, parent, id, menumgr,invmgr, gamedef, tsrc, fsrc, tdst, client))
{

}

/******************************************************************************/
guiFormSpecMenuGeneric::~guiFormSpecMenuGeneric()
{
	m_Interpreter_v1->drop();
	//m_Interpreter_v2->drop();
}

/******************************************************************************/
void guiFormSpecMenuGeneric::setFormSpec(const std::string &formspec_string,
		InventoryLocation inventory_location)
{
	// TODO first check which formspec version is provided

	m_Interpreter_v1->setFormSpec(formspec_string, inventory_location);
	//m_Interpreter_v2->setFormSpec(formspec_string, inventory_location);
}

/******************************************************************************/
void guiFormSpecMenuGeneric::setFormSource(IFormSource *form_src)
{
	// TODO first check which formspec version is provided

	m_Interpreter_v1->setFormSource(form_src);
	//m_Interpreter_v2->setFormSource(form_src);
}

/******************************************************************************/
void guiFormSpecMenuGeneric::setTextDest(TextDest *text_dst)
{
	m_Interpreter_v1->setTextDest(text_dst);
	//m_Interpreter_v2->setTextDest(text_dst);
}

/******************************************************************************/
void guiFormSpecMenuGeneric::lockSize(bool lock,v2u32 basescreensize)
{
	if (m_current_interpreter == FS_INTERPRETER_V1)
		return m_Interpreter_v1->lockSize(lock, basescreensize);

	if (m_current_interpreter == FS_INTERPRETER_V2)
		return m_Interpreter_v2->lockSize(lock, basescreensize);
}

/******************************************************************************/
void guiFormSpecMenuGeneric::setDoPause(bool new_value)
{
	m_Interpreter_v1->setDoPause(new_value);
	//m_Interpreter_v2->setDoPause(new_value);
}

/******************************************************************************/
void guiFormSpecMenuGeneric::quitMenu()
{
	m_Interpreter_v1->quitMenu();
	//m_Interpreter_v2->quitMenu();
}

/******************************************************************************/
gui::IGUIElement* guiFormSpecMenuGeneric::getGUIElement()
{
	if (m_current_interpreter == FS_INTERPRETER_V1)
		return m_Interpreter_v1;

	if (m_current_interpreter == FS_INTERPRETER_V2)
		return m_Interpreter_v2;

	assert("guiFormSpecMenuGeneric::getGUIElement() OOps we don't have a valid current interpreter" == 0);
	return NULL;
}

/******************************************************************************/
GUITable* guiFormSpecMenuGeneric::getTable(std::wstring tablename)
{
	if (m_current_interpreter == FS_INTERPRETER_V1)
		return m_Interpreter_v1->getTable(tablename);

	if (m_current_interpreter == FS_INTERPRETER_V2)
		return m_Interpreter_v2->getTable(tablename);

	assert("guiFormSpecMenuGeneric::getTable() OOps we don't have a valid current interpreter" == 0);
}

/******************************************************************************/
void guiFormSpecMenuGeneric::allowClose(bool value)
{
	m_Interpreter_v1->allowClose(value);
	//m_Interpreter_v2->allowClose(value);
}
