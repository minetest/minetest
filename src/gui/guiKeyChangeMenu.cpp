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
#include "guiButton.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
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
	GUI_ID_KEY_PITCH_MOVE,
	GUI_ID_KEY_CHAT_BUTTON,
	GUI_ID_KEY_CMD_BUTTON,
	GUI_ID_KEY_CMD_LOCAL_BUTTON,
	GUI_ID_KEY_CONSOLE_BUTTON,
	GUI_ID_KEY_SNEAK_BUTTON,
	GUI_ID_KEY_DROP_BUTTON,
	GUI_ID_KEY_INVENTORY_BUTTON,
	GUI_ID_KEY_HOTBAR_PREV_BUTTON,
	GUI_ID_KEY_HOTBAR_NEXT_BUTTON,
	GUI_ID_KEY_MUTE_BUTTON,
	GUI_ID_KEY_DEC_VOLUME_BUTTON,
	GUI_ID_KEY_INC_VOLUME_BUTTON,
	GUI_ID_KEY_RANGE_BUTTON,
	GUI_ID_KEY_ZOOM_BUTTON,
	GUI_ID_KEY_CAMERA_BUTTON,
	GUI_ID_KEY_MINIMAP_BUTTON,
	GUI_ID_KEY_SCREENSHOT_BUTTON,
	GUI_ID_KEY_CHATLOG_BUTTON,
	GUI_ID_KEY_HUD_BUTTON,
	GUI_ID_KEY_FOG_BUTTON,
	GUI_ID_KEY_DEC_RANGE_BUTTON,
	GUI_ID_KEY_INC_RANGE_BUTTON,
	GUI_ID_KEY_AUTOFWD_BUTTON,
	// other
	GUI_ID_CB_AUX1_DESCENDS,
	GUI_ID_CB_DOUBLETAP_JUMP,
	GUI_ID_CB_AUTOJUMP,
};

GUIKeyChangeMenu::GUIKeyChangeMenu(gui::IGUIEnvironment* env,
				gui::IGUIElement* parent, s32 id, IMenuManager *menumgr) :
GUIModalMenu(env, parent, id, menumgr)
{
	init_keys();
	for (key_setting *ks : key_settings)
		key_used.push_back(ks->key);
}

GUIKeyChangeMenu::~GUIKeyChangeMenu()
{
	removeChildren();

	for (key_setting *ks : key_settings) {
		delete[] ks->button_name;
		delete ks;
	}
	key_settings.clear();
}

void GUIKeyChangeMenu::removeChildren()
{
	const core::list<gui::IGUIElement*> &children = getChildren();
	core::list<gui::IGUIElement*> children_copy;
	for (gui::IGUIElement*i : children) {
		children_copy.push_back(i);
	}

	for (gui::IGUIElement *i : children_copy) {
		i->remove();
	}
}

