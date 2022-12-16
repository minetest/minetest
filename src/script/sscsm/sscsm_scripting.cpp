#include "sscsm_scripting.h"
#include "sscsm_gamedef.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_item.h"
#include "lua_api/l_itemstackmeta.h"
#include "lua_api/l_util.h"

SSCSMScripting::SSCSMScripting(SSCSMGameDef *gamedef):
		ScriptApiBase(ScriptingType::SSCSM)
{
	setGameDef(gamedef);

	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	// Initialize our lua_api modules
	InitializeModApi(L, top);
	lua_pop(L, 1);

	// Push builtin initialization type
	lua_pushstring(L, "sscsm");
	lua_setglobal(L, "INIT");
}

void SSCSMScripting::InitializeModApi(lua_State *L, int top)
{
	ModApiItemMod::InitializeSSCSM(L, top);
	ModApiUtil::InitializeSSCSM(L, top);
	LuaItemStack::Register(L);
	ItemStackMetaRef::Register(L);
}
