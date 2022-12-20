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

#include "debug.h"
#include "threading/ipc_channel.h"

extern IPCChannelEnd g_csm_script_ipc;

#define CSM_IPC(call) \
	do { \
		if (!g_csm_script_ipc.call) \
			FATAL_ERROR("CSM process IPC failed"); \
	} while (0)

inline size_t csm_recv_size() { return g_csm_script_ipc.getRecvSize(); }

inline const void *csm_recv_data() { return g_csm_script_ipc.getRecvData(); }

int csm_script_main(int argc, char *argv[]);
