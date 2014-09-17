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

#include "touchscreengui.h"
#include "irrlichttypes.h"
#include "irr_v2d.h"
#include "log.h"
#include "keycode.h"
#include "settings.h"
#include "gettime.h"
#include "util/numeric.h"
#include "porting.h"

#include <iostream>
#include <algorithm>

#include <ISceneCollisionManager.h>

using namespace irr::core;

extern Settings *g_settings;

const char** touchgui_button_imagenames = (const char*[]) {
	"up_arrow.png",
	"down_arrow.png",
	"left_arrow.png",
	"right_arrow.png",
	"jump_btn.png",
	"down.png",
	"inventory_btn.png",
	"chat_btn.png"
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
		case jump_id:
			key = "jump";
			break;
		case inventory_id:
			key = "inventory";
			break;
		case chat_id:
			key = "chat";
			break;
		case crunch_id:
			key = "sneak";
			break;
	}
	assert(key != "");
	return keyname_to_keycode(g_settings->get("keymap_" + key).c_str());
}

TouchScreenGUI *g_touchscreengui;

TouchScreenGUI::TouchScreenGUI(IrrlichtDevice *device, IEventReceiver* receiver):
	m_device(device),
	m_guienv(device->getGUIEnvironment()),
	m_camera_yaw(0.0),
	m_camera_pitch(0.0),
	m_visible(false),
	m_move_id(-1),
	m_receiver(receiver)
{
	for (unsigned int i=0; i < after_last_element_id; i++) {
		m_buttons[i].guibutton     =  0;
		m_buttons[i].repeatcounter = -1;
	}

	m_screensize = m_device->getVideoDriver()->getScreenSize();
}

void TouchScreenGUI::loadButtonTexture(button_info* btn, const char* path)
{
	unsigned int tid;
	video::ITexture *texture = m_texturesource->getTexture(path,&tid);
	if (texture) {
		btn->guibutton->setUseAlphaChannel(true);
		btn->guibutton->setImage(texture);
		btn->guibutton->setPressedImage(texture);
		btn->guibutton->setScaleImage(true);
		btn->guibutton->setDrawBorder(false);
		btn->guibutton->setText(L"");
		}
}

void TouchScreenGUI::initButton(touch_gui_button_id id, rect<s32> button_rect,
		std::wstring caption, bool immediate_release )
{

	button_info* btn       = &m_buttons[id];
	btn->guibutton         = m_guienv->addButton(button_rect, 0, id, caption.c_str());
	btn->guibutton->grab();
	btn->repeatcounter     = -1;
	btn->keycode           = id2keycode(id);
	btn->immediate_release = immediate_release;
	btn->ids.clear();

	loadButtonTexture(btn,touchgui_button_imagenames[id]);
}

static int getMaxControlPadSize(float density) {
	return 200 * density * g_settings->getFloat("gui_scaling");
}

void TouchScreenGUI::init(ISimpleTextureSource* tsrc, float density)
{
	assert(tsrc != 0);

	u32 control_pad_size =
			MYMIN((2 * m_screensize.Y) / 3,getMaxControlPadSize(density));

	u32 button_size      = control_pad_size / 3;
	m_visible            = true;
	m_texturesource      = tsrc;
	m_control_pad_rect   = rect<s32>(0, m_screensize.Y - 3 * button_size,
			3 * button_size, m_screensize.Y);
	/*
	draw control pad
	0 1 2
	3 4 5
	for now only 0, 1, 2, and 4 are used
	*/
	int number = 0;
	for (int y = 0; y < 2; ++y)
		for (int x = 0; x < 3; ++x, ++number) {
			rect<s32> button_rect(
					x * button_size, m_screensize.Y - button_size * (2 - y),
					(x + 1) * button_size, m_screensize.Y - button_size * (1 - y)
			);
			touch_gui_button_id id = after_last_element_id;
			std::wstring caption;
			switch (number) {
			case 0:
				id = left_id;
				caption = L"<";
				break;
			case 1:
				id = forward_id;
				caption = L"^";
				break;
			case 2:
				id = right_id;
				caption = L">";
				break;
			case 4:
				id = backward_id;
				caption = L"v";
				break;
			}
			if (id != after_last_element_id) {
				initButton(id, button_rect, caption, false);
				}
		}

	/* init inventory button */
	initButton(inventory_id,
			rect<s32>(0, m_screensize.Y - (button_size/2),
					(button_size/2), m_screensize.Y), L"inv", true);

	/* init jump button */
	initButton(jump_id,
			rect<s32>(m_screensize.X-(1.75*button_size),
					m_screensize.Y - (0.5*button_size),
					m_screensize.X-(0.25*button_size),
					m_screensize.Y),
			L"x",false);

	/* init crunch button */
	initButton(crunch_id,
			rect<s32>(m_screensize.X-(3.25*button_size),
					m_screensize.Y - (0.5*button_size),
					m_screensize.X-(1.75*button_size),
					m_screensize.Y),
			L"H",false);

	/* init chat button */
	initButton(chat_id,
			rect<s32>(m_screensize.X-(1.5*button_size), 0,
					m_screensize.X, button_size),
			L"Chat", true);
}

