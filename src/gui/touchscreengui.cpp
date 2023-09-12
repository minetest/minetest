/*
Copyright (C) 2014 sapier
Copyright (C) 2018 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>

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

#include <iostream>
#include <algorithm>

using namespace irr::core;

TouchScreenGUI *g_touchscreengui;

const std::string button_image_names[] = {
	"jump_btn.png",
	"down.png",
	"zoom.png",
	"aux1_btn.png"
};

const std::string joystick_image_names[] = {
	"joystick_off.png",
	"joystick_bg.png",
	"joystick_center.png"
};

static EKEY_CODE id_to_keycode(touch_gui_button_id id)
{
	// ESC isn't part of the keymap.
	if (id == exit_id)
		return KEY_ESCAPE;

	std::string key = "";
	switch (id) {
		case jump_id:
			key = "jump";
			break;
		case crunch_id:
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
	return keyname_to_keycode(g_settings->get("keymap_" + key).c_str());
}

static void load_button_texture(const button_info *btn, const std::string &path,
		const rect<s32> &button_rect, ISimpleTextureSource *tsrc, video::IVideoDriver *driver)
{
	u32 tid;
	video::ITexture *texture = guiScalingImageButton(driver,
			tsrc->getTexture(path, &tid), button_rect.getWidth(),
			button_rect.getHeight());
	if (texture) {
		btn->gui_button->setUseAlphaChannel(true);
		if (g_settings->getBool("gui_scaling_filter")) {
			rect<s32> txr_rect = rect<s32>(0, 0, button_rect.getWidth(), button_rect.getHeight());
			btn->gui_button->setImage(texture, txr_rect);
			btn->gui_button->setPressedImage(texture, txr_rect);
			btn->gui_button->setScaleImage(false);
		} else {
			btn->gui_button->setImage(texture);
			btn->gui_button->setPressedImage(texture);
			btn->gui_button->setScaleImage(true);
		}
		btn->gui_button->setDrawBorder(false);
		btn->gui_button->setText(L"");
	}
}

AutoHideButtonBar::AutoHideButtonBar(IrrlichtDevice *device,
		IEventReceiver *receiver) :
			m_driver(device->getVideoDriver()),
			m_guienv(device->getGUIEnvironment()),
			m_receiver(receiver)
{
}

void AutoHideButtonBar::init(ISimpleTextureSource *tsrc,
		const std::string &starter_img, int button_id, const v2s32 &UpperLeft,
		const v2s32 &LowerRight, autohide_button_bar_dir dir, float timeout)
{
	m_texturesource = tsrc;

	m_upper_left = UpperLeft;
	m_lower_right = LowerRight;

	rect<int> starter_rect = rect<s32>(UpperLeft.X, UpperLeft.Y, LowerRight.X, LowerRight.Y);

	IGUIButton *starter_gui_button = m_guienv->addButton(starter_rect, nullptr,
			button_id, L"", nullptr);

	m_starter.gui_button        = starter_gui_button;
	m_starter.gui_button->grab();
	m_starter.repeat_counter    = -1.0f;
	m_starter.keycode           = KEY_OEM_8; // use invalid keycode as it's not relevant
	m_starter.immediate_release = true;
	m_starter.ids.clear();

	load_button_texture(&m_starter, starter_img, starter_rect,
			m_texturesource, m_driver);

	m_dir = dir;
	m_timeout_value = timeout;

	m_initialized = true;
}

AutoHideButtonBar::~AutoHideButtonBar()
{
	if (m_starter.gui_button) {
		m_starter.gui_button->setVisible(false);
		m_starter.gui_button->drop();
		m_starter.gui_button = nullptr;
	}

	for (auto &button : m_buttons) {
		if (button->gui_button) {
			button->gui_button->drop();
			button->gui_button = nullptr;
		}
	}
}

void AutoHideButtonBar::addButton(touch_gui_button_id button_id, const wchar_t *caption,
		const std::string &btn_image)
{

	if (!m_initialized) {
		errorstream << "AutoHideButtonBar::addButton not yet initialized!" << std::endl;
		return;
	}

	int button_size = 0;

	if (m_dir == AHBB_Dir_Top_Bottom || m_dir == AHBB_Dir_Bottom_Top)
		button_size = m_lower_right.X - m_upper_left.X;
	else
		button_size = m_lower_right.Y - m_upper_left.Y;

	irr::core::rect<int> current_button;

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

		current_button = rect<s32>(x_start, m_upper_left.Y, x_end, m_lower_right.Y);
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

		current_button = rect<s32>(m_upper_left.X, y_start, m_lower_right.Y, y_end);
	}

	IGUIButton *btn_gui_button = m_guienv->addButton(current_button, nullptr, button_id,
			caption, nullptr);

	std::shared_ptr<button_info> btn(new button_info);
	btn->gui_button        = btn_gui_button;
	btn->gui_button->grab();
	btn->gui_button->setVisible(false);
	btn->gui_button->setEnabled(false);
	btn->repeat_counter    = -1.0f;
	btn->keycode           = id_to_keycode(button_id);
	btn->immediate_release = true;
	btn->ids.clear();

	load_button_texture(btn.get(), btn_image, current_button, m_texturesource, m_driver);

	m_buttons.push_back(btn);
}

void AutoHideButtonBar::addToggleButton(touch_gui_button_id button_id, const wchar_t *caption,
		const std::string &btn_image_1, const std::string &btn_image_2)
{
	addButton(button_id, caption, btn_image_1);
	std::shared_ptr<button_info> btn = m_buttons.back();
	btn->toggleable = button_info::FIRST_TEXTURE;
	btn->textures.push_back(btn_image_1);
	btn->textures.push_back(btn_image_2);
}

bool AutoHideButtonBar::isButton(const SEvent &event)
{
	IGUIElement *rootguielement = m_guienv->getRootGUIElement();

	if (rootguielement == nullptr)
		return false;

	gui::IGUIElement *element = rootguielement->getElementFromPoint(
			v2s32(event.TouchInput.X, event.TouchInput.Y));

	if (element == nullptr)
		return false;

	if (m_active) {
		// check for all buttons in vector
		for (const auto &button : m_buttons) {
			if (button->gui_button == element) {
				SEvent translated{};
				translated.EventType        = irr::EET_KEY_INPUT_EVENT;
				translated.KeyInput.Key     = button->keycode;
				translated.KeyInput.Control = false;
				translated.KeyInput.Shift   = false;
				translated.KeyInput.Char    = 0;

				// add this event
				translated.KeyInput.PressedDown = true;
				m_receiver->OnEvent(translated);

				// remove this event
				translated.KeyInput.PressedDown = false;
				m_receiver->OnEvent(translated);

				button->ids.push_back(event.TouchInput.ID);

				m_timeout = 0.0f;

				if (button->toggleable == button_info::FIRST_TEXTURE) {
					button->toggleable = button_info::SECOND_TEXTURE;
					load_button_texture(button.get(), button->textures[1],
							button->gui_button->getRelativePosition(),
							m_texturesource, m_driver);
				} else if (button->toggleable == button_info::SECOND_TEXTURE) {
					button->toggleable = button_info::FIRST_TEXTURE;
					load_button_texture(button.get(), button->textures[0],
							button->gui_button->getRelativePosition(),
							m_texturesource, m_driver);
				}

				return true;
			}
		}
	} else {
		// check for starter button only
		if (element == m_starter.gui_button) {
			m_starter.ids.push_back(event.TouchInput.ID);
			m_starter.gui_button->setVisible(false);
			m_starter.gui_button->setEnabled(false);
			m_active = true;
			m_timeout = 0.0f;

			for (const auto &button : m_buttons) {
				button->gui_button->setVisible(true);
				button->gui_button->setEnabled(true);
			}

			return true;
		}
	}
	return false;
}

void AutoHideButtonBar::step(float dtime)
{
	if (m_active) {
		m_timeout += dtime;

		if (m_timeout > m_timeout_value)
			deactivate();
	}
}

void AutoHideButtonBar::deactivate()
{
	if (m_visible) {
		m_starter.gui_button->setVisible(true);
		m_starter.gui_button->setEnabled(true);
	}
	m_active = false;

	for (const auto &button : m_buttons) {
		button->gui_button->setVisible(false);
		button->gui_button->setEnabled(false);
	}
}

void AutoHideButtonBar::hide()
{
	m_visible = false;
	m_starter.gui_button->setVisible(false);
	m_starter.gui_button->setEnabled(false);

	for (const auto &button : m_buttons) {
		button->gui_button->setVisible(false);
		button->gui_button->setEnabled(false);
	}
}

void AutoHideButtonBar::show()
{
	m_visible = true;

	if (m_active) {
		for (const auto &button : m_buttons) {
			button->gui_button->setVisible(true);
			button->gui_button->setEnabled(true);
		}
	} else {
		m_starter.gui_button->setVisible(true);
		m_starter.gui_button->setEnabled(true);
	}
}

TouchScreenGUI::TouchScreenGUI(IrrlichtDevice *device, IEventReceiver *receiver):
		m_device(device),
		m_guienv(device->getGUIEnvironment()),
		m_receiver(receiver),
		m_settings_bar(device, receiver),
		m_rare_controls_bar(device, receiver)
{
	for (auto &button : m_buttons) {
		button.gui_button     = nullptr;
		button.repeat_counter = -1.0f;
		button.repeat_delay   = BUTTON_REPEAT_DELAY;
	}

	m_touchscreen_threshold = g_settings->getU16("touchscreen_threshold");
	m_fixed_joystick = g_settings->getBool("fixed_virtual_joystick");
	m_joystick_triggers_aux1 = g_settings->getBool("virtual_joystick_triggers_aux1");
	m_screensize = m_device->getVideoDriver()->getScreenSize();
	button_size = MYMIN(m_screensize.Y / 4.5f,
			RenderingEngine::getDisplayDensity() * 65.0f *
					g_settings->getFloat("hud_scaling"));
}

void TouchScreenGUI::initButton(touch_gui_button_id id, const rect<s32> &button_rect,
		const std::wstring &caption, bool immediate_release, float repeat_delay)
{
	IGUIButton *btn_gui_button = m_guienv->addButton(button_rect, nullptr, id, caption.c_str());

	button_info *btn       = &m_buttons[id];
	btn->gui_button        = btn_gui_button;
	btn->gui_button->grab();
	btn->repeat_counter    = -1.0f;
	btn->repeat_delay      = repeat_delay;
	btn->keycode           = id_to_keycode(id);
	btn->immediate_release = immediate_release;
	btn->ids.clear();

	load_button_texture(btn, button_image_names[id], button_rect,
			m_texturesource, m_device->getVideoDriver());
}

std::shared_ptr<button_info> TouchScreenGUI::initJoystickButton(touch_gui_button_id id,
		const rect<s32> &button_rect, int texture_id, bool visible)
{
	IGUIButton *btn_gui_button = m_guienv->addButton(button_rect, nullptr, id, L"O");

	std::shared_ptr<button_info> btn(new button_info);
	btn->gui_button = btn_gui_button;
	btn->gui_button->setVisible(visible);
	btn->gui_button->grab();
	btn->ids.clear();

	load_button_texture(btn.get(), joystick_image_names[texture_id], button_rect,
			m_texturesource, m_device->getVideoDriver());

	return btn;
}

void TouchScreenGUI::init(ISimpleTextureSource *tsrc)
{
	assert(tsrc);

	m_visible       = true;
	m_texturesource = tsrc;

	// Initialize joystick display "button".
	// Joystick is placed on the bottom left of screen.
	if (m_fixed_joystick) {
		m_joystick_btn_off = initJoystickButton(joystick_off_id,
				rect<s32>(button_size,
						m_screensize.Y - button_size * 4,
						button_size * 4,
						m_screensize.Y - button_size), 0);
	} else {
		m_joystick_btn_off = initJoystickButton(joystick_off_id,
				rect<s32>(button_size,
						m_screensize.Y - button_size * 3,
						button_size * 3,
						m_screensize.Y - button_size), 0);
	}

	m_joystick_btn_bg = initJoystickButton(joystick_bg_id,
			rect<s32>(button_size,
					m_screensize.Y - button_size * 4,
					button_size * 4,
					m_screensize.Y - button_size),
			1, false);

	m_joystick_btn_center = initJoystickButton(joystick_center_id,
			rect<s32>(0, 0, button_size, button_size), 2, false);

	// init jump button
	initButton(jump_id,
			rect<s32>(m_screensize.X - 1.75f * button_size,
					m_screensize.Y - button_size,
					m_screensize.X - 0.25f * button_size,
					m_screensize.Y),
			L"x", false);

	// init crunch button
	initButton(crunch_id,
			rect<s32>(m_screensize.X - 3.25f * button_size,
					m_screensize.Y - button_size,
					m_screensize.X - 1.75f * button_size,
					m_screensize.Y),
			L"H", false);

	// init zoom button
	initButton(zoom_id,
			rect<s32>(m_screensize.X - 1.25f * button_size,
					m_screensize.Y - 4 * button_size,
					m_screensize.X - 0.25f * button_size,
					m_screensize.Y - 3 * button_size),
			L"z", false);

	// init aux1 button
	if (!m_joystick_triggers_aux1)
		initButton(aux1_id,
				rect<s32>(m_screensize.X - 1.25f * button_size,
						m_screensize.Y - 2.5f * button_size,
						m_screensize.X - 0.25f * button_size,
						m_screensize.Y - 1.5f * button_size),
				L"spc1", false);

	m_settings_bar.init(m_texturesource, "gear_icon.png", settings_starter_id,
			v2s32(m_screensize.X - 1.25f * button_size,
					m_screensize.Y - (SETTINGS_BAR_Y_OFFSET + 1.0f) * button_size
							+ 0.5f * button_size),
			v2s32(m_screensize.X - 0.25f * button_size,
					m_screensize.Y - SETTINGS_BAR_Y_OFFSET * button_size
							+ 0.5f * button_size),
			AHBB_Dir_Right_Left, 3.0f);

	m_settings_bar.addButton(fly_id, L"fly", "fly_btn.png");
	m_settings_bar.addButton(noclip_id, L"noclip", "noclip_btn.png");
	m_settings_bar.addButton(fast_id, L"fast", "fast_btn.png");
	m_settings_bar.addButton(debug_id, L"debug", "debug_btn.png");
	m_settings_bar.addButton(camera_id, L"camera", "camera_btn.png");
	m_settings_bar.addButton(range_id, L"rangeview", "rangeview_btn.png");
	m_settings_bar.addButton(minimap_id, L"minimap", "minimap_btn.png");

	// Chat is shown by default, so chat_hide_btn.png is shown first.
	m_settings_bar.addToggleButton(toggle_chat_id, L"togglechat",
			"chat_hide_btn.png", "chat_show_btn.png");

	m_rare_controls_bar.init(m_texturesource, "rare_controls.png",
			rare_controls_starter_id,
			v2s32(0.25f * button_size,
					m_screensize.Y - (RARE_CONTROLS_BAR_Y_OFFSET + 1.0f) * button_size
							+ 0.5f * button_size),
			v2s32(0.75f * button_size,
					m_screensize.Y - RARE_CONTROLS_BAR_Y_OFFSET * button_size
							+ 0.5f * button_size),
			AHBB_Dir_Left_Right, 2.0f);

	m_rare_controls_bar.addButton(chat_id, L"chat", "chat_btn.png");
	m_rare_controls_bar.addButton(inventory_id, L"inv", "inventory_btn.png");
	m_rare_controls_bar.addButton(drop_id, L"drop", "drop_btn.png");
	m_rare_controls_bar.addButton(exit_id, L"exit", "exit_btn.png");

	m_initialized = true;
}

touch_gui_button_id TouchScreenGUI::getButtonID(s32 x, s32 y)
{
	IGUIElement *rootguielement = m_guienv->getRootGUIElement();

	if (rootguielement != nullptr) {
		gui::IGUIElement *element =
				rootguielement->getElementFromPoint(v2s32(x, y));

		if (element)
			for (unsigned int i = 0; i < after_last_element_id; i++)
				if (element == m_buttons[i].gui_button)
					return (touch_gui_button_id) i;
	}

	return after_last_element_id;
}

touch_gui_button_id TouchScreenGUI::getButtonID(size_t eventID)
{
	for (unsigned int i = 0; i < after_last_element_id; i++) {
		button_info *btn = &m_buttons[i];

		auto id = std::find(btn->ids.begin(), btn->ids.end(), eventID);

		if (id != btn->ids.end())
			return (touch_gui_button_id) i;
	}

	return after_last_element_id;
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

void TouchScreenGUI::handleButtonEvent(touch_gui_button_id button,
		size_t eventID, bool action)
{
	button_info *btn = &m_buttons[button];
	SEvent translated{};
	translated.EventType        = irr::EET_KEY_INPUT_EVENT;
	translated.KeyInput.Key     = btn->keycode;
	translated.KeyInput.Control = false;
	translated.KeyInput.Shift   = false;
	translated.KeyInput.Char    = 0;

	// add this event
	if (action) {
		assert(std::find(btn->ids.begin(), btn->ids.end(), eventID) == btn->ids.end());

		btn->ids.push_back(eventID);

		if (btn->ids.size() > 1)
			return;

		btn->repeat_counter = 0.0f;
		translated.KeyInput.PressedDown = true;
		translated.KeyInput.Key = btn->keycode;
		m_receiver->OnEvent(translated);
	}

	// remove event
	if (!action || btn->immediate_release) {
		auto pos = std::find(btn->ids.begin(), btn->ids.end(), eventID);
		// has to be in touch list
		assert(pos != btn->ids.end());
		btn->ids.erase(pos);

		if (!btn->ids.empty())
			return;

		translated.KeyInput.PressedDown = false;
		btn->repeat_counter             = -1.0f;
		m_receiver->OnEvent(translated);
	}
}

void TouchScreenGUI::handleReleaseEvent(size_t evt_id)
{
	touch_gui_button_id button = getButtonID(evt_id);

	if (button != after_last_element_id) {
		// handle button events
		handleButtonEvent(button, evt_id, false);
	} else if (m_has_move_id && evt_id == m_move_id) {
		// handle the point used for moving view
		m_has_move_id = false;

		// if this pointer issued a mouse event issue symmetric release here
		if (m_move_sent_as_mouse_event) {
			SEvent translated {};
			translated.EventType               = EET_MOUSE_INPUT_EVENT;
			translated.MouseInput.X            = m_move_downlocation.X;
			translated.MouseInput.Y            = m_move_downlocation.Y;
			translated.MouseInput.Shift        = false;
			translated.MouseInput.Control      = false;
			translated.MouseInput.ButtonStates = 0;
			translated.MouseInput.Event        = EMIE_LMOUSE_LEFT_UP;
			if (m_draw_crosshair) {
				translated.MouseInput.X = m_screensize.X / 2;
				translated.MouseInput.Y = m_screensize.Y / 2;
			}
			m_receiver->OnEvent(translated);
		} else if (!m_move_has_really_moved) {
			doRightClick();
		}
	}

	// handle joystick
	else if (m_has_joystick_id && evt_id == m_joystick_id) {
		m_has_joystick_id = false;

		// reset joystick
		m_joystick_direction = 0.0f;
		m_joystick_speed = 0.0f;
		m_joystick_status_aux1 = false;
		applyJoystickStatus();

		m_joystick_btn_off->gui_button->setVisible(true);
		m_joystick_btn_bg->gui_button->setVisible(false);
		m_joystick_btn_center->gui_button->setVisible(false);
	} else {
		infostream << "TouchScreenGUI::translateEvent released unknown button: "
				<< evt_id << std::endl;
	}

	for (auto iter = m_known_ids.begin(); iter != m_known_ids.end(); ++iter) {
		if (iter->id == evt_id) {
			m_known_ids.erase(iter);
			break;
		}
	}
}

void TouchScreenGUI::translateEvent(const SEvent &event)
{
	if (!m_initialized)
		return;

	if (!m_visible) {
		infostream << "TouchScreenGUI::translateEvent got event but is not visible!"
				<< std::endl;
		return;
	}

	if (event.EventType != EET_TOUCH_INPUT_EVENT)
		return;

	const s32 half_button_size = button_size / 2.0f;
	const s32 fixed_joystick_range_sq = half_button_size * half_button_size * 3 * 3;
	const s32 X = event.TouchInput.X;
	const s32 Y = event.TouchInput.Y;
	const v2s32 touch_pos = v2s32(X, Y);
	const v2s32 fixed_joystick_center = v2s32(half_button_size * 5,
			m_screensize.Y - half_button_size * 5);
	const v2s32 dir_fixed = touch_pos - fixed_joystick_center;

	if (event.TouchInput.Event == ETIE_PRESSED_DOWN) {
		/*
		 * Add to own copy of event list...
		 * android would provide this information but Irrlicht guys don't
		 * wanna design an efficient interface
		 */
		id_status to_be_added{};
		to_be_added.id = event.TouchInput.ID;
		to_be_added.X  = event.TouchInput.X;
		to_be_added.Y  = event.TouchInput.Y;
		m_known_ids.push_back(to_be_added);

		size_t eventID = event.TouchInput.ID;

		touch_gui_button_id button = getButtonID(X, Y);

		// handle button events
		if (button != after_last_element_id) {
			handleButtonEvent(button, eventID, true);
			m_settings_bar.deactivate();
			m_rare_controls_bar.deactivate();
		} else if (isHotbarButton(event)) {
			m_settings_bar.deactivate();
			m_rare_controls_bar.deactivate();
			// already handled in isHotbarButton()
		} else if (m_settings_bar.isButton(event)) {
			m_rare_controls_bar.deactivate();
			// already handled in isSettingsBarButton()
		} else if (m_rare_controls_bar.isButton(event)) {
			m_settings_bar.deactivate();
			// already handled in isSettingsBarButton()
		} else {
			// handle non button events
			if (m_settings_bar.active() || m_rare_controls_bar.active()) {
				m_settings_bar.deactivate();
				m_rare_controls_bar.deactivate();
				return;
			}

			// Select joystick when joystick tapped (fixed joystick position) or
			// when left 1/3 of screen dragged (free joystick position)
			if ((m_fixed_joystick && dir_fixed.getLengthSQ() <= fixed_joystick_range_sq) ||
					(!m_fixed_joystick && X < m_screensize.X / 3.0f)) {
				// If we don't already have a starting point for joystick, make this the one.
				if (!m_has_joystick_id) {
					m_has_joystick_id           = true;
					m_joystick_id               = event.TouchInput.ID;
					m_joystick_has_really_moved = false;

					m_joystick_btn_off->gui_button->setVisible(false);
					m_joystick_btn_bg->gui_button->setVisible(true);
					m_joystick_btn_center->gui_button->setVisible(true);

					// If it's a fixed joystick, don't move the joystick "button".
					if (!m_fixed_joystick)
						m_joystick_btn_bg->gui_button->setRelativePosition(
								touch_pos - half_button_size * 3);

					m_joystick_btn_center->gui_button->setRelativePosition(
							touch_pos - half_button_size);
				}
			} else {
				// If we don't already have a moving point, make this the moving one.
				if (!m_has_move_id) {
					m_has_move_id              = true;
					m_move_id                  = event.TouchInput.ID;
					m_move_has_really_moved    = false;
					m_move_downtime            = porting::getTimeMs();
					m_move_downlocation        = touch_pos;
					m_move_sent_as_mouse_event = false;
					if (m_draw_crosshair)
						m_move_downlocation = v2s32(m_screensize.X / 2, m_screensize.Y / 2);
				}
			}
		}

		m_pointer_pos[event.TouchInput.ID] = touch_pos;
	}
	else if (event.TouchInput.Event == ETIE_LEFT_UP) {
		verbosestream << "Up event for pointerid: " << event.TouchInput.ID << std::endl;
		handleReleaseEvent(event.TouchInput.ID);
	} else {
		assert(event.TouchInput.Event == ETIE_MOVED);

		if (!(m_has_joystick_id && m_fixed_joystick) &&
				m_pointer_pos[event.TouchInput.ID] == touch_pos)
			return;

		const v2s32 free_joystick_center = v2s32(m_pointer_pos[event.TouchInput.ID].X,
				m_pointer_pos[event.TouchInput.ID].Y);
		const v2s32 dir_free = touch_pos - free_joystick_center;

		const double touch_threshold_sq = m_touchscreen_threshold * m_touchscreen_threshold;

		if (m_has_move_id) {
			if (event.TouchInput.ID == m_move_id &&
					(!m_move_sent_as_mouse_event || m_draw_crosshair)) {
				if (dir_free.getLengthSQ() > touch_threshold_sq || m_move_has_really_moved) {
					m_move_has_really_moved = true;

					// update camera_yaw and camera_pitch
					m_pointer_pos[event.TouchInput.ID] = touch_pos;

					// adapt to similar behavior as pc screen
					const double d = g_settings->getFloat("touchscreen_sensitivity", 0.001f, 10.0f) * 3.0f;

					m_camera_yaw_change -= dir_free.X * d;
					m_camera_pitch = MYMIN(MYMAX(m_camera_pitch + (dir_free.Y * d), -180.0f), 180.0f);

					// update shootline
					// no need to update (X, Y) when using crosshair since the shootline is not used
					m_shootline = m_device
							->getSceneManager()
							->getSceneCollisionManager()
							->getRayFromScreenCoordinates(touch_pos);
				}
			} else if (event.TouchInput.ID == m_move_id && m_move_sent_as_mouse_event) {
				m_shootline = m_device
						->getSceneManager()
						->getSceneCollisionManager()
						->getRayFromScreenCoordinates(touch_pos);
			}
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
					m_joystick_speed = distance / button_size;
					if (m_joystick_speed > 1.0f)
						m_joystick_speed = 1.0f;
				}

				m_joystick_status_aux1 = distance > (half_button_size * 3);

				if (distance > button_size) {
					// move joystick "button"
					v2s32 new_offset = dir * button_size / distance - half_button_size;
					if (m_fixed_joystick)
						m_joystick_btn_center->gui_button->setRelativePosition(
								fixed_joystick_center + new_offset);
					else
						m_joystick_btn_center->gui_button->setRelativePosition(
								free_joystick_center + new_offset);
				} else {
					m_joystick_btn_center->gui_button->setRelativePosition(
							touch_pos - half_button_size);
				}
			}
		}

		if (!m_has_move_id && !m_has_joystick_id)
			handleChangedButton(event);
	}
}

