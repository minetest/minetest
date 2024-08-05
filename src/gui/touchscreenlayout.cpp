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

#include "touchscreenlayout.h"
#include "gettext.h"
#include "client/renderingengine.h"
#include "settings.h"
#include "convert_json.h"
#include <json/json.h>

#include "IGUIStaticText.h"
#include "IGUIFont.h"

const char *button_names[] = {
	"jump",
	"sneak",
	"zoom",
	"aux1",
	"overflow",

	"chat",
	"inventory",
	"drop",
	"exit",

	"fly",
	"fast",
	"noclip",
	"debug",
	"camera",
	"range",
	"minimap",
	"toggle_chat",

	"joystick_off",
	"joystick_bg",
	"joystick_center",
};

// compare with GUIKeyChangeMenu::init_keys
const char *button_titles[] = {
	N_("Jump"),
	N_("Sneak"),
	N_("Zoom"),
	N_("Aux1"),
	N_("Overflow menu"),

	N_("Chat"),
	N_("Inventory"),
	N_("Drop"),
	N_("Exit"),

	N_("Toggle fly"),
	N_("Toggle fast"),
	N_("Toggle noclip"),
	N_("Toggle debug"),
	N_("Change camera"),
	N_("Range select"),
	N_("Toggle minimap"),
	N_("Toggle chat log"),

	N_("Joystick"),
	N_("Joystick"),
	N_("Joystick"),
};

const char *button_image_names[] = {
	"jump_btn.png",
	"down.png",
	"zoom.png",
	"aux1_btn.png",
	"overflow_btn.png",

	"chat_btn.png",
	"inventory_btn.png",
	"drop_btn.png",
	"exit_btn.png",

	"fly_btn.png",
	"fast_btn.png",
	"noclip_btn.png",
	"debug_btn.png",
	"camera_btn.png",
	"rangeview_btn.png",
	"minimap_btn.png",
	"chat_hide_btn.png", // FIXME

	"joystick_off.png",
	"joystick_bg.png",
	"joystick_center.png",
};

bool ButtonLayout::isButtonAllowed(touch_gui_button_id id)
{
	return id != joystick_off_id && id != joystick_bg_id && id != joystick_center_id &&
			id != touch_gui_button_id_END;
}

bool ButtonLayout::isButtonRequired(touch_gui_button_id id)
{
	return id == overflow_id;
}

s32 ButtonLayout::getButtonSize(v2u32 screensize)
{
	return std::min(screensize.Y / 4.5f,
			RenderingEngine::getDisplayDensity() * 65.0f *
					g_settings->getFloat("hud_scaling"));
}

ButtonLayout ButtonLayout::getDefault(v2u32 screensize)
{
	u32 button_size = getButtonSize(screensize);

	return {{
		{jump_id, {
			v2s32(screensize.X - 1.0f * button_size, screensize.Y - 0.5f * button_size),
			button_size,
		}},
		{sneak_id, {
			v2s32(screensize.X - 2.5f * button_size, screensize.Y - 0.5f * button_size),
			button_size,
		}},
		{zoom_id, {
			v2s32(screensize.X - 0.75f * button_size, screensize.Y - 3.5f * button_size),
			button_size,
		}},
		{aux1_id, {
			v2s32(screensize.X - 0.75f * button_size, screensize.Y - 2.0f * button_size),
			button_size,
		}},
		{overflow_id, {
			v2s32(screensize.X - 0.75f * button_size, screensize.Y - 5.0f * button_size),
			button_size,
		}},
	}};
}

ButtonLayout ButtonLayout::loadFromSettings(v2u32 screensize)
{
	bool restored = false;
	ButtonLayout layout;

	std::string str = g_settings->get("touch_layout");
	if (!str.empty()) {
		std::istringstream iss(str);
		try {
			layout.deserializeJson(iss);
			restored = true;
		} catch (const Json::Exception &e) {
			warningstream << "Could not parse touchscreen layout: " << e.what() << std::endl;
		}
	}

	if (!restored)
		return ButtonLayout::getDefault(screensize);

	return layout;
}

std::unordered_map<touch_gui_button_id, irr_ptr<video::ITexture>> ButtonLayout::texture_cache;

video::ITexture *ButtonLayout::getTexture(touch_gui_button_id btn, ISimpleTextureSource *tsrc)
{
	if (texture_cache.count(btn) > 0)
		return texture_cache.at(btn).get();

	video::ITexture *tex = tsrc->getTexture(button_image_names[btn]);
	if (!tex)
		// necessary in the mainmenu
		tex = tsrc->getTexture(porting::path_share + "/textures/base/pack/" +
				button_image_names[btn]);
	irr_ptr<video::ITexture> ptr;
	ptr.grab(tex);
	texture_cache[btn] = ptr;
	return tex;
}