touch_gui_button_id TouchScreenGUI::getButtonID(s32 x, s32 y)
{
	IGUIElement* rootguielement = m_guienv->getRootGUIElement();

	if (rootguielement != NULL) {
		gui::IGUIElement *element =
				rootguielement->getElementFromPoint(core::position2d<s32>(x,y));

		if (element) {
			for (unsigned int i=0; i < after_last_element_id; i++) {
				if (element == m_buttons[i].guibutton) {
					return (touch_gui_button_id) i;
				}
			}
		}
	}
	return after_last_element_id;
}

touch_gui_button_id TouchScreenGUI::getButtonID(int eventID)
{
	for (unsigned int i=0; i < after_last_element_id; i++) {
		button_info* btn = &m_buttons[i];

		std::vector<int>::iterator id =
				std::find(btn->ids.begin(),btn->ids.end(), eventID);

		if (id != btn->ids.end())
			return (touch_gui_button_id) i;
	}

	return after_last_element_id;
}

bool TouchScreenGUI::isHUDButton(const SEvent &event)
{
	// check if hud item is pressed
	for (std::map<int,rect<s32> >::iterator iter = m_hud_rects.begin();
			iter != m_hud_rects.end(); iter++) {
		if (iter->second.isPointInside(
				v2s32(event.TouchInput.X,
						event.TouchInput.Y)
			)) {
			if ( iter->first < 8) {
				SEvent* translated = new SEvent();
				memset(translated,0,sizeof(SEvent));
				translated->EventType = irr::EET_KEY_INPUT_EVENT;
				translated->KeyInput.Key         = (irr::EKEY_CODE) (KEY_KEY_1 + iter->first);
				translated->KeyInput.Control     = false;
				translated->KeyInput.Shift       = false;
				translated->KeyInput.PressedDown = true;
				m_receiver->OnEvent(*translated);
				m_hud_ids[event.TouchInput.ID]   = translated->KeyInput.Key;
				delete translated;
				return true;
			}
		}
	}
	return false;
}

bool TouchScreenGUI::isReleaseHUDButton(int eventID)
{
	std::map<int,irr::EKEY_CODE>::iterator iter = m_hud_ids.find(eventID);

	if (iter != m_hud_ids.end()) {
		SEvent* translated = new SEvent();
		memset(translated,0,sizeof(SEvent));
		translated->EventType            = irr::EET_KEY_INPUT_EVENT;
		translated->KeyInput.Key         = iter->second;
		translated->KeyInput.PressedDown = false;
		translated->KeyInput.Control     = false;
		translated->KeyInput.Shift       = false;
		m_receiver->OnEvent(*translated);
		m_hud_ids.erase(iter);
		delete translated;
		return true;
	}
	return false;
}

