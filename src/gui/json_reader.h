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

#include "convert_json.h"
#include "debug.h"
#include "ui_types.h"
#include "util/functional.h"
#include "util/string.h"

#include <array>
#include <initializer_list>
#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::string_literals;

namespace ui
{
	class JsonReader;

	struct JsonException : UiException
	{
		JsonException(const JsonReader *json, const std::string &message);
	};

	template<typename T, typename = void>
	struct Lim {};

	template<typename T>
	struct Lim<T, std::enable_if_t<std::is_integral<T>::value>>
	{
		static T low()
		{
			return std::numeric_limits<T>::min();
		}

		static T high()
		{
			return std::numeric_limits<T>::max();
		}
	};

	template<typename T>
	struct Lim<T, std::enable_if_t<std::is_floating_point<T>::value>>
	{
		static T low()
		{
			return -std::numeric_limits<T>::infinity();
		}

		static T high()
		{
			return std::numeric_limits<T>::infinity();
		}
	};

#define JSON_BIND_TO_0(member) std::bind(&JsonReader::member, pl::_1)
#define JSON_BIND_TO(member, ...) std::bind(&JsonReader::member, pl::_1, __VA_ARGS__)

#define JSON_BIND_READ_0(member) JSON_BIND_TO(member, pl::_2)
#define JSON_BIND_READ(member, ...) JSON_BIND_TO(member, pl::_2, __VA_ARGS__)

	class JsonReader
	{
	public:
		static constexpr float EXCLUSIVE_FLOAT = 0.000001f;
		static constexpr double EXCLUSIVE_DOUBLE = 0.000001;

	private:
		Json::Value m_value;
		std::string m_path;

	public:
		JsonReader() :
			m_path("json")
		{}

		JsonReader(Json::Value value) :
			JsonReader(std::move(value), "json")
		{}

		JsonReader(Json::Value value, std::string path) :
			m_value(std::move(value)),
			m_path(std::move(path))
		{}

		const Json::Value &getJson() const
		{
			return m_value;
		}

		const std::string &getPath() const
		{
			return m_path;
		}

		JsonReader operator[](const std::string &key) const
		{
			return JsonReader(m_value.get(key, Json::Value()), pathTo(key));
		}

		bool isNull() const
		{
			return m_value.isNull();
		}

		explicit operator bool() const
		{
			return !isNull();
		}

		bool toBool() const
		{
			if (!m_value.isBool()) {
				throwTypeException("boolean");
			}
			return m_value.asBool();
		}

		bool readBool(bool &into) const
		{
			if (!isNull()) {
				into = toBool();
				return true;
			}
			return false;
		}

		template<typename T, std::enable_if_t<
				std::is_integral<T>::value && std::is_signed<T>::value, int> = 0>
		T toNum(T min = Lim<T>::low(), T max = Lim<T>::high()) const
		{
			if (!m_value.isInt64()) {
				throwTypeException("integer");
			}

			T num = m_value.asInt64();

			if (num < min || num > max) {
				throw JsonException(this, "Expected integer in range "s +
						std::to_string(min) + ".." + std::to_string(max));
			}

			return num;
		}

		template<typename T, std::enable_if_t<
				std::is_unsigned<T>::value && !std::is_same<T, bool>::value, int> = 0>
		T toNum(T min = Lim<T>::low(), T max = Lim<T>::high()) const
		{
			if (!m_value.isUInt64()) {
				throwTypeException("unsigned integer");
			}

			T num = m_value.asUInt64();

			if (num < min || num > max) {
				throw JsonException(this, "Expected integer in range "s +
						std::to_string(min) + ".." + std::to_string(max));
			}
			return num;
		}

		template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
		T toNum(T min = Lim<T>::low(), T max = Lim<T>::high()) const
		{
			if (!m_value.isDouble()) {
				throwTypeException("number");
			}

			T num = m_value.asDouble();

			if (num < min || num > max) {
				throw JsonException(this, "Expected number in range "s +
						std::to_string(min) + ".." + std::to_string(max));
			}
			return num;
		}

		template<typename T>
		bool readNum(T &into, T min = Lim<T>::low(), T max = Lim<T>::high()) const
		{
			if (!isNull()) {
				into = toNum<T>(min, max);
				return true;
			}
			return false;
		}

