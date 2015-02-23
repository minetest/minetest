/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef __INPUT_HANDLER_H__
#define __INPUT_HANDLER_H__

#include "irrlichttypes_extrabloated.h"

class MyEventReceiver : public IEventReceiver
{
public:
	// This is the one method that we have to implement
	virtual bool OnEvent(const SEvent& event)
	{
		/*
			React to nothing here if a menu is active
		*/
		if (noMenuActive() == false) {
#ifdef HAVE_TOUCHSCREENGUI
			if (m_touchscreengui != 0) {
				m_touchscreengui->Toggle(false);
			}
#endif
			return g_menumgr.preprocessEvent(event);
		}

		// Remember whether each key is down or up
		// Get the Key that triggered the Event

		if (event.EventType == irr::EET_KEY_INPUT_EVENT) {
      			bool down = event.KeyInput.PressedDown;

		  	if (down) {
			     	if (keyIsDown[event.KeyInput]) {
					keyPressed.set(event.KeyInput);
			     	}
				keyDown.set(event.KeyInput);
				keyIsDown.set(event.KeyInput);
				keyWasDown.set(event.KeyInput);
		  	} else {
		     		keyDown.unset(event.KeyInput);
		     		keyIsDown.unset(event.KeyInput);
	     			keyPressed.unset(event.KeyInput);
				keyReleased.set(event.KeyInput);	
		  	}
		}

#ifdef HAVE_TOUCHSCREENGUI
		// case of touchscreengui we have to handle different events
		if ((m_touchscreengui != 0) &&
				(event.EventType == irr::EET_TOUCH_INPUT_EVENT)) {
			m_touchscreengui->translateEvent(event);
			return true;
		}
#endif
		// handle mouse events
		if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
			// handle mouse buttons
			// left
			if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
			     	if (keyIsDown["KEY_LBUTTON"]) {
					keyPressed.set("KEY_LBUTTON");
			     	}
				keyDown.set("KEY_LBUTTON");
				keyIsDown.set("KEY_LBUTTON");
				keyWasDown.set("KEY_LBUTTON");
		  	} else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
		     		keyDown.unset("KEY_LBUTTON");
		     		keyIsDown.unset("KEY_LBUTTON");
	     			keyPressed.unset("KEY_LBUTTON");
				keyReleased.set("KEY_LBUTTON");	
		  	}

			// right
			else if (event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) {
			     	if (keyIsDown["KEY_RBUTTON"]) {
					keyPressed.set("KEY_RBUTTON");
			     	}
				keyDown.set("KEY_RBUTTON");
				keyIsDown.set("KEY_RBUTTON");
				keyWasDown.set("KEY_RBUTTON");
		  	} else if (event.MouseInput.Event == EMIE_RMOUSE_LEFT_UP) {
		     		keyDown.unset("KEY_RBUTTON");
		     		keyIsDown.unset("KEY_RBUTTON");
	     			keyPressed.unset("KEY_RBUTTON");
				keyReleased.set("KEY_RBUTTON");	
		  	}

			// middle
			else if (event.MouseInput.Event == EMIE_MMOUSE_PRESSED_DOWN) {
			     	if (keyIsDown["KEY_MBUTTON"]) {
					keyPressed.set("KEY_MBUTTON");
			     	}
				keyDown.set("KEY_MBUTTON");
				keyIsDown.set("KEY_MBUTTON");
				keyWasDown.set("KEY_MBUTTON");
		  	} else if (event.MouseInput.Event == EMIE_MMOUSE_LEFT_UP) {
		     		keyDown.unset("KEY_MBUTTON");
		     		keyIsDown.set("KEY_MBUTTON");
	     			keyPressed.unset("KEY_MBUTTON");
				keyReleased.set("KEY_MBUTTON");	
		  	}

		  	else if (noMenuActive()) {
				if (event.MouseInput.Event == EMIE_MOUSE_WHEEL) {
					mouse_wheel += event.MouseInput.Wheel;
				}
			}
		}

		if (event.EventType == irr::EET_LOG_TEXT_EVENT) {
			dstream << std::string("Irrlicht log: ") + std::string(event.LogEvent.Text)
			        << std::endl;
			return true;
		}
		/* always return false in order to continue processing events */
		return false;
	}

	bool IsKeyDown(const KeyPress &keyCode) const
	{
		return keyDown[keyCode];
	}
	
	bool IsKeyActive(const KeyPress &keyCode) const
	{
		return keyPressed[keyCode] || keyIsDown[keyCode];
	}

	bool IsKeyReleased(const KeyPress &keyCode) const
	{
		return keyReleased[keyCode];
	}

	// Checks whether a key was down and resets the state
	bool WasKeyDown(const KeyPress &keyCode)
	{
		bool b = keyWasDown[keyCode];
		if (b)
			keyWasDown.unset(keyCode);
		return b;
	}

	void resetKeyClicked(const KeyPress &keyCode)
	{
		keyDown.unset(keyCode);
	}

	void resetKeyReleased(const KeyPress &keyCode)
	{
		keyReleased.unset(keyCode);
	}

	s32 getMouseWheel()
	{
		s32 a = mouse_wheel;
		mouse_wheel = 0;
		return a;
	}

	void clearInput()
	{
		keyDown.clear();
		keyIsDown.clear();
		keyWasDown.clear();
		keyReleased.clear();
		keyPressed.clear();

		mouse_wheel = 0;
	}

	MyEventReceiver()
	{
		clearInput();
#ifdef HAVE_TOUCHSCREENGUI
		m_touchscreengui = NULL;
#endif
	}

	s32 mouse_wheel;