void TouchScreenGUI::ButtonEvent(touch_gui_button_id button,
		int eventID, bool action)
{
	button_info* btn = &m_buttons[button];
	SEvent* translated = new SEvent();
	memset(translated,0,sizeof(SEvent));
	translated->EventType            = irr::EET_KEY_INPUT_EVENT;
	translated->KeyInput.Key         = btn->keycode;
	translated->KeyInput.Control     = false;
	translated->KeyInput.Shift       = false;
	translated->KeyInput.Char        = 0;

	/* add this event */
	if (action) {
		assert(std::find(btn->ids.begin(),btn->ids.end(), eventID) == btn->ids.end());

		btn->ids.push_back(eventID);

		if (btn->ids.size() > 1) return;

		btn->repeatcounter = 0;
		translated->KeyInput.PressedDown = true;
		translated->KeyInput.Key = btn->keycode;
		m_receiver->OnEvent(*translated);
	}
	/* remove event */
	if ((!action) || (btn->immediate_release)) {

		std::vector<int>::iterator pos =
				std::find(btn->ids.begin(),btn->ids.end(), eventID);
		/* has to be in touch list */
		assert(pos != btn->ids.end());
		btn->ids.erase(pos);

		if (btn->ids.size() > 0)  { return; }

		translated->KeyInput.PressedDown = false;
		btn->repeatcounter               = -1;
		m_receiver->OnEvent(*translated);
	}
	delete translated;
}

void TouchScreenGUI::translateEvent(const SEvent &event)
{
	if (!m_visible) {
		infostream << "TouchScreenGUI::translateEvent got event but not visible?!" << std::endl;
		return;
	}

	if (event.EventType != EET_TOUCH_INPUT_EVENT) {
		return;
	}

	if (event.TouchInput.Event == ETIE_PRESSED_DOWN) {

		/* add to own copy of eventlist ...
		 * android would provide this information but irrlicht guys don't
		 * wanna design a efficient interface
		 */
		id_status toadd;
		toadd.id = event.TouchInput.ID;
		toadd.X  = event.TouchInput.X;
		toadd.Y  = event.TouchInput.Y;
		m_known_ids.push_back(toadd);

		int eventID = event.TouchInput.ID;

		touch_gui_button_id button =
				getButtonID(event.TouchInput.X, event.TouchInput.Y);

		/* handle button events */
		if (button != after_last_element_id) {
			ButtonEvent(button,eventID,true);
		}
		else if (isHUDButton(event))
		{
			/* already handled in isHUDButton() */
		}
		/* handle non button events */
		else {
			/* if we don't already have a moving point make this the moving one */
			if (m_move_id == -1) {
				m_move_id                  = event.TouchInput.ID;
				m_move_has_really_moved    = false;
				m_move_downtime            = getTimeMs();
				m_move_downlocation        = v2s32(event.TouchInput.X, event.TouchInput.Y);
				m_move_sent_as_mouse_event = false;
			}
		}

		m_pointerpos[event.TouchInput.ID] = v2s32(event.TouchInput.X, event.TouchInput.Y);
	}
	else if (event.TouchInput.Event == ETIE_LEFT_UP) {
		verbosestream << "Up event for pointerid: " << event.TouchInput.ID << std::endl;

		touch_gui_button_id button = getButtonID(event.TouchInput.ID);

		/* handle button events */
		if (button != after_last_element_id) {
			ButtonEvent(button,event.TouchInput.ID,false);
		}
		/* handle hud button events */
		else if (isReleaseHUDButton(event.TouchInput.ID)) {
			/* nothing to do here */
		}
		/* handle the point used for moving view */
		else if (event.TouchInput.ID == m_move_id) {
			m_move_id = -1;

			/* if this pointer issued a mouse event issue symmetric release here */
			if (m_move_sent_as_mouse_event) {
				SEvent* translated = new SEvent;
				memset(translated,0,sizeof(SEvent));
				translated->EventType               = EET_MOUSE_INPUT_EVENT;
				translated->MouseInput.X            = m_move_downlocation.X;
				translated->MouseInput.Y            = m_move_downlocation.Y;
				translated->MouseInput.Shift        = false;
				translated->MouseInput.Control      = false;
				translated->MouseInput.ButtonStates = 0;
				translated->MouseInput.Event        = EMIE_LMOUSE_LEFT_UP;
				m_receiver->OnEvent(*translated);
				delete translated;
			}
			else {
				/* do double tap detection */
				doubleTapDetection();
			}
		}
		else {
			infostream
				<< "TouchScreenGUI::translateEvent released unknown button: "
				<< event.TouchInput.ID << std::endl;
		}

		for (std::vector<id_status>::iterator iter = m_known_ids.begin();
				iter != m_known_ids.end(); iter++) {
			if (iter->id == event.TouchInput.ID) {
				m_known_ids.erase(iter);
				break;
			}
		}
	}
	else {
		assert(event.TouchInput.Event == ETIE_MOVED);
		int move_idx = event.TouchInput.ID;

		if (m_pointerpos[event.TouchInput.ID] ==
				v2s32(event.TouchInput.X, event.TouchInput.Y)) {
			return;
		}

		if (m_move_id != -1) {
			if ((event.TouchInput.ID == m_move_id) &&
				(!m_move_sent_as_mouse_event)) {

				double distance = sqrt(
						(m_pointerpos[event.TouchInput.ID].X - event.TouchInput.X) *
						(m_pointerpos[event.TouchInput.ID].X - event.TouchInput.X) +
						(m_pointerpos[event.TouchInput.ID].Y - event.TouchInput.Y) *
						(m_pointerpos[event.TouchInput.ID].Y - event.TouchInput.Y));

				if ((distance > g_settings->getU16("touchscreen_threshold")) ||
						(m_move_has_really_moved)) {
					m_move_has_really_moved = true;
					s32 X = event.TouchInput.X;
					s32 Y = event.TouchInput.Y;

					// update camera_yaw and camera_pitch
					s32 dx = X - m_pointerpos[event.TouchInput.ID].X;
					s32 dy = Y - m_pointerpos[event.TouchInput.ID].Y;

					/* adapt to similar behaviour as pc screen */
					double d         = g_settings->getFloat("mouse_sensitivity") *4;
					double old_yaw   = m_camera_yaw;
					double old_pitch = m_camera_pitch;

					m_camera_yaw   -= dx * d;
					m_camera_pitch  = MYMIN(MYMAX( m_camera_pitch + (dy * d),-180),180);

					while (m_camera_yaw < 0)
						m_camera_yaw += 360;

					while (m_camera_yaw > 360)
						m_camera_yaw -= 360;

					// update shootline
					m_shootline = m_device
							->getSceneManager()
							->getSceneCollisionManager()
							->getRayFromScreenCoordinates(v2s32(X, Y));
					m_pointerpos[event.TouchInput.ID] = v2s32(X, Y);
				}
			}
			else if ((event.TouchInput.ID == m_move_id) &&
					(m_move_sent_as_mouse_event)) {
				m_shootline = m_device
						->getSceneManager()
						->getSceneCollisionManager()
						->getRayFromScreenCoordinates(
								v2s32(event.TouchInput.X,event.TouchInput.Y));
			}
		}
		else {
			handleChangedButton(event);
		}
	}
}

