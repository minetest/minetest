#pragma once

#include "gamedef.h"
#include "csm_message.h"

class IWritableItemDefManager;

class CSMGameDef : public IGameDef
{
public:
	CSMGameDef(FILE *from_controller, FILE *to_controller);
	~CSMGameDef();

	CSMRecvMsg recvEx()
	{
		return csm_recv_msg_ex(m_from_controller);
	}
	void sendEx(CSMMsgType type, size_t size = 0, const void *data = nullptr)
	{
		csm_send_msg_ex(m_to_controller, type, size, data);
	}

	IItemDefManager *getItemDefManager() override;
	const NodeDefManager *getNodeDefManager() override;
	ICraftDefManager *getCraftDefManager() override;
	IWritableItemDefManager *getWritableItemDefManager();
	NodeDefManager *getWritableNodeDefManager();

	u16 allocateUnknownNodeId(const std::string &name) override;

	const std::vector<ModSpec> &getMods() const override;
	const ModSpec *getModSpec(const std::string &modname) const override;
	ModStorageDatabase *getModStorageDatabase() override;

	bool joinModChannel(const std::string &channel) override;
	bool leaveModChannel(const std::string &channel) override;
	bool sendModChannelMessage(const std::string &channel,
			const std::string &message) override;
	ModChannel *getModChannel(const std::string &channel) override;

private:
	IWritableItemDefManager *m_itemdef;
	NodeDefManager *m_nodedef;
	FILE *m_from_controller;
	FILE *m_to_controller;
};
