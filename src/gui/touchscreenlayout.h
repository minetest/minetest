/*
Minetest
Copyright (C) 2024 grorp, Gregor Parzefall <grorp@posteo.de>

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

#pragma once

#include "client/texturesource.h"
#include "irr_ptr.h"
#include "irrlichttypes_bloated.h"
#include "ITexture.h"
#include "IGUIStaticText.h"
#include <unordered_map>

enum touch_gui_button_id : u8
{
	jump_id = 0,
	sneak_id,
	zoom_id,
	aux1_id,
	overflow_id,

	// formerly "rare controls bar"
	chat_id,
	inventory_id,
	drop_id,
	exit_id,

	// formerly "settings bar"
	fly_id,
	fast_id,
	noclip_id,
	debug_id,
	camera_id,
	range_id,
	minimap_id,
	toggle_chat_id,

	// the joystick
	joystick_off_id,
	joystick_bg_id,
	joystick_center_id,

	touch_gui_button_id_END,
};

extern const char *button_names[];
extern const char *button_titles[];
extern const char *button_image_names[];

struct ButtonMeta {
	// pos refers to the center of the button
	v2s32 pos;
	u32 height;
};

struct ButtonLayout {
	std::unordered_map<touch_gui_button_id, ButtonMeta> layout;

	static bool isButtonAllowed(touch_gui_button_id id);
	static bool isButtonRequired(touch_gui_button_id id);
	static s32 getButtonSize(v2u32 screensize);
    static ButtonLayout getDefault(v2u32 screensize);
	static ButtonLayout loadFromSettings(v2u32 screensize);

    static video::ITexture *getTexture(touch_gui_button_id btn, ISimpleTextureSource *tsrc);
	static void clearTextureCache();
	static core::recti getRect(touch_gui_button_id btn, const ButtonMeta &meta, ISimpleTextureSource *tsrc);

	std::vector<touch_gui_button_id> getMissingButtons();

	void serializeJson(std::ostream &os) const;
	void deserializeJson(std::istream &is);

private:
	static std::unordered_map<touch_gui_button_id, irr_ptr<video::ITexture>> texture_cache;
};

void layout_button_grid(v2u32 screensize, ISimpleTextureSource *tsrc,
		const std::vector<touch_gui_button_id> &buttons,
		// pos refers to the center of the button
		const std::function<void(touch_gui_button_id btn, v2s32 pos, core::recti rect)> &callback);

void make_button_grid_title(gui::IGUIStaticText *text,
		touch_gui_button_id btn,v2s32 pos, core::recti rect);
