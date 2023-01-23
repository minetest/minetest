/*
Minetest
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "threading/ipc_channel.h"
#include "util/struct_serialize.h"

extern IPCChannelEnd g_csm_script_ipc;

extern std::vector<char> g_csm_script_ipc_buf;

void csm_ipc_check(bool succeeded);

template<typename T>
void csm_exchange_msg(const T &val)
{
	bool succeeded = true;
	try {
		g_csm_script_ipc_buf.clear();
		struct_serialize(g_csm_script_ipc_buf, val);
		succeeded = g_csm_script_ipc.exchange(g_csm_script_ipc_buf.data(),
				g_csm_script_ipc_buf.size());
	} catch (...) {
		succeeded = false;
	}
	csm_ipc_check(succeeded);
}

inline size_t csm_recv_size() { return g_csm_script_ipc.getRecvSize(); }

inline const void *csm_recv_data() { return g_csm_script_ipc.getRecvData(); }

template<typename T>
T csm_deserialize_msg()
{
	return struct_deserialize<T>(csm_recv_data(), csm_recv_size());
}
