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

#include <iostream>

#include "content/mods.h"
#include "environment.h"
#include "gamedef.h"
#include "map.h"
#include "nodedef.h"

/*
	MapEditEvent
*/

#define MAPTYPE_MOCK 3

#define UNIMPLEMENTED(retval)                                                            \
	warn_unimplemented(__FUNCTION__);                                                \
	return retval;

class MockGameDef : public IGameDef
{
public:
	MockGameDef(std::ostream &dout) : dout(dout), m_ndef(), m_modspec(){};

	virtual IItemDefManager *getItemDefManager() { UNIMPLEMENTED(nullptr) }

	virtual const NodeDefManager *getNodeDefManager() { return &m_ndef; }

	virtual ICraftDefManager *getCraftDefManager() { UNIMPLEMENTED(nullptr) }
	virtual u16 allocateUnknownNodeId(const std::string &name) { UNIMPLEMENTED(0) }
	virtual const std::vector<ModSpec> &getMods() const { UNIMPLEMENTED(m_modspec) }

	virtual const ModSpec *getModSpec(const std::string &modname) const
	{
		UNIMPLEMENTED(nullptr)
	}

	virtual std::string getModStoragePath() const { UNIMPLEMENTED("") }
	virtual bool registerModStorage(ModMetadata *storage) { UNIMPLEMENTED(false) }
	virtual void unregisterModStorage(const std::string &name) { UNIMPLEMENTED(;) }
	virtual bool joinModChannel(const std::string &channel) { UNIMPLEMENTED(false) }
	virtual bool leaveModChannel(const std::string &channel) { UNIMPLEMENTED(false) }

	virtual bool sendModChannelMessage(
			const std::string &channel, const std::string &message)
	{
		UNIMPLEMENTED(false)
	}

	virtual ModChannel *getModChannel(const std::string &channel){
			UNIMPLEMENTED(nullptr)}

	content_t registerContent(const std::string &name, const ContentFeatures &def)
	{
		return m_ndef.set(name, def);
	}

protected:
	void warn_unimplemented(const std::string &function) const
	{
		dout << "Warning: Implement " << function << std::endl;
	}

	std::ostream &dout;
	NodeDefManager m_ndef;
	std::vector<ModSpec> m_modspec;
};

class MockMap : public Map
{
public:
	MockMap(std::ostream &dout, IGameDef *gamedef) : Map(dout, gamedef) {}

	virtual s32 mapType() const { return MAPTYPE_MOCK; }

	// For debug printing. Prints "Map: ", "ServerMap: " or "ClientMap: "
	virtual void PrintInfo(std::ostream &out) { out << "MockMap: "; }

	virtual MapSector *emergeSector(v2s16 p);
	virtual MapBlock *emergeBlock(v3s16 p, bool create_blank = true);
	void setMockNode(v3s16 p, content_t c);
};

class MockEnvironment : public Environment
{
public:
	MockEnvironment(std::ostream &dout) :
			Environment(new MockGameDef(dout)), m_map(dout, m_gamedef)
	{
	}

	virtual void step(f32 dtime) {}
	virtual Map &getMap() { return m_map; }

	virtual void getSelectedActiveObjects(const core::line3d<f32> &shootline_on_map,
			std::vector<PointedThing> &objects)
	{
	}

	MockMap &getMockMap() { return m_map; }

	MockGameDef *getMockGameDef() { return dynamic_cast<MockGameDef *>(m_gamedef); }

protected:
	MockMap m_map;
};
