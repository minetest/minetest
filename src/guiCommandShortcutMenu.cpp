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

#include "guiCommandShortcutMenu.h"
#include "debug.h"
#include "serialization.h"
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

enum
{
	GUI_ID_BACK_BUTTON = 401, GUI_ID_ABORT_BUTTON, GUI_ID_SCROLL_BAR,
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

GUICommandShortcutMenu::GUICommandShortcutMenu(gui::IGUIEnvironment* env,
				gui::IGUIElement* parent, s32 id, IMenuManager *menumgr) :
GUIModalMenu(env, parent, id, menumgr)
{
	m_command_active_id = -1;
	m_command_adding = false;
	shift_down = false;
	control_down = false;
	activeKey = -1;
	this->key_used_text = NULL;
}

GUICommandShortcutMenu::~GUICommandShortcutMenu()
{
	removeChildren();
}

void GUICommandShortcutMenu::removeChildren()
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

void GUICommandShortcutMenu::regenerateGui(v2u32 screensize)
{
	removeChildren();
	v2s32 size(620, 260);
	
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
		const wchar_t *text = wgettext("Chat command shortcuts");
		Environment->addStaticText(text,
								   rect, false, true, this, -1);
		delete[] text;
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	// Build buttons

	v2s32 offset(25, 60);

	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y;
		u32 option_w = 350;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			m_command_combo = Environment->addComboBox(rect, this, GUI_ID_KEY_ALIAS_COMBOBOX);
			for (std::vector<KeyCommand>::iterator it = keys.aliases.begin();
					it != keys.aliases.end(); ++it)
			{
				const wchar_t* text = wgettext(it->setting_name.c_str());
				m_command_combo->addItem(text,it - keys.aliases.begin());
				delete[] text;
			}
		}
		option_x += 360;
		option_w = 100;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t* text = wgettext("Add");
			m_command_add = Environment->addButton(rect, this, GUI_ID_KEY_ALIAS_ADD, text );
			delete[] text;
		}
		option_x += 100;
		option_w = 100;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t* text = wgettext("Delete");
			m_command_remove = Environment->addButton(rect, this, GUI_ID_KEY_ALIAS_REMOVE, text);
			delete[] text;
		}
	}
	offset.Y += 10;
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
		u32 option_w = 150;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y + 5);
			const wchar_t* text = wgettext("Command");
			m_command_label = Environment->addStaticText(text, rect, false, true, this,-1);
			delete[] text;
		}
		option_x += 160;
		option_w = 350;
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
	offset.Y += 10;
	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y;
		u32 option_w = 150;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y + 5);
			const wchar_t* text = wgettext("Key combo");
			m_command_key_label = Environment->addStaticText(text, rect, false, true, this,-1);
			delete[] text;
		}
		option_x += 160;
		option_w = 250;
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

void GUICommandShortcutMenu::drawMenu()
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

bool GUICommandShortcutMenu::acceptInput()
{
	commandComboChanged();

	// Set only alias settings, not keys
	keys.setAliasSettings();

	clearKeyCache();
	clearCommandKeyCache();

	g_gamecallback->signalKeyConfigChange();

	return true;
}

bool GUICommandShortcutMenu::resetMenu()
{
	if (activeKey >= 0)
	{
		if (activeKey == GUI_ID_KEY_ALIAS_BUTTON)
		{
			KeyCommand &kc = keys.aliases[m_command_active_id];
			set_key_text(m_command_key,kc);
		}
		activeKey = -1;
		return false;
	}
	return true;
}
bool GUICommandShortcutMenu::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT && activeKey >= 0
			&& event.KeyInput.PressedDown) {
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
		std::wstring ku = keys.keyUsedBy(activeKey, kp, shift_down, control_down,
				activeKey == GUI_ID_KEY_ALIAS_BUTTON ? m_command_active_id : -1);
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
	    if (activeKey == GUI_ID_KEY_ALIAS_BUTTON)
		{
			KeyCommand &kc = keys.aliases[m_command_active_id];
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
	} else if (event.EventType == EET_KEY_INPUT_EVENT && activeKey < 0
			&& event.KeyInput.PressedDown
			&& event.KeyInput.Key == irr::KEY_ESCAPE) {
		quitMenu();
		return true;
	} else if (event.EventType == EET_GUI_EVENT) {
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
						for (std::vector<KeyCommand>::iterator it = keys.aliases.begin();
							 it != keys.aliases.end(); ++it)
							if (it->setting_name == kc.setting_name)
							{
								exists = true;
								m_command_combo->setSelected(it - keys.aliases.begin());
								break;
							}
						if (!exists)
						{
							kc.modifier_control = false;
							kc.modifier_shift = false;
							kc.command = "";
							keys.addAlias(kc);
							const wchar_t* text = wgettext(kc.setting_name.c_str());
							m_command_combo->addItem(text,keys.aliases.size()-1);
							delete[] text;
							m_command_combo->setSelected(keys.aliases.size()-1);
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
				case GUI_ID_KEY_ALIAS_REMOVE:
					if(!m_command_adding)
					{
						s32 sel = m_command_combo->getSelected();
						m_command_combo->removeItem(sel);
						keys.aliases.erase(keys.aliases.begin()+sel);
						m_command_combo->setSelected(keys.aliases.size()-1);
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
				case GUI_ID_KEY_ALIAS_KEY_BUTTON:
					// Key Alias
					resetMenu();
					shift_down = false;
					control_down = false;
					activeKey = GUI_ID_KEY_ALIAS_BUTTON;
					set_text(m_command_key, "press key");
					return true;
				default:
					warningstream<<"GUICommandShortcutMenu: "
							<<"Unknown button clicked"<<std::endl;
					break;
			}
			Environment->setFocus(this);
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}

void GUICommandShortcutMenu::commandComboChanged()
{
	s32 active_id = m_command_combo->getSelected();
	if (activeKey == GUI_ID_KEY_ALIAS_BUTTON)
		resetMenu();
	if (m_command_active_id>=0)
	{
		KeyCommand &k = keys.aliases[m_command_active_id];
		k.command = wide_to_narrow(m_command->getText());
	}
	if (active_id>=0)
	{
		KeyCommand k = keys.aliases[active_id];
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

