/*
Copyright (C) 2014 sapier

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

#include "irrlichttypes.h"
#include <IEventReceiver.h>
#include <IGUIButton.h>
#include <IGUIEnvironment.h>
#include <IrrlichtDevice.h>

#include <map>
#include <vector>

#include "client/tile.h"
#include "client/game.h"

using namespace irr;
using namespace irr::core;
using namespace irr::gui;

typedef enum
{
	jump_id = 0,
	crunch_id,
	zoom_id,
	aux1_id,
	after_last_element_id,
	settings_starter_id,
	rare_controls_starter_id,
	fly_id,
	noclip_id,
	fast_id,
	debug_id,
	camera_id,
	range_id,
	minimap_id,
	toggle_chat_id,
	chat_id,
	inventory_id,
	drop_id,
	forward_id,
	backward_id,
	left_id,
	right_id,
	joystick_off_id,
	joystick_bg_id,
	joystick_center_id
} touch_gui_button_id;

typedef enum
{
	j_forward = 0,
	j_backward,
	j_left,
	j_right,
	j_aux1
} touch_gui_joystick_move_id;

typedef enum
{
	AHBB_Dir_Top_Bottom,
	AHBB_Dir_Bottom_Top,
	AHBB_Dir_Left_Right,
	AHBB_Dir_Right_Left
} autohide_button_bar_dir;

#define MIN_DIG_TIME_MS 500
#define BUTTON_REPEAT_DELAY 0.2f
#define SETTINGS_BAR_Y_OFFSET 5
#define RARE_CONTROLS_BAR_Y_OFFSET 5

// Very slow button repeat frequency
#define SLOW_BUTTON_REPEAT 1.0f

extern const char **button_imagenames;
extern const char **joystick_imagenames;

struct button_info
{
	float repeatcounter;
	float repeatdelay;
	irr::EKEY_CODE keycode;
	std::vector<size_t> ids;
	IGUIButton *guibutton = nullptr;
	bool immediate_release;

	// 0: false, 1: (true) first texture, 2: (true) second texture
	int togglable = 0;
	std::vector<const char *> textures;
};

class AutoHideButtonBar
{
public:
	AutoHideButtonBar(IrrlichtDevice *device, IEventReceiver *receiver);

	void init(ISimpleTextureSource *tsrc, const char *starter_img, int button_id,
			const v2s32 &UpperLeft, const v2s32 &LowerRight,
			autohide_button_bar_dir dir, float timeout);

	~AutoHideButtonBar();

	// add button to be shown
	void addButton(touch_gui_button_id id, const wchar_t *caption,
			const char *btn_image);

	// add toggle button to be shown
	void addToggleButton(touch_gui_button_id id, const wchar_t *caption,
			const char *btn_image_1, const char *btn_image_2);

	// detect settings bar button events
	bool isButton(const SEvent &event);

	// step handler
	void step(float dtime);

	// deactivate button bar
	void deactivate();

	// hide the whole buttonbar
	void hide();

	// unhide the buttonbar
	void show();

private:
	ISimpleTextureSource *m_texturesource = nullptr;
	irr::video::IVideoDriver *m_driver;
	IGUIEnvironment *m_guienv;
	IEventReceiver *m_receiver;
	button_info m_starter;
	std::vector<button_info *> m_buttons;

	v2s32 m_upper_left;
	v2s32 m_lower_right;

	// show settings bar
	bool m_active = false;

	bool m_visible = true;

	// settings bar timeout
	float m_timeout = 0.0f;
	float m_timeout_value = 3.0f;
	bool m_initialized = false;
	autohide_button_bar_dir m_dir = AHBB_Dir_Right_Left;
};

class TouchScreenGUI
{
public:
	TouchScreenGUI(IrrlichtDevice *device, IEventReceiver *receiver);
	~TouchScreenGUI();

	void translateEvent(const SEvent &event);

	void init(ISimpleTextureSource *tsrc);

	double getYawChange()
	{
		double res = m_camera_yaw_change;
		m_camera_yaw_change = 0;
		return res;
	}

	double getPitch() { return m_camera_pitch; }

	/*
	 * Returns a line which describes what the player is pointing at.
	 * The starting point and looking direction are significant,
	 * the line should be scaled to match its length to the actual distance
	 * the player can reach.
	 * The line starts at the camera and ends on the camera's far plane.
	 * The coordinates do not contain the camera offset.
	 */
	line3d<f32> getShootline() { return m_shootline; }

	void step(float dtime);
	void resetHud();
	void registerHudItem(int index, const rect<s32> &rect);
	inline void setUseCrosshair(bool use_crosshair) { m_draw_crosshair = use_crosshair; }
	void Toggle(bool visible);

	void hide();
	void show();