void ButtonLayout::clearTextureCache()
{
	texture_cache.clear();
}

core::recti ButtonLayout::getRect(touch_gui_button_id btn, const ButtonMeta &meta, ISimpleTextureSource *tsrc)
{
	v2u32 orig_size = getTexture(btn, tsrc)->getOriginalSize();
	v2s32 size((f32)orig_size.X / (f32)orig_size.Y * meta.height, meta.height);
	return core::recti(meta.pos - size / 2, core::dimension2di(size));
}

std::vector<touch_gui_button_id> ButtonLayout::getMissingButtons()
{
	std::vector<touch_gui_button_id> missing_buttons;
	for (u8 i = 0; i < touch_gui_button_id_END; i++) {
		touch_gui_button_id btn = (touch_gui_button_id)i;
		if (isButtonAllowed(btn) && layout.count(btn) == 0)
			missing_buttons.push_back(btn);
	}
	return missing_buttons;
}

void ButtonLayout::serializeJson(std::ostream &os) const
{
	Json::Value root = Json::objectValue;

	for (const auto &[id, meta] : layout) {
		Json::Value button = Json::objectValue;
		button["x"] = meta.pos.X;
		button["y"] = meta.pos.Y;
		button["height"] = meta.height;

		root[button_names[id]] = button;
	}

	fastWriteJson(root, os);
}

static touch_gui_button_id button_name_to_id(const std::string &name)
{
	for (u8 i = 0; i < touch_gui_button_id_END; i++) {
		if (name == button_names[i])
			return (touch_gui_button_id)i;
	}
	return touch_gui_button_id_END;
}

void ButtonLayout::deserializeJson(std::istream &is)
{
	Json::Value root;
	is >> root;

	layout.clear();

	Json::ValueIterator iter;
	for (iter = root.begin(); iter != root.end(); iter++) {
		touch_gui_button_id id = button_name_to_id(iter.name());
		if (!isButtonAllowed(id))
			continue;

		Json::Value &value = *iter;
		if (!value.isObject())
			continue;

		ButtonMeta meta;

		if (!value["x"].isInt() || !value["y"].isInt() || !value["height"].isInt())
			continue;
		meta.pos.X = value["x"].asInt();
		meta.pos.Y = value["y"].asInt();
		meta.height = value["height"].asInt();

		layout.emplace(id, meta);
	}
}

void layout_button_grid(v2u32 screensize, ISimpleTextureSource *tsrc,
		const std::vector<touch_gui_button_id> &buttons,
		// pos refers to the center of the button
		const std::function<void(touch_gui_button_id btn, v2s32 pos, core::recti rect)> &callback)
{
	s32 cols = 4;
	s32 rows = 3;
	f32 screen_aspect = (f32)screensize.X / (f32)screensize.Y;
	while ((s32)buttons.size() > cols * rows) {
		f32 aspect = (f32)cols / (f32)rows;
		if (aspect > screen_aspect)
			rows++;
		else
			cols++;
	}

	u32 btn_height = ButtonLayout::getButtonSize(screensize);
	v2s32 spacing(screensize.X / (cols + 1), screensize.Y / (rows + 1));
	v2s32 pos(spacing);

	for (touch_gui_button_id btn : buttons) {
		core::recti rect = ButtonLayout::getRect(
				btn, ButtonMeta{pos, btn_height}, tsrc);

		if (rect.LowerRightCorner.X > (s32)screensize.X) {
			pos.X = spacing.X;
			pos.Y += spacing.Y;
			rect = ButtonLayout::getRect(btn, ButtonMeta{pos, btn_height}, tsrc);
		}

		callback(btn, pos, rect);

		pos.X += spacing.X;
	}
}

void make_button_grid_title(gui::IGUIStaticText *text, touch_gui_button_id btn, v2s32 pos, core::recti rect)
{
	std::wstring str = wstrgettext(button_titles[btn]);
	text->setText(str.c_str());
	gui::IGUIFont *font = text->getActiveFont();
	core::dimension2du dim = font->getDimension(str.c_str());
	dim = core::dimension2du(dim.Width * 1.25f, dim.Height * 1.25f); // avoid clipping
	text->setRelativePosition(core::recti(pos.X - dim.Width / 2, rect.LowerRightCorner.Y,
			pos.X + dim.Width / 2, rect.LowerRightCorner.Y + dim.Height));
	text->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
}