void TouchScreenGUI::handleChangedButton(const SEvent &event)
{
	for (unsigned int i = 0; i < after_last_element_id; i++) {
		if (m_buttons[i].ids.empty())
			continue;

		for (auto iter = m_buttons[i].ids.begin(); iter != m_buttons[i].ids.end(); ++iter) {
			if (event.TouchInput.ID == *iter) {
				auto current_button_id = getButtonID(event.TouchInput.X, event.TouchInput.Y);

				if (current_button_id == i)
					continue;

				// remove old button
				handleButtonEvent((touch_gui_button_id) i, *iter, false);

				if (current_button_id == after_last_element_id)
					return;

				handleButtonEvent((touch_gui_button_id) current_button_id, *iter, true);
				return;
			}
		}
	}

	int current_button_id = getButtonID(event.TouchInput.X, event.TouchInput.Y);

	if (current_button_id == after_last_element_id)
		return;

	button_info *btn = &m_buttons[current_button_id];
	if (std::find(btn->ids.begin(), btn->ids.end(), event.TouchInput.ID) == btn->ids.end())
		handleButtonEvent((touch_gui_button_id) current_button_id, event.TouchInput.ID, true);
}

bool TouchScreenGUI::doRightClick()
{
	v2s32 mPos = v2s32(m_move_downlocation.X, m_move_downlocation.Y);
	if (m_draw_crosshair) {
		mPos.X = m_screensize.X / 2;
		mPos.Y = m_screensize.Y / 2;
	}

	SEvent translated {};
	translated.EventType               = EET_MOUSE_INPUT_EVENT;
	translated.MouseInput.X            = mPos.X;
	translated.MouseInput.Y            = mPos.Y;
	translated.MouseInput.Shift        = false;
	translated.MouseInput.Control      = false;
	translated.MouseInput.ButtonStates = EMBSM_RIGHT;

	// update shootline
	m_shootline = m_device
			->getSceneManager()
			->getSceneCollisionManager()
			->getRayFromScreenCoordinates(mPos);

	translated.MouseInput.Event = EMIE_RMOUSE_PRESSED_DOWN;
	verbosestream << "TouchScreenGUI::translateEvent right click press" << std::endl;
	m_receiver->OnEvent(translated);

	translated.MouseInput.ButtonStates = 0;
	translated.MouseInput.Event = EMIE_RMOUSE_LEFT_UP;
	verbosestream << "TouchScreenGUI::translateEvent right click release" << std::endl;
	m_receiver->OnEvent(translated);

	return true;
}