#ifdef HAVE_TOUCHSCREENGUI
	TouchScreenGUI* m_touchscreengui;
#endif

private:
	// The current state of keys
	KeyList keyDown;
	KeyList keyIsDown;
	KeyList keyPressed;
   	KeyList keyReleased;
	// Whether a key has been pressed or not
	KeyList keyWasDown;
};


/*
	Separated input handler
*/

class RealInputHandler : public InputHandler
{
public:
	RealInputHandler(IrrlichtDevice *device, MyEventReceiver *receiver):
		m_device(device),
		m_receiver(receiver),
		m_mousepos(0,0)
	{
	}
	virtual bool isKeyDown(const KeyPress &keyCode)
	{
		return m_receiver->IsKeyDown(keyCode);
	}
	virtual bool wasKeyDown(const KeyPress &keyCode)
	{
		return m_receiver->WasKeyDown(keyCode);
	}
	virtual bool getKeyState(const KeyPress &keyCode)
	{
		return m_receiver->IsKeyActive(keyCode);
	}
	virtual bool getKeyReleased(const KeyPress &keyCode)
	{
		return m_receiver->IsKeyReleased(keyCode);
	}
	virtual void resetKeyClicked(const KeyPress &keyCode)
	{
		m_receiver->resetKeyClicked(keyCode);
	}
	virtual void resetKeyReleased(const KeyPress &keyCode)
	{
		m_receiver->resetKeyReleased(keyCode);
	}

	virtual v2s32 getMousePos()
	{
		if (m_device->getCursorControl()) {
			return m_device->getCursorControl()->getPosition();
		} else {
			return m_mousepos;
		}
	}
	virtual void setMousePos(s32 x, s32 y)
	{
		if (m_device->getCursorControl()) {
			m_device->getCursorControl()->setPosition(x, y);
		} else {
			m_mousepos = v2s32(x,y);
		}
	}

	virtual s32 getMouseWheel()
	{
		return m_receiver->getMouseWheel();
	}

	void clear()
	{
		m_receiver->clearInput();
	}
private:
	IrrlichtDevice  *m_device;
	MyEventReceiver *m_receiver;
	v2s32           m_mousepos;
};

