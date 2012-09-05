/*
 Minetest-c55
 Copyright (C) 2010-11 celeron55, Perttu Ahola <celeron55@gmail.com>
 Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>
 Copyright (C) 2011 teddydestodes <derkomtur@schattengang.net>

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
#include "settings.h"
#include <algorithm>

#define KMaxButtonPerColumns 12

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
};

GUIKeyChangeMenu::GUIKeyChangeMenu(gui::IGUIEnvironment* env,
				gui::IGUIElement* parent, s32 id, IMenuManager *menumgr) :
GUIModalMenu(env, parent, id, menumgr)
{
	shift_down = false;
	activeKey = -1;
	this->key_used_text = NULL;
	init_keys();
	for(size_t i=0; i<key_settings.size(); i++)
		this->key_used.push_back(key_settings.at(i)->key);
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
	v2s32 size(620, 430);
	
	core::rect < s32 > rect(screensize.X / 2 - size.X / 2,
							screensize.Y / 2 - size.Y / 2, screensize.X / 2 + size.X / 2,
							screensize.Y / 2 + size.Y / 2);

	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 topleft(0, 0);
	changeCtype("");
	{
		core::rect < s32 > rect(0, 0, 600, 40);
		rect += topleft + v2s32(25, 3);
		//gui::IGUIStaticText *t =
		Environment->addStaticText(wgettext("Keybindings. (If this menu screws up, remove stuff from minetest.conf)"),
								   rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	// Build buttons

	v2s32 offset(25, 60);

	for(size_t i = 0; i < key_settings.size(); i++)
	{
		key_setting *k = key_settings.at(i);
		{
			core::rect < s32 > rect(0, 0, 100, 20);
			rect += topleft + v2s32(offset.X, offset.Y);
			Environment->addStaticText(k->button_name, rect, false, true, this, -1);
		}

		{
			core::rect < s32 > rect(0, 0, 100, 30);
			rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
			k->button = Environment->addButton(rect, this, k->id, wgettext(k->key.name()));
		}
		if(i + 1 == KMaxButtonPerColumns)
			offset = v2s32(250, 60);
		else
			offset += v2s32(0, 25);
	}
	
	{
		s32 option_x = offset.X + 10;
		s32 option_y = offset.Y;
		u32 option_w = 180;
		{
			core::rect<s32> rect(0, 0, option_w, 30);
			rect += topleft + v2s32(option_x, option_y);
			Environment->addCheckBox(g_settings->getBool("aux1_descends"), rect, this,
					GUI_ID_CB_AUX1_DESCENDS, wgettext("\"Use\" = climb down"));
		}
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(size.X - 100 - 20, size.Y - 40);
		Environment->addButton(rect, this, GUI_ID_BACK_BUTTON,
							   wgettext("Save"));
	}
	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(size.X - 100 - 20 - 100 - 20, size.Y - 40);
		Environment->addButton(rect, this, GUI_ID_ABORT_BUTTON,
							   wgettext("Cancel"));
	}
	changeCtype("C");
	
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
	clearKeyCache();
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
				k->button->setText(wgettext(k->key.name()));
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
		changeCtype("");
		bool prefer_character = shift_down;
		KeyPress kp(event.KeyInput, prefer_character);
		
		bool shift_went_down = false;
		if(!shift_down &&
				(event.KeyInput.Key == irr::KEY_SHIFT ||
				event.KeyInput.Key == irr::KEY_LSHIFT ||
				event.KeyInput.Key == irr::KEY_RSHIFT))
			shift_went_down = true;

		// Remove Key already in use message
		if(this->key_used_text)
		{
			this->key_used_text->remove();
			this->key_used_text = NULL;
		}
		// Display Key already in use message
		if (std::find(this->key_used.begin(), this->key_used.end(), kp) != this->key_used.end())
		{
			core::rect < s32 > rect(0, 0, 600, 40);
			rect += v2s32(0, 0) + v2s32(25, 30);
			this->key_used_text = Environment->addStaticText(wgettext("Key already in use"),
									rect, false, true, this, -1);
			//infostream << "Key already in use" << std::endl;
		}

		// But go on
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
			k->button->setText(wgettext(k->key.name()));

			this->key_used.push_back(kp);

			changeCtype("C");
			// Allow characters made with shift
			if(shift_went_down){
				shift_down = true;
				return false;
			}else{
				activeKey = -1;
				return true;
			}
		}
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
		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED)
		{
			if(event.GUIEvent.Caller->getID() != GUI_ID_BACK_BUTTON &&
			   event.GUIEvent.Caller->getID() != GUI_ID_ABORT_BUTTON)
			{
				changeCtype("");
			}

			switch (event.GUIEvent.Caller->getID())
			{
				case GUI_ID_BACK_BUTTON: //back
					acceptInput();
					quitMenu();
					return true;
				case GUI_ID_ABORT_BUTTON: //abort
					quitMenu();
					return true;
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
					activeKey = event.GUIEvent.Caller->getID();
					k->button->setText(wgettext("press key"));
					this->key_used.erase(std::remove(this->key_used.begin(),
							this->key_used.end(), k->key), this->key_used.end());
					break;
			}
			Environment->setFocus(this);
			//Buttons
			changeCtype("C");
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}

void GUIKeyChangeMenu::add_key(int id, std::string button_name, std::string setting_name)
{
	key_setting *k = new key_setting;
	k->id = id;
	k->button_name = wgettext(button_name.c_str());
	k->setting_name = setting_name;
	k->key = getKeySetting(k->setting_name.c_str());
	key_settings.push_back(k);
}

void GUIKeyChangeMenu::init_keys()
{
	this->add_key(GUI_ID_KEY_FORWARD_BUTTON, "Forward", "keymap_forward");
	this->add_key(GUI_ID_KEY_BACKWARD_BUTTON, "Backward", "keymap_backward");
	this->add_key(GUI_ID_KEY_LEFT_BUTTON, "Left", "keymap_left");
	this->add_key(GUI_ID_KEY_RIGHT_BUTTON, "Right", "keymap_right");
	this->add_key(GUI_ID_KEY_USE_BUTTON, "Use", "keymap_special1");
	this->add_key(GUI_ID_KEY_JUMP_BUTTON, "Jump", "keymap_jump");
	this->add_key(GUI_ID_KEY_SNEAK_BUTTON, "Sneak", "keymap_sneak");
	this->add_key(GUI_ID_KEY_DROP_BUTTON, "Drop", "keymap_drop");
	this->add_key(GUI_ID_KEY_INVENTORY_BUTTON, "Inventory", "keymap_inventory");
	this->add_key(GUI_ID_KEY_CHAT_BUTTON, "Chat", "keymap_chat");
	this->add_key(GUI_ID_KEY_CMD_BUTTON, "Command", "keymap_cmd");
	this->add_key(GUI_ID_KEY_CONSOLE_BUTTON, "Console", "keymap_console");
	this->add_key(GUI_ID_KEY_FLY_BUTTON, "Toggle fly", "keymap_freemove");
	this->add_key(GUI_ID_KEY_FAST_BUTTON, "Toggle fast", "keymap_fastmove");
	this->add_key(GUI_ID_KEY_RANGE_BUTTON, "Range select", "keymap_rangeselect");
	this->add_key(GUI_ID_KEY_DUMP_BUTTON, "Print stacks", "keymap_print_debug_stacks");
}