void TouchScreenGUI::handleChangedButton(const SEvent &event)
{
	for (unsigned int i = 0; i < after_last_element_id; i++) {

		if (m_buttons[i].ids.empty()) {
			continue;
		}
		for(std::vector<int>::iterator iter = m_buttons[i].ids.begin();
				iter != m_buttons[i].ids.end(); iter++) {

			if (event.TouchInput.ID == *iter) {

				int current_button_id =
						getButtonID(event.TouchInput.X, event.TouchInput.Y);

				if (current_button_id == i) {
					continue;
				}

				/* remove old button */
				ButtonEvent((touch_gui_button_id) i,*iter,false);

				if (current_button_id == after_last_element_id) {
					return;
				}
				ButtonEvent((touch_gui_button_id) current_button_id,*iter,true);
				return;

			}
		}
	}

	int current_button_id = getButtonID(event.TouchInput.X, event.TouchInput.Y);

	if (current_button_id == after_last_element_id) {
		return;
	}

	button_info* btn = &m_buttons[current_button_id];
	if (std::find(btn->ids.begin(),btn->ids.end(), event.TouchInput.ID) == btn->ids.end()) {
		ButtonEvent((touch_gui_button_id) current_button_id,event.TouchInput.ID,true);
	}

}

bool TouchScreenGUI::doubleTapDetection()
{
	m_key_events[0].down_time = m_key_events[1].down_time;
	m_key_events[0].x         = m_key_events[1].x;
	m_key_events[0].y         = m_key_events[1].y;
	m_key_events[1].down_time = m_move_downtime;
	m_key_events[1].x         = m_move_downlocation.X;
	m_key_events[1].y         = m_move_downlocation.Y;

	u32 delta = porting::getDeltaMs(m_key_events[0].down_time,getTimeMs());
	if (delta > 400)
		return false;

	double distance = sqrt(
			(m_key_events[0].x - m_key_events[1].x) * (m_key_events[0].x - m_key_events[1].x) +
			(m_key_events[0].y - m_key_events[1].y) * (m_key_events[0].y - m_key_events[1].y));


	if (distance >(20 + g_settings->getU16("touchscreen_threshold")))
		return false;

	SEvent* translated = new SEvent();
	memset(translated,0,sizeof(SEvent));
	translated->EventType               = EET_MOUSE_INPUT_EVENT;
	translated->MouseInput.X            = m_key_events[0].x;
	translated->MouseInput.Y            = m_key_events[0].y;
	translated->MouseInput.Shift        = false;
	translated->MouseInput.Control      = false;
	translated->MouseInput.ButtonStates = EMBSM_RIGHT;

	// update shootline
	m_shootline = m_device
			->getSceneManager()
			->getSceneCollisionManager()
			->getRayFromScreenCoordinates(v2s32(m_key_events[0].x, m_key_events[0].y));

	translated->MouseInput.Event = EMIE_RMOUSE_PRESSED_DOWN;
	verbosestream << "TouchScreenGUI::translateEvent right click press" << std::endl;
	m_receiver->OnEvent(*translated);

	translated->MouseInput.ButtonStates = 0;
	translated->MouseInput.Event        = EMIE_RMOUSE_LEFT_UP;
	verbosestream << "TouchScreenGUI::translateEvent right click release" << std::endl;
	m_receiver->OnEvent(*translated);
	delete translated;
	return true;

}

