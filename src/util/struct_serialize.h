/*
Minetest
Copyright (C) 2023 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "exceptions.h"
#include "util/type_traits.h"
#include <map>
#include <stddef.h>
#include <string>
#include <string.h>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>


template<typename T>
std::enable_if_t<
		is_map<T>::value || is_unordered_map<T>::value, T
> struct_deserialize(const void *buf, size_t size, size_t *use = nullptr);


template<typename T>
std::enable_if_t<
		is_tuple<T>::value || is_pair<T>::value
> struct_serialize(std::vector<char> &buf, const T &val);


template<typename T>
std::enable_if_t<
		is_tuple<T>::value || is_pair<T>::value, T
> struct_deserialize(const void *buf, size_t size, size_t *use = nullptr);


template<typename T>
std::enable_if_t<
		std::is_trivially_copyable<T>::value
				&& !is_basic_string<T>::value
				&& !is_map<T>::value
				&& !is_pair<T>::value
				&& !is_tuple<T>::value
				&& !is_unordered_map<T>::value
				&& !is_vector<T>::value
> struct_serialize(std::vector<char> &buf, const T &val)
{
	size_t old_size = buf.size();
	size_t new_size = old_size + sizeof(T);
	if (new_size < old_size)
		throw SerializationError("Output buffer size overflow");
	buf.resize(new_size);
	memcpy(buf.data() + old_size, &val, sizeof(T));
}


template<typename T>
std::enable_if_t<
		std::is_trivially_copyable<T>::value
				&& !is_basic_string<T>::value
				&& !is_map<T>::value
				&& !is_pair<T>::value
				&& !is_tuple<T>::value
				&& !is_unordered_map<T>::value
				&& !is_vector<T>::value,
		T
> struct_deserialize(const void *buf, size_t size, size_t *use = nullptr)
{
	if (size < sizeof(T))
		throw SerializationError("Buffer is too short");
	T val;
	memcpy(&val, buf, sizeof(T));
	if (use)
		*use = sizeof(T);
	return val;
}


template<typename T>
std::enable_if_t<
		is_basic_string<T>::value
				|| is_map<T>::value
				|| is_unordered_map<T>::value
				|| is_vector<T>::value
> struct_serialize(std::vector<char> &buf, const T &val)
{
	struct_serialize(buf, val.size());
	for (const auto &elem : val)
		struct_serialize(buf, elem);
}


template<typename T>
std::enable_if_t<
		is_basic_string<T>::value || is_vector<T>::value, T
> struct_deserialize(const void *buf, size_t size,
		size_t *use = nullptr)
{
	size_t use_val_len;
	size_t val_len = struct_deserialize<size_t>(buf, size, &use_val_len);
	T val;
	val.reserve(val_len);
	const char *buf_elems = (const char *)buf + use_val_len;
	size_t size_elems = size - use_val_len;
	for (size_t i = 0; i < val_len; i++) {
		size_t use_elem;
		val.push_back(struct_deserialize<typename T::value_type>(buf_elems, size_elems,
				&use_elem));
		buf_elems += use_elem;
		size_elems -= use_elem;
	}
	if (use)
		*use = size - size_elems;
	return val;
}


template<typename T>
std::enable_if_t<
		is_map<T>::value || is_unordered_map<T>::value, T
> struct_deserialize(const void *buf, size_t size, size_t *use)
{
	size_t use_val_len;
	size_t val_len = struct_deserialize<size_t>(buf, size, &use_val_len);
	T val;
	const char *buf_elems = (const char *)buf + use_val_len;
	size_t size_elems = size - use_val_len;
	for (size_t i = 0; i < val_len; i++) {
		using K = typename T::key_type;
		using V = typename T::mapped_type;
		size_t use_elem;
		auto elem = struct_deserialize<std::pair<K, V> >(buf_elems, size_elems, &use_elem);
		val[std::move(elem.first)] = std::move(elem.second);
		buf_elems += use_elem;
		size_elems -= use_elem;
	}
	if (use)
		*use = size - size_elems;
	return val;
}


template<typename T, size_t INDEX>
std::enable_if_t<
		INDEX == std::tuple_size<T>::value
> struct_serialize_tuple(std::vector<char> &buf, const T &val)
{
}

template<typename T, size_t INDEX>
std::enable_if_t<
		INDEX < std::tuple_size<T>::value
> struct_serialize_tuple(std::vector<char> &buf, const T &val)
{
	struct_serialize(buf, std::get<INDEX>(val));
	struct_serialize_tuple<T, INDEX + 1>(buf, val);
}

template<typename T>
std::enable_if_t<
		is_tuple<T>::value || is_pair<T>::value
> struct_serialize(std::vector<char> &buf, const T &val)
{
	struct_serialize_tuple<T, 0>(buf, val);
}


template<typename T, size_t INDEX>
std::enable_if_t<
		INDEX == std::tuple_size<T>::value
> struct_deserialize_tuple(const char *buf, size_t size, size_t *use, T &val)
{
	if (use)
		*use = 0;
}

template<typename T, size_t INDEX>
std::enable_if_t<
		INDEX < std::tuple_size<T>::value
> struct_deserialize_tuple(const char *buf, size_t size, size_t *use, T &val)
{
	size_t use_index;
	std::get<INDEX>(val) = struct_deserialize<typename std::tuple_element<INDEX, T>::type>(buf,
			size, &use_index);
	size_t use_after;
	struct_deserialize_tuple<T, INDEX + 1>(buf + use_index, size - use_index, &use_after, val);
	if (use)
		*use = use_index + use_after;
}

template<typename T>
std::enable_if_t<
		is_tuple<T>::value || is_pair<T>::value, T
> struct_deserialize(const void *buf, size_t size, size_t *use)
{
	T val;
	struct_deserialize_tuple<T, 0>((const char *)buf, size, use, val);
	return val;
}
