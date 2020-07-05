#include <iostream>

#include "environment.h"
#include "gamedef.h"
#include "map.h"

/*
        MapEditEvent
*/

#define MAPTYPE_MOCK 3

#include UNIMPLEMENTED(retval) { warn_unimplemented(__FUNCTION__); return retval; }

class MockGameDef: public IGameDef
{
public:
	MockGameDef(std::ostream &dout);

	virtual IItemDefManager* getItemDefManager() UNIMPLEMENTED(NULL)

	virtual const NodeDefManager* getNodeDefManager()
	{
		return m_ndef;
	}

	virtual ICraftDefManager* getCraftDefManager() UNIMPLEMENTED(NULL)
	virtual u16 allocateUnknownNodeId(const std::string &name)
		UNIMPLEMENTED(0)
	virtual const std::vector<ModSpec> &getMods() const
		UNIMPLEMENTED(std::vector<ModSpec>())
	virtual const ModSpec* getModSpec(const std::string &modname) const
		UNIMPLEMENTED(NULL)
	virtual std::string getModStoragePath() const
		UNIMPLEMENTED("")
	virtual bool registerModStorage(ModMetadata *storage)
		UNIMPLEMENTED(false)
	virtual void unregisterModStorage(const std::string &name)
		UNIMPLEMENTED(;)
	virtual bool joinModChannel(const std::string &channel)
		UNIMPLEMENTED(false)
	virtual bool leaveModChannel(const std::string &channel)
		UNIMPLEMENTED(false)
	virtual bool sendModChannelMessage(const std::string &channel,
		const std::string &message) UNIMPLEMENTED(false)
	virtual ModChannel *getModChannel(const std::string &channel)
		UNIMPLEMENTED(NULL)

protected:
        void warn_unimplemented(string function)
	{
		dout << "Warning: Implement " << function << std::endl;
	}

	std::ostream &dout;
	NodeDefManager m_ndef;
};

class MockMap: public Map
{
public:
	MockMap(std::ostream &dout, IGameDef *gamedef): Map(dout, gamedef) {}

	virtual s32 mapType() const
        {
                return MAPTYPE_MOCK;
        }

	// For debug printing. Prints "Map: ", "ServerMap: " or "ClientMap: "
        virtual void PrintInfo(std::ostream &out)
	{
		out << "MockMap: ";
	}

	bool CreateSector(v2s16 p2d, string definition);
};

class MockEnvironment: public Environment
{
public:
	MockEnvironment(std::ostream &dout):
		Environment(new MockMap(dout)),
		m_map(dout, m_gamedef) {}
	virtual void step(f32 dtime) {}
	virtual Map &getMap() { return m_map; }
	virtual void getSelectedActiveObjects(const core::line3d<f32> &shootline_on_map,
                        std::vector<PointedThing> &objects) {}
	MockMap &getMockMap() { return m_map; }
protected:
	MockMap m_map;
	
};
