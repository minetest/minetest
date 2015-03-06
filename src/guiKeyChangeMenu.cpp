/*
 Minetest
 Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
 Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>
 Copyright (C) 2013 teddydestodes <derkomtur@schattengang.net>

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

#include "guiKeyChangeMenu.h"
#include "debug.h"
#include "serialization.h"
#include "main.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include <IGUIComboBox.h>
#include "settings.h"
#include <algorithm>

#include "mainmenumanager.h"  // for g_gamecallback

#define KMaxButtonPerColumns 12

extern MainGameCallback *g_gamecallback;

enum
{
	GUI_ID_BACK_BUTTON = 101, GUI_ID_ABORT_BUTTON, GUI_ID_SCROLL_BAR,
	// buttons
	GUI_ID_KEY_FORWARD_BUTTON,
	GUI_ID_KEY_BACKWARD_BUTTON,
	GUI_ID_KEY_LEFT_BUTTON,
	GUI_ID_KEY_RIGHT_BUTTON,
	GUI_ID_KEY_USE_BUTTON,
	GUI_ID_KEY_FLY_BUTTON,
	GUI_ID_KEY_FAST_BUTTON,
	GUI_ID_KEY_JUMP_BUTTON,
	GUI_ID_KEY_NOCLIP_BUTTON,
	GUI_ID_KEY_CINEMATIC_BUTTON,
	GUI_ID_KEY_CHAT_BUTTON,
	GUI_ID_KEY_CMD_BUTTON,
	GUI_ID_KEY_CONSOLE_BUTTON,
	GUI_ID_KEY_SNEAK_BUTTON,
	GUI_ID_KEY_DROP_BUTTON,
	GUI_ID_KEY_INVENTORY_BUTTON,
	GUI_ID_KEY_DUMP_BUTTON,
	GUI_ID_KEY_RANGE_BUTTON,
	// other
	GUI_ID_CB_AUX1_DESCENDS,
	GUI_ID_CB_DOUBLETAP_JUMP,
	// key alias GUI
	GUI_ID_KEY_ALIAS_BUTTON, // opens the GUI for commands
	GUI_ID_KEY_ALIAS_COMBOBOX, // List box showing the list of commands
	GUI_ID_KEY_ALIAS_ADD, // Button to add new command
	GUI_ID_KEY_ALIAS_REMOVE, // Button to remove command
	GUI_ID_KEY_ALIAS_NAME_ENTRY, // Entry for the name of the command
	GUI_ID_KEY_ALIAS_COMMAND_ENTRY, // Entry for the chat command
	GUI_ID_KEY_ALIAS_KEY_BUTTON // Button used to set the keyboard shortcut
};

static void set_text(gui::IGUIElement *el, const std::string &textval)
{
    const wchar_t* text = wgettext(textval.c_str());
    el->setText(text);
    delete[] text;
}

static void set_key_text(gui::IGUIElement *el, const KeyCommand &kc)
{
    if (std::string(kc.key.name()) != "")
    {
        std::string keytext = std::string(kc.modifier_control?"[Ctrl]-":"")
                            + std::string(kc.modifier_shift?"[Shft]-":"")
                            + kc.key.name();
        set_text(el,keytext);
    }
    else
        set_text(el,"Not set");
}

GUIKeyChangeMenu::GUIKeyChangeMenu(gui::IGUIEnvironment* env,
				gui::IGUIElement* parent, s32 id, IMenuManager *menumgr) :
GUIModalMenu(env, parent, id, menumgr)
{
	m_command_active_id = -1;
	m_command_adding = false;
	shift_down = false;
	control_down = false;
	activeKey = -1;
	this->key_used_text = NULL;
	init_keys();
}

GUIKeyChangeMenu::~GUIKeyChangeMenu()
{
	removeChildren();

	for (std::vector<key_setting*>::iterator iter = key_settings.begin();
			iter != key_settings.end(); iter ++) {
		delete[] (*iter)->button_name;
		delete (*iter);
	}
	key_settings.clear();
}

void GUIKeyChangeMenu::removeChildren()
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

void GUIKeyChangeMenu::regenerateGui(v2u32 screensize)
{
	removeChildren();
	v2s32 size(620, 460);
	
	core::rect < s32 > rect(screensize.X / 2 - size.X / 2,
							screensize.Y / 2 - size.Y / 2, screensize.X / 2 + size.X / 2,
							screensize.Y / 2 + size.Y / 2);

	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 topleft(0, 0);
	
	{
		core::rect < s32 > rect(0, 0, 600, 40);
		rect += topleft + v2s32(25, 3);
		//gui::IGUIStaticText *t =
		const wchar_t *text = wgettext("Keybindings. (If this menu screws up, remove stuff from minetest.conf)");
		Environment->addStaticText(text,
								   rect, false, true, this, -1);
		delete[] text;
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	// Build buttons

	v2s32 offset(25, 60);

	for(size_t i = 0; i < key_settings.size(); i++)
	{
		key_setting *k = key_settings.at(i);
		{
			core::rect < s32 > rect(0, 0, 110, 20);
			rect += topleft + v2s32(offset.X, offset.Y);
			Environment->addStaticText(k->button_name, rect, false, true, this, -1);
		}

		{
			core::rect < s32 > rect(0, 0, 100, 30);
			rect += topleft + v2s32(offset.X + 115, offset.Y - 5);
			const wchar_t *text = wgettext(k->key.name());
			k->button = Environment->addButton(rect, this, k->id, text);
			delete[] text;
		}
		if(i + 1 == KMaxButtonPerColumns)
			offset = v2s32(260, 60);
		else
			offset += v2s32(0, 25);
	}
	
	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y + 5;
		u32 option_w = 180;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t *text = wgettext("\"Use\" = climb down");
			Environment->addCheckBox(g_settings->getBool("aux1_descends"), rect, this,
					GUI_ID_CB_AUX1_DESCENDS, text);
			delete[] text;
		}
		offset += v2s32(0, 25);
	}

	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y + 5;
		u32 option_w = 280;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t *text = wgettext("Double tap \"jump\" to toggle fly");
			Environment->addCheckBox(g_settings->getBool("doubletap_jump"), rect, this,
					GUI_ID_CB_DOUBLETAP_JUMP, text);
			delete[] text;
		}
		offset += v2s32(0, 35);
	}
	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y;
		u32 option_w = 200;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t* text = wgettext("Chat Commands");
			Environment->addStaticText(text, rect, false, true, this,-1);
			delete[] text;
		}
		offset += v2s32(0, 25);
	}
	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y;
		u32 option_w = 200;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			m_command_combo = Environment->addComboBox(rect, this, GUI_ID_KEY_ALIAS_COMBOBOX);
			for (std::vector<KeyCommand>::iterator it = key_alias_settings.begin();
					it != key_alias_settings.end(); ++it)
			{
				const wchar_t* text = wgettext(it->setting_name.c_str());
				m_command_combo->addItem(text,it - key_alias_settings.begin());
				delete[] text;
			}
		}
		option_x += 210;
		option_w = 50;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t* text = wgettext("Add");
			m_command_add = Environment->addButton(rect, this, GUI_ID_KEY_ALIAS_ADD, text );
			delete[] text;
		}
		option_x += 50;
		option_w = 50;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t* text = wgettext("Delete");
			m_command_remove = Environment->addButton(rect, this, GUI_ID_KEY_ALIAS_REMOVE, text);
			delete[] text;
		}
	}
	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y;
		u32 option_w = 200;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t* text = wgettext("Command Name");
			m_command_name = Environment->addEditBox(text, rect, false, this,
						   GUI_ID_KEY_ALIAS_NAME_ENTRY);
			m_command_name->setVisible(false);
			delete[] text;
		}
		offset += v2s32(0, 35);
	}
	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y;
		u32 option_w = 100;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y + 5);
			const wchar_t* text = wgettext("Command");
			m_command_label = Environment->addStaticText(text, rect, false, true, this,-1);
			delete[] text;
		}
		option_x += 110;
		option_w = 200;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t* text = wgettext("");
			m_command = Environment->addEditBox(text, rect, false, this,
						   GUI_ID_KEY_ALIAS_COMMAND_ENTRY);
			delete[] text;
		}
		offset += v2s32(0, 35);
	}
	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y;
		u32 option_w = 100;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y + 5);
			const wchar_t* text = wgettext("Key combo");
			m_command_key_label = Environment->addStaticText(text, rect, false, true, this,-1);
			delete[] text;
		}
		option_x += 110;
		option_w = 200;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t* text = wgettext("Key");
			m_command_key = Environment->addButton(rect, this,
						  GUI_ID_KEY_ALIAS_KEY_BUTTON, text);
			delete[] text;
		}
	}
	commandComboChanged();

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(size.X - 100 - 20, size.Y - 40);
		const wchar_t *text =  wgettext("Save");
		Environment->addButton(rect, this, GUI_ID_BACK_BUTTON,
				 text);
		delete[] text;
	}
	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(size.X - 100 - 20 - 100 - 20, size.Y - 40);
		const wchar_t *text = wgettext("Cancel");
		Environment->addButton(rect, this, GUI_ID_ABORT_BUTTON,
				text);
		delete[] text;
	}	
}

void GUIKeyChangeMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();

	video::SColor bgcolor(140, 0, 0, 0);

	{
		core::rect < s32 > rect(0, 0, 620, 620);
		rect += AbsoluteRect.UpperLeftCorner;
		driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
	}

	gui::IGUIElement::draw();
}

bool GUIKeyChangeMenu::acceptInput()
{
	for(size_t i = 0; i < key_settings.size(); i++)
	{
		key_setting *k = key_settings.at(i);
		g_settings->set(k->setting_name, k->key.sym());
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CB_AUX1_DESCENDS);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			g_settings->setBool("aux1_descends", ((gui::IGUICheckBox*)e)->isChecked());
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CB_DOUBLETAP_JUMP);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			g_settings->setBool("doubletap_jump", ((gui::IGUICheckBox*)e)->isChecked());
	}

	clearKeyCache();

	g_gamecallback->signalKeyConfigChange();

	return true;
}

bool GUIKeyChangeMenu::resetMenu()
{
	if (activeKey >= 0)
	{
		for(size_t i = 0; i < key_settings.size(); i++)
		{
			key_setting *k = key_settings.at(i);
			if(k->id == activeKey)
			{
				const wchar_t *text = wgettext(k->key.name());
				k->button->setText(text);
				delete[] text;
				break;
			}
		}
		activeKey = -1;
		return false;
	}
	return true;
}
bool GUIKeyChangeMenu::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT && activeKey >= 0
		&& event.KeyInput.PressedDown)
	{
		bool prefer_character = shift_down && (activeKey != GUI_ID_KEY_ALIAS_BUTTON);
		KeyPress kp(event.KeyInput, prefer_character);
		
		bool shift_went_down = false;
		if(!shift_down &&
				(event.KeyInput.Key == irr::KEY_SHIFT ||
				event.KeyInput.Key == irr::KEY_LSHIFT ||
				event.KeyInput.Key == irr::KEY_RSHIFT))
			shift_went_down = true;

		bool control_went_down = false;
		if(!control_down &&
				(event.KeyInput.Key == irr::KEY_CONTROL ||
				event.KeyInput.Key == irr::KEY_LCONTROL ||
				event.KeyInput.Key == irr::KEY_RCONTROL))
			control_went_down = true;

		// Remove Key already in use message
		if(this->key_used_text)
		{
			this->key_used_text->remove();
			this->key_used_text = NULL;
		}
		// Display Key already in use message
		std::wstring ku = keyUsedBy(activeKey, kp, shift_down, control_down);
		if (ku != L"")
		{
			core::rect < s32 > rect(0, 0, 600, 40);
			rect += v2s32(0, 0) + v2s32(25, 30);
			std::wstring text = std::wstring(L"Key already in use by ")+ku;
			this->key_used_text = Environment->addStaticText(text.c_str(),
					rect, false, true, this, -1);
			//infostream << "Key already in use" << std::endl;
		}

		// But go on
	    if (activeKey != GUI_ID_KEY_ALIAS_BUTTON)
		{
			key_setting *k=NULL;
			for(size_t i = 0; i < key_settings.size(); i++)
			{
				if(key_settings.at(i)->id == activeKey)
				{
					k = key_settings.at(i);
					break;
				}
			}
			assert(k);
			k->key = kp;
			const wchar_t *text = wgettext(k->key.name());
			k->button->setText(text);
			delete[] text;

		}
		else
		{
			KeyCommand &kc = key_alias_settings[m_command_active_id];
			kc.key = kp;
			kc.modifier_shift = shift_down;
			kc.modifier_control = control_down;
			set_key_text(m_command_key,kc);
		}
 
		if (!shift_went_down && !control_went_down)
		{
			 activeKey = -1;
			 return true;
		}
		if (shift_went_down)
			shift_down = true;
		if (control_went_down)
			control_down = true;
		return false;
 	}
 	if (event.EventType == EET_GUI_EVENT)
 	{
		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST
			&& isVisible())
		{
			if (!canTakeFocus(event.GUIEvent.Element))
			{
				dstream << "GUIMainMenu: Not allowing focus change."
				<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_COMBO_BOX_CHANGED)
		{
			commandComboChanged();
			return true;
		}
		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED)
		{
			switch (event.GUIEvent.Caller->getID())
			{
				case GUI_ID_BACK_BUTTON: //back
					acceptInput();
					quitMenu();
					return true;
				case GUI_ID_ABORT_BUTTON: //abort
					quitMenu();
					return true;
				case GUI_ID_KEY_ALIAS_ADD:
					{
						if(!m_command_adding)
						{
							m_command_adding = true;
							m_command_name->setVisible(true);
							m_command_combo->setVisible(false);
							m_command_label->setVisible(false);
							m_command->setVisible(false);
							m_command_key_label->setVisible(false);
							m_command_key->setVisible(false);
							set_text(m_command_name,"Command Name");
							set_text(m_command_add,"Add");
							set_text(m_command_remove,"Cancel");
							Environment->setFocus(m_command_name);
						}
						else
						{
							KeyCommand kc;
							kc.setting_name = wide_to_narrow(m_command_name->getText());
							std::replace(kc.setting_name.begin(), kc.setting_name.end(), ' ', '_');
							bool exists = false;
							for (std::vector<KeyCommand>::iterator it = key_alias_settings.begin();
								 it != key_alias_settings.end(); ++it)
								if (it->setting_name == kc.setting_name)
								{
									exists = true;
									m_command_combo->setSelected(it - key_alias_settings.begin());
									break;
								}
							if (!exists)
							{
								kc.modifier_control = false;
								kc.modifier_shift = false;
								kc.command = "";
								add_command_alias_key(kc);
								const wchar_t* text = wgettext(kc.setting_name.c_str());
								m_command_combo->addItem(text,key_alias_settings.size()-1);
								delete[] text;
								m_command_combo->setSelected(key_alias_settings.size()-1);
							}
							commandComboChanged();

							m_command_adding = false;
							m_command_name->setVisible(false);
							m_command_combo->setVisible(true);
							m_command->setVisible(true);
							m_command_label->setVisible(true);
							m_command_key->setVisible(true);
							m_command_key_label->setVisible(true);
							set_text(m_command_name,"Command Name");
							set_text(m_command_add,"Add");
							set_text(m_command_remove,"Delete");
						}
						return true;
					}
				case GUI_ID_KEY_ALIAS_REMOVE:
					{
						if(!m_command_adding)
						{
							s32 sel = m_command_combo->getSelected();
							m_command_combo->removeItem(sel);
							key_alias_settings.erase(key_alias_settings.begin()+sel);
							m_command_combo->setSelected(key_alias_settings.size()-1);
							m_command_active_id = -1;
							commandComboChanged();
							return true;
						}
						else
						{
							m_command_adding = false;
							m_command_name->setVisible(false);
							m_command_combo->setVisible(true);
							m_command->setVisible(true);
							m_command_label->setVisible(true);
							m_command_key->setVisible(true);
							m_command_key_label->setVisible(true);
							set_text(m_command_name,"Command Name");
							set_text(m_command_add,"Add");
							set_text(m_command_remove,"Delete");
						}
					}
				case GUI_ID_KEY_ALIAS_KEY_BUTTON:
					{
						// Key Alias
						resetMenu();
						shift_down = false;
						control_down = false;
						activeKey = GUI_ID_KEY_ALIAS_BUTTON;
						set_text(m_command_key, "press key");
						return true;
					}
				default:
					key_setting *k = NULL;
					for(size_t i = 0; i < key_settings.size(); i++)
					{
						if(key_settings.at(i)->id == event.GUIEvent.Caller->getID())
						{
							k = key_settings.at(i);
							break;
						}
					}
					assert(k);

					resetMenu();
					shift_down = false;
					control_down = false;
					activeKey = event.GUIEvent.Caller->getID();
					set_text(k->button,"press key");
					break;
			}
			Environment->setFocus(this);
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}

void GUIKeyChangeMenu::commandComboChanged()
{
	s32 active_id = m_command_combo->getSelected();
	if (activeKey == GUI_ID_KEY_ALIAS_BUTTON)
		resetMenu();
	if (m_command_active_id>=0)
	{
		KeyCommand &k = key_alias_settings[m_command_active_id];
		k.command = wide_to_narrow(m_command->getText());
	}
	if (active_id>=0)
	{
		KeyCommand k = key_alias_settings[active_id];
		set_text(m_command_name,k.setting_name);
		set_text(m_command,k.command);
		set_key_text(m_command_key,k);
		m_command->setEnabled(true);
	m_command_key->setEnabled(true);
		m_command_remove->setEnabled(true);
	}
	else
	{
		m_command->setEnabled(false);
		m_command_key->setEnabled(false);
		m_command_remove->setEnabled(false);
	}
	m_command_active_id = active_id;
}

std::wstring GUIKeyChangeMenu::keyUsedBy(int id, const KeyPress &key, bool modifier_shift, bool modifier_control)
{
	for (std::vector<key_setting *>::iterator it = key_settings.begin(); it != key_settings.end(); ++it)
	{
		if ((*it)->id == id)
			continue;
		if ((*it)->key == key)
			return std::wstring((*it)->button_name);
	}
	for (std::vector<KeyCommand>::iterator it = key_alias_settings.begin(); it != key_alias_settings.end(); ++it)
	{
		if (id == GUI_ID_KEY_ALIAS_BUTTON && m_command_active_id == it - key_alias_settings.begin())
			continue;
		if (it->key == key)
		{
			if (id != GUI_ID_KEY_ALIAS_BUTTON)
				return std::wstring(L"chat command \"" + narrow_to_wide(it->setting_name) + L"\"");
			else
				if (it->modifier_shift == modifier_shift
					&& it->modifier_control == modifier_control)
					return std::wstring(L"chat command \"" + narrow_to_wide(it->setting_name) + L"\"");
		}
	}
	return L"";
}

void GUIKeyChangeMenu::add_key(int id, const wchar_t *button_name, const std::string &setting_name)
{
	key_setting *k = new key_setting;
	k->id = id;

	k->button_name = button_name;
	k->setting_name = setting_name;
	k->key = getKeySetting(k->setting_name.c_str());
	key_settings.push_back(k);
}

void GUIKeyChangeMenu::add_command_alias_key(const KeyCommand &key)
{
	key_alias_settings.push_back(key);
}

void GUIKeyChangeMenu::init_keys()
{
	this->add_key(GUI_ID_KEY_FORWARD_BUTTON,   wgettext("Forward"),          "keymap_forward");
	this->add_key(GUI_ID_KEY_BACKWARD_BUTTON,  wgettext("Backward"),         "keymap_backward");
	this->add_key(GUI_ID_KEY_LEFT_BUTTON,      wgettext("Left"),             "keymap_left");
	this->add_key(GUI_ID_KEY_RIGHT_BUTTON,     wgettext("Right"),            "keymap_right");
	this->add_key(GUI_ID_KEY_USE_BUTTON,       wgettext("Use"),              "keymap_special1");
	this->add_key(GUI_ID_KEY_JUMP_BUTTON,      wgettext("Jump"),             "keymap_jump");
	this->add_key(GUI_ID_KEY_SNEAK_BUTTON,     wgettext("Sneak"),            "keymap_sneak");
	this->add_key(GUI_ID_KEY_DROP_BUTTON,      wgettext("Drop"),             "keymap_drop");
	this->add_key(GUI_ID_KEY_INVENTORY_BUTTON, wgettext("Inventory"),        "keymap_inventory");
	this->add_key(GUI_ID_KEY_CHAT_BUTTON,      wgettext("Chat"),             "keymap_chat");
	this->add_key(GUI_ID_KEY_CMD_BUTTON,       wgettext("Command"),          "keymap_cmd");
	this->add_key(GUI_ID_KEY_CONSOLE_BUTTON,   wgettext("Console"),          "keymap_console");
	this->add_key(GUI_ID_KEY_FLY_BUTTON,       wgettext("Toggle fly"),       "keymap_freemove");
	this->add_key(GUI_ID_KEY_FAST_BUTTON,      wgettext("Toggle fast"),      "keymap_fastmove");
	this->add_key(GUI_ID_KEY_CINEMATIC_BUTTON, wgettext("Toggle Cinematic"), "keymap_cinematic");
	this->add_key(GUI_ID_KEY_NOCLIP_BUTTON,    wgettext("Toggle noclip"),    "keymap_noclip");
	this->add_key(GUI_ID_KEY_RANGE_BUTTON,     wgettext("Range select"),     "keymap_rangeselect");
	this->add_key(GUI_ID_KEY_DUMP_BUTTON,      wgettext("Print stacks"),     "keymap_print_debug_stacks");

	for (size_t i = 0; i<getCommandKeySettingCount(); ++i)
		add_command_alias_key(getCommandKeySetting(i));
}

