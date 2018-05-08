#pragma once
#include <iostream>
#include <memory>

struct DataRichPresence {
	std::string state = "Playing in Minefrance Server";
	std::string details = "In the Capital";
	uint64_t startTimestamp;
	std::string largeImageKey = "default";
	std::string smallImageKey = "small_default";
	std::string partyId = "";
	uint32_t partySize = 30;
	uint32_t partyMax = 200;
};

class Discord
{
public:
	Discord();
	~Discord();

	static void create();
	void init();

	static Discord* getInstance();

	void setState(const std::string &state);
	void setDetails(const std::string &details);
	void updatePresence();

	static void handleDiscordError(int errcode, const char* message);
	static void handleDiscordReady();

private:
	static const std::string s_applicationId;
	
	DataRichPresence m_data;

	static std::unique_ptr<Discord> s_pDiscord;
};