		template<typename T>
		bool readVec(
			Vec<T> &into,
			Vec<T> min = {Lim<T>::low(), Lim<T>::low()},
			Vec<T> max = {Lim<T>::high(), Lim<T>::high()}) const
		{
			if (!isNull()) {
				auto arr = toArray<2>();
				arr[0].readNum(into.X, min.X, max.X);
				arr[1].readNum(into.Y, min.Y, max.Y);
				return true;
			}
			return false;
		}

		template<typename T>
		bool readDim(
			Dim<T> &into,
			Dim<T> min = {0, 0},
			Dim<T> max = {Lim<T>::high(), Lim<T>::high()}) const
		{
			if (!isNull()) {
				auto arr = toArray<2>();
				arr[0].readNum(into.Width, min.Width, max.Width);
				arr[1].readNum(into.Height, min.Height, max.Height);
				return true;
			}
			return false;
		}

		template<typename T>
		bool readRect(
			Rect<T> &into,
			Rect<T> inside = {
				Lim<T>::low(), Lim<T>::low(), Lim<T>::high(), Lim<T>::high()
			}) const
		{
			if (!isNull()) {
				auto arr = toArray<4>();
				arr[0].readNum(left(into), left(inside), right(inside));
				arr[1].readNum(top(into), top(inside), bottom(inside));
				arr[2].readNum(right(into), left(into), right(inside));
				arr[3].readNum(bottom(into), top(into), bottom(inside));
				return true;
			}
			return false;
		}

		template<typename T>
		bool readEdges(
			Rect<T> &into,
			Rect<T> min = {0.0f, 0.0f, 0.0f, 0.0f},
			Rect<T> max = {
				Lim<T>::high(), Lim<T>::high(), Lim<T>::high(), Lim<T>::high()
			}) const
		{
			if (!isNull()) {
				auto arr = toArray<4>();
				arr[0].readNum(left(into), left(min), left(max));
				arr[1].readNum(top(into), top(min), top(max));
				arr[2].readNum(right(into), right(min), right(max));
				arr[3].readNum(bottom(into), bottom(min), bottom(max));
				return true;
			}
			return false;
		}

		std::string toString() const
		{
			if (!m_value.isString()) {
				throwTypeException("string");
			}
			return m_value.asString();
		}

		bool readString(std::string &into) const
		{
			if (!isNull()) {
				into = toString();
				return true;
			}
			return false;
		}

		std::string toId(const char *allowed_chars, bool allow_empty) const;

		bool readId(std::string &into, const char *allowed_chars, bool allow_empty) const
		{
			if (!isNull()) {
				into = toId(allowed_chars, allow_empty);
				return true;
			}
			return false;
		}

		Color toColor() const
		{
			Color color;
			if (!parseColorString(toString(), color, true)) {
				throw JsonException(this, "Invalid color string");
			}
			return color;
		}

		bool readColor(Color &into) const
		{
			if (!isNull()) {
				into = toColor();
				return true;
			}
			return false;
		}

		template<typename T>
		T toEnum(const EnumNameMap<T> &map) const
		{
			std::string name = toString();
			T value;
			if (!str_to_enum(&value, map, name.c_str())) {
				throw JsonException(this, "Invalid enum value");
			}
			return value;
		}

		template<typename T>
		bool readEnum(T &into, const EnumNameMap<T> &map) const
		{
			if (!isNull()) {
				into = toEnum(map);
				return true;
			}
			return false;
		}

		std::vector<JsonReader> toVector() const;
		std::vector<JsonReader> toVector(size_t size) const;
		std::vector<JsonReader> toVector(size_t min, size_t max) const;

		template<typename Container>
		std::vector<JsonReader> toVector(const Container &sizes) const
		{
			checkArrayType();
			if (std::find(sizes.begin(), sizes.end(), m_value.size()) == sizes.end()) {
				throw JsonException(this, "Expected array to have size of one of "s +
						str_join(sizes, ", "));
			}
			return toVector();
		}

		template<size_t N>
		std::array<JsonReader, N> toArray() const
		{
			checkArrayType();
			if (m_value.size() != N) {
				throw JsonException(this, "Expected array to have size "s + std::to_string(N));
			}

			std::array<JsonReader, N> arr;
			for (size_t i = 0; i < N; i++) {
				arr[i] = JsonReader(m_value[(Json::ArrayIndex)i], pathTo(i));
			}
			return arr;
		}

