#include "csm_gamedef.h"
#include "itemdef.h"
#include "nodedef.h"
#include "content/mods.h"

CSMGameDef::CSMGameDef(FILE *from_controller, FILE *to_controller):
	m_itemdef(createItemDefManager()),
	m_nodedef(createNodeDefManager()),
	m_from_controller(from_controller),
	m_to_controller(to_controller)
{
}

CSMGameDef::~CSMGameDef()
{
	fclose(m_to_controller);
	fclose(m_from_controller);
	delete m_nodedef;
	delete m_itemdef;
}

IItemDefManager *CSMGameDef::getItemDefManager()
{
	return m_itemdef;
}

const NodeDefManager *CSMGameDef::getNodeDefManager()
{
	return m_nodedef;
}

ICraftDefManager *CSMGameDef::getCraftDefManager()
{
	return nullptr;
}

IWritableItemDefManager *CSMGameDef::getWritableItemDefManager()
{
	return m_itemdef;
}

NodeDefManager *CSMGameDef::getWritableNodeDefManager()
{
	return m_nodedef;
}

u16 CSMGameDef::allocateUnknownNodeId(const std::string &name)
{
	return CONTENT_IGNORE;
}

const std::vector<ModSpec> &CSMGameDef::getMods() const
{
	static const std::vector<ModSpec> mods;
	return mods;
}

const ModSpec *CSMGameDef::getModSpec(const std::string &modname) const
{
	return nullptr;
}

ModStorageDatabase *CSMGameDef::getModStorageDatabase()
{
	return nullptr;
}

bool CSMGameDef::joinModChannel(const std::string &channel)
{
	return false;
}

bool CSMGameDef::leaveModChannel(const std::string &channel)
{
	return false;
}

bool CSMGameDef::sendModChannelMessage(const std::string &channel,
			const std::string &message)
{
	return false;
}

ModChannel *CSMGameDef::getModChannel(const std::string &channel)
{
	return nullptr;
}
