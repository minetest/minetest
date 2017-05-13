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

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "irrlichttypes_extrabloated.h"
#include "joystick_controller.h"
#include <list>
#include "keycode.h"

#ifdef HAVE_TOUCHSCREENGUI
#include "touchscreengui.h"
#endif

class KeyList : private std::list<KeyPress>
{
	typedef std::list<KeyPress> super;
	typedef super::iterator iterator;
	typedef super::const_iterator const_iterator;

	virtual const_iterator find(const KeyPress &key) const
	{
		const_iterator f(begin());
		const_iterator e(end());

		while (f != e) {
			if (*f == key)
				return f;

			++f;
		}

		return e;
	}

	virtual iterator find(const KeyPress &key)
	{
		iterator f(begin());
		iterator e(end());

		while (f != e) {
			if (*f == key)
				return f;

			++f;
		}

		return e;
	}

public:
	void clear() { super::clear(); }

	void set(const KeyPress &key)
	{
		if (find(key) == end())
			push_back(key);
	}

	void unset(const KeyPress &key)
	{
		iterator p(find(key));

		if (p != end())
			erase(p);
	}

	void toggle(const KeyPress &key)
	{
		iterator p(this->find(key));

		if (p != end())
			erase(p);
		else
			push_back(key);
	}

	bool operator[](const KeyPress &key) const { return find(key) != end(); }
};

class MyEventReceiver : public IEventReceiver
{
public:
	// This is the one method that we have to implement
	virtual bool OnEvent(const SEvent &event);

	bool IsKeyDown(const KeyPress &keyCode) const { return keyIsDown[keyCode]; }

	// Checks whether a key was down and resets the state
	bool WasKeyDown(const KeyPress &keyCode)
	{
		bool b = keyWasDown[keyCode];
		if (b)
			keyWasDown.unset(keyCode);
		return b;
	}

	void listenForKey(const KeyPress &keyCode) { keysListenedFor.set(keyCode); }
	void dontListenForKeys() { keysListenedFor.clear(); }

	s32 getMouseWheel()
	{
		s32 a = mouse_wheel;
		mouse_wheel = 0;
		return a;
	}

	void clearInput()
	{
		keyIsDown.clear();
		keyWasDown.clear();

		leftclicked = false;
		rightclicked = false;
		leftreleased = false;
		rightreleased = false;

		left_active = false;
		middle_active = false;
		right_active = false;

		mouse_wheel = 0;
	}

	MyEventReceiver()
	{
		clearInput();
#ifdef HAVE_TOUCHSCREENGUI
		m_touchscreengui = NULL;
#endif
	}

	bool leftclicked;
	bool rightclicked;
	bool leftreleased;
	bool rightreleased;

	bool left_active;
	bool middle_active;
	bool right_active;

	s32 mouse_wheel;

	JoystickController *joystick;

#ifdef HAVE_TOUCHSCREENGUI
	TouchScreenGUI *m_touchscreengui;
#endif

private:
	// The current state of keys
	KeyList keyIsDown;
	// Whether a key has been pressed or not
	KeyList keyWasDown;
	// List of keys we listen for
	// TODO perhaps the type of this is not really
	// performant as KeyList is designed for few but
	// often changing keys, and keysListenedFor is expected
	// to change seldomly but contain lots of keys.
	KeyList keysListenedFor;
};

class InputHandler
{
public:
	InputHandler() {}
	virtual ~InputHandler() {}

	virtual bool isKeyDown(const KeyPress &keyCode) = 0;
	virtual bool wasKeyDown(const KeyPress &keyCode) = 0;

	virtual void listenForKey(const KeyPress &keyCode) {}
	virtual void dontListenForKeys() {}

	virtual v2s32 getMousePos() = 0;
	virtual void setMousePos(s32 x, s32 y) = 0;

	virtual bool getLeftState() = 0;
	virtual bool getRightState() = 0;

