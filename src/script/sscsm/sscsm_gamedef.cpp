#include "sscsm_gamedef.h"
#include "itemdef.h"
#include "nodedef.h"
#include "content/mods.h"

SSCSMGameDef::SSCSMGameDef(FILE *from_controller, FILE *to_controller):
	m_itemdef(createItemDefManager()),
	m_nodedef(createNodeDefManager()),
	m_from_controller(from_controller),
	m_to_controller(to_controller)
{
}

SSCSMGameDef::~SSCSMGameDef()
{
	fclose(m_to_controller);
	fclose(m_from_controller);
	delete m_nodedef;
	delete m_itemdef;
}

IItemDefManager *SSCSMGameDef::getItemDefManager()
{
	return m_itemdef;
}

const NodeDefManager *SSCSMGameDef::getNodeDefManager()
{
	return m_nodedef;
}

ICraftDefManager *SSCSMGameDef::getCraftDefManager()
{
	return nullptr;
}

IWritableItemDefManager *SSCSMGameDef::getWritableItemDefManager()
{
	return m_itemdef;
}

NodeDefManager *SSCSMGameDef::getWritableNodeDefManager()
{
	return m_nodedef;
}

u16 SSCSMGameDef::allocateUnknownNodeId(const std::string &name)
{
	return CONTENT_IGNORE;
}

const std::vector<ModSpec> &SSCSMGameDef::getMods() const
{
	static const std::vector<ModSpec> mods;
	return mods;
}

const ModSpec *SSCSMGameDef::getModSpec(const std::string &modname) const
{
	return nullptr;
}

ModStorageDatabase *SSCSMGameDef::getModStorageDatabase()
{
	return nullptr;
}

bool SSCSMGameDef::joinModChannel(const std::string &channel)
{
	return false;
}

bool SSCSMGameDef::leaveModChannel(const std::string &channel)
{
	return false;
}

bool SSCSMGameDef::sendModChannelMessage(const std::string &channel,
			const std::string &message)
{
	return false;
}

ModChannel *SSCSMGameDef::getModChannel(const std::string &channel)
{
	return nullptr;
}
