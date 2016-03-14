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

#define KMaxButtonPerColumns 12

enum
{
	GUI_ID_BACK_BUTTON = 101, GUI_ID_ABORT_BUTTON, GUI_ID_SCROLL_BAR,
	// buttons
	GUI_ID_KEY_START = 200,
	GUI_ID_KEY_END = 299,
	// other
	GUI_ID_CB_AUX1_DESCENDS = 300,
	GUI_ID_CB_DOUBLETAP_JUMP,
	// buttons for launching other dialogs
	GUI_ID_CHAT_COMMAND_SHORTCUTS_BUTTON,
};

static void set_text(gui::IGUIElement *el, const std::string &textval)
{
    const wchar_t* text = wgettext(textval.c_str());
    el->setText(text);
    delete[] text;
}

GUIKeyChangeMenu::GUIKeyChangeMenu(gui::IGUIEnvironment* env,
				gui::IGUIElement* parent, s32 id, IMenuManager *menumgr) :
GUIModalMenu(env, parent, id, menumgr)
{
	shift_down = false;
	control_down = false;
	activeKey = -1;
	this->key_used_text = NULL;
}

GUIKeyChangeMenu::~GUIKeyChangeMenu()
{
	removeChildren();

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

	for(size_t i = 0; i < keys.settings.size(); i++)
	{
		KeySetting *k = &keys.settings.at(i);
		{
			core::rect < s32 > rect(0, 0, 140, 20);
			rect += topleft + v2s32(offset.X, offset.Y);
			Environment->addStaticText(k->button_name, rect, false, true, this, -1);
		}

		{
			core::rect < s32 > rect(0, 0, 100, 30);
			rect += topleft + v2s32(offset.X + 145, offset.Y - 5);
			const wchar_t *text = wgettext(k->key.name());
			if(key_buttons.size() < i + 1)
				key_buttons.resize(i + 1);
			else
				key_buttons[i]->remove();
			key_buttons[i] = Environment->addButton(rect, this, GUI_ID_KEY_START + k->id, text);
			delete[] text;
		}
		if(i + 1 == KMaxButtonPerColumns)
			offset = v2s32(300, 60);
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
		core::rect < s32 > rect(0, 0, 250, 30);
		rect += topleft + v2s32(20, size.Y - 40);
		const wchar_t *text =  wgettext("Chat command shortcuts");
		Environment->addButton(rect, this, GUI_ID_CHAT_COMMAND_SHORTCUTS_BUTTON,
				 text);
		delete[] text;
	}

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

	// Set only key settings, not aliases
	keys.setKeySettings();

	clearKeyCache();
	clearCommandKeyCache();

	g_gamecallback->signalKeyConfigChange();

	return true;
}

bool GUIKeyChangeMenu::resetMenu()
{
	if (activeKey >= 0)
	{
		for (size_t i = 0; i < keys.settings.size(); i++)
		{
			KeySetting *k = &keys.settings.at(i);
			if(GUI_ID_KEY_START + k->id == activeKey)
			{
				const wchar_t *text = wgettext(k->key.name());
				key_buttons[i]->setText(text);
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
			&& event.KeyInput.PressedDown) {
		bool prefer_character = shift_down;
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
		std::wstring ku = keys.keyUsedBy(activeKey, kp, shift_down, control_down, -1);
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
		{
			KeySetting *k = NULL;
			for(size_t i = 0; i < keys.settings.size(); i++)
			{
				if(GUI_ID_KEY_START + keys.settings.at(i).id == activeKey)
				{
					k = &keys.settings.at(i);
					break;
				}
			}
			FATAL_ERROR_IF(k == NULL, "Key setting not found");
			k->key = kp;
			const wchar_t *text = wgettext(k->key.name());
			key_buttons[k->index]->setText(text);
			delete[] text;

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
				case GUI_ID_CHAT_COMMAND_SHORTCUTS_BUTTON:
					{
						GUICommandShortcutMenu *menu = new GUICommandShortcutMenu(
								Environment, Parent, -1, &g_menumgr);
						menu->drop();
					}
					return true;
				default:
					KeySetting *k = NULL;
					for(size_t i = 0; i < keys.settings.size(); i++)
					{
						if(GUI_ID_KEY_START + keys.settings.at(i).id ==
								event.GUIEvent.Caller->getID())
						{
							k = &keys.settings.at(i);
							break;
						}
					}
					FATAL_ERROR_IF(k == NULL, "Key setting not found");

					resetMenu();
					shift_down = false;
					control_down = false;
					activeKey = event.GUIEvent.Caller->getID();
					set_text(key_buttons[k->index],"press key");
					break;
			}
			Environment->setFocus(this);
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}

void GUIKeyChangeMenu::setVisible(bool visible)
{
	GUIModalMenu::setVisible(visible);

	if (visible) {
		// Refresh aliases from settings in case we came back from the command
		// shortcut dialog so that we can check if keys are used by aliases
		keys.refreshAliases();
		resetMenu();
	}
}