	virtual bool getLeftClicked() = 0;
	virtual bool getRightClicked() = 0;
	virtual void resetLeftClicked() = 0;
	virtual void resetRightClicked() = 0;

	virtual bool getLeftReleased() = 0;
	virtual bool getRightReleased() = 0;
	virtual void resetLeftReleased() = 0;
	virtual void resetRightReleased() = 0;

	virtual s32 getMouseWheel() = 0;

	virtual void step(float dtime) {}

	virtual void clear() {}

	JoystickController joystick;
};
/*
	Separated input handler
*/

class RealInputHandler : public InputHandler
{
public:
	RealInputHandler(IrrlichtDevice *device, MyEventReceiver *receiver)
	    : m_device(device), m_receiver(receiver), m_mousepos(0, 0)
	{
		m_receiver->joystick = &joystick;
	}
	virtual bool isKeyDown(const KeyPress &keyCode)
	{
		return m_receiver->IsKeyDown(keyCode);
	}
	virtual bool wasKeyDown(const KeyPress &keyCode)
	{
		return m_receiver->WasKeyDown(keyCode);
	}
	virtual void listenForKey(const KeyPress &keyCode)
	{
		m_receiver->listenForKey(keyCode);
	}
	virtual void dontListenForKeys() { m_receiver->dontListenForKeys(); }
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
			m_mousepos = v2s32(x, y);
		}
	}

	virtual bool getLeftState() { return m_receiver->left_active; }
	virtual bool getRightState() { return m_receiver->right_active; }

	virtual bool getLeftClicked() { return m_receiver->leftclicked; }
	virtual bool getRightClicked() { return m_receiver->rightclicked; }
	virtual void resetLeftClicked() { m_receiver->leftclicked = false; }
	virtual void resetRightClicked() { m_receiver->rightclicked = false; }

	virtual bool getLeftReleased() { return m_receiver->leftreleased; }
	virtual bool getRightReleased() { return m_receiver->rightreleased; }
	virtual void resetLeftReleased() { m_receiver->leftreleased = false; }
	virtual void resetRightReleased() { m_receiver->rightreleased = false; }

	virtual s32 getMouseWheel() { return m_receiver->getMouseWheel(); }

	void clear()
	{
		joystick.clear();
		m_receiver->clearInput();
	}

private:
	IrrlichtDevice *m_device;
	MyEventReceiver *m_receiver;
	v2s32 m_mousepos;
};

class RandomInputHandler : public InputHandler
{
public:
	RandomInputHandler()
	{
		leftdown = false;
		rightdown = false;
		leftclicked = false;
		rightclicked = false;
		leftreleased = false;
		rightreleased = false;
		keydown.clear();
	}
	virtual bool isKeyDown(const KeyPress &keyCode) { return keydown[keyCode]; }
	virtual bool wasKeyDown(const KeyPress &keyCode) { return false; }
	virtual v2s32 getMousePos() { return mousepos; }
	virtual void setMousePos(s32 x, s32 y) { mousepos = v2s32(x, y); }

	virtual bool getLeftState() { return leftdown; }
	virtual bool getRightState() { return rightdown; }

	virtual bool getLeftClicked() { return leftclicked; }
	virtual bool getRightClicked() { return rightclicked; }
	virtual void resetLeftClicked() { leftclicked = false; }
	virtual void resetRightClicked() { rightclicked = false; }

	virtual bool getLeftReleased() { return leftreleased; }
	virtual bool getRightReleased() { return rightreleased; }
	virtual void resetLeftReleased() { leftreleased = false; }
	virtual void resetRightReleased() { rightreleased = false; }

	virtual s32 getMouseWheel() { return 0; }

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
		mousepos += mousespeed;
	}

	s32 Rand(s32 min, s32 max);

private:
	KeyList keydown;
	v2s32 mousepos;
	v2s32 mousespeed;
	bool leftdown;
	bool rightdown;
	bool leftclicked;
	bool rightclicked;
	bool leftreleased;
	bool rightreleased;
};

#endif
