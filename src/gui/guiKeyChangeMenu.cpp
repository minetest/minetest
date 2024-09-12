// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>
// Copyright (C) 2013 teddydestodes <derkomtur@schattengang.net>

#include "guiKeyChangeMenu.h"
#include "filesys.h"
#include "gettext.h"
#include "porting.h"
#include <sstream>
#include <vector>

struct setting_entry {
	const std::string display_name;
	const std::string setting_name;
};

// compare with button_titles in touchcontrols.cpp
static const std::vector<setting_entry> key_settings {
	{N_("Forward"),          "keymap_forward"},
	{N_("Backward"),         "keymap_backward"},
	{N_("Left"),             "keymap_left"},
	{N_("Right"),            "keymap_right"},
	{N_("Aux1"),             "keymap_aux1"},
	{N_("Jump"),             "keymap_jump"},
	{N_("Sneak"),            "keymap_sneak"},
	{N_("Drop"),             "keymap_drop"},
	{N_("Inventory"),        "keymap_inventory"},
	{N_("Prev. item"),       "keymap_hotbar_previous"},
	{N_("Next item"),        "keymap_hotbar_next"},
	{N_("Zoom"),             "keymap_zoom"},
	{N_("Change camera"),    "keymap_camera_mode"},
	{N_("Toggle minimap"),   "keymap_minimap"},
	{N_("Toggle fly"),       "keymap_freemove"},
	{N_("Toggle pitchmove"), "keymap_pitchmove"},
	{N_("Toggle fast"),      "keymap_fastmove"},
	{N_("Toggle noclip"),    "keymap_noclip"},
	{N_("Mute"),             "keymap_mute"},
	{N_("Dec. volume"),      "keymap_decrease_volume"},
	{N_("Inc. volume"),      "keymap_increase_volume"},
	{N_("Autoforward"),      "keymap_autoforward"},
	{N_("Chat"),             "keymap_chat"},
	{N_("Screenshot"),       "keymap_screenshot"},
	{N_("Range select"),     "keymap_rangeselect"},
	{N_("Dec. range"),       "keymap_decrease_viewing_range_min"},
	{N_("Inc. range"),       "keymap_increase_viewing_range_min"},
	{N_("Console"),          "keymap_console"},
	{N_("Command"),          "keymap_cmd"},
	{N_("Local command"),    "keymap_cmd_local"},
	{N_("Block bounds"),     "keymap_toggle_block_bounds"},
	{N_("Toggle HUD"),       "keymap_toggle_hud"},
	{N_("Toggle chat log"),  "keymap_toggle_chat"},
	{N_("Toggle fog"),       "keymap_toggle_fog"}
};

static const std::vector<setting_entry> control_settings {
	{N_("\"Aux1\" = climb down"), "aux1_descends"},
	{N_("Double tap \"jump\" to toggle fly"), "doubletap_jump"},
	{N_("Automatic jumping"), "autojump"}
};

void GUIKeyChangeMenu::KeyChangeFormspecHandler::gotText(const StringMap &fields)
{
	if (fields.find("btn_save") != fields.end()) {
		form->saveSettings();
		return;
	}
	if (fields.find("btn_cancel") != fields.end())
		return;

	if (const auto &field = fields.find("scrollbar"); field != fields.end()) {
		if (const auto &stringval = field->second; str_starts_with(stringval, "CHG:"))
			form->scroll_position = stof(stringval.substr(4));
	}

	for (const auto &field: fields) {
		const auto &name = field.first;
		if (str_starts_with(name, "checkbox_"))
			form->control_options[name.substr(9)] = (field.second == "true");
		else if (str_starts_with(name, "btn_clear_"))
			form->keymap[name.substr(10)] = KeyPress();
		else if (str_starts_with(name, "btn_set_")) {
			const auto &setting = name.substr(8);
			if (setting == form->active_key)
				form->active_key = "";
			else
				form->active_key = setting;
		}
	}

	form->updateFormSource();
}

std::string GUIKeyChangeMenu::getTexture(const std::string &name) const
{
	return has_client ? name : porting::path_share + "/textures/base/pack/" + name;
}

