#pragma once

#include "lua_api/l_base.h"

class Crypto;

class LuaCrypto : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];

	// Garbage collector
	static int gc_object(lua_State *L);

public:
	LuaCrypto();
	~LuaCrypto();

	static void create(lua_State *L);

	static int create_object(lua_State *L);

	static LuaCrypto *checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);

	static int l_generate_wallet(lua_State *L);

	static int l_sign_message(lua_State *L);

	static int l_create_signed_transfer_transaction(lua_State *L);
};
