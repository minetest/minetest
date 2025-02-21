// SPDX-FileCopyrightText: 2025 Luanti authors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"

class ModApiSSCSM : public ModApiBase
{
private:
	// print(text)
	static int l_print(lua_State *L);

	// log([level], text)
	static int l_log(lua_State *L);

	// get_node_or_nil(pos)
	static int l_get_node_or_nil(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