		std::unordered_map<std::string, JsonReader> toMap() const;

		template<typename T, typename Func, typename ...Args>
		std::vector<T> mapVector(const Func &func, Args &&...args) const
		{
			return map_vec<T>(toVector(std::forward<Args>(args)...), func);
		}

		template<typename T, typename Func, typename ...Args>
		bool readVector(std::vector<T> &into, const Func &func, Args &&...args) const
		{
			if (!isNull()) {
				into = mapVector<T>(func, std::forward<Args>(args)...);
				return true;
			}
			return false;
		}

		template<typename T, typename Func, typename ...Args>
		std::unordered_set<T> mapSet(const Func &func, Args &&...args) const
		{
			return map_set<T>(toVector(std::forward<Args>(args)...), func);
		}

		template<typename T, typename Func, typename ...Args>
		bool readSet(std::unordered_set<T> &into, const Func &func, Args &&...args) const
		{
			if (!isNull()) {
				into = mapSet<T>(func, std::forward<Args>(args)...);
				return true;
			}
			return false;
		}

		template<typename T, size_t N, typename Func>
		bool readArray(std::array<T, N> &into, const Func &func) const
		{
			if (!isNull()) {
				auto arr = toArray<N>();
				for (size_t i = 0; i < N; i++) {
					func(arr[i], into[i]);
				}
				return true;
			}
			return false;
		}

	private:
		std::string pathTo(size_t index) const
		{
			return m_path + '[' + std::to_string(index) + ']';
		}

		std::string pathTo(const std::string &key) const
		{
			return m_path + '[' + fastWriteJson(Json::Value(key)) + ']';
		}

		void checkArrayType() const
		{
			if (!m_value.isArray()) {
				throwTypeException("array");
			}
		}

		void throwTypeException(const std::string &type) const
		{
			throw JsonException(this, "Expected value of type \""s + type + "\"");
		}
	};

	class JsonWriter
	{
		static Json::Value array()
		{
			return Json::Value(Json::arrayValue);
		}

		static Json::Value object()
		{
			return Json::Value(Json::objectValue);
		}

		static Json::Value fromBool(bool value)
		{
			return Json::Value(value);
		}

		template<typename T>
		static Json::Value fromNum(T num)
		{
			return Json::Value(num);
		}

		template<typename T>
		static Json::Value fromVec(const Vec<T> &vec)
		{
			Json::Value json = array();
			json[0] = fromNum(vec.X);
			json[1] = fromNum(vec.Y);
			return json;
		}

		template<typename T>
		static Json::Value fromDim(const Dim<T> &dim)
		{
			Json::Value json = array();
			json[0] = fromNum(dim.Width);
			json[1] = fromNum(dim.Height);
			return json;
		}

		template<typename T>
		static Json::Value fromRect(const Rect<T> &rect)
		{
			Json::Value json = array();
			json[0] = fromNum(left(rect));
			json[1] = fromNum(top(rect));
			json[2] = fromNum(right(rect));
			json[3] = fromNum(bottom(rect));
			return json;
		}

		template<typename T>
		static Json::Value fromEdges(const Rect<T> &edges)
		{
			return fromRect(edges);
		}

		static Json::Value fromString(const std::string &str)
		{
			return Json::Value(str);
		}

		static Json::Value fromId(const std::string &str)
		{
			return fromString(str);
		}

		static Json::Value fromColor(Color color)
		{
			return Json::Value(to_color_string(color));
		}

		template<typename T>
		static Json::Value fromEnum(T value, const EnumNameMap<T> &map)
		{
			const char *name = enum_to_str(map, value);
			sanity_check(name != nullptr);
			return Json::Value(name);
		}

		template<typename Container>
		static Json::Value fromList(const Container &cont)
		{
			Json::Value json = array();
			for (const auto &value : cont) {
				json.append(value);
			}
			return json;
		}

		template<typename Map>
		static Json::Value fromMap(const Map &map)
		{
			Json::Value json = JsonWriter::object();
			for (const auto &pair : map) {
				json[std::to_string(pair.first)] = pair.second;
			}
			return json;
		}
	};
}