private:
	IrrlichtDevice *m_device;
	IGUIEnvironment *m_guienv;
	IEventReceiver *m_receiver;
	ISimpleTextureSource *m_texturesource;
	v2u32 m_screensize;
	s32 button_size;
	double m_touchscreen_threshold;
	std::map<int, rect<s32>> m_hud_rects;
	std::map<size_t, irr::EKEY_CODE> m_hud_ids;
	bool m_visible; // is the gui visible

	// value in degree
	double m_camera_yaw_change = 0.0;
	double m_camera_pitch = 0.0;

	// forward, backward, left, right
	touch_gui_button_id m_joystick_names[5] = {
			forward_id, backward_id, left_id, right_id, aux1_id};
	bool m_joystick_status[5] = {false, false, false, false, false};

	/*
	 * A line starting at the camera and pointing towards the
	 * selected object.
	 * The line ends on the camera's far plane.
	 * The coordinates do not contain the camera offset.
	 */
	line3d<f32> m_shootline;

	bool m_has_move_id = false;
	size_t m_move_id;
	bool m_move_has_really_moved = false;
	u64 m_move_downtime = 0;
	bool m_move_sent_as_mouse_event = false;
	v2s32 m_move_downlocation = v2s32(-10000, -10000);

	bool m_has_joystick_id = false;
	size_t m_joystick_id;
	bool m_joystick_has_really_moved = false;
	bool m_fixed_joystick = false;
	bool m_joystick_triggers_aux1 = false;
	bool m_draw_crosshair = false;
	button_info *m_joystick_btn_off = nullptr;
	button_info *m_joystick_btn_bg = nullptr;
	button_info *m_joystick_btn_center = nullptr;

	button_info m_buttons[after_last_element_id];

	// gui button detection
	touch_gui_button_id getButtonID(s32 x, s32 y);

	// gui button by eventID
	touch_gui_button_id getButtonID(size_t eventID);

	// check if a button has changed
	void handleChangedButton(const SEvent &event);

	// initialize a button
	void initButton(touch_gui_button_id id, const rect<s32> &button_rect,
			const std::wstring &caption, bool immediate_release,
			float repeat_delay = BUTTON_REPEAT_DELAY);

	// initialize a joystick button
	button_info *initJoystickButton(touch_gui_button_id id,
			const rect<s32> &button_rect, int texture_id,
			bool visible = true);

	struct id_status
	{
		size_t id;
		int X;
		int Y;
	};

	// vector to store known ids and their initial touch positions
	std::vector<id_status> m_known_ids;

	// handle a button event
	void handleButtonEvent(touch_gui_button_id bID, size_t eventID, bool action);

	// handle pressed hud buttons
	bool isHUDButton(const SEvent &event);

	// handle double taps
	bool doubleTapDetection();

	// handle release event
	void handleReleaseEvent(size_t evt_id);

	// apply joystick status
	void applyJoystickStatus();

	// double-click detection variables
	struct key_event
	{
		u64 down_time;
		s32 x;
		s32 y;
	};

	// array for saving last known position of a pointer
	std::map<size_t, v2s32> m_pointerpos;

	// array for double tap detection
	key_event m_key_events[2];

	// settings bar
	AutoHideButtonBar m_settingsbar;

	// rare controls bar
	AutoHideButtonBar m_rarecontrolsbar;
};

extern TouchScreenGUI *g_touchscreengui;
