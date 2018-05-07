#include "client\discord.h"
#include <ctime>
#include "discord_rpc.h"
#include <string>

std::unique_ptr<Discord> Discord::s_pDiscord;

Discord::Discord()
{	
	m_data.startTimestamp = time(0);
}

Discord::~Discord()
{
	Discord_Shutdown();
}

void Discord::create()
{
	s_pDiscord = std::make_unique<Discord>();
}

void Discord::init()
{
	DiscordEventHandlers handlers = {};
	/*
		Handlers list :
		ready, disconnected, errored, joinGame, spectateGame, joinRequest
	*/
	handlers.ready = Discord::handleDiscordReady;
	handlers.errored = Discord::handleDiscordError;
	Discord_Shutdown();
	Discord_Initialize(s_clientId, &handlers, 1, NULL);
}

Discord *Discord::getInstance()
{
	return s_pDiscord.get();
}

void Discord::setState(const std::string &state)
{
	m_data.state = state;
}

void Discord::setDetails(const std::string &details)
{
	m_data.details = details;
}

void Discord::updatePresence()
{
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));

	discordPresence.state = m_data.state.c_str();
	discordPresence.details = m_data.details.c_str();
	discordPresence.startTimestamp = m_data.startTimestamp;
	discordPresence.largeImageKey = m_data.largeImageKey.c_str();
	discordPresence.smallImageKey = m_data.smallImageKey.c_str();
	discordPresence.partyId = m_data.partyId.c_str();
	discordPresence.partySize = m_data.partySize;
	discordPresence.partyMax = m_data.partyMax;
	// discordPresence.matchSecret = "4b2fdce12f639de8bfa7e3591b71a0d679d7c93f";
	// discordPresence.spectateSecret = "e7eb30d2ee025ed05c71ea495f770b76454ee4e0";
	// discordPresence.instance = 1;
	
	Discord_UpdatePresence(&discordPresence);
}

void Discord::handleDiscordReady()
{
	std::cout << "Discord is ready" << std::endl;
}

void Discord::handleDiscordError(int errcode, const char* message)
{
	std::cout << "Error discord : " << std::to_string(errcode) << " " << message << std::endl;
}
