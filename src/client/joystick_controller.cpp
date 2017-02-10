/*
Minetest
Copyright (C) 2016 est31, <MTest31@outlook.com>

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

#include "joystick_controller.h"
#include "irrlichttypes_extrabloated.h"
#include "keys.h"
#include "settings.h"
#include "gettime.h"

bool JoystickButtonCmb::isTriggered(const irr::SEvent::SJoystickEvent &ev) const
{
	u32 buttons = ev.ButtonStates;

	buttons &= filter_mask;
	return buttons == compare_mask;
}

bool JoystickAxisCmb::isTriggered(const irr::SEvent::SJoystickEvent &ev) const
{
	s16 ax_val = ev.Axis[axis_to_compare];

	return (ax_val * direction < 0) && (thresh * direction > ax_val * direction);
}

// spares many characters
#define JLO_B_PB(A, B, C)    jlo.button_keys.push_back(JoystickButtonCmb(A, B, C))
#define JLO_A_PB(A, B, C, D) jlo.axis_keys.push_back(JoystickAxisCmb(A, B, C, D))

static JoystickLayout create_default_layout()
{
	JoystickLayout jlo;

	jlo.axes_dead_border = 1024;

	const JoystickAxisLayout axes[JA_COUNT] = {
		{0, 1}, // JA_SIDEWARD_MOVE
		{1, 1}, // JA_FORWARD_MOVE
		{3, 1}, // JA_FRUSTUM_HORIZONTAL
		{4, 1}, // JA_FRUSTUM_VERTICAL
	};
	memcpy(jlo.axes, axes, sizeof(jlo.axes));

	u32 sb = 1 << 7; // START button mask
	u32 fb = 1 << 3; // FOUR button mask
	u32 bm = sb | fb; // Mask for Both Modifiers

	// The back button means "ESC".
	JLO_B_PB(KeyType::ESC,        1 << 6,      1 << 6);

	// The start button counts as modifier as well as use key.
	// JLO_B_PB(KeyType::USE,        sb,          sb));

	// Accessible without start modifier button pressed
	// regardless whether four is pressed or not
	JLO_B_PB(KeyType::SNEAK,      sb | 1 << 2, 1 << 2);

	// Accessible without four modifier button pressed
	// regardless whether start is pressed or not
	JLO_B_PB(KeyType::MOUSE_L,    fb | 1 << 4, 1 << 4);
	JLO_B_PB(KeyType::MOUSE_R,    fb | 1 << 5, 1 << 5);

	// Accessible without any modifier pressed
	JLO_B_PB(KeyType::JUMP,       bm | 1 << 0, 1 << 0);
	JLO_B_PB(KeyType::SPECIAL1,   bm | 1 << 1, 1 << 1);

	// Accessible with start button not pressed, but four pressed
	// TODO find usage for button 0
	JLO_B_PB(KeyType::DROP,       bm | 1 << 1, fb | 1 << 1);
	JLO_B_PB(KeyType::SCROLL_UP,  bm | 1 << 4, fb | 1 << 4);
	JLO_B_PB(KeyType::SCROLL_DOWN,bm | 1 << 5, fb | 1 << 5);

	// Accessible with start button and four pressed
	// TODO find usage for buttons 0, 1 and 4, 5

	// Now about the buttons simulated by the axes

	// Movement buttons, important for vessels
	JLO_A_PB(KeyType::FORWARD,  1,  1, 1024);
	JLO_A_PB(KeyType::BACKWARD, 1, -1, 1024);
	JLO_A_PB(KeyType::LEFT,     0,  1, 1024);
	JLO_A_PB(KeyType::RIGHT,    0, -1, 1024);

	// Scroll buttons
	JLO_A_PB(KeyType::SCROLL_UP,   2, -1, 1024);
	JLO_A_PB(KeyType::SCROLL_DOWN, 5, -1, 1024);

	return jlo;
}

static const JoystickLayout default_layout = create_default_layout();

JoystickController::JoystickController()
{
	m_layout = &default_layout;
	doubling_dtime = g_settings->getFloat("repeat_joystick_button_time");

	for (size_t i = 0; i < KeyType::INTERNAL_ENUM_COUNT; i++) {
		m_past_pressed_time[i] = 0;
	}
	clear();
}

bool JoystickController::handleEvent(const irr::SEvent::SJoystickEvent &ev)
{
	m_internal_time = getTimeMs() / 1000.f;

	std::bitset<KeyType::INTERNAL_ENUM_COUNT> keys_pressed;

	// First generate a list of keys pressed

	for (size_t i = 0; i < m_layout->button_keys.size(); i++) {
		if (m_layout->button_keys[i].isTriggered(ev)) {
			keys_pressed.set(m_layout->button_keys[i].key);
		}
	}

	for (size_t i = 0; i < m_layout->axis_keys.size(); i++) {
		if (m_layout->axis_keys[i].isTriggered(ev)) {
			keys_pressed.set(m_layout->axis_keys[i].key);
		}
	}

	// Then update the values

	for (size_t i = 0; i < KeyType::INTERNAL_ENUM_COUNT; i++) {
		if (keys_pressed[i]) {
			if (!m_past_pressed_keys[i] &&
					m_past_pressed_time[i] < m_internal_time - doubling_dtime) {
				m_past_pressed_keys[i] = true;
				m_past_pressed_time[i] = m_internal_time;
			}
		} else if (m_pressed_keys[i]) {
			m_past_released_keys[i] = true;
		}

		m_pressed_keys[i] = keys_pressed[i];
	}

	for (size_t i = 0; i < JA_COUNT; i++) {
		const JoystickAxisLayout &ax_la = m_layout->axes[i];
		m_axes_vals[i] = ax_la.invert * ev.Axis[ax_la.axis_id];
	}


	return true;
}

void JoystickController::clear()
{
	m_pressed_keys.reset();
	m_past_pressed_keys.reset();
	m_past_released_keys.reset();
	memset(m_axes_vals, 0, sizeof(m_axes_vals));
}

s16 JoystickController::getAxisWithoutDead(JoystickAxis axis)
{
	s16 v = m_axes_vals[axis];
	if (((v > 0) && (v < m_layout->axes_dead_border)) ||
			((v < 0) && (v > -m_layout->axes_dead_border)))
		return 0;
	return v;
}
