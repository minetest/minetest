#pragma once

#include "cpp_api/s_base.h"

class ScriptApiCSM : public virtual ScriptApiBase
{
public:
	// Called on environment step
	void environment_Step(float dtime);
};