void TouchScreenGUI::applyJoystickStatus()
{
	if (m_joystick_triggers_aux1) {
		SEvent translated{};
		translated.EventType            = irr::EET_KEY_INPUT_EVENT;
		translated.KeyInput.Key         = id_to_keycode(aux1_id);
		translated.KeyInput.PressedDown = false;
		m_receiver->OnEvent(translated);

		if (m_joystick_status_aux1) {
			translated.KeyInput.PressedDown = true;
			m_receiver->OnEvent(translated);
		}
	}
}

TouchScreenGUI::~TouchScreenGUI()
{
	if (!m_initialized)
		return;

	for (auto &button : m_buttons) {
		if (button.gui_button) {
			button.gui_button->drop();
			button.gui_button = nullptr;
		}
	}

	if (m_joystick_btn_off->gui_button) {
		m_joystick_btn_off->gui_button->drop();
		m_joystick_btn_off->gui_button = nullptr;
	}

	if (m_joystick_btn_bg->gui_button) {
		m_joystick_btn_bg->gui_button->drop();
		m_joystick_btn_bg->gui_button = nullptr;
	}

	if (m_joystick_btn_center->gui_button) {
		m_joystick_btn_center->gui_button->drop();
		m_joystick_btn_center->gui_button = nullptr;
	}
}

