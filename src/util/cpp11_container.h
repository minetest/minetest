/*
Minetest
Copyright (C) 2016 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#ifndef MT_CPP11CONTAINER_HEADER
#define MT_CPP11CONTAINER_HEADER

#if __cplusplus >= 201103L
#define USE_UNORDERED_CONTAINERS
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1600
#define USE_UNORDERED_CONTAINERS
#endif

#ifdef USE_UNORDERED_CONTAINERS
#include <unordered_map>
#include <unordered_set>
#define UNORDERED_MAP std::unordered_map
#define UNORDERED_SET std::unordered_set
#else
#include <map>
#include <set>
#define UNORDERED_MAP std::map
#define UNORDERED_SET std::set
#endif

#endif
