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

#include "env.h"

#include "manager.h"
#include "client/renderingengine.h"

#include <unordered_set>

using namespace std::string_literals;

namespace ui
{
	BadEnvException::BadEnvException(const Env *env, const std::string &message) :
		UiException("Bad UI (env \""s + env->getId() + "\"): "s + message)
	{}

	BadElemException::BadElemException(const Elem *elem, const std::string &message) :
		UiException("Bad UI (env \""s + elem->getEnv().getId() + "\", elem \"" +
				elem->getId() + "\"): "s + message)
	{}

	Elem &Env::getElem(const std::string &id)
	{
		auto it = m_elems.find(id);
		if (it == m_elems.end()) {
			throw BadEnvException(this, "No element exists with ID \""s + id + "\"");
		}
		return *it->second;
	}

	float Env::getPixelSize() const
	{
		return g_manager.getPixelSize(m_is_hud);
	}

	Dim<float> Env::getScreenSize() const
	{
		Dim<s32> screen_size = Texture().getSize();
		float pixel_size = getPixelSize();

		return Dim<float>(
			screen_size.Width / pixel_size,
			screen_size.Height / pixel_size
		);
	}

	void Env::drawAll()
	{
		Elem &root = getElem(m_root_elem);

		Dim<s32> screen_size = Texture().getSize();
		if (m_needs_layout || m_last_screen_size != screen_size) {
			root.layout();
			m_needs_layout = false;
			m_last_screen_size = screen_size;
		}

		root.draw();
	}

	void Env::applyJson(const JsonReader &json)
	{
		dissolveStructure();

		std::unordered_map<std::string, std::unique_ptr<Elem>> old_elems;
		m_elems.swap(old_elems);

		for (const auto &elem : json["elems"].toVector()) {
			std::string id = elem["id"].toId(MAIN_ID_CHARS, false);
			std::string type = elem["type"].toString();

			bool edit = false;
			elem["edit"].readBool(edit);

			if (m_elems.count(id)) {
				throw BadEnvException(this, "Redefinition of element with ID \""s +
						id + "\"");
			}

			if (edit) {
				if (!old_elems.count(id)) {
					throw BadEnvException(this, "Attempt to edit a non-existent "
							"element \""s + id + "\"");
				}

				std::unique_ptr<Elem> old_elem = std::move(old_elems.at(id));

				const std::string &old_type = g_registry.getElemType(*old_elem);
				if (old_type != type) {
					throw BadEnvException(this, "Attempt to edit an element with "
							"type \""s + old_type + "\" to a different type of \"" +
							type + "\"");
				}

				m_elems.emplace(id, std::move(old_elem));
			} else {
				auto ctor = g_registry.getElemCtor(type);
				if (ctor == nullptr) {
					throw BadEnvException(this, "No element exists with type \""s +
							type + "\"");
				}
				m_elems.emplace(id, (*ctor)(*this, id));
			}

			m_elems.at(id)->applyJson(elem);
		}

		m_root_elem = json["root_elem"].toId(MAIN_ID_CHARS, false);
		getElem(m_root_elem);

		// If the user sets the active element, it must exist or it is an error.
		// If the user doesn't change it, we need to set it to nothing if the
		// old active element no longer exists.
		bool found_active = json["active_elem"].readId(m_active_elem, VIRT_ID_CHARS, true);
		if (found_active && m_active_elem != NO_ID) {
			getElem(m_active_elem);
		} else if (!hasElem(m_active_elem)) {
			m_active_elem = NO_ID;
		}

		resolveStructure();
		requireLayout();
	}

	void Env::dissolveStructure()
	{
		for (auto &pair : m_elems) {
			pair.second->setParent(NO_ID);
		}
	}

	void Env::resolveStructure()
	{
		std::unordered_set<std::string> found_ids;
		resolveElementStructure(getElem(m_root_elem), found_ids);
	}

	void Env::resolveElementStructure(const Elem &elem,
			std::unordered_set<std::string> &found_ids)
	{
		const std::string &id = elem.getId();
		if (found_ids.count(id)) {
			throw BadElemException(&elem, "Element is a child of multiple elements or "
					"is used in a circular descendant loop");
		}
		found_ids.insert(id);

		const std::vector<std::string> &children = elem.getChildren();
		for (size_t i = 0; i < children.size(); i++) {
			Elem &child = getElem(children[i]);
			child.setParent(id);
			resolveElementStructure(child, found_ids);
		}
	}
}
