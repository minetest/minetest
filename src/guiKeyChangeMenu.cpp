/*
 Minetest-c55
 Copyright (C) 2010-11 celeron55, Perttu Ahola <celeron55@gmail.com>
 Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>
 Copyright (C) 2011 teddydestodes <derkomtur@schattengang.net>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "guiKeyChangeMenu.h"
#include "debug.h"
#include "serialization.h"
#include "keycode.h"
#include "main.h"
#include <string>

GUIKeyChangeMenu::GUIKeyChangeMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id, IMenuManager *menumgr) :
	GUIModalMenu(env, parent, id, menumgr)
{
	activeKey = -1;
	init_keys();
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
	/*
	 Remove stuff
	 */
	removeChildren();

	/*
	 Calculate new sizes and positions
	 */

	v2s32 size(620, 430);

	core::rect < s32 > rect(screensize.X / 2 - size.X / 2,
			screensize.Y / 2 - size.Y / 2, screensize.X / 2 + size.X / 2,
			screensize.Y / 2 + size.Y / 2);

	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 topleft(0, 0);

	{
		core::rect < s32 > rect(0, 0, 125, 20);
		rect += topleft + v2s32(25, 3);
		//gui::IGUIStaticText *t =
		Environment->addStaticText(chartowchar_t(gettext("KEYBINDINGS")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}
	v2s32 offset(25, 40);
	// buttons

	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Forward")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->forward = Environment->addButton(rect, this,
				GUI_ID_KEY_FORWARD_BUTTON,
				narrow_to_wide(KeyNames[key_forward]).c_str());
	}

	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Backward")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->backward = Environment->addButton(rect, this,
				GUI_ID_KEY_BACKWARD_BUTTON,
				narrow_to_wide(KeyNames[key_backward]).c_str());
	}
	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Left")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->left = Environment->addButton(rect, this, GUI_ID_KEY_LEFT_BUTTON,
				narrow_to_wide(KeyNames[key_left]).c_str());
	}
	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Right")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->right = Environment->addButton(rect, this,
				GUI_ID_KEY_RIGHT_BUTTON,
				narrow_to_wide(KeyNames[key_right]).c_str());
	}
	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Use")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->use = Environment->addButton(rect, this, GUI_ID_KEY_USE_BUTTON,
				narrow_to_wide(KeyNames[key_use]).c_str());
	}
	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Sneak")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->sneak = Environment->addButton(rect, this,
				GUI_ID_KEY_SNEAK_BUTTON,
				narrow_to_wide(KeyNames[key_sneak]).c_str());
	}
	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Jump")), rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->jump = Environment->addButton(rect, this, GUI_ID_KEY_JUMP_BUTTON,
				narrow_to_wide(KeyNames[key_jump]).c_str());
	}

	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Inventory")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->inventory = Environment->addButton(rect, this,
				GUI_ID_KEY_INVENTORY_BUTTON,
				narrow_to_wide(KeyNames[key_inventory]).c_str());
	}
	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Chat")), rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->chat = Environment->addButton(rect, this, GUI_ID_KEY_CHAT_BUTTON,
				narrow_to_wide(KeyNames[key_chat]).c_str());
	}

	//next col
	offset = v2s32(250, 40);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Toggle fly")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->fly = Environment->addButton(rect, this, GUI_ID_KEY_FLY_BUTTON,
				narrow_to_wide(KeyNames[key_fly]).c_str());
	}
	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Toggle fast")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->fast = Environment->addButton(rect, this, GUI_ID_KEY_FAST_BUTTON,
				narrow_to_wide(KeyNames[key_fast]).c_str());
	}
	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Range select")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->range = Environment->addButton(rect, this,
				GUI_ID_KEY_RANGE_BUTTON,
				narrow_to_wide(KeyNames[key_range]).c_str());
	}

	offset += v2s32(0, 25);
	{
		core::rect < s32 > rect(0, 0, 100, 20);
		rect += topleft + v2s32(offset.X, offset.Y);
		Environment->addStaticText(chartowchar_t(gettext("Print stacks")),
				rect, false, true, this, -1);
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(offset.X + 105, offset.Y - 5);
		this->dump = Environment->addButton(rect, this, GUI_ID_KEY_DUMP_BUTTON,
				narrow_to_wide(KeyNames[key_dump]).c_str());
	}
	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(size.X - 100 - 20, size.Y - 40);
		Environment->addButton(rect, this, GUI_ID_BACK_BUTTON,
		chartowchar_t(gettext("Save")));
	}
	{
		core::rect < s32 > rect(0, 0, 100, 30);
		rect += topleft + v2s32(size.X - 100 - 20 - 100 - 20, size.Y - 40);
		Environment->addButton(rect, this, GUI_ID_ABORT_BUTTON,
		chartowchar_t(gettext("Cancel")));
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
	g_settings.set("keymap_forward", keycode_to_keyname(key_forward));
	g_settings.set("keymap_backward", keycode_to_keyname(key_backward));
	g_settings.set("keymap_left", keycode_to_keyname(key_left));
	g_settings.set("keymap_right", keycode_to_keyname(key_right));
	g_settings.set("keymap_jump", keycode_to_keyname(key_jump));
	g_settings.set("keymap_sneak", keycode_to_keyname(key_sneak));
	g_settings.set("keymap_inventory", keycode_to_keyname(key_inventory));
	g_settings.set("keymap_chat", keycode_to_keyname(key_chat));
	g_settings.set("keymap_rangeselect", keycode_to_keyname(key_range));
	g_settings.set("keymap_freemove", keycode_to_keyname(key_fly));
	g_settings.set("keymap_fastmove", keycode_to_keyname(key_fast));
	g_settings.set("keymap_special1", keycode_to_keyname(key_use));
	g_settings.set("keymap_print_debug_stacks", keycode_to_keyname(key_dump));
	clearKeyCache();
	return true;
}
void GUIKeyChangeMenu::init_keys()
{
	key_forward = getKeySetting("keymap_forward");
	key_backward = getKeySetting("keymap_backward");
	key_left = getKeySetting("keymap_left");
	key_right = getKeySetting("keymap_right");
	key_jump = getKeySetting("keymap_jump");
	key_sneak = getKeySetting("keymap_sneak");
	key_inventory = getKeySetting("keymap_inventory");
	key_chat = getKeySetting("keymap_chat");
	key_range = getKeySetting("keymap_rangeselect");
	key_fly = getKeySetting("keymap_freemove");
	key_fast = getKeySetting("keymap_fastmove");
	key_use = getKeySetting("keymap_special1");
	key_dump = getKeySetting("keymap_print_debug_stacks");
}

