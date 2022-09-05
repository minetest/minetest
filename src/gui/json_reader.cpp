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

#include "json_reader.h"

namespace ui
{
	JsonException::JsonException(const JsonReader *json, const std::string &message) :
		UiException("Bad UI JSON ("s + json->getPath() + "): " + message)
	{}

	std::string JsonReader::toId(const char *allowed_chars, bool allow_empty) const
	{
		std::string id = toString();
		if (!allow_empty && id.empty()) {
			throw JsonException(this, "ID may not be empty");
		} else if (!string_allowed(id, allowed_chars)) {
			throw JsonException(this, "Invalid characters in ID: only \""s +
					allowed_chars + "\" are allowed");
		}
		return id;
	}

	std::vector<JsonReader> JsonReader::toVector() const
	{
		checkArrayType();

		std::vector<JsonReader> vec;
		vec.reserve(m_value.size());
		for (size_t i = 0; i < m_value.size(); i++) {
			vec.emplace_back(m_value[(Json::ArrayIndex)i], pathTo(i));
		}
		return vec;
	}

	std::vector<JsonReader> JsonReader::toVector(size_t size) const
	{
		checkArrayType();
		if (m_value.size() != size) {
			throw JsonException(this, "Expected array to have size "s +
					std::to_string(size));
		}
		return toVector();
	}

	std::vector<JsonReader> JsonReader::toVector(size_t min, size_t max) const
	{
		checkArrayType();
		if (m_value.size() < min || m_value.size() > max) {
			throw JsonException(this, "Expected array to have size in range "s +
					std::to_string(min) + ".." + std::to_string(max));
		}
		return toVector();
	}

	std::unordered_map<std::string, JsonReader> JsonReader::toMap() const
	{
		if (!m_value.isObject()) {
			throwTypeException("object");
		}

		std::unordered_map<std::string, JsonReader> map;
		std::vector<std::string> keys = m_value.getMemberNames();
		for (size_t i = 0; i < keys.size(); i++) {
			const std::string &key = keys[i];
			map[key] = JsonReader(m_value[key], pathTo(key));
		}
		return map;
	}
}
