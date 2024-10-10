#include "c_window_info.h"
#include "c_converter.h"

void push_window_info(lua_State *L, const ClientDynamicInfo &info)
{
	lua_newtable(L);
	int top = lua_gettop(L);

	lua_pushstring(L, "size");
	push_v2u32(L, info.render_target_size);
	lua_settable(L, top);

	lua_pushstring(L, "insets");
	lua_newtable(L);
	int top2 = lua_gettop(L);

	lua_pushstring(L, "bottom");
	lua_pushinteger(L, info.insets.LowerRightCorner.Y);
	lua_settable(L, top2);

	lua_pushstring(L, "left");
	lua_pushinteger(L, info.insets.UpperLeftCorner.X);
	lua_settable(L, top2);

	lua_pushstring(L, "right");
	lua_pushinteger(L, info.insets.LowerRightCorner.X);
	lua_settable(L, top2);

	lua_pushstring(L, "top");
	lua_pushinteger(L, info.insets.UpperLeftCorner.Y);
	lua_settable(L, top2);

	lua_settable(L, top);

	lua_pushstring(L, "max_formspec_size");
	push_v2f(L, info.max_fs_size);
	lua_settable(L, top);

	lua_pushstring(L, "real_gui_scaling");
	lua_pushnumber(L, info.real_gui_scaling);
	lua_settable(L, top);

	lua_pushstring(L, "real_hud_scaling");
	lua_pushnumber(L, info.real_hud_scaling);
	lua_settable(L, top);

	lua_pushstring(L, "touch_controls");
	lua_pushboolean(L, info.touch_controls);
	lua_settable(L, top);
}
