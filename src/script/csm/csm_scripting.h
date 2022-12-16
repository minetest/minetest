
#pragma once

#include "cpp_api/s_base.h"
#include "cpp_api/s_csm.h"

class CSMGameDef;

class CSMScripting:
		virtual public ScriptApiBase,
		public ScriptApiCSM
{
public:
	CSMScripting(CSMGameDef *gamedef);

private:
	void InitializeModApi(lua_State *L, int top);
};