void GUIKeyChangeMenu::regenerateGui(v2u32 screensize)
{
	removeChildren();

	const float s = m_gui_scale;
	DesiredRect = core::rect<s32>(
		screensize.X / 2 - 835 * s / 2,
		screensize.Y / 2 - 430 * s / 2,
		screensize.X / 2 + 835 * s / 2,
		screensize.Y / 2 + 430 * s / 2
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	v2s32 topleft(0, 0);

	{
		core::rect<s32> rect(0, 0, 600 * s, 40 * s);
		rect += topleft + v2s32(25 * s, 3 * s);
		//gui::IGUIStaticText *t =
		const wchar_t *text = wgettext("Keybindings. (If this menu screws up, remove stuff from minetest.conf)");
		Environment->addStaticText(text,
								   rect, false, true, this, -1);
		delete[] text;
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	// Build buttons

	v2s32 offset(25 * s, 60 * s);

	for(size_t i = 0; i < key_settings.size(); i++)
	{
		key_setting *k = key_settings.at(i);
		{
			core::rect<s32> rect(0, 0, 150 * s, 20 * s);
			rect += topleft + v2s32(offset.X, offset.Y);
			Environment->addStaticText(k->button_name, rect, false, true, this, -1);
		}

		{
			core::rect<s32> rect(0, 0, 100 * s, 30 * s);
			rect += topleft + v2s32(offset.X + 150 * s, offset.Y - 5 * s);
			const wchar_t *text = wgettext(k->key.name());
			k->button = GUIButton::addButton(Environment, rect, this, k->id, text);
			delete[] text;
		}
		if ((i + 1) % KMaxButtonPerColumns == 0) {
			offset.X += 260 * s;
			offset.Y = 60 * s;
		} else {
			offset += v2s32(0, 25 * s);
		}
	}

	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y + 5 * s;
		u32 option_w = 180 * s;
		{
			core::rect<s32> rect(0, 0, option_w, 30 * s);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t *text = wgettext("\"Special\" = climb down");
			Environment->addCheckBox(g_settings->getBool("aux1_descends"), rect, this,
					GUI_ID_CB_AUX1_DESCENDS, text);
			delete[] text;
		}
		offset += v2s32(0, 25 * s);
	}

	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y + 5 * s;
		u32 option_w = 280 * s;
		{
			core::rect<s32> rect(0, 0, option_w, 30 * s);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t *text = wgettext("Double tap \"jump\" to toggle fly");
			Environment->addCheckBox(g_settings->getBool("doubletap_jump"), rect, this,
					GUI_ID_CB_DOUBLETAP_JUMP, text);
			delete[] text;
		}
		offset += v2s32(0, 25 * s);
	}

	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y + 5 * s;
		u32 option_w = 280;
		{
			core::rect<s32> rect(0, 0, option_w, 30 * s);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t *text = wgettext("Automatic jumping");
			Environment->addCheckBox(g_settings->getBool("autojump"), rect, this,
					GUI_ID_CB_AUTOJUMP, text);
			delete[] text;
		}
		offset += v2s32(0, 25);
	}

	{
		core::rect<s32> rect(0, 0, 100 * s, 30 * s);
		rect += topleft + v2s32(size.X / 2 - 105 * s, size.Y - 40 * s);
		const wchar_t *text =  wgettext("Save");
		GUIButton::addButton(Environment, rect, this, GUI_ID_BACK_BUTTON, text);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 100 * s, 30 * s);
		rect += topleft + v2s32(size.X / 2 + 5 * s, size.Y - 40 * s);
		const wchar_t *text = wgettext("Cancel");
		GUIButton::addButton(Environment, rect, this, GUI_ID_ABORT_BUTTON, text);
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
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
}

bool GUIKeyChangeMenu::acceptInput()
{
	for (key_setting *k : key_settings) {
		g_settings->set(k->setting_name, k->key.sym());
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CB_AUX1_DESCENDS);
		if(e && e->getType() == gui::EGUIET_CHECK_BOX)
			g_settings->setBool("aux1_descends", ((gui::IGUICheckBox*)e)->isChecked());
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CB_DOUBLETAP_JUMP);
		if(e && e->getType() == gui::EGUIET_CHECK_BOX)
			g_settings->setBool("doubletap_jump", ((gui::IGUICheckBox*)e)->isChecked());
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CB_AUTOJUMP);
		if(e && e->getType() == gui::EGUIET_CHECK_BOX)
			g_settings->setBool("autojump", ((gui::IGUICheckBox*)e)->isChecked());
	}

	clearKeyCache();

	g_gamecallback->signalKeyConfigChange();

	return true;
}

