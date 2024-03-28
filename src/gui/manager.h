/*
Minetest
Copyright (C) 2023 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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
#include "gui/texture.h"
#include "gui/window.h"
#include "util/basic_macros.h"

#include <iostream>
#include <map>
#include <sstream>
#include <string>

class Client;

namespace ui
{
	// Define a few functions that are particularly useful for UI serialization
	// and deserialization.
	bool testShift(u32 &bits);

	// The UI purposefully avoids dealing with SerializationError, so it uses
	// always uses truncating or null-terminated string functions. Hence, we
	// make convenience wrappers around the string functions in "serialize.h".
	std::string readStr16(std::istream &is);
	std::string readStr32(std::istream &is);
	std::string readNullStr(std::istream &is);

	void writeStr16(std::ostream &os, const std::string &str);
	void writeStr32(std::ostream &os, const std::string &str);
	void writeNullStr(std::ostream &os, const std::string &str);

	// Convenience functions to create new binary string streams.
	std::istringstream newIs(std::string str);
	std::ostringstream newOs();

	class Manager
	{
	public:
		// Serialized enum; do not change values of entries.
		enum ReceiveAction
		{
			OPEN_WINDOW   = 0x00,
			REOPEN_WINDOW = 0x01,
			UPDATE_WINDOW = 0x02,
			CLOSE_WINDOW  = 0x03,
		};

	private:
		Client *m_client;

		float m_gui_pixel_size = 0.0f;
		float m_hud_pixel_size = 0.0f;

		// Use map rather than unordered_map so that windows are always sorted
		// by window ID to make sure that they are drawn in order of creation.
		std::map<u64, Window> m_windows;

	public:
		Manager()
		{
			reset();
		}

		DISABLE_CLASS_COPY(Manager)

		Client *getClient() const { return m_client; }
		void setClient(Client *client) { m_client = client; }

		Texture getTexture(const std::string &name) const;

		float getPixelSize(WindowType type) const;
		d2f32 getScreenSize(WindowType type) const;

		void reset();
		void removeWindow(u64 id);

		void receiveMessage(const std::string &data);

		void preDraw();
		void drawType(WindowType type);
	};

	extern Manager g_manager;

	// Inconveniently, we need a way to draw the "gui" window types after the
	// chat console but before other GUIs like the key change menu, formspecs,
	// etc. So, we inject our own mini Irrlicht element in between.
	class GUIManagerElem : public gui::IGUIElement
	{
	public:
		GUIManagerElem(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id) :
			gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rs32())
		{}

		virtual void draw() override
		{
			g_manager.drawType(ui::WindowType::GUI);
			gui::IGUIElement::draw();
		}
	};
}
