#pragma once

extern "C" {
#include <lua.h>
}

#include "clientdynamicinfo.h"

void push_window_info(lua_State *L, const ClientDynamicInfo &info);
