/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

/******************************************************************************/
/******************************************************************************/
/* WARNING!!!! do NOT add this header in any include file or any code file    */
/*             not being a modapi file!!!!!!!!                                */
/******************************************************************************/
/******************************************************************************/

#pragma once

#include "common/c_internal.h"
#include "util/basic_macros.h"
#include "porting.h"

#define luamethod(class, name) {#name, class::l_##name}

#define luamethod_dep(class, good, bad)                                     \
		{#bad, [](lua_State *L) -> int {                                    \
			return l_deprecated_function(L, #good, #bad, &class::l_##good); \
		}}

#define luamethod_aliased(class, good, bad) \
		luamethod(class, good),               \
		luamethod_dep(class, good, bad)

#define API_FCT(name) registerFunction(L, #name, l_##name, top)

// For future use
#define MAP_LOCK_REQUIRED ((void)0)
#define NO_MAP_LOCK_REQUIRED ((void)0)

/* In debug mode ensure no code tries to retrieve the server env when it isn't
 * actually available (in CSM) */
#if !defined(SERVER) && !defined(NDEBUG)
#define DEBUG_ASSERT_NO_CLIENTAPI                    \
	FATAL_ERROR_IF(getClient(L) != nullptr, "Tried " \
		"to retrieve ServerEnvironment on client")
#else
#define DEBUG_ASSERT_NO_CLIENTAPI ((void)0)
#endif

// Retrieve ServerEnvironment pointer as `env` (no map lock)
#define GET_ENV_PTR_NO_MAP_LOCK                              \
	DEBUG_ASSERT_NO_CLIENTAPI;                               \
	ServerEnvironment *env = (ServerEnvironment *)getEnv(L); \
	if (env == NULL)                                         \
		return 0

// Retrieve ServerEnvironment pointer as `env`
#define GET_ENV_PTR         \
	MAP_LOCK_REQUIRED;      \
	GET_ENV_PTR_NO_MAP_LOCK

// Retrieve Environment pointer as `env` (no map lock)
#define GET_PLAIN_ENV_PTR_NO_MAP_LOCK            \
	Environment *env = getEnv(L);                \
	if (env == NULL)                             \
		return 0

// Retrieve Environment pointer as `env`
#define GET_PLAIN_ENV_PTR         \
	MAP_LOCK_REQUIRED;            \
	GET_PLAIN_ENV_PTR_NO_MAP_LOCK

// LuaJIT FFI function prototypes.
#if USE_LUAJIT
#define FFI_FCT(rettype, name, ...) \
	extern "C" EXPORT_ATTRIBUTE rettype mtffi_##name(__VA_ARGS__) noexcept
#else
#define FFI_FCT(rettype, name, ...) \
	UNUSED_ATTRIBUTE static rettype mtffi_##name(__VA_ARGS__) noexcept
#endif
