// SPDX-FileCopyrightText: 2024 Luanti authors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "sscsm_ievent.h"
#include "debug.h"
#include "irrlichttypes.h"
#include "irr_v3d.h"
#include "sscsm_environment.h"
#include "mapnode.h"

struct SSCSMEventTearDown : public ISSCSMEvent
{
	void exec(SSCSMEnvironment *env) override
	{
		FATAL_ERROR("SSCSMEventTearDown needs to be handled by SSCSMEnvironment::run()");
	}
};

struct SSCSMEventOnStep : public ISSCSMEvent
{
	f32 dtime;

	SSCSMEventOnStep(f32 dtime_) : dtime(dtime_) {}

	void exec(SSCSMEnvironment *env) override
	{
		// example
		env->requestGetNode(v3s16(0, 0, 0));
	}
};

