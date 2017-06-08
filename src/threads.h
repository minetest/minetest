/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

///////////////
#include <thread>

//
// threadid_t, threadhandle_t
//
typedef std::thread::id threadid_t;
typedef std::thread::native_handle_type threadhandle_t;

//
// ThreadStartFunc
//
typedef void *ThreadStartFunc(void *param);


inline threadid_t thr_get_current_thread_id()
{
	return std::this_thread::get_id();
}

inline bool thr_compare_thread_id(threadid_t thr1, threadid_t thr2)
{
	return thr1 == thr2;
}

inline bool thr_is_current_thread(threadid_t thr)
{
	return thr_compare_thread_id(thr_get_current_thread_id(), thr);
}