bool GUIKeyChangeMenu::resetMenu()
{
	if (activeKey >= 0)
	{
		for (key_setting *k : key_settings) {
			if (k->id == activeKey) {
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
			&& event.KeyInput.PressedDown) {

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
			const wchar_t *text = wgettext("Key already in use");
			this->key_used_text = Environment->addStaticText(text,
					rect, false, true, this, -1);
			delete[] text;
			//infostream << "Key already in use" << std::endl;
		}

		// But go on
		{
			key_setting *k = NULL;
			for (key_setting *ks : key_settings) {
				if (ks->id == activeKey) {
					k = ks;
					break;
				}
			}
			FATAL_ERROR_IF(k == NULL, "Key setting not found");
			k->key = kp;
			const wchar_t *text = wgettext(k->key.name());
			k->button->setText(text);
			delete[] text;

			this->key_used.push_back(kp);

			// Allow characters made with shift
			if(shift_went_down){
				shift_down = true;
				return false;
			}

			activeKey = -1;
			return true;
		}
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
				default:
					key_setting *k = NULL;

					for (key_setting *ks : key_settings) {
						if (ks->id == event.GUIEvent.Caller->getID()) {
							k = ks;
							break;
						}
					}
					FATAL_ERROR_IF(k == NULL, "Key setting not found");

					resetMenu();
					shift_down = false;
					activeKey = event.GUIEvent.Caller->getID();
					const wchar_t *text = wgettext("press key");
					k->button->setText(text);
					delete[] text;
					this->key_used.erase(std::remove(this->key_used.begin(),
							this->key_used.end(), k->key), this->key_used.end());
					break;
			}
			Environment->setFocus(this);
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
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

void GUIKeyChangeMenu::init_keys()
{
	this->add_key(GUI_ID_KEY_FORWARD_BUTTON,   wgettext("Forward"),          "keymap_forward");
	this->add_key(GUI_ID_KEY_BACKWARD_BUTTON,  wgettext("Backward"),         "keymap_backward");
	this->add_key(GUI_ID_KEY_LEFT_BUTTON,      wgettext("Left"),             "keymap_left");
	this->add_key(GUI_ID_KEY_RIGHT_BUTTON,     wgettext("Right"),            "keymap_right");
	this->add_key(GUI_ID_KEY_USE_BUTTON,       wgettext("Special"),          "keymap_special1");
	this->add_key(GUI_ID_KEY_JUMP_BUTTON,      wgettext("Jump"),             "keymap_jump");
	this->add_key(GUI_ID_KEY_SNEAK_BUTTON,     wgettext("Sneak"),            "keymap_sneak");
	this->add_key(GUI_ID_KEY_DROP_BUTTON,      wgettext("Drop"),             "keymap_drop");
	this->add_key(GUI_ID_KEY_INVENTORY_BUTTON, wgettext("Inventory"),        "keymap_inventory");
	this->add_key(GUI_ID_KEY_HOTBAR_PREV_BUTTON,wgettext("Prev. item"),      "keymap_hotbar_previous");
	this->add_key(GUI_ID_KEY_HOTBAR_NEXT_BUTTON,wgettext("Next item"),       "keymap_hotbar_next");
	this->add_key(GUI_ID_KEY_ZOOM_BUTTON,      wgettext("Zoom"),             "keymap_zoom");
	this->add_key(GUI_ID_KEY_CAMERA_BUTTON,    wgettext("Change camera"),    "keymap_camera_mode");
	this->add_key(GUI_ID_KEY_MINIMAP_BUTTON,   wgettext("Toggle minimap"),   "keymap_minimap");
	this->add_key(GUI_ID_KEY_FLY_BUTTON,       wgettext("Toggle fly"),       "keymap_freemove");
	this->add_key(GUI_ID_KEY_PITCH_MOVE,       wgettext("Toggle pitchmove"), "keymap_pitchmove");
	this->add_key(GUI_ID_KEY_FAST_BUTTON,      wgettext("Toggle fast"),      "keymap_fastmove");
	this->add_key(GUI_ID_KEY_NOCLIP_BUTTON,    wgettext("Toggle noclip"),    "keymap_noclip");
	this->add_key(GUI_ID_KEY_MUTE_BUTTON,      wgettext("Mute"),             "keymap_mute");
	this->add_key(GUI_ID_KEY_DEC_VOLUME_BUTTON,wgettext("Dec. volume"),      "keymap_decrease_volume");
	this->add_key(GUI_ID_KEY_INC_VOLUME_BUTTON,wgettext("Inc. volume"),      "keymap_increase_volume");
	this->add_key(GUI_ID_KEY_AUTOFWD_BUTTON,   wgettext("Autoforward"),      "keymap_autoforward");
	this->add_key(GUI_ID_KEY_CHAT_BUTTON,      wgettext("Chat"),             "keymap_chat");
	this->add_key(GUI_ID_KEY_SCREENSHOT_BUTTON,wgettext("Screenshot"),       "keymap_screenshot");
	this->add_key(GUI_ID_KEY_RANGE_BUTTON,     wgettext("Range select"),     "keymap_rangeselect");
	this->add_key(GUI_ID_KEY_DEC_RANGE_BUTTON, wgettext("Dec. range"),       "keymap_decrease_viewing_range_min");
	this->add_key(GUI_ID_KEY_INC_RANGE_BUTTON, wgettext("Inc. range"),       "keymap_increase_viewing_range_min");
	this->add_key(GUI_ID_KEY_CONSOLE_BUTTON,   wgettext("Console"),          "keymap_console");
	this->add_key(GUI_ID_KEY_CMD_BUTTON,       wgettext("Command"),          "keymap_cmd");
	this->add_key(GUI_ID_KEY_CMD_LOCAL_BUTTON, wgettext("Local command"),    "keymap_cmd_local");
	this->add_key(GUI_ID_KEY_HUD_BUTTON,       wgettext("Toggle HUD"),       "keymap_toggle_hud");
	this->add_key(GUI_ID_KEY_CHATLOG_BUTTON,   wgettext("Toggle chat log"),  "keymap_toggle_chat");
	this->add_key(GUI_ID_KEY_FOG_BUTTON,       wgettext("Toggle fog"),       "keymap_toggle_fog");
}
