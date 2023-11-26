#pragma once
#include "util/Config.h"
#include "json/json.h"


// See example.toml for notes about these values.


class MineConfig : public rwr::Config
{
protected:
	void assignOptions(std::shared_ptr<cpptoml::table> group);

public:
	std::string WORLD_NAME;
	int RANDOM_SEED;
	int SERVER_PORT;

	std::string LOG_LEVEL;

	MineConfig();

	void validateConfig();

	void loadFromJson(Json::Value &root);
	void resetConfig();
};

extern MineConfig gMineConfig;

