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

#ifndef GAMEDEF_HEADER
#define GAMEDEF_HEADER

#include <string>
#include <vector>
#include "irrlichttypes.h"

class IItemDefManager;
class INodeDefManager;
class ICraftDefManager;
class ITextureSource;
class ISoundManager;
class IShaderSource;
class MtEventManager;
class IRollbackManager;
class EmergeManager;
class Camera;
class ModMetadata;

namespace irr { namespace scene {
	class IAnimatedMesh;
	class ISceneManager;
}}

struct ModSpec;
/*
	An interface for fetching game-global definitions like tool and
	mapnode properties
*/

class IGameDef
{
public:
	// These are thread-safe IF they are not edited while running threads.
	// Thus, first they are set up and then they are only read.
	virtual IItemDefManager* getItemDefManager()=0;
	virtual INodeDefManager* getNodeDefManager()=0;
	virtual ICraftDefManager* getCraftDefManager()=0;

	// Used for keeping track of names/ids of unknown nodes
	virtual u16 allocateUnknownNodeId(const std::string &name)=0;

	virtual MtEventManager* getEventManager()=0;

	// Only usable on the server, and NOT thread-safe. It is usable from the
	// environment thread.
	virtual IRollbackManager* getRollbackManager() { return NULL; }

	// Shorthands
	IItemDefManager  *idef()     { return getItemDefManager(); }
	INodeDefManager  *ndef()     { return getNodeDefManager(); }
	ICraftDefManager *cdef()     { return getCraftDefManager(); }

	MtEventManager   *event()    { return getEventManager(); }
	IRollbackManager *rollback() { return getRollbackManager(); }

	virtual const std::vector<ModSpec> &getMods() const = 0;
	virtual const ModSpec* getModSpec(const std::string &modname) const = 0;
	virtual std::string getWorldPath() const { return ""; }
	virtual std::string getModStoragePath() const = 0;
	virtual bool registerModStorage(ModMetadata *storage) = 0;
	virtual void unregisterModStorage(const std::string &name) = 0;
};

#endif