bool GUIKeyChangeMenu::resetMenu()
{
	if (activeKey >= 0)
	{
		switch (activeKey)
		{
		case GUI_ID_KEY_FORWARD_BUTTON:
			this->forward->setText(
					narrow_to_wide(KeyNames[key_forward]).c_str());
			break;
		case GUI_ID_KEY_BACKWARD_BUTTON:
			this->backward->setText(
					narrow_to_wide(KeyNames[key_backward]).c_str());
			break;
		case GUI_ID_KEY_LEFT_BUTTON:
			this->left->setText(narrow_to_wide(KeyNames[key_left]).c_str());
			break;
		case GUI_ID_KEY_RIGHT_BUTTON:
			this->right->setText(narrow_to_wide(KeyNames[key_right]).c_str());
			break;
		case GUI_ID_KEY_JUMP_BUTTON:
			this->jump->setText(narrow_to_wide(KeyNames[key_jump]).c_str());
			break;
		case GUI_ID_KEY_SNEAK_BUTTON:
			this->sneak->setText(narrow_to_wide(KeyNames[key_sneak]).c_str());
			break;
		case GUI_ID_KEY_INVENTORY_BUTTON:
			this->inventory->setText(
					narrow_to_wide(KeyNames[key_inventory]).c_str());
			break;
		case GUI_ID_KEY_CHAT_BUTTON:
			this->chat->setText(narrow_to_wide(KeyNames[key_chat]).c_str());
			break;
		case GUI_ID_KEY_RANGE_BUTTON:
			this->range->setText(narrow_to_wide(KeyNames[key_range]).c_str());
			break;
		case GUI_ID_KEY_FLY_BUTTON:
			this->fly->setText(narrow_to_wide(KeyNames[key_fly]).c_str());
			break;
		case GUI_ID_KEY_FAST_BUTTON:
			this->fast->setText(narrow_to_wide(KeyNames[key_fast]).c_str());
			break;
		case GUI_ID_KEY_USE_BUTTON:
			this->use->setText(narrow_to_wide(KeyNames[key_use]).c_str());
			break;
		case GUI_ID_KEY_DUMP_BUTTON:
			this->dump->setText(narrow_to_wide(KeyNames[key_dump]).c_str());
			break;
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
		if (activeKey == GUI_ID_KEY_FORWARD_BUTTON)
		{
			this->forward->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_forward = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_BACKWARD_BUTTON)
		{
			this->backward->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_backward = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_LEFT_BUTTON)
		{
			this->left->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_left = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_RIGHT_BUTTON)
		{
			this->right->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_right = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_JUMP_BUTTON)
		{
			this->jump->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_jump = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_SNEAK_BUTTON)
		{
			this->sneak->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_sneak = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_INVENTORY_BUTTON)
		{
			this->inventory->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_inventory = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_CHAT_BUTTON)
		{
			this->chat->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_chat = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_RANGE_BUTTON)
		{
			this->range->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_range = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_FLY_BUTTON)
		{
			this->fly->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_fly = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_FAST_BUTTON)
		{
			this->fast->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_fast = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_USE_BUTTON)
		{
			this->use->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_use = event.KeyInput.Key;
		}
		else if (activeKey == GUI_ID_KEY_DUMP_BUTTON)
		{
			this->dump->setText(
					narrow_to_wide(KeyNames[event.KeyInput.Key]).c_str());
			this->key_dump = event.KeyInput.Key;
		}

		activeKey = -1;
		return true;
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
			switch (event.GUIEvent.Caller->getID())
			{
			case GUI_ID_BACK_BUTTON: //back
				acceptInput();
				quitMenu();
				return true;
			case GUI_ID_ABORT_BUTTON: //abort
				quitMenu();
				return true;
			case GUI_ID_KEY_FORWARD_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->forward->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_BACKWARD_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->backward->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_LEFT_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->left->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_RIGHT_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->right->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_USE_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->use->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_FLY_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->fly->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_FAST_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->fast->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_JUMP_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->jump->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_CHAT_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->chat->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_SNEAK_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->sneak->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_INVENTORY_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->inventory->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_DUMP_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->dump->setText(chartowchar_t(gettext("press Key")));
				break;
			case GUI_ID_KEY_RANGE_BUTTON:
				resetMenu();
				activeKey = event.GUIEvent.Caller->getID();
				this->range->setText(chartowchar_t(gettext("press Key")));
				break;
			}
			//Buttons

		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}

