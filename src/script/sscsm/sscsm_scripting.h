
#pragma once

#include "cpp_api/s_base.h"
#include "cpp_api/s_sscsm.h"

class SSCSMGameDef;

class SSCSMScripting:
		virtual public ScriptApiBase,
		public ScriptApiSSCSM
{
public:
	SSCSMScripting(SSCSMGameDef *gamedef);

private:
	void InitializeModApi(lua_State *L, int top);
};
