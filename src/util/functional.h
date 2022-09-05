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

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pl = std::placeholders;

template<typename T, typename Container, typename Func>
std::vector<T> map_vec(const Container &cont, const Func &func)
{
	std::vector<T> result;
	result.reserve(cont.size());

	std::transform(cont.begin(), cont.end(), std::back_inserter(result), func);
	return result;
}

template<typename T, typename Container, typename Func>
std::unordered_set<T> map_set(const Container &cont, const Func &func)
{
	std::unordered_set<T> result;
	result.reserve(cont.size());

	std::transform(cont.begin(), cont.end(), std::inserter(result, result.begin()), func);
	return result;
}

template<typename T, size_t N, typename Value, typename Func>
std::array<T, N> map_array(const std::array<Value, N> &cont, const Func &func)
{
	std::array<T, N> result;
	std::transform(cont.begin(), cont.end(), result.begin(), func);
	return result;
}

template<typename K, typename V, typename Map, typename Func>
std::unordered_map<K, V> map_pair_map(const Map &map, const Func &func)
{
	std::unordered_map<K, V> result;
	result.reserve(map.size());

	for (const auto &pair : map) {
		result.insert(func(pair.first, pair.second));
	}
	return result;
}

template<typename K, typename V, typename Map, typename Func>
std::unordered_map<K, V> map_key_map(const Map &map, const Func &func)
{
	return map_pair_map<K, V>(map, [&](auto key, auto value) {
		return std::make_pair(key, func(value));
	});
}

template<typename K, typename V, typename Map, typename Func>
std::unordered_map<K, V> map_value_map(const Map &map, const Func &func)
{
	return map_pair_map<K, V>(map, [&](auto key, auto value) {
		return std::make_pair(func(key), value);
	});
}
