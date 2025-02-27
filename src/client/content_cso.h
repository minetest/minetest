// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "clientsimpleobject.h"
#include "irr_v3d.h"
#include "irr_v2d.h"

namespace irr::scene
{
	class ISceneManager;
}

ClientSimpleObject* createSmokePuff(scene::ISceneManager *smgr,
		ClientEnvironment *env, v3f pos, v2f size);
