/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "util/string.h"

class GUIModalMenu;

class IMenuManager
{
public:
	// A GUIModalMenu calls these when this class is passed as a parameter
	virtual void createdMenu(gui::IGUIElement *menu) = 0;
	virtual void deletingMenu(gui::IGUIElement *menu) = 0;
};

// Remember to drop() the menu after creating, so that it can
// remove itself when it wants to.

class GUIModalMenu : public gui::IGUIElement
{
public:
	GUIModalMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr, bool remap_dbl_click = true);
	virtual ~GUIModalMenu();

	void allowFocusRemoval(bool allow);
	bool canTakeFocus(gui::IGUIElement *e);
	void draw();
	void quitMenu();
	void removeChildren();

	virtual void regenerateGui(v2u32 screensize) = 0;
	virtual void drawMenu() = 0;
	virtual bool preprocessEvent(const SEvent &event);
	virtual bool OnEvent(const SEvent &event) { return false; };
	virtual bool pausesGame() { return false; } // Used for pause menu
#ifdef __ANDROID__
	virtual bool getAndroidUIInput() { return false; }
	bool hasAndroidUIInput();
#endif

protected:
	virtual std::wstring getLabelByID(s32 id) = 0;
	virtual std::string getNameByID(s32 id) = 0;

	/**
	 * check if event is part of a double click
	 * @param event event to evaluate
	 * @return true/false if a doubleclick was detected
	 */
	bool DoubleClickDetection(const SEvent &event);

	v2s32 m_pointer;
	v2s32 m_old_pointer;  // Mouse position after previous mouse event
	v2u32 m_screensize_old;
	float m_gui_scale;
#ifdef __ANDROID__
	v2s32 m_down_pos;
	std::string m_jni_field_name;
#endif
#ifdef HAVE_TOUCHSCREENGUI
	bool m_touchscreen_visible = true;
#endif

private:
	struct clickpos
	{
		v2s32 pos;
		s64 time;
	};
	clickpos m_doubleclickdetect[2];

	IMenuManager *m_menumgr;
	/* If true, remap a double-click (or double-tap) action to ESC. This is so
	 * that, for example, Android users can double-tap to close a formspec.
	*
	 * This value can (currently) only be set by the class constructor
	 * and the default value for the setting is true.
	 */
	bool m_remap_dbl_click;
	// This might be necessary to expose to the implementation if it
	// wants to launch other menus
	bool m_allow_focus_removal = false;
};
