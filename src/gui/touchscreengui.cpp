/*
Copyright (C) 2014 sapier
Copyright (C) 2018 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>
Copyright (C) 2024 grorp, Gregor Parzefall
		<gregor.parzefall@posteo.de>

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

#include "touchscreengui.h"

#include "gettime.h"
#include "irr_v2d.h"
#include "log.h"
#include "porting.h"
#include "settings.h"
#include "client/guiscalingfilter.h"
#include "client/keycode.h"
#include "client/renderingengine.h"
#include "util/numeric.h"
#include <ISceneCollisionManager.h>

#include <iostream>
#include <algorithm>

TouchScreenGUI *g_touchscreengui;

static const char *button_image_names[] = {
	"jump_btn.png",
	"down.png",
	"zoom.png",
	"aux1_btn.png",
	"gear_icon.png",
	"rare_controls.png",

	"fly_btn.png",
	"noclip_btn.png",
	"fast_btn.png",
	"debug_btn.png",
	"camera_btn.png",
	"rangeview_btn.png",
	"minimap_btn.png",
	"",

	"chat_btn.png",
	"inventory_btn.png",
	"drop_btn.png",
	"exit_btn.png",

	"joystick_off.png",
	"joystick_bg.png",
	"joystick_center.png",
};

static void load_button_texture(IGUIImage *gui_button, const std::string &path,
		const recti &button_rect, ISimpleTextureSource *tsrc, video::IVideoDriver *driver)
{
	video::ITexture *texture = guiScalingImageButton(driver,
			tsrc->getTexture(path), button_rect.getWidth(),
			button_rect.getHeight());
	gui_button->setImage(texture);
	gui_button->setScaleImage(true);
}

void button_info::emitAction(bool action, video::IVideoDriver *driver,
		IEventReceiver *receiver, ISimpleTextureSource *tsrc)
{
	if (keycode == KEY_UNKNOWN)
		return;

	SEvent translated{};
	translated.EventType        = EET_KEY_INPUT_EVENT;
	translated.KeyInput.Key     = keycode;
	translated.KeyInput.Control = false;
	translated.KeyInput.Shift   = false;
	translated.KeyInput.Char    = 0;

	if (action) {
		translated.KeyInput.PressedDown = true;
		receiver->OnEvent(translated);

		if (toggleable == button_info::FIRST_TEXTURE) {
			toggleable = button_info::SECOND_TEXTURE;
			load_button_texture(gui_button.get(), toggle_textures[1],
					gui_button->getRelativePosition(),
					tsrc, driver);
		} else if (toggleable == button_info::SECOND_TEXTURE) {
			toggleable = button_info::FIRST_TEXTURE;
			load_button_texture(gui_button.get(), toggle_textures[0],
					gui_button->getRelativePosition(),
					tsrc, driver);
		}
	} else {
		translated.KeyInput.PressedDown = false;
		receiver->OnEvent(translated);
	}
}

static bool buttons_handlePress(std::vector<button_info> &buttons, size_t pointer_id, IGUIElement *element,
		video::IVideoDriver *driver, IEventReceiver *receiver, ISimpleTextureSource *tsrc)
{
	if (!element)
		return false;

	for (button_info &btn : buttons) {
		if (btn.gui_button.get() == element) {
			assert(std::find(btn.pointer_ids.begin(), btn.pointer_ids.end(), pointer_id) == btn.pointer_ids.end());
			btn.pointer_ids.push_back(pointer_id);

			if (btn.pointer_ids.size() > 1)
				return true;

			btn.emitAction(true, driver, receiver, tsrc);
			btn.repeat_counter = -BUTTON_REPEAT_DELAY;
			return true;
		}
	}

	return false;
}


static bool buttons_handleRelease(std::vector<button_info> &buttons, size_t pointer_id,
		video::IVideoDriver *driver, IEventReceiver *receiver, ISimpleTextureSource *tsrc)
{
	for (button_info &btn : buttons) {
		auto it = std::find(btn.pointer_ids.begin(), btn.pointer_ids.end(), pointer_id);
		if (it != btn.pointer_ids.end()) {
			btn.pointer_ids.erase(it);

			if (!btn.pointer_ids.empty())
				return true;

			btn.emitAction(false, driver, receiver, tsrc);
			return true;
		}
	}

	return false;
}

static bool buttons_step(std::vector<button_info> &buttons, float dtime,
		video::IVideoDriver *driver, IEventReceiver *receiver, ISimpleTextureSource *tsrc)
{
	bool has_pointers = false;

	for (button_info &btn : buttons) {
		if (btn.pointer_ids.empty())
			continue;
		has_pointers = true;

		btn.repeat_counter += dtime;
		if (btn.repeat_counter < BUTTON_REPEAT_INTERVAL)
			continue;

		btn.emitAction(false, driver, receiver, tsrc);
		btn.emitAction(true, driver, receiver, tsrc);
		btn.repeat_counter = 0.0f;
	}

	return has_pointers;
}

static EKEY_CODE id_to_keycode(touch_gui_button_id id)
{
	EKEY_CODE code;
	// ESC isn't part of the keymap.
	if (id == exit_id)
		return KEY_ESCAPE;

	std::string key = "";
	switch (id) {
		case jump_id:
			key = "jump";
			break;
		case sneak_id:
			key = "sneak";
			break;
		case zoom_id:
			key = "zoom";
			break;
		case aux1_id:
			key = "aux1";
			break;
		case fly_id:
			key = "freemove";
			break;
		case noclip_id:
			key = "noclip";
			break;
		case fast_id:
			key = "fastmove";
			break;
		case debug_id:
			key = "toggle_debug";
			break;
		case camera_id:
			key = "camera_mode";
			break;
		case range_id:
			key = "rangeselect";
			break;
		case minimap_id:
			key = "minimap";
			break;
		case toggle_chat_id:
			key = "toggle_chat";
			break;
		case chat_id:
			key = "chat";
			break;
		case inventory_id:
			key = "inventory";
			break;
		case drop_id:
			key = "drop";
			break;
		default:
			break;
	}
	assert(!key.empty());
	std::string resolved = g_settings->get("keymap_" + key);
	try {
		code = keyname_to_keycode(resolved.c_str());
	} catch (UnknownKeycode &e) {
		code = KEY_UNKNOWN;
		warningstream << "TouchScreenGUI: Unknown key '" << resolved
			      << "' for '" << key << "', hiding button." << std::endl;
	}
	return code;
}

AutoHideButtonBar::AutoHideButtonBar(IrrlichtDevice *device, ISimpleTextureSource *tsrc,
		touch_gui_button_id starter_id, const std::string &starter_img,
		recti starter_rect, autohide_button_bar_dir dir) :
			m_driver(device->getVideoDriver()),
			m_guienv(device->getGUIEnvironment()),
			m_receiver(device->getEventReceiver()),
			m_texturesource(tsrc)
{
	m_upper_left = starter_rect.UpperLeftCorner;
	m_lower_right = starter_rect.LowerRightCorner;

	IGUIImage *starter_gui_button = m_guienv->addImage(starter_rect, nullptr,
			starter_id);
	load_button_texture(starter_gui_button, starter_img, starter_rect,
			m_texturesource, m_driver);

	m_starter = grab_gui_element<IGUIImage>(starter_gui_button);
	m_dir = dir;
}

void AutoHideButtonBar::addButton(touch_gui_button_id id, const std::string &image)
{
	int button_size = 0;

	if (m_dir == AHBB_Dir_Top_Bottom || m_dir == AHBB_Dir_Bottom_Top)
		button_size = m_lower_right.X - m_upper_left.X;
	else
		button_size = m_lower_right.Y - m_upper_left.Y;

	recti current_button;

	if (m_dir == AHBB_Dir_Right_Left || m_dir == AHBB_Dir_Left_Right) {
		int x_start = 0;
		int x_end = 0;

		if (m_dir == AHBB_Dir_Left_Right) {
			x_start = m_lower_right.X + button_size * 1.25f * m_buttons.size()
					+ button_size * 0.25f;
			x_end = x_start + button_size;
		} else {
			x_end = m_upper_left.X - button_size * 1.25f * m_buttons.size()
					- button_size * 0.25f;
			x_start = x_end - button_size;
		}

		current_button = recti(x_start, m_upper_left.Y, x_end, m_lower_right.Y);
	} else {
		double y_start = 0;
		double y_end = 0;

		if (m_dir == AHBB_Dir_Top_Bottom) {
			y_start = m_lower_right.X + button_size * 1.25f * m_buttons.size()
					+ button_size * 0.25f;
			y_end = y_start + button_size;
		} else {
			y_end = m_upper_left.X - button_size * 1.25f * m_buttons.size()
					- button_size * 0.25f;
			y_start = y_end - button_size;
		}

		current_button = recti(m_upper_left.X, y_start, m_lower_right.Y, y_end);
	}

	IGUIImage *btn_gui_button = m_guienv->addImage(current_button, nullptr, id);
	btn_gui_button->setVisible(false);
	btn_gui_button->setEnabled(false);
	load_button_texture(btn_gui_button, image, current_button, m_texturesource, m_driver);

	button_info btn{};
	btn.keycode = id_to_keycode(id);
	btn.gui_button = grab_gui_element<IGUIImage>(btn_gui_button);
	m_buttons.push_back(btn);
}

void AutoHideButtonBar::addToggleButton(touch_gui_button_id id,
		const std::string &image_1, const std::string &image_2)
{
	addButton(id, image_1);
	button_info &btn = m_buttons.back();
	btn.toggleable = button_info::FIRST_TEXTURE;
	btn.toggle_textures[0] = image_1;
	btn.toggle_textures[1] = image_2;
}

bool AutoHideButtonBar::handlePress(size_t pointer_id, IGUIElement *element)
{
	if (m_active) {
		return buttons_handlePress(m_buttons, pointer_id, element, m_driver,
				m_receiver, m_texturesource);
	}
	if (m_starter.get() == element) {
		activate();
		return true;
	}
	return false;
}

bool AutoHideButtonBar::handleRelease(size_t pointer_id)
{
	return buttons_handleRelease(m_buttons, pointer_id, m_driver,
			m_receiver, m_texturesource);
}

void AutoHideButtonBar::step(float dtime)
{
	// Since buttons can stay pressed after the buttonbar is deactivated,
	// we call the step function even if the buttonbar is inactive.
	bool has_pointers = buttons_step(m_buttons, dtime, m_driver, m_receiver,
			m_texturesource);

	if (m_active) {
		if (!has_pointers) {
			m_timeout += dtime;
			if (m_timeout > BUTTONBAR_HIDE_DELAY)
				deactivate();
		} else {
			m_timeout = 0.0f;
		}
	}

}

void AutoHideButtonBar::updateVisibility() {
	bool starter_visible = m_visible && !m_active;
	bool inner_visible = m_visible && m_active;

	m_starter->setVisible(starter_visible);
	m_starter->setEnabled(starter_visible);

	for (auto &button : m_buttons) {
		button.gui_button->setVisible(inner_visible);
		button.gui_button->setEnabled(inner_visible);
	}
}

void AutoHideButtonBar::activate()
{
	m_active = true;
	m_timeout = 0.0f;
	updateVisibility();
}

void AutoHideButtonBar::deactivate()
{
	m_active = false;
	updateVisibility();
}

void AutoHideButtonBar::show()
{
	m_visible = true;
	updateVisibility();
}

void AutoHideButtonBar::hide()
{
	m_visible = false;
	updateVisibility();
}

TouchScreenGUI::TouchScreenGUI(IrrlichtDevice *device, ISimpleTextureSource *tsrc):
		m_device(device),
		m_guienv(device->getGUIEnvironment()),
		m_receiver(device->getEventReceiver()),
		m_texturesource(tsrc)
{
	m_touchscreen_threshold = g_settings->getU16("touchscreen_threshold");
	m_long_tap_delay = g_settings->getU16("touch_long_tap_delay");
	m_fixed_joystick = g_settings->getBool("fixed_virtual_joystick");
	m_joystick_triggers_aux1 = g_settings->getBool("virtual_joystick_triggers_aux1");
	m_screensize = m_device->getVideoDriver()->getScreenSize();
	m_button_size = MYMIN(m_screensize.Y / 4.5f,
			RenderingEngine::getDisplayDensity() * 65.0f *
					g_settings->getFloat("hud_scaling"));

	// Initialize joystick display "button".
	// Joystick is placed on the bottom left of screen.
	if (m_fixed_joystick) {
		m_joystick_btn_off = grab_gui_element<IGUIImage>(makeJoystickButton(joystick_off_id,
				recti(m_button_size,
						m_screensize.Y - m_button_size * 4,
						m_button_size * 4,
						m_screensize.Y - m_button_size), true));
	} else {
		m_joystick_btn_off = grab_gui_element<IGUIImage>(makeJoystickButton(joystick_off_id,
				recti(m_button_size,
						m_screensize.Y - m_button_size * 3,
						m_button_size * 3,
						m_screensize.Y - m_button_size), true));
	}

	m_joystick_btn_bg = grab_gui_element<IGUIImage>(makeJoystickButton(joystick_bg_id,
			recti(m_button_size,
					m_screensize.Y - m_button_size * 4,
					m_button_size * 4,
					m_screensize.Y - m_button_size), false));

	m_joystick_btn_center = grab_gui_element<IGUIImage>(makeJoystickButton(joystick_center_id,
			recti(0, 0, m_button_size, m_button_size), false));

	// init jump button
	addButton(jump_id, button_image_names[jump_id],
			recti(m_screensize.X - 1.75f * m_button_size,
					m_screensize.Y - m_button_size,
					m_screensize.X - 0.25f * m_button_size,
					m_screensize.Y));

	// init sneak button
	addButton(sneak_id, button_image_names[sneak_id],
			recti(m_screensize.X - 3.25f * m_button_size,
					m_screensize.Y - m_button_size,
					m_screensize.X - 1.75f * m_button_size,
					m_screensize.Y));

	// init zoom button
	addButton(zoom_id, button_image_names[zoom_id],
			recti(m_screensize.X - 1.25f * m_button_size,
					m_screensize.Y - 4 * m_button_size,
					m_screensize.X - 0.25f * m_button_size,
					m_screensize.Y - 3 * m_button_size));

	// init aux1 button
	if (!m_joystick_triggers_aux1)
		addButton(aux1_id, button_image_names[aux1_id],
				recti(m_screensize.X - 1.25f * m_button_size,
						m_screensize.Y - 2.5f * m_button_size,
						m_screensize.X - 0.25f * m_button_size,
						m_screensize.Y - 1.5f * m_button_size));

	AutoHideButtonBar &settings_bar = m_buttonbars.emplace_back(m_device, m_texturesource,
			settings_starter_id, button_image_names[settings_starter_id],
			recti(m_screensize.X - 1.25f * m_button_size,
					m_screensize.Y - (SETTINGS_BAR_Y_OFFSET + 1.0f) * m_button_size
							+ 0.5f * m_button_size,
					m_screensize.X - 0.25f * m_button_size,
					m_screensize.Y - SETTINGS_BAR_Y_OFFSET * m_button_size
							+ 0.5f * m_button_size),
			AHBB_Dir_Right_Left);

	const static touch_gui_button_id settings_bar_buttons[] {
		fly_id, noclip_id, fast_id, debug_id, camera_id, range_id, minimap_id,
	};
	for (auto id : settings_bar_buttons) {
		if (id_to_keycode(id) == KEY_UNKNOWN)
			continue;
		settings_bar.addButton(id, button_image_names[id]);
	}

	// Chat is shown by default, so chat_hide_btn.png is shown first.
	settings_bar.addToggleButton(toggle_chat_id,
			"chat_hide_btn.png", "chat_show_btn.png");

	AutoHideButtonBar &rare_controls_bar = m_buttonbars.emplace_back(m_device, m_texturesource,
			rare_controls_starter_id, button_image_names[rare_controls_starter_id],
			recti(0.25f * m_button_size,
					m_screensize.Y - (RARE_CONTROLS_BAR_Y_OFFSET + 1.0f) * m_button_size
							+ 0.5f * m_button_size,
					0.75f * m_button_size,
					m_screensize.Y - RARE_CONTROLS_BAR_Y_OFFSET * m_button_size
							+ 0.5f * m_button_size),
			AHBB_Dir_Left_Right);

	const static touch_gui_button_id rare_controls_bar_buttons[] {
		chat_id, inventory_id, drop_id, exit_id,
	};
	for (auto id : rare_controls_bar_buttons) {
		if (id_to_keycode(id) == KEY_UNKNOWN)
			continue;
		rare_controls_bar.addButton(id, button_image_names[id]);
	}
}

void TouchScreenGUI::addButton(touch_gui_button_id id, const std::string &image, const recti &rect)
{
	IGUIImage *btn_gui_button = m_guienv->addImage(rect, nullptr, id);
	load_button_texture(btn_gui_button, image, rect,
			m_texturesource, m_device->getVideoDriver());

	button_info &btn = m_buttons.emplace_back();
	btn.keycode = id_to_keycode(id);
	btn.gui_button = grab_gui_element<IGUIImage>(btn_gui_button);
}

IGUIImage *TouchScreenGUI::makeJoystickButton(touch_gui_button_id id,
		const recti &button_rect, bool visible)
{
	IGUIImage *btn_gui_button = m_guienv->addImage(button_rect, nullptr, id);
	btn_gui_button->setVisible(visible);
	load_button_texture(btn_gui_button, button_image_names[id], button_rect,
			m_texturesource, m_device->getVideoDriver());

	return btn_gui_button;
}

bool TouchScreenGUI::isHotbarButton(const SEvent &event)
{
	const v2s32 touch_pos = v2s32(event.TouchInput.X, event.TouchInput.Y);
	// check if hotbar item is pressed
	for (auto &[index, rect] : m_hotbar_rects) {
		if (rect.isPointInside(touch_pos)) {
			// We can't just emit a keypress event because the number keys
			// range from 1 to 9, but there may be more hotbar items.
			m_hotbar_selection = index;
			return true;
		}
	}
	return false;
}

std::optional<u16> TouchScreenGUI::getHotbarSelection()
{
	auto selection = m_hotbar_selection;
	m_hotbar_selection = std::nullopt;
	return selection;
}

void TouchScreenGUI::handleReleaseEvent(size_t pointer_id)
{
	// By the way: Android reuses pointer IDs, so m_pointer_pos[pointer_id]
	// will be overwritten soon anyway.
	m_pointer_downpos.erase(pointer_id);
	m_pointer_pos.erase(pointer_id);

	// handle buttons
	if (buttons_handleRelease(m_buttons, pointer_id, m_device->getVideoDriver(),
			m_receiver, m_texturesource))
		return;

	// handle buttonbars
	for (AutoHideButtonBar &bar : m_buttonbars) {
		if (bar.handleRelease(pointer_id))
			return;
	}

	if (m_has_move_id && pointer_id == m_move_id) {
		// handle the point used for moving view
		m_has_move_id = false;

		// If m_tap_state is already set to TapState::ShortTap, we must keep
		// that value. Otherwise, many short taps will be ignored if you tap
		// very fast.
		if (!m_move_has_really_moved && m_tap_state != TapState::LongTap) {
			m_tap_state = TapState::ShortTap;
		} else {
			m_tap_state = TapState::None;
		}
	}

	// handle joystick
	else if (m_has_joystick_id && pointer_id == m_joystick_id) {
		m_has_joystick_id = false;

		// reset joystick
		m_joystick_direction = 0.0f;
		m_joystick_speed = 0.0f;
		m_joystick_status_aux1 = false;
		applyJoystickStatus();

		m_joystick_btn_off->setVisible(true);
		m_joystick_btn_bg->setVisible(false);
		m_joystick_btn_center->setVisible(false);
	} else {
		infostream << "TouchScreenGUI::translateEvent released unknown button: "
				<< pointer_id << std::endl;
	}
}

void TouchScreenGUI::translateEvent(const SEvent &event)
{
	if (!m_visible) {
		infostream << "TouchScreenGUI::translateEvent got event but is not visible!"
				<< std::endl;
		return;
	}

	if (event.EventType != EET_TOUCH_INPUT_EVENT)
		return;

	const s32 half_button_size = m_button_size / 2.0f;
	const s32 fixed_joystick_range_sq = half_button_size * half_button_size * 3 * 3;
	const s32 X = event.TouchInput.X;
	const s32 Y = event.TouchInput.Y;
	const v2s32 touch_pos = v2s32(X, Y);
	const v2s32 fixed_joystick_center = v2s32(half_button_size * 5,
			m_screensize.Y - half_button_size * 5);
	const v2s32 dir_fixed = touch_pos - fixed_joystick_center;

	if (event.TouchInput.Event == ETIE_PRESSED_DOWN) {
		size_t pointer_id = event.TouchInput.ID;
		m_pointer_downpos[pointer_id] = touch_pos;
		m_pointer_pos[pointer_id] = touch_pos;

		IGUIElement *element = m_guienv->getRootGUIElement()->getElementFromPoint(touch_pos);

		// handle buttons
		if (buttons_handlePress(m_buttons, pointer_id, element,
				m_device->getVideoDriver(), m_receiver, m_texturesource)) {
			for (AutoHideButtonBar &bar : m_buttonbars)
				bar.deactivate();
			return;
		}

		// handle buttonbars
		for (AutoHideButtonBar &bar : m_buttonbars) {
			if (bar.handlePress(pointer_id, element)) {
				for (AutoHideButtonBar &other : m_buttonbars)
					if (other != bar)
						other.deactivate();
				return;
			}
		}

		// handle hotbar
		if (isHotbarButton(event)) {
			// already handled in isHotbarButton()
			for (AutoHideButtonBar &bar : m_buttonbars)
				bar.deactivate();
			return;
		}

		// handle non button events
		for (AutoHideButtonBar &bar : m_buttonbars) {
			if (bar.isActive()) {
				bar.deactivate();
				return;
			}
		}

		// Select joystick when joystick tapped (fixed joystick position) or
		// when left 1/3 of screen dragged (free joystick position)
		if ((m_fixed_joystick && dir_fixed.getLengthSQ() <= fixed_joystick_range_sq) ||
				(!m_fixed_joystick && X < m_screensize.X / 3.0f)) {
			// If we don't already have a starting point for joystick, make this the one.
			if (!m_has_joystick_id) {
				m_has_joystick_id           = true;
				m_joystick_id               = pointer_id;
				m_joystick_has_really_moved = false;

				m_joystick_btn_off->setVisible(false);
				m_joystick_btn_bg->setVisible(true);
				m_joystick_btn_center->setVisible(true);

				// If it's a fixed joystick, don't move the joystick "button".
				if (!m_fixed_joystick)
					m_joystick_btn_bg->setRelativePosition(
							touch_pos - half_button_size * 3);

				m_joystick_btn_center->setRelativePosition(
						touch_pos - half_button_size);
			}
		} else {
			// If we don't already have a moving point, make this the moving one.
			if (!m_has_move_id) {
				m_has_move_id              = true;
				m_move_id                  = pointer_id;
				m_move_has_really_moved    = false;
				m_move_downtime            = porting::getTimeMs();
				m_move_pos                 = touch_pos;
				// DON'T reset m_tap_state here, otherwise many short taps
				// will be ignored if you tap very fast.
				m_had_move_id              = true;
			}
		}
	}
	else if (event.TouchInput.Event == ETIE_LEFT_UP) {
		verbosestream << "Up event for pointerid: " << event.TouchInput.ID << std::endl;
		handleReleaseEvent(event.TouchInput.ID);
	} else {
		assert(event.TouchInput.Event == ETIE_MOVED);

		if (!(m_has_joystick_id && m_fixed_joystick) &&
				m_pointer_pos[event.TouchInput.ID] == touch_pos)
			return;

		const v2s32 dir_free_original = touch_pos - m_pointer_downpos[event.TouchInput.ID];
		const v2s32 free_joystick_center = m_pointer_pos[event.TouchInput.ID];
		const v2s32 dir_free = touch_pos - free_joystick_center;

		const double touch_threshold_sq = m_touchscreen_threshold * m_touchscreen_threshold;

		if (m_has_move_id && event.TouchInput.ID == m_move_id) {
			m_move_pos = touch_pos;
			m_pointer_pos[event.TouchInput.ID] = touch_pos;

			// update camera_yaw and camera_pitch
			const double d = g_settings->getFloat("touchscreen_sensitivity", 0.001f, 10.0f)
					* 6.0f / RenderingEngine::getDisplayDensity();
			m_camera_yaw_change -= dir_free.X * d;
			m_camera_pitch_change += dir_free.Y * d;

			if (dir_free_original.getLengthSQ() > touch_threshold_sq)
				m_move_has_really_moved = true;
		}

		if (m_has_joystick_id && event.TouchInput.ID == m_joystick_id) {
			v2s32 dir = dir_free;
			if (m_fixed_joystick)
				dir = dir_fixed;

			const bool inside_joystick = dir_fixed.getLengthSQ() <= fixed_joystick_range_sq;
			const double distance_sq = dir.getLengthSQ();

			if (m_joystick_has_really_moved || inside_joystick ||
					(!m_fixed_joystick && distance_sq > touch_threshold_sq)) {
				m_joystick_has_really_moved = true;

				m_joystick_direction = atan2(dir.X, -dir.Y);

				const double distance = sqrt(distance_sq);
				if (distance <= m_touchscreen_threshold) {
					m_joystick_speed = 0.0f;
				} else {
					m_joystick_speed = distance / m_button_size;
					if (m_joystick_speed > 1.0f)
						m_joystick_speed = 1.0f;
				}

				m_joystick_status_aux1 = distance > (half_button_size * 3);

				if (distance > m_button_size) {
					// move joystick "button"
					v2s32 new_offset = dir * m_button_size / distance - half_button_size;
					if (m_fixed_joystick)
						m_joystick_btn_center->setRelativePosition(
								fixed_joystick_center + new_offset);
					else
						m_joystick_btn_center->setRelativePosition(
								free_joystick_center + new_offset);
				} else {
					m_joystick_btn_center->setRelativePosition(
							touch_pos - half_button_size);
				}
			}
		}
	}
}

void TouchScreenGUI::applyJoystickStatus()
{
	if (m_joystick_triggers_aux1) {
		SEvent translated{};
		translated.EventType            = EET_KEY_INPUT_EVENT;
		translated.KeyInput.Key         = id_to_keycode(aux1_id);
		translated.KeyInput.PressedDown = false;
		m_receiver->OnEvent(translated);

		if (m_joystick_status_aux1) {
			translated.KeyInput.PressedDown = true;
			m_receiver->OnEvent(translated);
		}
	}
}

void TouchScreenGUI::step(float dtime)
{
	// simulate keyboard repeats
	buttons_step(m_buttons, dtime, m_device->getVideoDriver(), m_receiver, m_texturesource);
	for (AutoHideButtonBar &bar : m_buttonbars)
		bar.step(dtime);

	// joystick
	applyJoystickStatus();

	// if a new placed pointer isn't moved for some time start digging
	if (m_has_move_id && !m_move_has_really_moved && m_tap_state == TapState::None) {
		u64 delta = porting::getDeltaMs(m_move_downtime, porting::getTimeMs());

		if (delta > m_long_tap_delay) {
			m_tap_state = TapState::LongTap;
		}
	}

	// Update the shootline.
	// Since not only the pointer position, but also the player position and
	// thus the camera position can change, it doesn't suffice to update the
	// shootline when a touch event occurs.
	// Note that the shootline isn't used if touch_use_crosshair is enabled.
	// Only updating when m_has_move_id means that the shootline will stay at
	// it's last in-world position when the player doesn't need it.
	if (!m_draw_crosshair && (m_has_move_id || m_had_move_id)) {
		v2s32 pointer_pos = getPointerPos();
		m_shootline = m_device
				->getSceneManager()
				->getSceneCollisionManager()
				->getRayFromScreenCoordinates(pointer_pos);
	}
	m_had_move_id = false;
}

void TouchScreenGUI::resetHotbarRects()
{
	m_hotbar_rects.clear();
}

void TouchScreenGUI::registerHotbarRect(u16 index, const recti &rect)
{
	m_hotbar_rects[index] = rect;
}

void TouchScreenGUI::setVisible(bool visible)
{
	m_visible = visible;
	for (auto &button : m_buttons) {
		if (button.gui_button)
			button.gui_button->setVisible(visible);
	}

	if (m_joystick_btn_off)
		m_joystick_btn_off->setVisible(visible);

	// clear all active buttons
	if (!visible) {
		while (!m_pointer_pos.empty())
			handleReleaseEvent(m_pointer_pos.begin()->first);
		for (AutoHideButtonBar &bar : m_buttonbars)
			bar.hide();
	} else {
		for (AutoHideButtonBar &bar : m_buttonbars)
			bar.show();
	}
}

void TouchScreenGUI::hide()
{
	if (!m_visible)
		return;

	setVisible(false);
}

void TouchScreenGUI::show()
{
	if (m_visible) 
		return;

	setVisible(true);
}

v2s32 TouchScreenGUI::getPointerPos()
{
	if (m_draw_crosshair)
		return v2s32(m_screensize.X / 2, m_screensize.Y / 2);
	// We can't just use m_pointer_pos[m_move_id] because applyContextControls
	// may emit release events after m_pointer_pos[m_move_id] is erased.
	return m_move_pos;
}

void TouchScreenGUI::emitMouseEvent(EMOUSE_INPUT_EVENT type)
{
	v2s32 pointer_pos = getPointerPos();

	SEvent event{};
	event.EventType               = EET_MOUSE_INPUT_EVENT;
	event.MouseInput.X            = pointer_pos.X;
	event.MouseInput.Y            = pointer_pos.Y;
	event.MouseInput.Shift        = false;
	event.MouseInput.Control      = false;
	event.MouseInput.ButtonStates = 0;
	event.MouseInput.Event        = type;
	m_receiver->OnEvent(event);
}

void TouchScreenGUI::applyContextControls(const TouchInteractionMode &mode)
{
	// Since the pointed thing has already been determined when this function
	// is called, we cannot use this function to update the shootline.

	sanity_check(mode != TouchInteractionMode_USER);
	u64 now = porting::getTimeMs();
	bool target_dig_pressed = false;
	bool target_place_pressed = false;

	// If the meanings of short and long taps have been swapped, abort any ongoing
	// short taps because they would do something else than the player expected.
	// Long taps don't need this, they're adjusted to the swapped meanings instead.
	if (mode != m_last_mode) {
		m_dig_pressed_until = 0;
		m_place_pressed_until = 0;
	}
	m_last_mode = mode;

	switch (m_tap_state) {
	case TapState::ShortTap:
		if (mode == SHORT_DIG_LONG_PLACE) {
			if (!m_dig_pressed) {
				// The button isn't currently pressed, we can press it.
				m_dig_pressed_until = now + SIMULATED_CLICK_DURATION_MS;
				// We're done with this short tap.
				m_tap_state = TapState::None;
			}  else {
				// The button is already pressed, perhaps due to another short tap.
				// Release it now, press it again during the next client step.
				// We can't release and press during the same client step because
				// the digging code simply ignores that.
				m_dig_pressed_until = 0;
			}
		} else {
			if (!m_place_pressed) {
				// The button isn't currently pressed, we can press it.
				m_place_pressed_until = now + SIMULATED_CLICK_DURATION_MS;
				// We're done with this short tap.
				m_tap_state = TapState::None;
			}  else {
				// The button is already pressed, perhaps due to another short tap.
				// Release it now, press it again during the next client step.
				// We can't release and press during the same client step because
				// the digging code simply ignores that.
				m_place_pressed_until = 0;
			}
		}
		break;

	case TapState::LongTap:
		if (mode == SHORT_DIG_LONG_PLACE)
			target_place_pressed = true;
		else
			target_dig_pressed = true;
		break;

	case TapState::None:
		break;
	}

	// Apply short taps.
	target_dig_pressed |= now < m_dig_pressed_until;
	target_place_pressed |= now < m_place_pressed_until;

	if (target_dig_pressed && !m_dig_pressed) {
		emitMouseEvent(EMIE_LMOUSE_PRESSED_DOWN);
		m_dig_pressed = true;

	} else if (!target_dig_pressed && m_dig_pressed) {
		emitMouseEvent(EMIE_LMOUSE_LEFT_UP);
		m_dig_pressed = false;
	}

	if (target_place_pressed && !m_place_pressed) {
		emitMouseEvent(EMIE_RMOUSE_PRESSED_DOWN);
		m_place_pressed = true;

	} else if (!target_place_pressed && m_place_pressed) {
		emitMouseEvent(EMIE_RMOUSE_LEFT_UP);
		m_place_pressed = false;
	}
}
