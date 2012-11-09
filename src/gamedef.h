/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "irrlichttypes.h"

class IItemDefManager;
class INodeDefManager;
class ICraftDefManager;
class ITextureSource;
class ISoundManager;
class MtEventManager;
class IRollbackReportSink;

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

	// This is always thread-safe, but referencing the irrlicht texture
	// pointers in other threads than main thread will make things explode.
	virtual ITextureSource* getTextureSource()=0;
	
	// Used for keeping track of names/ids of unknown nodes
	virtual u16 allocateUnknownNodeId(const std::string &name)=0;
	
	// Only usable on the client
	virtual ISoundManager* getSoundManager()=0;
	virtual MtEventManager* getEventManager()=0;

	// Only usable on the server, and NOT thread-safe. It is usable from the
	// environment thread.
	virtual IRollbackReportSink* getRollbackReportSink(){return NULL;}
	
	// Used on the client
	virtual bool checkLocalPrivilege(const std::string &priv)
	{ return false; }
	
	// Shorthands
	IItemDefManager* idef(){return getItemDefManager();}
	INodeDefManager* ndef(){return getNodeDefManager();}
	ICraftDefManager* cdef(){return getCraftDefManager();}
	ITextureSource* tsrc(){return getTextureSource();}
	ISoundManager* sound(){return getSoundManager();}
	MtEventManager* event(){return getEventManager();}
	IRollbackReportSink* rollback(){return getRollbackReportSink();}
};

#endif

