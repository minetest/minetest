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


#ifndef GUIFORMSPECMENU_HEADER
#define GUIFORMSPECMENU_HEADER

#include <utility>

#include "irrlichttypes_extrabloated.h"
#include "inventory.h"
#include "inventorymanager.h"
#include "modalMenu.h"
#include "guiTable.h"
#include "clientserver.h"
#include <IGUIElement.h>

class IGameDef;
class InventoryManager;
class ISimpleTextureSource;
class Client;
class guiFormSpecMenu_v1;
class guiFormSpecMenu_v2;

typedef enum {
	f_Button,
	f_Table,
	f_TabHeader,
	f_CheckBox,
	f_DropDown,
	f_ScrollBar,
	f_Unknown
} FormspecFieldType;

typedef enum {
	quit_mode_no,
	quit_mode_accept,
	quit_mode_cancel
} FormspecQuitMode;

struct TextDest
{
	virtual ~TextDest() {};
	// This is deprecated I guess? -celeron55
	virtual void gotText(std::wstring text){}
	virtual void gotText(std::map<std::string, std::string> fields) = 0;
	virtual void setFormName(std::string formname)
	{ m_formname = formname;};

	std::string m_formname;
};

class IFormSource
{
public:
	virtual ~IFormSource(){}
	virtual std::string getForm() = 0;
	// Fill in variables in field text
	virtual std::string resolveText(std::string str){ return str; }
};

class FormspecFormSource: public IFormSource
{
public:
	FormspecFormSource(std::string formspec)
	{
		m_formspec = formspec;
	}

	~FormspecFormSource()
	{}

	void setForm(std::string formspec) {
		m_formspec = FORMSPEC_VERSION_STRING + formspec;
	}

	std::string getForm()
	{
		return m_formspec;
	}

	std::string m_formspec;
};

class IGUIFormSpecMenu
{
public:
	IGUIFormSpecMenu()
	{ }

	virtual ~IGUIFormSpecMenu()
	{ }

	/**
	 * set formspec desctiption as well as where to get inventory from
	 *
	 * @param formspec_string the formspec itself
	 * @param inventory_location inventory to use for displaying
	 */
	virtual void setFormSpec(const std::string &formspec_string,
			InventoryLocation inventory_location) = 0;

	/**
	 * set a Interface to get form source from
	 * Note: form_src has to be deleted by implementation of IGUIFormspecMenu
	 *
	 * @param form_src pointer to form source interface
	 */
	virtual void setFormSource(IFormSource *form_src) = 0;

	/**
	 * textdest is our handler for events
	 * Note: text_dst has to be deleted by implementation of IGUIFormspecMenu
	 *
	 * @param text_dst pointer to handler
	 */
	//
	virtual void setTextDest(TextDest *text_dst) = 0;


	/**
	 * tell formspec exit game is allowed
	 *
	 * @param value true/false if close is allowed or not
	 */
	virtual void allowClose(bool value) = 0;

	/**
	 *
	 * tell formspec to show a form in fixed size mode
	 * DEPRECATED!
	 * @param lock true/false to en-/disable the fixed size mode
	 * @param basescreensize size of screen to assume for fixed size mode
	 */
	void lockSize(bool lock,v2u32 basescreensize=v2u32(0,0));

	/**
	 *  make this menu pause the game
	 *  @param new_value new_value the value true/false to set
	 */
	virtual void setDoPause(bool new_value) = 0;

	/**
	 * get pointer to table content
	 *
	 * @param tablename name of table to get pointer from
	 * @return table pointer
	 */
	virtual GUITable* getTable(std::wstring tablename) = 0;

};


/**
 * base formspec class
 */

class guiFormSpecBase : public IGUIFormSpecMenu, public GUIModalMenu
{
public:
	guiFormSpecBase(irr::IrrlichtDevice* dev,
		gui::IGUIElement* parent, s32 id, IMenuManager *menumgr,
		InventoryManager *invmgr, IGameDef *gamedef,
		ISimpleTextureSource *tsrc, IFormSource* fsrc, TextDest* tdst,
		Client* client) :

