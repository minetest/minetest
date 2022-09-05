/*
Minetest
Copyright (C) 2022 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

#include "client/client.h"
#include "client/renderingengine.h"
#include "elem.h"
#include "env.h"
#include "ui_types.h"

#include <memory>
#include <string>
#include <unordered_map>

using namespace std::string_literals;

namespace ui
{
	struct BadUiException : UiException
	{
		BadUiException(const std::string &message) :
			UiException("Bad UI: "s + message)
		{}
	};

	class Manager
	{
	private:
		/* We store the Envs in unique_ptrs because we have references to an Env
		 * in each Elem, so they need to have the same memory location at all
		 * times. Since we move them around between maps, this is not true
		 * without the unique_ptrs.
		 */
		std::unordered_map<std::string, std::unique_ptr<Env>> m_envs;
		std::string m_active_env = NO_ID;

		Client *m_client = nullptr;

		mutable float m_gui_pixel_size = 0.0f; // Cached value
		mutable float m_hud_pixel_size = 0.0f; // Cached value

	public:
		Manager() = default;

		Manager(const Manager &) = delete;
		Manager &operator=(const Manager &) = delete;

		bool hasEnv(const std::string &id)
		{
			return m_envs.count(id);
		}

		Env &getEnv(const std::string &id);

		void drawAll();

		void resetAll()
		{
			m_envs.clear();
			m_active_env = NO_ID;
		}

		void applyJson(const JsonReader &json);

		Client *getClient()
		{
			return m_client;
		}

		const Client *getClient() const
		{
			return m_client;
		}

		void setClient(Client *client)
		{
			m_client = client;
		}

		Texture getTexture(const std::string &name)
		{
			return Texture(m_client->tsrc()->getTexture(name));
		}

		u32 getTime() const
		{
			return RenderingEngine::get_raw_device()->getTimer()->getTime();
		}

		float getPixelSize(bool is_hud) const;
	};

	extern Manager g_manager;
}
