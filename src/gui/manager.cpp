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

#include "manager.h"

#include "elem_registry.h"
#include "util/string.h"

namespace ui
{
	constexpr EnumNameMap<bool> ENV_TYPE_MAP = {
		{"gui", false},
		{"hud", true}
	};

	Env &Manager::getEnv(const std::string &id)
	{
		auto it = m_envs.find(id);
		if (it == m_envs.end()) {
			throw BadUiException("No env exists with ID \""s + id + "\"");
		}
		return *it->second;
	}

	void Manager::drawAll()
	{
		for (auto &pair : m_envs) {
			pair.second->drawAll();
		}
	}

	void Manager::applyJson(const JsonReader &json)
	{
		if (auto edit = json["edit"]) {
			if (!edit.toBool()) {
				resetAll();
			}
		}

		std::unordered_map<std::string, std::unique_ptr<Env>> old_envs;
		m_envs.swap(old_envs);

		for (const auto &env : json["envs"].toVector()) {
			std::string id = env["id"].toId(MAIN_ID_CHARS, false);
			bool is_hud = env["type"].toEnum(ENV_TYPE_MAP);

			bool edit = false;
			json["edit"].readBool(edit);

			if (m_envs.count(id)) {
				throw BadUiException("Redefinition of environment with ID \""s +
						id + "\"");
			}

			if (edit) {
				if (!old_envs.count(id)) {
					throw BadUiException("Attempt to edit a non-existent "
							"environment \""s + id + "\"");
				}

				std::unique_ptr<Env> old_env = std::move(old_envs.at(id));
				if (old_env->isHud() != is_hud) {
					throw BadEnvException(old_env.get(),
							"Attempt to edit an environment with type \""s +
							enum_to_str(ENV_TYPE_MAP, old_env->isHud()) +
							"\" with a different type of \"" +
							enum_to_str(ENV_TYPE_MAP, is_hud) + "\"");
				}

				m_envs.emplace(id, std::move(old_env));
			} else {
				m_envs.emplace(id, std::make_unique<Env>(id, is_hud));
			}

			m_envs.at(id)->applyJson(env);
		}

		m_active_env = json["active_env"].toId(MAIN_ID_CHARS, true);
		if (m_active_env != NO_ID) {
			getEnv(m_active_env);
		}
	}

	float Manager::getPixelSize(bool is_hud) const
	{
		if (m_gui_pixel_size == 0.0f) {
			float base_size = RenderingEngine::getDisplayDensity();
			m_gui_pixel_size = base_size * g_settings->getFloat("gui_scaling");
			m_hud_pixel_size = base_size * g_settings->getFloat("hud_scaling");
		}

		return (is_hud) ? m_hud_pixel_size : m_gui_pixel_size;
	}

	Manager g_manager;
}
