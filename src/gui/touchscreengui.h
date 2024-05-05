/*
Copyright (C) 2014 sapier
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

#pragma once

#include "irrlichttypes.h"
#include <IEventReceiver.h>
#include <IGUIImage.h>
#include <IGUIEnvironment.h>
#include <IrrlichtDevice.h>

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "itemdef.h"
#include "client/game.h"

using namespace irr;
using namespace irr::core;
using namespace irr::gui;


// We cannot use irr_ptr for Irrlicht GUI elements we own.
// Option 1: Pass IGUIElement* returned by IGUIEnvironment::add* into irr_ptr
//           constructor.
//           -> We steal the reference owned by IGUIEnvironment and drop it later,
//           causing the IGUIElement to be deleted while IGUIEnvironment still
//           references it.
// Option 2: Pass IGUIElement* returned by IGUIEnvironment::add* into irr_ptr::grab.
//           -> We add another reference and drop it later, but since IGUIEnvironment
//           still references the IGUIElement, it is never deleted.
// To make IGUIEnvironment drop its reference to the IGUIElement, we have to call
// IGUIElement::remove, so that's what we'll do.
template <typename T>
std::shared_ptr<T> grab_gui_element(T *element)
{
	static_assert(std::is_base_of_v<IGUIElement, T>,
			"grab_gui_element only works for IGUIElement");
	return std::shared_ptr<T>(element, [](T *e) {
		e->remove();
	});
}


enum class TapState
{
	None,
	ShortTap,
	LongTap,
};

enum touch_gui_button_id
{
	jump_id = 0,
	sneak_id,
	zoom_id,
	aux1_id,
	settings_starter_id,
	rare_controls_starter_id,

	// usually in the "settings bar"
	fly_id,
	noclip_id,
	fast_id,
	debug_id,
	camera_id,
	range_id,
	minimap_id,
	toggle_chat_id,

	// usually in the "rare controls bar"
	chat_id,
	inventory_id,
	drop_id,
	exit_id,

	// the joystick
	joystick_off_id,
	joystick_bg_id,
	joystick_center_id,
};

enum autohide_button_bar_dir
{
	AHBB_Dir_Top_Bottom,
	AHBB_Dir_Bottom_Top,
	AHBB_Dir_Left_Right,
	AHBB_Dir_Right_Left
};

#define BUTTON_REPEAT_DELAY 0.5f
#define BUTTON_REPEAT_INTERVAL 0.333f
#define BUTTONBAR_HIDE_DELAY 3.0f
#define SETTINGS_BAR_Y_OFFSET 5
#define RARE_CONTROLS_BAR_Y_OFFSET 5

// Our simulated clicks last some milliseconds so that server-side mods have a
// chance to detect them via l_get_player_control.
// If you tap faster than this value, the simulated clicks are of course shorter.
#define SIMULATED_CLICK_DURATION_MS 50

struct button_info
{
	float repeat_counter;
	EKEY_CODE keycode;
	std::vector<size_t> pointer_ids;
	std::shared_ptr<IGUIImage> gui_button = nullptr;

	enum {
		NOT_TOGGLEABLE,
		FIRST_TEXTURE,
		SECOND_TEXTURE
	} toggleable = NOT_TOGGLEABLE;
	std::string toggle_textures[2];

	void emitAction(bool action, video::IVideoDriver *driver,
			IEventReceiver *receiver, ISimpleTextureSource *tsrc);
};

class AutoHideButtonBar
{
public:
	AutoHideButtonBar(IrrlichtDevice *device, ISimpleTextureSource *tsrc,
			touch_gui_button_id starter_id, const std::string &starter_image,
			recti starter_rect, autohide_button_bar_dir dir);

	void addButton(touch_gui_button_id id, const std::string &image);
	void addToggleButton(touch_gui_button_id id,
			const std::string &image_1, const std::string &image_2);

	bool handlePress(size_t pointer_id, IGUIElement *element);
	bool handleRelease(size_t pointer_id);

	void step(float dtime);

	void activate();
	void deactivate();
	bool isActive() { return m_active; }

	void show();
	void hide();

	bool operator==(const AutoHideButtonBar &other)
			{ return m_starter.get() == other.m_starter.get(); }
	bool operator!=(const AutoHideButtonBar &other)
			{ return m_starter.get() != other.m_starter.get(); }

private:
	irr::video::IVideoDriver *m_driver = nullptr;
	IGUIEnvironment *m_guienv = nullptr;
	IEventReceiver *m_receiver = nullptr;
	ISimpleTextureSource *m_texturesource = nullptr;
	std::shared_ptr<IGUIImage> m_starter;
	std::vector<button_info> m_buttons;

	v2s32 m_upper_left;
	v2s32 m_lower_right;

	bool m_active = false;
	bool m_visible = true;
	float m_timeout = 0.0f;
	autohide_button_bar_dir m_dir = AHBB_Dir_Right_Left;

	void updateVisibility();
};

class TouchScreenGUI
{
public:
	TouchScreenGUI(IrrlichtDevice *device, ISimpleTextureSource *tsrc);

	void translateEvent(const SEvent &event);
	void applyContextControls(const TouchInteractionMode &mode);

	double getYawChange()
	{
		double res = m_camera_yaw_change;
		m_camera_yaw_change = 0;
		return res;
	}

	double getPitchChange() {
		double res = m_camera_pitch_change;
		m_camera_pitch_change = 0;
		return res;
	}

	/**
	 * Returns a line which describes what the player is pointing at.
	 * The starting point and looking direction are significant,
	 * the line should be scaled to match its length to the actual distance
	 * the player can reach.
	 * The line starts at the camera and ends on the camera's far plane.
	 * The coordinates do not contain the camera offset.
	 */
	line3d<f32> getShootline() { return m_shootline; }

	float getMovementDirection() { return m_joystick_direction; }
	float getMovementSpeed() { return m_joystick_speed; }

	void step(float dtime);
	inline void setUseCrosshair(bool use_crosshair) { m_draw_crosshair = use_crosshair; }

	void setVisible(bool visible);
	void hide();
	void show();

	void resetHotbarRects();
	void registerHotbarRect(u16 index, const recti &rect);
	std::optional<u16> getHotbarSelection();

