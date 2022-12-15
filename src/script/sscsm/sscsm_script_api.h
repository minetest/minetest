#pragma once

struct lua_State;

void sscsm_load_libraries(lua_State *L);

void sscsm_load_mods(lua_State *L);

void sscsm_run_step(lua_State *L, float dtime);