TouchScreenGUI::~TouchScreenGUI()
{
	for (unsigned int i=0; i < after_last_element_id; i++) {
		button_info* btn = &m_buttons[i];
		if (btn->guibutton != 0) {
			btn->guibutton->drop();
			btn->guibutton = NULL;
		}
	}
}

void TouchScreenGUI::step(float dtime)
{
	/* simulate keyboard repeats */
	for (unsigned int i=0; i < after_last_element_id; i++) {
		button_info* btn = &m_buttons[i];

		if (btn->ids.size() > 0) {
			btn->repeatcounter += dtime;

			if (btn->repeatcounter < 0.2) continue;

			btn->repeatcounter              = 0;
			SEvent translated;
			memset(&translated,0,sizeof(SEvent));
			translated.EventType            = irr::EET_KEY_INPUT_EVENT;
			translated.KeyInput.Key         = btn->keycode;
			translated.KeyInput.PressedDown = false;
			m_receiver->OnEvent(translated);

			translated.KeyInput.PressedDown = true;
			m_receiver->OnEvent(translated);
		}
	}

	/* if a new placed pointer isn't moved for some time start digging */
	if ((m_move_id != -1) &&
			(!m_move_has_really_moved) &&
			(!m_move_sent_as_mouse_event)) {

		u32 delta = porting::getDeltaMs(m_move_downtime,getTimeMs());

		if (delta > MIN_DIG_TIME_MS) {
			m_shootline = m_device
					->getSceneManager()
					->getSceneCollisionManager()
					->getRayFromScreenCoordinates(
							v2s32(m_move_downlocation.X,m_move_downlocation.Y));

			SEvent translated;
			memset(&translated,0,sizeof(SEvent));
			translated.EventType               = EET_MOUSE_INPUT_EVENT;
			translated.MouseInput.X            = m_move_downlocation.X;
			translated.MouseInput.Y            = m_move_downlocation.Y;
			translated.MouseInput.Shift        = false;
			translated.MouseInput.Control      = false;
			translated.MouseInput.ButtonStates = EMBSM_LEFT;
			translated.MouseInput.Event        = EMIE_LMOUSE_PRESSED_DOWN;
			verbosestream << "TouchScreenGUI::step left click press" << std::endl;
			m_receiver->OnEvent(translated);
			m_move_sent_as_mouse_event         = true;
		}
	}
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
	for (unsigned int i=0; i < after_last_element_id; i++) {
		button_info* btn = &m_buttons[i];
		if (btn->guibutton != 0) {
			btn->guibutton->setVisible(visible);
		}
	}
}

void TouchScreenGUI::Hide()
{
	Toggle(false);
}

void TouchScreenGUI::Show()
{
	Toggle(true);
}
