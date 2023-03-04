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
#include "irrlichttypes.h"
#include "irr_v2d.h"
#include "log.h"
#include "client/keycode.h"
#include "settings.h"
#include "gettime.h"
#include "util/numeric.h"
#include "porting.h"
#include "client/guiscalingfilter.h"
#include "client/renderingengine.h"

#include <iostream>
#include <algorithm>

using namespace irr::core;

const char **button_imagenames = (const char *[]) {
	"jump_btn.png",
	"down.png",
	"zoom.png",
	"aux1_btn.png"
};

const char **joystick_imagenames = (const char *[]) {
	"joystick_off.png",
	"joystick_bg.png",
	"joystick_center.png"
};

static irr::EKEY_CODE id2keycode(touch_gui_button_id id)
{
	std::string key = "";
	switch (id) {
		case forward_id:
			key = "forward";
			break;
		case left_id:
			key = "left";
			break;
		case right_id:
			key = "right";
			break;
		case backward_id:
			key = "backward";
			break;
		case inventory_id:
			key = "inventory";
			break;
		case drop_id:
			key = "drop";
			break;
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
		case toggle_chat_id:
			key = "toggle_chat";
			break;
		case minimap_id:
			key = "minimap";
			break;
		case chat_id:
			key = "chat";
			break;
		case camera_id:
			key = "camera_mode";
			break;
		case range_id:
			key = "rangeselect";
			break;
		default:
			break;
	}
	assert(!key.empty());
	return keyname_to_keycode(g_settings->get("keymap_" + key).c_str());
}

TouchScreenGUI *g_touchscreengui;