private:
	IrrlichtDevice *m_device = nullptr;
	IGUIEnvironment *m_guienv = nullptr;
	IEventReceiver *m_receiver = nullptr;
	ISimpleTextureSource *m_texturesource = nullptr;
	v2u32 m_screensize;
	s32 m_button_size;
	double m_touchscreen_threshold;
	u16 m_long_tap_delay;
	bool m_visible = true; // is the whole touch screen gui visible

	std::unordered_map<u16, recti> m_hotbar_rects;
	std::optional<u16> m_hotbar_selection = std::nullopt;

	// value in degree
	double m_camera_yaw_change = 0.0;
	double m_camera_pitch_change = 0.0;

	/**
	 * A line starting at the camera and pointing towards the selected object.
	 * The line ends on the camera's far plane.
	 * The coordinates do not contain the camera offset.
	 */
	line3d<f32> m_shootline;

	bool m_has_move_id = false;
	size_t m_move_id;
	bool m_move_has_really_moved = false;
	u64 m_move_downtime = 0;
	// m_move_pos stays valid even after m_move_id has been released.
	v2s32 m_move_pos;
	// This is needed so that we don't miss if m_has_move_id is true for less
	// than one client step, i.e. press and release happen in the same step.
	bool m_had_move_id = false;

	bool m_has_joystick_id = false;
	size_t m_joystick_id;
	bool m_joystick_has_really_moved = false;
	float m_joystick_direction = 0.0f; // assume forward
	float m_joystick_speed = 0.0f; // no movement
	bool m_joystick_status_aux1 = false;
	bool m_fixed_joystick = false;
	bool m_joystick_triggers_aux1 = false;
	bool m_draw_crosshair = false;
	std::shared_ptr<IGUIImage> m_joystick_btn_off;
	std::shared_ptr<IGUIImage> m_joystick_btn_bg;
	std::shared_ptr<IGUIImage> m_joystick_btn_center;

	std::vector<button_info> m_buttons;

	// initialize a button
	void addButton(touch_gui_button_id id, const std::string &image,
			const recti &rect);

	// initialize a joystick button
	IGUIImage *makeJoystickButton(touch_gui_button_id id,
			const recti &rect, bool visible);

	// handle pressing hotbar items
	bool isHotbarButton(const SEvent &event);

	// handle release event
	void handleReleaseEvent(size_t pointer_id);

	// apply joystick status
	void applyJoystickStatus();

	// map to store the IDs and original positions of currently pressed pointers
	std::unordered_map<size_t, v2s32> m_pointer_downpos;
	// map to store the IDs and positions of currently pressed pointers
	std::unordered_map<size_t, v2s32> m_pointer_pos;

	std::vector<AutoHideButtonBar> m_buttonbars;

	v2s32 getPointerPos();
	void emitMouseEvent(EMOUSE_INPUT_EVENT type);
	TouchInteractionMode m_last_mode = TouchInteractionMode_END;
	TapState m_tap_state = TapState::None;

	bool m_dig_pressed = false;
	u64 m_dig_pressed_until = 0;

	bool m_place_pressed = false;
	u64 m_place_pressed_until = 0;
};

extern TouchScreenGUI *g_touchscreengui;
