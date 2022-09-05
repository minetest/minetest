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

#include "elem.h"
#include "elem_registry.h"
#include "ui_types.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace ui
{
	class Env;

	struct BadEnvException : UiException
	{
		BadEnvException(const Env *env, const std::string &message);
	};

	struct BadElemException : UiException
	{
		BadElemException(const Elem *elem, const std::string &message);
	};

	class Env
	{
	private:
		std::string m_id; // Computed; cross reference to ID in Manager
		bool m_is_hud;

		std::unordered_map<std::string, std::unique_ptr<Elem>> m_elems;

		std::string m_root_elem = NO_ID;
		std::string m_active_elem = NO_ID;

	public:
		Env(std::string id, bool is_hud) :
			m_id(std::move(id)),
			m_is_hud(is_hud)
		{}

		Env(const Env &) = delete;
		Env &operator=(const Env &) = delete;

		const std::string &getId() const
		{
			return m_id;
		}

		bool hasElem(const std::string &id)
		{
			return m_elems.count(id);
		}

		Elem &getElem(const std::string &id);

		template<typename T>
		T &getElem(const std::string &id)
		{
			Elem &elem = getElem(id);
			T *typed = dynamic_cast<T *>(&elem);
			if (typed == nullptr) {
				throw BadElemException(elem, "Expected element of type \""s +
						g_registry.getElemType<T>() + "\", got type \"" +
						g_registry.getElemType(elem) + "\"");
			}

			return typed;
		}

		bool isHud() const
		{
			return m_is_hud;
		}

		float getPixelSize() const;

		Dim<float> getScreenSize() const
		{
			Dim<s32> int_size = Texture().getSize();
			return Dim<float>(
				int_size.Width / m_pixel_size,
				int_size.Height / m_pixel_size
			);
		}

		void drawAll();

		void applyJson(const JsonReader &json);

	private:
		void dissolveStructure();
		void resolveStructure();
		void resolveElementStructure(const Elem &elem,
				std::unordered_set<std::string> &found_ids);
	};
}