static void load_button_texture(button_info *btn, const char *path,
		const rect<s32> &button_rect, ISimpleTextureSource *tsrc, video::IVideoDriver *driver)
{
	unsigned int tid;
	video::ITexture *texture = guiScalingImageButton(driver,
			tsrc->getTexture(path, &tid), button_rect.getWidth(),
			button_rect.getHeight());
	if (texture) {
		btn->guibutton->setUseAlphaChannel(true);
		if (g_settings->getBool("gui_scaling_filter")) {
			rect<s32> txr_rect = rect<s32>(0, 0, button_rect.getWidth(), button_rect.getHeight());
			btn->guibutton->setImage(texture, txr_rect);
			btn->guibutton->setPressedImage(texture, txr_rect);
			btn->guibutton->setScaleImage(false);
		} else {
			btn->guibutton->setImage(texture);
			btn->guibutton->setPressedImage(texture);
			btn->guibutton->setScaleImage(true);
		}
		btn->guibutton->setDrawBorder(false);
		btn->guibutton->setText(L"");
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
		const char *starter_img, int button_id, const v2s32 &UpperLeft,
		const v2s32 &LowerRight, autohide_button_bar_dir dir, float timeout)
{
	m_texturesource = tsrc;

	m_upper_left = UpperLeft;
	m_lower_right = LowerRight;

	// init settings bar

	irr::core::rect<int> current_button = rect<s32>(UpperLeft.X, UpperLeft.Y,
			LowerRight.X, LowerRight.Y);

	m_starter.guibutton         = m_guienv->addButton(current_button, nullptr, button_id, L"", nullptr);
	m_starter.guibutton->grab();
	m_starter.repeatcounter     = -1;
	m_starter.keycode           = KEY_OEM_8; // use invalid keycode as it's not relevant
	m_starter.immediate_release = true;
	m_starter.ids.clear();

	load_button_texture(&m_starter, starter_img, current_button,
			m_texturesource, m_driver);

	m_dir = dir;
	m_timeout_value = timeout;

	m_initialized = true;
}

AutoHideButtonBar::~AutoHideButtonBar()
{
	if (m_starter.guibutton) {
		m_starter.guibutton->setVisible(false);
		m_starter.guibutton->drop();
	}
}

void AutoHideButtonBar::addButton(touch_gui_button_id button_id,
		const wchar_t *caption, const char *btn_image)
{

	if (!m_initialized) {
		errorstream << "AutoHideButtonBar::addButton not yet initialized!"
				<< std::endl;
		return;
	}
	int button_size = 0;

	if ((m_dir == AHBB_Dir_Top_Bottom) || (m_dir == AHBB_Dir_Bottom_Top))
		button_size = m_lower_right.X - m_upper_left.X;
	else
		button_size = m_lower_right.Y - m_upper_left.Y;

	irr::core::rect<int> current_button;

	if ((m_dir == AHBB_Dir_Right_Left) || (m_dir == AHBB_Dir_Left_Right)) {
		int x_start = 0;
		int x_end = 0;

		if (m_dir == AHBB_Dir_Left_Right) {
			x_start = m_lower_right.X + (button_size * 1.25 * m_buttons.size())
					+ (button_size * 0.25);
			x_end = x_start + button_size;
		} else {
			x_end = m_upper_left.X - (button_size * 1.25 * m_buttons.size())
					- (button_size * 0.25);
			x_start = x_end - button_size;
		}

		current_button = rect<s32>(x_start, m_upper_left.Y, x_end,
				m_lower_right.Y);
	} else {
		double y_start = 0;
		double y_end = 0;

		if (m_dir == AHBB_Dir_Top_Bottom) {
			y_start = m_lower_right.X + (button_size * 1.25 * m_buttons.size())
					+ (button_size * 0.25);
			y_end = y_start + button_size;
		} else {
			y_end = m_upper_left.X - (button_size * 1.25 * m_buttons.size())
					- (button_size * 0.25);
			y_start = y_end - button_size;
		}

		current_button = rect<s32>(m_upper_left.X, y_start,
				m_lower_right.Y, y_end);
	}

	auto *btn              = new button_info();
	btn->guibutton         = m_guienv->addButton(current_button,
					nullptr, button_id, caption, nullptr);
	btn->guibutton->grab();
	btn->guibutton->setVisible(false);
	btn->guibutton->setEnabled(false);
	btn->repeatcounter     = -1;
	btn->keycode           = id2keycode(button_id);
	btn->immediate_release = true;
	btn->ids.clear();

	load_button_texture(btn, btn_image, current_button, m_texturesource,
			m_driver);

	m_buttons.push_back(btn);
}

void AutoHideButtonBar::addToggleButton(touch_gui_button_id button_id,
		const wchar_t *caption, const char *btn_image_1,
		const char *btn_image_2)
{
	addButton(button_id, caption, btn_image_1);
	button_info *btn = m_buttons.back();
	btn->togglable = 1;
	btn->textures.push_back(btn_image_1);
	btn->textures.push_back(btn_image_2);
}

bool AutoHideButtonBar::isButton(const SEvent &event)
{
	IGUIElement *rootguielement = m_guienv->getRootGUIElement();

	if (rootguielement == nullptr)
		return false;

	gui::IGUIElement *element = rootguielement->getElementFromPoint(
			core::position2d<s32>(event.TouchInput.X, event.TouchInput.Y));

	if (element == nullptr)
		return false;

	if (m_active) {
		// check for all buttons in vector
		auto iter = m_buttons.begin();

		while (iter != m_buttons.end()) {
			if ((*iter)->guibutton == element) {

				auto *translated = new SEvent();
				memset(translated, 0, sizeof(SEvent));
				translated->EventType            = irr::EET_KEY_INPUT_EVENT;
				translated->KeyInput.Key         = (*iter)->keycode;
				translated->KeyInput.Control     = false;
				translated->KeyInput.Shift       = false;
				translated->KeyInput.Char        = 0;

				// add this event
				translated->KeyInput.PressedDown = true;
				m_receiver->OnEvent(*translated);

				// remove this event
				translated->KeyInput.PressedDown = false;
				m_receiver->OnEvent(*translated);

				delete translated;

				(*iter)->ids.push_back(event.TouchInput.ID);

				m_timeout = 0;

				if ((*iter)->togglable == 1) {
					(*iter)->togglable = 2;
					load_button_texture(*iter, (*iter)->textures[1],
							(*iter)->guibutton->getRelativePosition(),
							m_texturesource, m_driver);
				} else if ((*iter)->togglable == 2) {
					(*iter)->togglable = 1;
					load_button_texture(*iter, (*iter)->textures[0],
							(*iter)->guibutton->getRelativePosition(),
							m_texturesource, m_driver);
				}

				return true;
			}
			++iter;
		}
	} else {
		// check for starter button only
		if (element == m_starter.guibutton) {
			m_starter.ids.push_back(event.TouchInput.ID);
			m_starter.guibutton->setVisible(false);
			m_starter.guibutton->setEnabled(false);
			m_active = true;
			m_timeout = 0;

			auto iter = m_buttons.begin();

			while (iter != m_buttons.end()) {
				(*iter)->guibutton->setVisible(true);
				(*iter)->guibutton->setEnabled(true);
				++iter;
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
		m_starter.guibutton->setVisible(true);
		m_starter.guibutton->setEnabled(true);
	}
	m_active = false;

	auto iter = m_buttons.begin();

	while (iter != m_buttons.end()) {
		(*iter)->guibutton->setVisible(false);
		(*iter)->guibutton->setEnabled(false);
		++iter;
	}
}

void AutoHideButtonBar::hide()
{
	m_visible = false;
	m_starter.guibutton->setVisible(false);
	m_starter.guibutton->setEnabled(false);

	auto iter = m_buttons.begin();

	while (iter != m_buttons.end()) {
		(*iter)->guibutton->setVisible(false);
		(*iter)->guibutton->setEnabled(false);
		++iter;
	}
}

void AutoHideButtonBar::show()
{
	m_visible = true;

	if (m_active) {
		auto iter = m_buttons.begin();

		while (iter != m_buttons.end()) {
			(*iter)->guibutton->setVisible(true);
			(*iter)->guibutton->setEnabled(true);
			++iter;
		}
	} else {
		m_starter.guibutton->setVisible(true);
		m_starter.guibutton->setEnabled(true);
	}
}

TouchScreenGUI::TouchScreenGUI(IrrlichtDevice *device, IEventReceiver *receiver):
	m_device(device),
	m_guienv(device->getGUIEnvironment()),
	m_receiver(receiver),
	m_settingsbar(device, receiver),
	m_rarecontrolsbar(device, receiver)
{
	for (auto &button : m_buttons) {
		button.guibutton     = nullptr;
		button.repeatcounter = -1;
		button.repeatdelay   = BUTTON_REPEAT_DELAY;
	}

	m_touchscreen_threshold = g_settings->getU16("touchscreen_threshold");
	m_fixed_joystick = g_settings->getBool("fixed_virtual_joystick");
	m_joystick_triggers_aux1 = g_settings->getBool("virtual_joystick_triggers_aux1");
	m_screensize = m_device->getVideoDriver()->getScreenSize();
	button_size = MYMIN(m_screensize.Y / 4.5f,
			RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("hud_scaling") * 65.0f);
}

void TouchScreenGUI::initButton(touch_gui_button_id id, const rect<s32> &button_rect,
		const std::wstring &caption, bool immediate_release, float repeat_delay)
{
	button_info *btn       = &m_buttons[id];
	btn->guibutton         = m_guienv->addButton(button_rect, nullptr, id, caption.c_str());
	btn->guibutton->grab();
	btn->repeatcounter     = -1;
	btn->repeatdelay       = repeat_delay;
	btn->keycode           = id2keycode(id);
	btn->immediate_release = immediate_release;
	btn->ids.clear();

	load_button_texture(btn, button_imagenames[id], button_rect,
			m_texturesource, m_device->getVideoDriver());
}

button_info *TouchScreenGUI::initJoystickButton(touch_gui_button_id id,
		const rect<s32> &button_rect, int texture_id, bool visible)
{
	auto *btn = new button_info();
	btn->guibutton = m_guienv->addButton(button_rect, nullptr, id, L"O");
	btn->guibutton->setVisible(visible);
	btn->guibutton->grab();
	btn->ids.clear();

	load_button_texture(btn, joystick_imagenames[texture_id],
			button_rect, m_texturesource, m_device->getVideoDriver());

	return btn;
}

void TouchScreenGUI::init(ISimpleTextureSource *tsrc)
{
	assert(tsrc);

	m_visible       = true;
	m_texturesource = tsrc;

	/* Init joystick display "button"
	 * Joystick is placed on bottom left of screen.
	 */
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
			rect<s32>(m_screensize.X - (1.75 * button_size),
					m_screensize.Y - button_size,
					m_screensize.X - (0.25 * button_size),
					m_screensize.Y),
			L"x", false);

	// init crunch button
	initButton(crunch_id,
			rect<s32>(m_screensize.X - (3.25 * button_size),
					m_screensize.Y - button_size,
					m_screensize.X - (1.75 * button_size),
					m_screensize.Y),
			L"H", false);

	// init zoom button
	initButton(zoom_id,
			rect<s32>(m_screensize.X - (1.25 * button_size),
					m_screensize.Y - (4 * button_size),
					m_screensize.X - (0.25 * button_size),
					m_screensize.Y - (3 * button_size)),
			L"z", false);

	// init aux1 button
	if (!m_joystick_triggers_aux1)
		initButton(aux1_id,
				rect<s32>(m_screensize.X - (1.25 * button_size),
						m_screensize.Y - (2.5 * button_size),
						m_screensize.X - (0.25 * button_size),
						m_screensize.Y - (1.5 * button_size)),
				L"spc1", false);

	m_settingsbar.init(m_texturesource, "gear_icon.png", settings_starter_id,
		v2s32(m_screensize.X - (1.25 * button_size),
			m_screensize.Y - ((SETTINGS_BAR_Y_OFFSET + 1.0) * button_size)
				+ (0.5 * button_size)),
		v2s32(m_screensize.X - (0.25 * button_size),
			m_screensize.Y - (SETTINGS_BAR_Y_OFFSET * button_size)
				+ (0.5 * button_size)),
		AHBB_Dir_Right_Left, 3.0);

	m_settingsbar.addButton(fly_id,     L"fly",       "fly_btn.png");
	m_settingsbar.addButton(noclip_id,  L"noclip",    "noclip_btn.png");
	m_settingsbar.addButton(fast_id,    L"fast",      "fast_btn.png");
	m_settingsbar.addButton(debug_id,   L"debug",     "debug_btn.png");
	m_settingsbar.addButton(camera_id,  L"camera",    "camera_btn.png");
	m_settingsbar.addButton(range_id,   L"rangeview", "rangeview_btn.png");
	m_settingsbar.addButton(minimap_id, L"minimap",   "minimap_btn.png");

	// Chat is shown by default, so chat_hide_btn.png is shown first.
	m_settingsbar.addToggleButton(toggle_chat_id, L"togglechat",
			"chat_hide_btn.png", "chat_show_btn.png");

	m_rarecontrolsbar.init(m_texturesource, "rare_controls.png",
		rare_controls_starter_id,
		v2s32(0.25 * button_size,
			m_screensize.Y - ((RARE_CONTROLS_BAR_Y_OFFSET + 1.0) * button_size)
				+ (0.5 * button_size)),
		v2s32(0.75 * button_size,
			m_screensize.Y - (RARE_CONTROLS_BAR_Y_OFFSET * button_size)
				+ (0.5 * button_size)),
		AHBB_Dir_Left_Right, 2.0);

	m_rarecontrolsbar.addButton(chat_id,      L"Chat", "chat_btn.png");
	m_rarecontrolsbar.addButton(inventory_id, L"inv",  "inventory_btn.png");
	m_rarecontrolsbar.addButton(drop_id,      L"drop", "drop_btn.png");
}

touch_gui_button_id TouchScreenGUI::getButtonID(s32 x, s32 y)
{
	IGUIElement *rootguielement = m_guienv->getRootGUIElement();

	if (rootguielement != nullptr) {
		gui::IGUIElement *element =
				rootguielement->getElementFromPoint(core::position2d<s32>(x, y));

		if (element)
			for (unsigned int i = 0; i < after_last_element_id; i++)
				if (element == m_buttons[i].guibutton)
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

bool TouchScreenGUI::isHUDButton(const SEvent &event)
{
	// check if hud item is pressed
	for (auto &hud_rect : m_hud_rects) {
		if (hud_rect.second.isPointInside(v2s32(event.TouchInput.X,
				event.TouchInput.Y))) {
			auto *translated = new SEvent();
			memset(translated, 0, sizeof(SEvent));
			translated->EventType = irr::EET_KEY_INPUT_EVENT;
			translated->KeyInput.Key         = (irr::EKEY_CODE) (KEY_KEY_1 + hud_rect.first);
			translated->KeyInput.Control     = false;
			translated->KeyInput.Shift       = false;
			translated->KeyInput.PressedDown = true;
			m_receiver->OnEvent(*translated);
			m_hud_ids[event.TouchInput.ID]   = translated->KeyInput.Key;
			delete translated;
			return true;
		}
	}
	return false;
}

void TouchScreenGUI::handleButtonEvent(touch_gui_button_id button,
		size_t eventID, bool action)
{
	button_info *btn = &m_buttons[button];
	auto *translated = new SEvent();
	memset(translated, 0, sizeof(SEvent));
	translated->EventType            = irr::EET_KEY_INPUT_EVENT;
	translated->KeyInput.Key         = btn->keycode;
	translated->KeyInput.Control     = false;
	translated->KeyInput.Shift       = false;
	translated->KeyInput.Char        = 0;

	// add this event
	if (action) {
		assert(std::find(btn->ids.begin(), btn->ids.end(), eventID) == btn->ids.end());

		btn->ids.push_back(eventID);

		if (btn->ids.size() > 1) return;

		btn->repeatcounter = 0;
		translated->KeyInput.PressedDown = true;
		translated->KeyInput.Key = btn->keycode;
		m_receiver->OnEvent(*translated);
	}

	// remove event
	if ((!action) || (btn->immediate_release)) {
		auto pos = std::find(btn->ids.begin(), btn->ids.end(), eventID);
		// has to be in touch list
		assert(pos != btn->ids.end());
		btn->ids.erase(pos);

		if (!btn->ids.empty())
			return;

		translated->KeyInput.PressedDown = false;
		btn->repeatcounter               = -1;
		m_receiver->OnEvent(*translated);
	}
	delete translated;
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
			auto *translated = new SEvent;
			memset(translated, 0, sizeof(SEvent));
			translated->EventType               = EET_MOUSE_INPUT_EVENT;
			translated->MouseInput.X            = m_move_downlocation.X;
			translated->MouseInput.Y            = m_move_downlocation.Y;
			translated->MouseInput.Shift        = false;
			translated->MouseInput.Control      = false;
			translated->MouseInput.ButtonStates = 0;
			translated->MouseInput.Event        = EMIE_LMOUSE_LEFT_UP;
			if (m_draw_crosshair) {
				translated->MouseInput.X = m_screensize.X / 2;
				translated->MouseInput.Y = m_screensize.Y / 2;
			}
			m_receiver->OnEvent(*translated);
			delete translated;
		} else {
			// do double tap detection
			doubleTapDetection();
		}
	}

	// handle joystick
	else if (m_has_joystick_id && evt_id == m_joystick_id) {
		m_has_joystick_id = false;

		// reset joystick
		for (unsigned int i = 0; i < 4; i++)
			m_joystick_status[i] = false;
		applyJoystickStatus();

		m_joystick_btn_off->guibutton->setVisible(true);
		m_joystick_btn_bg->guibutton->setVisible(false);
		m_joystick_btn_center->guibutton->setVisible(false);
	} else {
		infostream
			<< "TouchScreenGUI::translateEvent released unknown button: "
			<< evt_id << std::endl;
	}

	for (auto iter = m_known_ids.begin();
			iter != m_known_ids.end(); ++iter) {
		if (iter->id == evt_id) {
			m_known_ids.erase(iter);
			break;
		}
	}
}

void TouchScreenGUI::translateEvent(const SEvent &event)
{
	if (!m_visible) {
		infostream
			<< "TouchScreenGUI::translateEvent got event but not visible!"
			<< std::endl;
		return;
	}

	if (event.EventType != EET_TOUCH_INPUT_EVENT)
		return;

	if (event.TouchInput.Event == ETIE_PRESSED_DOWN) {
		/*
		 * Add to own copy of event list...
		 * android would provide this information but Irrlicht guys don't
		 * wanna design an efficient interface
		 */
		id_status toadd{};
		toadd.id = event.TouchInput.ID;
		toadd.X  = event.TouchInput.X;
		toadd.Y  = event.TouchInput.Y;
		m_known_ids.push_back(toadd);

		size_t eventID = event.TouchInput.ID;

		touch_gui_button_id button =
				getButtonID(event.TouchInput.X, event.TouchInput.Y);

		// handle button events
		if (button != after_last_element_id) {
			handleButtonEvent(button, eventID, true);
			m_settingsbar.deactivate();
			m_rarecontrolsbar.deactivate();
		} else if (isHUDButton(event)) {
			m_settingsbar.deactivate();
			m_rarecontrolsbar.deactivate();
			// already handled in isHUDButton()
		} else if (m_settingsbar.isButton(event)) {
			m_rarecontrolsbar.deactivate();
			// already handled in isSettingsBarButton()
		} else if (m_rarecontrolsbar.isButton(event)) {
			m_settingsbar.deactivate();
			// already handled in isSettingsBarButton()
		} else {
			// handle non button events
			m_settingsbar.deactivate();
			m_rarecontrolsbar.deactivate();

			s32 dxj = event.TouchInput.X - button_size * 5.0f / 2.0f;
			s32 dyj = event.TouchInput.Y - (s32)m_screensize.Y + button_size * 5.0f / 2.0f;

			/* Select joystick when left 1/3 of screen dragged or
			 * when joystick tapped (fixed joystick position)
			 */
			if ((m_fixed_joystick && dxj * dxj + dyj * dyj <= button_size * button_size * 1.5 * 1.5) ||
					(!m_fixed_joystick && event.TouchInput.X < m_screensize.X / 3.0f)) {
				// If we don't already have a starting point for joystick make this the one.
				if (!m_has_joystick_id) {
					m_has_joystick_id           = true;
					m_joystick_id               = event.TouchInput.ID;
					m_joystick_has_really_moved = false;

					m_joystick_btn_off->guibutton->setVisible(false);
					m_joystick_btn_bg->guibutton->setVisible(true);
					m_joystick_btn_center->guibutton->setVisible(true);

					// If it's a fixed joystick, don't move the joystick "button".
					if (!m_fixed_joystick)
						m_joystick_btn_bg->guibutton->setRelativePosition(v2s32(
								event.TouchInput.X - button_size * 3.0f / 2.0f,
								event.TouchInput.Y - button_size * 3.0f / 2.0f));

					m_joystick_btn_center->guibutton->setRelativePosition(v2s32(
							event.TouchInput.X - button_size / 2.0f,
							event.TouchInput.Y - button_size / 2.0f));
				}
			} else {
				// If we don't already have a moving point make this the moving one.
				if (!m_has_move_id) {
					m_has_move_id              = true;
					m_move_id                  = event.TouchInput.ID;
					m_move_has_really_moved    = false;
					m_move_downtime            = porting::getTimeMs();
					m_move_downlocation        = v2s32(event.TouchInput.X, event.TouchInput.Y);
					m_move_sent_as_mouse_event = false;
					if (m_draw_crosshair)
						m_move_downlocation = v2s32(m_screensize.X / 2, m_screensize.Y / 2);
				}
			}
		}

		m_pointerpos[event.TouchInput.ID] = v2s32(event.TouchInput.X, event.TouchInput.Y);
	}
	else if (event.TouchInput.Event == ETIE_LEFT_UP) {
		verbosestream
			<< "Up event for pointerid: " << event.TouchInput.ID << std::endl;
		handleReleaseEvent(event.TouchInput.ID);
	} else {
		assert(event.TouchInput.Event == ETIE_MOVED);

		if (!(m_has_joystick_id && m_fixed_joystick) &&
				m_pointerpos[event.TouchInput.ID] ==
						v2s32(event.TouchInput.X, event.TouchInput.Y))
			return;

		if (m_has_move_id) {
			if (event.TouchInput.ID == m_move_id &&
					(!m_move_sent_as_mouse_event || m_draw_crosshair)) {
				double distance = sqrt(
						(m_pointerpos[event.TouchInput.ID].X - event.TouchInput.X) *
						(m_pointerpos[event.TouchInput.ID].X - event.TouchInput.X) +
						(m_pointerpos[event.TouchInput.ID].Y - event.TouchInput.Y) *
						(m_pointerpos[event.TouchInput.ID].Y - event.TouchInput.Y));

				if ((distance > m_touchscreen_threshold) ||
						(m_move_has_really_moved)) {
					m_move_has_really_moved = true;
					s32 X = event.TouchInput.X;
					s32 Y = event.TouchInput.Y;

					// update camera_yaw and camera_pitch
					s32 dx = X - m_pointerpos[event.TouchInput.ID].X;
					s32 dy = Y - m_pointerpos[event.TouchInput.ID].Y;
					m_pointerpos[event.TouchInput.ID] = v2s32(X, Y);

					// adapt to similar behavior as pc screen
					const double d = g_settings->getFloat("mouse_sensitivity", 0.001f, 10.0f) * 3.0f;

					m_camera_yaw_change -= dx * d;
					m_camera_pitch = MYMIN(MYMAX(m_camera_pitch + (dy * d), -180), 180);

					// update shootline
					// no need to update (X, Y) when using crosshair since the shootline is not used
					m_shootline = m_device
							->getSceneManager()
							->getSceneCollisionManager()
							->getRayFromScreenCoordinates(v2s32(X, Y));
				}
			} else if ((event.TouchInput.ID == m_move_id) &&
					(m_move_sent_as_mouse_event)) {
				m_shootline = m_device
						->getSceneManager()
						->getSceneCollisionManager()
						->getRayFromScreenCoordinates(
								v2s32(event.TouchInput.X, event.TouchInput.Y));
			}
		}

		if (m_has_joystick_id && event.TouchInput.ID == m_joystick_id) {
			s32 X = event.TouchInput.X;
			s32 Y = event.TouchInput.Y;

			s32 dx = X - m_pointerpos[event.TouchInput.ID].X;
			s32 dy = Y - m_pointerpos[event.TouchInput.ID].Y;
			if (m_fixed_joystick) {
				dx = X - button_size * 5.0f / 2.0f;
				dy = Y - (s32)m_screensize.Y + button_size * 5.0f / 2.0f;
			}

			double distance_sq = dx * dx + dy * dy;

			s32 dxj = event.TouchInput.X - button_size * 5.0f / 2.0f;
			s32 dyj = event.TouchInput.Y - (s32)m_screensize.Y + button_size * 5.0f / 2.0f;
			bool inside_joystick = (dxj * dxj + dyj * dyj <= button_size * button_size * 1.5 * 1.5);

			if (m_joystick_has_really_moved || inside_joystick ||
					(!m_fixed_joystick &&
					distance_sq > m_touchscreen_threshold * m_touchscreen_threshold)) {
				m_joystick_has_really_moved = true;
				double distance = sqrt(distance_sq);

				// angle in degrees
				double angle = acos(dx / distance) * 180 / M_PI;
				if (dy < 0)
					angle *= -1;
				// rotate to make comparing easier
				angle = fmod(angle + 180 + 22.5, 360);

				// reset state before applying
				for (bool & joystick_status : m_joystick_status)
					joystick_status = false;

				if (distance <= m_touchscreen_threshold) {
					// do nothing
				} else if (angle < 45)
					m_joystick_status[j_left] = true;
				else if (angle < 90) {
					m_joystick_status[j_forward] = true;
					m_joystick_status[j_left] = true;
				} else if (angle < 135)
					m_joystick_status[j_forward] = true;
				else if (angle < 180) {
					m_joystick_status[j_forward] = true;
					m_joystick_status[j_right] = true;
				} else if (angle < 225)
					m_joystick_status[j_right] = true;
				else if (angle < 270) {
					m_joystick_status[j_backward] = true;
					m_joystick_status[j_right] = true;
				} else if (angle < 315)
					m_joystick_status[j_backward] = true;
				else if (angle <= 360) {
					m_joystick_status[j_backward] = true;
					m_joystick_status[j_left] = true;
				}

				if (distance > button_size) {
					m_joystick_status[j_aux1] = true;
					// move joystick "button"
					s32 ndx = button_size * dx / distance - button_size / 2.0f;
					s32 ndy = button_size * dy / distance - button_size / 2.0f;
					if (m_fixed_joystick) {
						m_joystick_btn_center->guibutton->setRelativePosition(v2s32(
							button_size * 5.0f / 2.0f + ndx,
							m_screensize.Y - button_size * 5.0f / 2.0f + ndy));
					} else {
						m_joystick_btn_center->guibutton->setRelativePosition(v2s32(
							m_pointerpos[event.TouchInput.ID].X + ndx,
							m_pointerpos[event.TouchInput.ID].Y + ndy));
					}
				} else {
					m_joystick_btn_center->guibutton->setRelativePosition(
							v2s32(X - button_size / 2, Y - button_size / 2));
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

		for (auto iter = m_buttons[i].ids.begin();
				iter != m_buttons[i].ids.end(); ++iter) {
			if (event.TouchInput.ID == *iter) {
				int current_button_id =
						getButtonID(event.TouchInput.X, event.TouchInput.Y);

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
	if (std::find(btn->ids.begin(), btn->ids.end(), event.TouchInput.ID)
			== btn->ids.end())
		handleButtonEvent((touch_gui_button_id) current_button_id,
				event.TouchInput.ID, true);
}

bool TouchScreenGUI::doubleTapDetection()
{
	m_key_events[0].down_time = m_key_events[1].down_time;
	m_key_events[0].x         = m_key_events[1].x;
	m_key_events[0].y         = m_key_events[1].y;
	m_key_events[1].down_time = m_move_downtime;
	m_key_events[1].x         = m_move_downlocation.X;
	m_key_events[1].y         = m_move_downlocation.Y;

	u64 delta = porting::getDeltaMs(m_key_events[0].down_time, porting::getTimeMs());
	if (delta > 400)
		return false;

	double distance = sqrt(
			(m_key_events[0].x - m_key_events[1].x) *
			(m_key_events[0].x - m_key_events[1].x) +
			(m_key_events[0].y - m_key_events[1].y) *
			(m_key_events[0].y - m_key_events[1].y));

	if (distance > (20 + m_touchscreen_threshold))
		return false;

	v2s32 mPos = v2s32(m_key_events[0].x, m_key_events[0].y);
	if (m_draw_crosshair) {
		mPos.X = m_screensize.X / 2;
		mPos.Y = m_screensize.Y / 2;
	}

	auto *translated = new SEvent();
	memset(translated, 0, sizeof(SEvent));
	translated->EventType               = EET_MOUSE_INPUT_EVENT;
	translated->MouseInput.X            = mPos.X;
	translated->MouseInput.Y            = mPos.Y;
	translated->MouseInput.Shift        = false;
	translated->MouseInput.Control      = false;
	translated->MouseInput.ButtonStates = EMBSM_RIGHT;

	// update shootline
	m_shootline = m_device
			->getSceneManager()
			->getSceneCollisionManager()
			->getRayFromScreenCoordinates(mPos);

	translated->MouseInput.Event = EMIE_RMOUSE_PRESSED_DOWN;
	verbosestream << "TouchScreenGUI::translateEvent right click press" << std::endl;
	m_receiver->OnEvent(*translated);

	translated->MouseInput.ButtonStates = 0;
	translated->MouseInput.Event = EMIE_RMOUSE_LEFT_UP;
	verbosestream << "TouchScreenGUI::translateEvent right click release" << std::endl;
	m_receiver->OnEvent(*translated);
	delete translated;
	return true;
}

void TouchScreenGUI::applyJoystickStatus()
{
	for (unsigned int i = 0; i < 5; i++) {
		if (i == 4 && !m_joystick_triggers_aux1)
			continue;

		SEvent translated{};
		translated.EventType            = irr::EET_KEY_INPUT_EVENT;
		translated.KeyInput.Key         = id2keycode(m_joystick_names[i]);
		translated.KeyInput.PressedDown = false;
		m_receiver->OnEvent(translated);

		if (m_joystick_status[i]) {
			translated.KeyInput.PressedDown = true;
			m_receiver->OnEvent(translated);
		}
	}
}

TouchScreenGUI::~TouchScreenGUI()
{
	for (auto &button : m_buttons) {
		if (button.guibutton) {
			button.guibutton->drop();
			button.guibutton = nullptr;
		}
	}

	if (m_joystick_btn_off->guibutton) {
		m_joystick_btn_off->guibutton->drop();
		m_joystick_btn_off->guibutton = nullptr;
	}

	if (m_joystick_btn_bg->guibutton) {
		m_joystick_btn_bg->guibutton->drop();
		m_joystick_btn_bg->guibutton = nullptr;
	}

	if (m_joystick_btn_center->guibutton) {
		m_joystick_btn_center->guibutton->drop();
		m_joystick_btn_center->guibutton = nullptr;
	}
}

void TouchScreenGUI::step(float dtime)
{
	// simulate keyboard repeats
	for (auto &button : m_buttons) {
		if (!button.ids.empty()) {
			button.repeatcounter += dtime;

			// in case we're moving around digging does not happen
			if (m_has_move_id)
				m_move_has_really_moved = true;

			if (button.repeatcounter < button.repeatdelay)
				continue;

			button.repeatcounter            = 0;
			SEvent translated;
			memset(&translated, 0, sizeof(SEvent));
			translated.EventType            = irr::EET_KEY_INPUT_EVENT;
			translated.KeyInput.Key         = button.keycode;
			translated.KeyInput.PressedDown = false;
			m_receiver->OnEvent(translated);

			translated.KeyInput.PressedDown = true;
			m_receiver->OnEvent(translated);
		}
	}

	// joystick
	for (unsigned int i = 0; i < 4; i++) {
		if (m_joystick_status[i]) {
			applyJoystickStatus();
			break;
		}
	}

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
					->getRayFromScreenCoordinates(
							v2s32(mX, mY));

			SEvent translated;
			memset(&translated, 0, sizeof(SEvent));
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

	m_settingsbar.step(dtime);
	m_rarecontrolsbar.step(dtime);
}

void TouchScreenGUI::resetHud()
{
	m_hud_rects.clear();
}

void TouchScreenGUI::registerHudItem(int index, const rect<s32> &rect)
{
	m_hud_rects[index] = rect;
}

void TouchScreenGUI::Toggle(bool visible)
{
	m_visible = visible;
	for (auto &button : m_buttons) {
		if (button.guibutton)
			button.guibutton->setVisible(visible);
	}

	if (m_joystick_btn_off->guibutton)
		m_joystick_btn_off->guibutton->setVisible(visible);

	// clear all active buttons
	if (!visible) {
		while (!m_known_ids.empty())
			handleReleaseEvent(m_known_ids.begin()->id);

		m_settingsbar.hide();
		m_rarecontrolsbar.hide();
	} else {
		m_settingsbar.show();
		m_rarecontrolsbar.show();
	}
}

void TouchScreenGUI::hide()
{
	if (!m_visible)
		return;

	Toggle(false);
}

void TouchScreenGUI::show()
{
	if (m_visible)
		return;

	Toggle(true);
}