void GUIKeyChangeMenu::updateFormSource(const std::string &message)
{
	// this is more or less make_scrollbaroptions_for_scroll_container from Lua
	float formspec_height = 13;
	float control_options_offset = formspec_height - 2 - (control_settings.size()/2)*0.5;

	std::ostringstream os;
	os << "formspec_version[7]"
		<< "size[15.5," << formspec_height << "]"
		<< "button_exit[3.5," << formspec_height-1.5 << ";4,1;btn_save;" << strgettext("Save") << "]"
		<< "button_exit[8," << formspec_height-1.5 << ";4,1;btn_cancel;" << strgettext("Cancel") << "]";

	if (!message.empty()) {
		os << "label[0.5," << formspec_height - 2 << ";" << message << "]";
		control_options_offset -= 0.5;
	}

	float pos_x = 0.5;
	float pos_y = control_options_offset;

	bool alt = false;
	for (const auto &setting: control_settings) {
		os << "checkbox[" << pos_x << "," << pos_y << ";"
			<< "checkbox_" << setting.setting_name << ";"
			<< strgettext(setting.display_name) << ";"
			<< (getControlOption(setting.setting_name)?"true":"false") << "]";
		if (alt) {
			pos_x = 0.5;
			pos_y += 0.5;
		} else {
			pos_x = 8;
		}
		alt = !alt;
	}

	float container_height = control_options_offset - 1;
	float container_total_height = 1.5*key_settings.size()-0.5;
	float container_scroll_max = (container_total_height-container_height)*10;
	float container_scroll_thumb = container_height / container_total_height * container_scroll_max;
	os << "scrollbaroptions[min=0;max=" << container_scroll_max << ";"
		<< "thumbsize=" << container_scroll_thumb <<  "]"
		<< "scrollbar[14.5,0.5;0.5," << container_height << ";vertical;scrollbar;" << scroll_position << "]"
		<< "scroll_container[0.5,0.5;14," << container_height << ";scrollbar;vertical]";

	pos_y = 0;
	for (const auto &setting: key_settings) {
		const auto &key = getKeySetting(setting.setting_name);
		std::string keydesc(key.name());
		bool show_clear = false;
		if (!keydesc.empty()) {
			keydesc = strgettext(keydesc);
			show_clear = true;
		}
		if (active_key == setting.setting_name) {
			keydesc = strgettext("press key");
			show_clear = false;
		}

		float btn_width = show_clear ? 5 : 6;
		os << "label[0," << pos_y + 0.5 << ";" << strgettext(setting.display_name) << "]"
			<< "button[7.5," << pos_y << ";" << btn_width << ",1;btn_set_" << setting.setting_name << ";"
			<< keydesc << "]";
		if (show_clear)
			os << "image_button[12.5," << pos_y << ";1,1;" << getTexture("clear.png")
				<< ";btn_clear_" << setting.setting_name << ";]";
		pos_y += 1.5;
	}

	os << "scroll_container_end[]";

	// The formspec text is deleted by guiFormSpecMenu
	auto *form_src = new FormspecFormSource(os.str());
	setFormSource(form_src);
}

void GUIKeyChangeMenu::saveSettings()
{
	for (const auto &setting: keymap)
		g_settings->set(setting.first, setting.second.sym());

	for (const auto &setting: control_options)
		g_settings->setBool(setting.first, setting.second);

	clearKeyCache();
	g_gamecallback->signalKeyConfigChange();
}

bool GUIKeyChangeMenu::OnEvent(const SEvent &event)
{
	if (!active_key.empty() && event.EventType == EET_KEY_INPUT_EVENT
			&& event.KeyInput.PressedDown) {
		if (event.KeyInput.Key == irr::KEY_ESCAPE) {
			// Do not allow binding to the escape key
			updateFormSource(fmtgettext("Binding to the Escape key is not allowed"));
			return true;
		}

		KeyPress kp(event.KeyInput);
		keymap[active_key] = kp;

		std::string conflict;
		for (const auto &setting: key_settings) {
			if (setting.setting_name == active_key)
				continue;
			if (getKeySetting(setting.setting_name) == kp) {
				conflict = strgettext(setting.display_name);
				break;
			}
		}
		active_key = "";
		if (conflict.empty())
			updateFormSource();
		else
			updateFormSource(fmtgettext("Key already in use for another action: %s", conflict.c_str()));

		return false;
	}
	return super::OnEvent(event);
}
