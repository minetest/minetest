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

#pragma once

#include "irrlichttypes_extrabloated.h"
#include "joystick_controller.h"
#include <list>
#include "keycode.h"
#include "renderingengine.h"

#ifdef HAVE_TOUCHSCREENGUI
#include "gui/touchscreengui.h"
#endif

class InputHandler;

/****************************************************************************
 Fast key cache for main game loop
 ****************************************************************************/

/* This is faster than using getKeySetting with the tradeoff that functions
 * using it must make sure that it's initialised before using it and there is
 * no error handling (for example bounds checking). This is really intended for
 * use only in the main running loop of the client (the_game()) where the faster
 * (up to 10x faster) key lookup is an asset. Other parts of the codebase
 * (e.g. formspecs) should continue using getKeySetting().
 */
struct KeyCache
{

	KeyCache()
	{
		handler = NULL;
		populate();
		populate_nonchanging();
	}

	void populate();

	// Keys that are not settings dependent
	void populate_nonchanging();

	KeyPress key[KeyType::INTERNAL_ENUM_COUNT];
	InputHandler *handler;
};

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
#ifdef HAVE_TOUCHSCREENGUI
		m_touchscreengui = NULL;
#endif
	}

	bool leftclicked = false;
	bool rightclicked = false;
	bool leftreleased = false;
	bool rightreleased = false;

	bool left_active = false;
	bool middle_active = false;
	bool right_active = false;

	s32 mouse_wheel = 0;

	JoystickController *joystick = nullptr;

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
	InputHandler()
	{
		keycache.handler = this;
		keycache.populate();
	}

	virtual ~InputHandler() = default;

	virtual bool isRandom() const
	{
		return false;
	}

	virtual bool isKeyDown(GameKeyType k) = 0;
	virtual bool wasKeyDown(GameKeyType k) = 0;
	virtual bool cancelPressed() = 0;

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
	KeyCache keycache;
};
/*
	Separated input handler
*/

class RealInputHandler : public InputHandler
{
public:
	RealInputHandler(MyEventReceiver *receiver) : m_receiver(receiver)
	{
		m_receiver->joystick = &joystick;
	}
	virtual bool isKeyDown(GameKeyType k)
	{
		return m_receiver->IsKeyDown(keycache.key[k]) || joystick.isKeyDown(k);
	}
	virtual bool wasKeyDown(GameKeyType k)
	{
		return m_receiver->WasKeyDown(keycache.key[k]) || joystick.wasKeyDown(k);
	}
	virtual bool cancelPressed()
	{
		return wasKeyDown(KeyType::ESC) || m_receiver->WasKeyDown(CancelKey);
	}
	virtual void listenForKey(const KeyPress &keyCode)
	{
		m_receiver->listenForKey(keyCode);
	}
	virtual void dontListenForKeys() { m_receiver->dontListenForKeys(); }
	virtual v2s32 getMousePos()
	{
		if (RenderingEngine::get_raw_device()->getCursorControl()) {
			return RenderingEngine::get_raw_device()
					->getCursorControl()
					->getPosition();
		}

		return m_mousepos;
	}

	virtual void setMousePos(s32 x, s32 y)
	{
		if (RenderingEngine::get_raw_device()->getCursorControl()) {
			RenderingEngine::get_raw_device()
					->getCursorControl()
					->setPosition(x, y);
		} else {
			m_mousepos = v2s32(x, y);
		}
	}

	virtual bool getLeftState()
	{
		return m_receiver->left_active || joystick.isKeyDown(KeyType::MOUSE_L);
	}
	virtual bool getRightState()
	{
		return m_receiver->right_active || joystick.isKeyDown(KeyType::MOUSE_R);
	}

	virtual bool getLeftClicked()
	{
		return m_receiver->leftclicked ||
		       joystick.getWasKeyDown(KeyType::MOUSE_L);
	}
	virtual bool getRightClicked()
	{
		return m_receiver->rightclicked ||
		       joystick.getWasKeyDown(KeyType::MOUSE_R);
	}

	virtual void resetLeftClicked()
	{
		m_receiver->leftclicked = false;
		joystick.clearWasKeyDown(KeyType::MOUSE_L);
	}
	virtual void resetRightClicked()
	{
		m_receiver->rightclicked = false;
		joystick.clearWasKeyDown(KeyType::MOUSE_R);
	}

	virtual bool getLeftReleased()
	{
		return m_receiver->leftreleased ||
		       joystick.wasKeyReleased(KeyType::MOUSE_L);
	}
	virtual bool getRightReleased()
	{
		return m_receiver->rightreleased ||
		       joystick.wasKeyReleased(KeyType::MOUSE_R);
	}

	virtual void resetLeftReleased()
	{
		m_receiver->leftreleased = false;
		joystick.clearWasKeyReleased(KeyType::MOUSE_L);
	}
	virtual void resetRightReleased()
	{
		m_receiver->rightreleased = false;
		joystick.clearWasKeyReleased(KeyType::MOUSE_R);
	}

	virtual s32 getMouseWheel() { return m_receiver->getMouseWheel(); }

	void clear()
	{
		joystick.clear();
		m_receiver->clearInput();
	}

private:
	MyEventReceiver *m_receiver = nullptr;
	v2s32 m_mousepos;
};

class RandomInputHandler : public InputHandler
{
public:
	RandomInputHandler() = default;

	bool isRandom() const
	{
		return true;
	}

	virtual bool isKeyDown(GameKeyType k) { return keydown[keycache.key[k]]; }
	virtual bool wasKeyDown(GameKeyType k) { return false; }
	virtual bool cancelPressed() { return false; }
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

	virtual void step(float dtime);

	s32 Rand(s32 min, s32 max);

private:
	KeyList keydown;
	v2s32 mousepos;
	v2s32 mousespeed;
	bool leftdown = false;
	bool rightdown = false;
	bool leftclicked = false;
	bool rightclicked = false;
	bool leftreleased = false;
	bool rightreleased = false;
};
