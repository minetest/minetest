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

#include "irrlichttypes.h"
#include "util/Optional.h"
#include <stdio.h>
#include <vector>

enum class CSMMsgType {
	// controller -> script
	C2S_RUN_STEP,
	C2S_RUN_LOAD_MODS,
	C2S_GET_NODE,

	// script -> controller
	S2C_DONE,
	S2C_GET_NODE,
};

struct CSMRecvMsg {
	CSMMsgType type;
	std::vector<u8> data;
};

bool csm_send_msg(FILE *f, CSMMsgType type, size_t size, const void *data);

Optional<CSMRecvMsg> csm_recv_msg(FILE *f);

void csm_send_msg_ex(FILE *f, CSMMsgType type, size_t size, const void *data);

CSMRecvMsg csm_recv_msg_ex(FILE *f);
