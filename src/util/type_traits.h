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

#include <map>
#include <string>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

template<typename T>
struct is_basic_string
{
	static constexpr bool value = false;
};

template<template<typename...> typename T, typename U>
struct is_basic_string<T<U> >
{
	static constexpr bool value = std::is_same<T<U>, std::basic_string<U> >::value;
};


template<typename T>
struct is_map
{
	static constexpr bool value = false;
};

template<template<typename...> typename T, typename U, typename V>
struct is_map<T<U, V> >
{
	static constexpr bool value = std::is_same<T<U, V>, std::map<U, V> >::value;
};


template<typename T>
struct is_pair
{
	static constexpr bool value = false;
};

template<template<typename...> typename T, typename U, typename V>
struct is_pair<T<U, V> >
{
	static constexpr bool value = std::is_same<T<U, V>, std::pair<U, V> >::value;
};


template<typename T>
struct is_tuple
{
	static constexpr bool value = false;
};

template<template<typename...> typename T, typename... U>
struct is_tuple<T<U...> >
{
	static constexpr bool value = std::is_same<T<U...>, std::tuple<U...> >::value;
};


template<typename T>
struct is_unordered_map
{
	static constexpr bool value = false;
};

template<template<typename...> typename T, typename U, typename V>
struct is_unordered_map<T<U, V> >
{
	static constexpr bool value = std::is_same<T<U, V>, std::unordered_map<U, V> >::value;
};


template<typename T>
struct is_vector
{
	static constexpr bool value = false;
};

template<template<typename...> typename T, typename U>
struct is_vector<T<U> >
{
	static constexpr bool value = std::is_same<T<U>, std::vector<U> >::value;
};
