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

#ifndef MODALMENU_HEADER
#define MODALMENU_HEADER

#include "irrlichttypes_extrabloated.h"
#ifdef HAVE_TOUCHSCREENGUI
#include "touchscreengui.h"
#endif

class GUIModalMenu;

class IMenuManager
{
public:
	// A GUIModalMenu calls these when this class is passed as a parameter
	virtual void createdMenu(GUIModalMenu *menu) = 0;
	virtual void deletingMenu(GUIModalMenu *menu) = 0;
};

/*
	Remember to drop() the menu after creating, so that it can
	remove itself when it wants to.
*/

class GUIModalMenu : public gui::IGUIElement
{
public:
	GUIModalMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IMenuManager *menumgr):
		IGUIElement(gui::EGUIET_ELEMENT, env, parent, id,
				core::rect<s32>(0,0,100,100))
	{
		//m_force_regenerate_gui = false;
		
		m_menumgr = menumgr;
		m_allow_focus_removal = false;
		m_screensize_old = v2u32(0,0);

		setVisible(true);
		Environment->setFocus(this);
		m_menumgr->createdMenu(this);
	}
	virtual ~GUIModalMenu()
	{
		m_menumgr->deletingMenu(this);
	}

	void allowFocusRemoval(bool allow)
	{
		m_allow_focus_removal = allow;
	}

	bool canTakeFocus(gui::IGUIElement *e)
	{
		return (e && (e == this || isMyChild(e))) || m_allow_focus_removal;
	}

	void draw()
	{
		if(!IsVisible)
			return;
			
		video::IVideoDriver* driver = Environment->getVideoDriver();
		v2u32 screensize = driver->getScreenSize();
		if(screensize != m_screensize_old /*|| m_force_regenerate_gui*/)
		{
			m_screensize_old = screensize;
			regenerateGui(screensize);
			//m_force_regenerate_gui = false;
		}

		drawMenu();
	}
	
	/*
		This should be called when the menu wants to quit.

		WARNING: THIS DEALLOCATES THE MENU FROM MEMORY. Return
		immediately if you call this from the menu itself.

		(More precisely, this decrements the reference count.)
	*/
	void quitMenu()
	{
		allowFocusRemoval(true);
		// This removes Environment's grab on us
		Environment->removeFocus(this);
		m_menumgr->deletingMenu(this);
		this->remove();
#ifdef HAVE_TOUCHSCREENGUI
		if (g_touchscreengui)
			g_touchscreengui->Show();
#endif
	}

	void removeChildren()
	{
		const core::list<gui::IGUIElement*> &children = getChildren();
		core::list<gui::IGUIElement*> children_copy;
		for(core::list<gui::IGUIElement*>::ConstIterator
				i = children.begin(); i != children.end(); i++)
		{
			children_copy.push_back(*i);
		}
		for(core::list<gui::IGUIElement*>::Iterator
				i = children_copy.begin();
				i != children_copy.end(); i++)
		{
			(*i)->remove();
		}
	}

	virtual void regenerateGui(v2u32 screensize) = 0;
	virtual void drawMenu() = 0;
	virtual bool preprocessEvent(const SEvent& event) { return false; };
	virtual bool OnEvent(const SEvent& event) { return false; };
	virtual bool pausesGame(){ return false; } // Used for pause menu

protected:
	//bool m_force_regenerate_gui;
	v2u32 m_screensize_old;
private:
	IMenuManager *m_menumgr;
	// This might be necessary to expose to the implementation if it
	// wants to launch other menus
	bool m_allow_focus_removal;
};


#endif