class RandomInputHandler : public InputHandler
{
public:
	RandomInputHandler()
	{
		leftdown = false;
		rightdown = false;
		middledown = false;
		leftclicked = false;
		rightclicked = false;
		middleclicked = false;
		leftreleased = false;
		rightreleased = false;
		middlereleased = false;
		keydown.clear();
	}
	virtual bool isKeyDown(const KeyPress &keyCode)
	{
		if (keyCode == getKeySetting("keymap_dig")) {
			return leftclicked;
		}
		else if (keyCode == getKeySetting("keymap_place")) {
			return rightclicked;
		} /*else if (keyCode == getKeySetting("keymap_middle")) {
			return middleclicked;
		}*/
		return keydown[keyCode];
	}
	virtual bool wasKeyDown(const KeyPress &keyCode)
	{
		return false;
	}
	virtual bool getKeyState(const KeyPress &keyCode)
	{
		if (keyCode == getKeySetting("keymap_dig")) {
			return leftdown;
		} else if (keyCode == getKeySetting("keymap_place")) {
			return rightdown;
		} /*else if (keyCode == getKeySetting("keymap_middle")) {
			return middledown;
		}*/
		return false;
	}
	virtual bool getKeyReleased(const KeyPress &keyCode)
	{
		if (keyCode == getKeySetting("keymap_dig")) {
			return leftreleased;
		} else if (keyCode == getKeySetting("keymap_place")) {
			return rightreleased;
		} /*else if (keyCode == getKeySetting("keymap_middle")) {
			return middlereleased;
		}*/
		return false;
	}
	virtual void resetKeyClicked(const KeyPress &keyCode)
	{
		if (keyCode == getKeySetting("keymap_dig")) {
			leftclicked = false;
			return;
		} else if (keyCode == getKeySetting("keymap_place")) {
			rightclicked = false;
			return;
		} /*else if (keyCode == getKeySetting("keymap_middle")) {
			middleclicked = false;
			return;
		}*/
	}
	virtual void resetKeyReleased(const KeyPress &keyCode)
	{
		if (keyCode == getKeySetting("keymap_dig")) {
			leftreleased = false;
			return;
		} else if (keyCode == getKeySetting("keymap_place")) {
			rightreleased = false;
			return;
		} /*else if (keyCode == getKeySetting("keymap_middle")) {
			middlereleased = false;
			return;
		}*/
	}

	virtual v2s32 getMousePos()
	{
		return mousepos;
	}
	virtual void setMousePos(s32 x, s32 y)
	{
		mousepos = v2s32(x, y);
	}
	virtual s32 getMouseWheel()
	{
		return 0;
	}

	virtual void step(float dtime)
	{
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_jump"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_special1"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_forward"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 40);
				keydown.toggle(getKeySetting("keymap_left"));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 20);
				mousespeed = v2s32(Rand(-20, 20), Rand(-15, 20));
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 30);
				leftdown = !leftdown;
				if (leftdown)
					leftclicked = true;
				if (!leftdown)
					leftreleased = true;
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 15);
				rightdown = !rightdown;
				if (rightdown)
					rightclicked = true;
				if (!rightdown)
					rightreleased = true;
			}
		}
		{
			static float counter1 = 0;
			counter1 -= dtime;
			if (counter1 < 0.0) {
				counter1 = 0.1 * Rand(1, 45);
				middledown = !middledown;
				if (middledown)
					middleclicked = true;
				if (!middledown)
					middlereleased = true;
			}
		}
		mousepos += mousespeed;
	}

	s32 Rand(s32 min, s32 max)
	{
		return (myrand()%(max-min+1))+min;
	}
private:
	KeyList keydown;
	v2s32 mousepos;
	v2s32 mousespeed;
	bool leftdown;
	bool rightdown;
	bool middledown;
	bool leftclicked;
	bool rightclicked;
	bool middleclicked;
	bool leftreleased;
	bool rightreleased;
	bool middlereleased;
};

#endif