void TouchScreenGUI::step(float dtime)
{
	if (!m_initialized)
		return;

	// simulate keyboard repeats
	for (auto &button : m_buttons) {
		if (!button.ids.empty()) {
			button.repeat_counter += dtime;

			if (button.repeat_counter < button.repeat_delay)
				continue;

			button.repeat_counter           = 0.0f;
			SEvent translated {};
			translated.EventType            = irr::EET_KEY_INPUT_EVENT;
			translated.KeyInput.Key         = button.keycode;
			translated.KeyInput.PressedDown = false;
			m_receiver->OnEvent(translated);

			translated.KeyInput.PressedDown = true;
			m_receiver->OnEvent(translated);
		}
	}

	// joystick
	applyJoystickStatus();

	// if a new placed pointer isn't moved for some time start digging
	if (m_has_move_id &&
			(!m_move_has_really_moved) &&
			(!m_move_sent_as_mouse_event)) {
		u64 delta = porting::getDeltaMs(m_move_downtime, porting::getTimeMs());

		if (delta > MIN_DIG_TIME_MS) {
			s32 mX = m_move_downlocation.X;
			s32 mY = m_move_downlocation.Y;
			if (m_draw_crosshair) {
				mX = m_screensize.X / 2;
				mY = m_screensize.Y / 2;
			}
			m_shootline = m_device
					->getSceneManager()
					->getSceneCollisionManager()
					->getRayFromScreenCoordinates(v2s32(mX, mY));

			SEvent translated {};
			translated.EventType               = EET_MOUSE_INPUT_EVENT;
			translated.MouseInput.X            = mX;
			translated.MouseInput.Y            = mY;
			translated.MouseInput.Shift        = false;
			translated.MouseInput.Control      = false;
			translated.MouseInput.ButtonStates = EMBSM_LEFT;
			translated.MouseInput.Event        = EMIE_LMOUSE_PRESSED_DOWN;
			verbosestream << "TouchScreenGUI::step left click press" << std::endl;
			m_receiver->OnEvent(translated);
			m_move_sent_as_mouse_event         = true;
		}
	}

	m_settings_bar.step(dtime);
	m_rare_controls_bar.step(dtime);
}

void TouchScreenGUI::resetHotbarRects()
{
	m_hotbar_rects.clear();
}

void TouchScreenGUI::registerHotbarRect(u16 index, const rect<s32> &rect)
{
	m_hotbar_rects[index] = rect;
}

void TouchScreenGUI::setVisible(bool visible)
{
	if (!m_initialized)
		return;

	m_visible = visible;
	for (auto &button : m_buttons) {
		if (button.gui_button)
			button.gui_button->setVisible(visible);
	}

	if (m_joystick_btn_off->gui_button)
		m_joystick_btn_off->gui_button->setVisible(visible);

	// clear all active buttons
	if (!visible) {
		while (!m_known_ids.empty())
			handleReleaseEvent(m_known_ids.begin()->id);

		m_settings_bar.hide();
		m_rare_controls_bar.hide();
	} else {
		m_settings_bar.show();
		m_rare_controls_bar.show();
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
