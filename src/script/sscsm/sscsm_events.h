// SPDX-FileCopyrightText: 2024 Luanti authors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "sscsm_ievent.h"
#include "debug.h"
#include "irrlichttypes.h"
#include "sscsm_environment.h"

struct SSCSMEventTearDown : public ISSCSMEvent
{
	void exec(SSCSMEnvironment *env) override
	{
		FATAL_ERROR("SSCSMEventTearDown needs to be handled by SSCSMEnvironment::run()");
	}
};

struct SSCSMEventUpdateVFSFiles : public ISSCSMEvent
{
	// pairs are virtual path and file content
	std::vector<std::pair<std::string, std::string>> files;

	void exec(SSCSMEnvironment *env) override
	{
		env->updateVFSFiles(std::move(files));
	}
};

struct SSCSMEventLoadMods : public ISSCSMEvent
{
	// paths to init.lua files, in load order
	std::vector<std::string> init_paths;

	void exec(SSCSMEnvironment *env) override
	{
		env->getScript()->load_mods(init_paths);
	}
};

struct SSCSMEventOnStep : public ISSCSMEvent
{
	f32 dtime;

	void exec(SSCSMEnvironment *env) override
	{
		env->getScript()->environment_step(dtime);
	}
};

