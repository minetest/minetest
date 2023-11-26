#include "MineConfig.h"
#include "util/Logging.h"

#include <cstdlib>


MineConfig gMineConfig;
MineConfig gBackupConfig;

MineConfig::MineConfig()
{
	WORLD_NAME = "";
	RANDOM_SEED = 0;
	SERVER_PORT = 6000;
	LOG_LEVEL = "Info";
}

const std::vector<const char *> kMineOptionNames = {"INCLUDE", "RANDOM_SEED", "SERVER_PORT", "WORLD_NAME" };

void MineConfig::assignOptions(std::shared_ptr<cpptoml::table> group)
{
	std::string includeOtherCfg;
	assignString(group, "INCLUDE", includeOtherCfg);
	if (includeOtherCfg != "") {
		load(includeOtherCfg);
		// Let it continue processing so you can overwrite values in the included cfg
	}

	std::string strayName;
	CHECK(checkStrays(group, kMineOptionNames, strayName))
			<< "Unknown config option: " << strayName;

	assignString(group, "WORLD_NAME", WORLD_NAME);
	assignString(group, "LOG_LEVEL", LOG_LEVEL);
	assignPositiveInt(group, "RANDOM_SEED", RANDOM_SEED);
	assignPositiveInt(group, "SERVER_PORT", SERVER_PORT);


	gBackupConfig = *this;
}

void MineConfig::validateConfig()
{
	
}

void MineConfig::loadFromJson(Json::Value &root)
{
	if (root.isNull())
		return;

	RANDOM_SEED = root.get("RANDOM_SEED", RANDOM_SEED).asInt();

	validateConfig();
}

void MineConfig::resetConfig()
{
	*this = gBackupConfig;
}

