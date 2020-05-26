/*
Minetest
Copyright (C) 2020 rubenwardy

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

#if __cplusplus == 201103L

namespace std {

	template<typename T, typename... Args>
	std::unique_ptr<T> make_unique(Args &&... args) {
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

}

#endif
