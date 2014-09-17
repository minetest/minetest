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

#ifndef TOUCHSCREENGUI_HEADER
#define TOUCHSCREENGUI_HEADER

#include <IGUIEnvironment.h>
#include <IGUIButton.h>
#include <IEventReceiver.h>

#include <vector>
#include <map>

#include "game.h"
#include "tile.h"

using namespace irr;
using namespace irr::core;
using namespace irr::gui;

typedef enum {
	forward_id = 0,
	backward_id,
	left_id,
	right_id,
	jump_id,
	crunch_id,
	inventory_id,
	chat_id,
	after_last_element_id
} touch_gui_button_id;

#define MIN_DIG_TIME_MS 500
#define MAX_TOUCH_COUNT 64

extern const char** touchgui_button_imagenames;

class TouchScreenGUI
{
public:
	TouchScreenGUI(IrrlichtDevice *device, IEventReceiver* receiver);
	~TouchScreenGUI();

	void translateEvent(const SEvent &event);

	void init(ISimpleTextureSource* tsrc,float density);

	double getYaw() { return m_camera_yaw; }
	double getPitch() { return m_camera_pitch; }
	line3d<f32> getShootline() { return m_shootline; }

	void step(float dtime);
	void resetHud();
	void registerHudItem(int index, const rect<s32> &rect);
	void Toggle(bool visible);

	void Hide();
	void Show();

private:
	IrrlichtDevice*         m_device;
	IGUIEnvironment*        m_guienv;
	IEventReceiver*         m_receiver;
	ISimpleTextureSource*   m_texturesource;
	v2u32                   m_screensize;
	std::map<int,rect<s32> > m_hud_rects;
	std::map<int,irr::EKEY_CODE> m_hud_ids;
	bool                    m_visible; // is the gui visible

	/* value in degree */
	double                  m_camera_yaw;
	double                  m_camera_pitch;

	line3d<f32>             m_shootline;

	rect<s32>               m_control_pad_rect;

	int                     m_move_id;
	bool                    m_move_has_really_moved;
	s32                     m_move_downtime;
	bool                    m_move_sent_as_mouse_event;
	v2s32                   m_move_downlocation;

	struct button_info {
		float            repeatcounter;
		irr::EKEY_CODE   keycode;
		std::vector<int> ids;
		IGUIButton*      guibutton;
		bool             immediate_release;
	};

	button_info m_buttons[after_last_element_id];

	/* gui button detection */
	touch_gui_button_id getButtonID(s32 x, s32 y);

	/* gui button by eventID */
	touch_gui_button_id getButtonID(int eventID);

	/* check if a button has changed */
	void handleChangedButton(const SEvent &event);

	/* initialize a button */
	void initButton(touch_gui_button_id id, rect<s32> button_rect,
			std::wstring caption, bool immediate_release );

	/* load texture */
	void loadButtonTexture(button_info* btn, const char* path);

	struct id_status{
		int id;
		int X;
		int Y;
	};

	/* vector to store known ids and their initial touch positions*/
	std::vector<id_status> m_known_ids;

	/* handle a button event */
	void ButtonEvent(touch_gui_button_id bID, int eventID, bool action);

	/* handle pressed hud buttons */
	bool isHUDButton(const SEvent &event);

	/* handle released hud buttons */
	bool isReleaseHUDButton(int eventID);

	/* handle double taps */
	bool doubleTapDetection();

	/* doubleclick detection variables */
	struct key_event {
		unsigned int down_time;
		s32 x;
		s32 y;
	};

	/* array for saving last known position of a pointer */
	v2s32 m_pointerpos[MAX_TOUCH_COUNT];

	/* array for doubletap detection */
	key_event m_key_events[2];
};
extern TouchScreenGUI *g_touchscreengui;
#endif
