#include "csm_scripting.h"
#include "csm_gamedef.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_item.h"
#include "lua_api/l_itemstackmeta.h"
#include "lua_api/l_util.h"

CSMScripting::CSMScripting(CSMGameDef *gamedef):
		ScriptApiBase(ScriptingType::CSM)
{
	setGameDef(gamedef);

	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	// Initialize our lua_api modules
	InitializeModApi(L, top);
	lua_pop(L, 1);

	// Push builtin initialization type
	lua_pushstring(L, "csm");
	lua_setglobal(L, "INIT");
}

void CSMScripting::InitializeModApi(lua_State *L, int top)
{
	ModApiItemMod::InitializeCSM(L, top);
	ModApiUtil::InitializeCSM(L, top);
	LuaItemStack::Register(L);
	ItemStackMetaRef::Register(L);
}
