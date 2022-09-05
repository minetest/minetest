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

#include <memory>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

namespace ui
{
	class Env;

	class ElemRegistry
	{
	public:
		using ElemCtor = std::unique_ptr<Elem> (*)(Env &env, std::string id);

	private:
		std::unordered_map<std::type_index, std::string> m_elem_to_str;
		std::unordered_map<std::string, ElemCtor> m_str_to_ctor;

	public:
		ElemRegistry();

		template<typename T>
		void registerElem(const std::string &name)
		{
			m_elem_to_str.emplace(std::type_index(typeid(T)), name);
			m_str_to_ctor.emplace(name, [](Env &env, std::string id) {
				return std::make_unique<T>(env, std::move(id));
			});
		}

		template<typename T>
		const std::string &getElemType() const
		{
			return m_elem_to_str.at(std::type_index(typeid(T)));
		}

		const std::string &getElemType(const Elem &elem) const
		{
			return m_elem_to_str.at(std::type_index(typeid(elem)));
		}

		const ElemCtor *getElemCtor(const std::string &name) const
		{
			auto it = m_str_to_ctor.find(name);
			if (it == m_str_to_ctor.end()) {
				return nullptr;
			}
			return &it->second;
		}
	};

	extern ElemRegistry g_registry;
}