		GUIModalMenu(dev->getGUIEnvironment(), parent, id, menumgr),
		m_device(dev),
		m_invmgr(invmgr),
		m_gamedef(gamedef),
		m_tsrc(tsrc),
		m_client(client),
		m_allowclose(true),
		m_pauses_game(false)
	{ }

	virtual ~guiFormSpecBase()
	{ }

	/**
	 * tell formspec exit game is allowed
	 *
	 * @param value true/false if close is allowed or not
	 */
	void allowClose(bool value) { m_allowclose = value; }

	/**
	 * does this menu pause game?
	 *
	 * @return true/false
	 */
	bool pausesGame() { return m_pauses_game; }

	/**
	 *  make this menu pause the game
	 *  @param new_value new_value the value true/false to set
	 */
	void setDoPause(bool new_value) { m_pauses_game = new_value; }

protected:
	irr::IrrlichtDevice     *m_device;
	InventoryManager        *m_invmgr;
	IGameDef                *m_gamedef;
	ISimpleTextureSource    *m_tsrc;
	Client                  *m_client;

	bool                     m_allowclose;
	bool                     m_pauses_game;
};

/**
 * class switching between different formspec interpreters
 * Note: as this is a wrapper class for formspecs it may need to provide some
 * modal menu functions too
 */
class guiFormSpecMenuGeneric : public IGUIFormSpecMenu , public IReferenceCounted
{
public:
	guiFormSpecMenuGeneric(irr::IrrlichtDevice* dev,
		gui::IGUIElement* parent, s32 id, IMenuManager *menumgr,
		InventoryManager *invmgr, IGameDef *gamedef,
		ISimpleTextureSource *tsrc, IFormSource* fsrc, TextDest* tdst,
		Client* client);

	~guiFormSpecMenuGeneric();

	/**
	 * set formspec desctiption as well as where to get inventory from
	 *
	 * @param formspec_string the formspec itself
	 * @param inventory_location inventory to use for displaying
	 */
	void setFormSpec(const std::string &formspec_string,
			InventoryLocation inventory_location);

	/**
	 * set a Interface to get form source from
	 * Note: form_src has to be deleted by implementation of IGUIFormspecMenu
	 *
	 * @param form_src pointer to form source interface
	 */
	void setFormSource(IFormSource *form_src);

	/**
	 * textdest is our handler for events
	 * Note: text_dst has to be deleted by implementation of IGUIFormspecMenu
	 *
	 * @param text_dst pointer to handler
	 */
	//
	void setTextDest(TextDest *text_dst);


	/**
	 * tell formspec exit game is allowed
	 *
	 * @param value true/false if close is allowed or not
	 */
	void allowClose(bool value);

	/**
	 *
	 * tell formspec to show a form in fixed size mode
	 * DEPRECATED!
	 * @param lock true/false to en-/disable the fixed size mode
	 * @param basescreensize size of screen to assume for fixed size mode
	 */
	void lockSize(bool lock,v2u32 basescreensize=v2u32(0,0));

	/**
	 *  make this menu pause the game
	 *  @param new_value new_value the value true/false to set
	 */
	void setDoPause(bool new_value);


	/**
	 * quit this menu
	 */
	void quitMenu();


	/**
	 * get the current active gui element
	 */
	gui::IGUIElement* getGUIElement();

	/**
	 * get pointer to table content
	 *
	 * @param tablename name of table to get pointer from
	 * @return table pointer
	 */
	GUITable* getTable(std::wstring tablename);


private:
	typedef enum {
		FS_INTERPRETER_V1,
		FS_INTERPRETER_V2
	} fs_interpreter_version;

	fs_interpreter_version m_current_interpreter;

	guiFormSpecMenu_v1 *m_Interpreter_v1;
	guiFormSpecMenu_v2 *m_Interpreter_v2;
};

std::vector<std::string> split(const std::string &s, char delim);

bool isChild(gui::IGUIElement * tocheck, gui::IGUIElement * parent);

#endif

