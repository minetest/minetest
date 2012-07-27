/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef ROLLBACK_HEADER
#define ROLLBACK_HEADER

#include <string>
#include "irr_v3d.h"
#include "rollback_interface.h"
#include <list>

class IGameDef;

class IRollbackManager: public IRollbackReportSink
{
public:
	// IRollbackReportManager
	virtual void reportAction(const RollbackAction &action) = 0;
	virtual std::string getActor() = 0;
	virtual bool isActorGuess() = 0;
	virtual void setActor(const std::string &actor, bool is_guess) = 0;
	virtual std::string getSuspect(v3s16 p, float nearness_shortcut,
			float min_nearness) = 0;

	virtual ~IRollbackManager(){}
	virtual void flush() = 0;
	// Get last actor that did something to position p, but not further than
	// <seconds> in history
	virtual std::string getLastNodeActor(v3s16 p, int range, int seconds,
			v3s16 *act_p, int *act_seconds) = 0;
	// Get actions to revert <seconds> of history made by <actor>
	virtual std::list<RollbackAction> getRevertActions(const std::string &actor,
			int seconds) = 0;
};

IRollbackManager *createRollbackManager(const std::string &filepath, IGameDef *gamedef);

#endif
