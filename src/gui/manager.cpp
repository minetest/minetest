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

#include "gui/manager.h"

#include "debug.h"
#include "log.h"
#include "settings.h"
#include "client/client.h"
#include "client/renderingengine.h"
#include "client/tile.h"
#include "util/serialize.h"

namespace ui
{
	bool testShift(u32 &bits)
	{
		bool test = bits & 1;
		bits >>= 1;
		return test;
	}

	std::string readStr16(std::istream &is)
	{
		return deSerializeString16(is, true);
	}

	std::string readStr32(std::istream &is)
	{
		return deSerializeString32(is, true);
	}

	std::string readNullStr(std::istream &is)
	{
		std::string str;
		std::getline(is, str, '\0');
		return str;
	}

	void writeStr16(std::ostream &os, const std::string &str)
	{
		os << serializeString16(str, true);
	}

	void writeStr32(std::ostream &os, const std::string &str)
	{
		os << serializeString32(str, true);
	}

	void writeNullStr(std::ostream &os, const std::string &str)
	{
		os << str.substr(0, strlen(str.c_str())) << '\0';
	}

	std::istringstream newIs(std::string str)
	{
		return std::istringstream(std::move(str), std::ios_base::binary);
	}

	std::ostringstream newOs()
	{
		return std::ostringstream(std::ios_base::binary);
	}

	Texture Manager::getTexture(const std::string &name) const
	{
		return Texture(m_client->tsrc()->getTexture(name));
	}

	float Manager::getPixelSize(WindowType type) const
	{
		if (type == WindowType::GUI || type == WindowType::MESSAGE) {
			return m_gui_pixel_size;
		}
		return m_hud_pixel_size;
	}

	d2f32 Manager::getScreenSize(WindowType type) const
	{
		video::IVideoDriver *driver = RenderingEngine::get_video_driver();
		d2u32 screen_size = driver->getScreenSize();

		float pixel_size = getPixelSize(type);

		return d2f32(
			screen_size.Width / pixel_size,
			screen_size.Height / pixel_size
		);
	}

	void Manager::reset()
	{
		m_client = nullptr;

		m_windows.clear();
	}

	void Manager::removeWindow(u64 id)
	{
		auto it = m_windows.find(id);
		if (it == m_windows.end()) {
			infostream << "Window " << id << " is already closed" << std::endl;
			return;
		}

		m_windows.erase(it);
	}

	void Manager::receiveMessage(const std::string &data)
	{
		auto is = newIs(data);

		u32 action = readU8(is);
		u64 id = readU64(is);

		switch (action) {
		case REOPEN_WINDOW: {
			u64 close_id = readU64(is);
			removeWindow(close_id);
		}
		// fallthrough

		case OPEN_WINDOW: {
			auto it = m_windows.find(id);
			if (it != m_windows.end()) {
				infostream << "Window " << id << " is already open" << std::endl;
				break;
			}

			it = m_windows.emplace(id, Window(id)).first;
			it->second.read(is, true);
			break;
		}

		case UPDATE_WINDOW: {
			auto it = m_windows.find(id);
			if (it != m_windows.end()) {
				it->second.read(is, false);
			} else {
				infostream << "Window " << id << " does not exist" << std::endl;
			}
			break;
		}

		case CLOSE_WINDOW:
			removeWindow(id);
			break;

		default:
			errorstream << "Invalid manager action: " << action << std::endl;
			break;
		}
	}

	void Manager::preDraw()
	{
		float base_size = RenderingEngine::getDisplayDensity();
		m_gui_pixel_size = base_size * g_settings->getFloat("gui_scaling");
		m_hud_pixel_size = base_size * g_settings->getFloat("hud_scaling");
	}

	void Manager::drawType(WindowType type)
	{
		Texture::begin();

		for (auto &it : m_windows) {
			if (it.second.getType() == type) {
				it.second.drawAll();
			}
		}

		Texture::end();
	}

	Manager g_manager;
}
