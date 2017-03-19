/*
Minetest
Copyright (C) 2010-2016 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2014-2016 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "remoteplayer.h"
#include <json/json.h>
#include "content_sao.h"
#include "filesys.h"
#include "gamedef.h"
#include "porting.h"  // strlcpy
#include "server.h"
#include "settings.h"

/*
	RemotePlayer
*/
// static config cache for remoteplayer
bool RemotePlayer::m_setting_cache_loaded = false;
float RemotePlayer::m_setting_chat_message_limit_per_10sec = 0.0f;
u16 RemotePlayer::m_setting_chat_message_limit_trigger_kick = 0;

RemotePlayer::RemotePlayer(const char *name, IItemDefManager *idef):
	Player(name, idef),
	protocol_version(0),
	m_sao(NULL),
	m_dirty(false),
	m_last_chat_message_sent(time(NULL)),
	m_chat_message_allowance(5.0f),
	m_message_rate_overhead(0),
	hud_hotbar_image(""),
	hud_hotbar_selected_image("")
{
	if (!RemotePlayer::m_setting_cache_loaded) {
		RemotePlayer::m_setting_chat_message_limit_per_10sec =
			g_settings->getFloat("chat_message_limit_per_10sec");
		RemotePlayer::m_setting_chat_message_limit_trigger_kick =
			g_settings->getU16("chat_message_limit_trigger_kick");
		RemotePlayer::m_setting_cache_loaded = true;
	}
	movement_acceleration_default   = g_settings->getFloat("movement_acceleration_default")   * BS;
	movement_acceleration_air       = g_settings->getFloat("movement_acceleration_air")       * BS;
	movement_acceleration_fast      = g_settings->getFloat("movement_acceleration_fast")      * BS;
	movement_speed_walk             = g_settings->getFloat("movement_speed_walk")             * BS;
	movement_speed_crouch           = g_settings->getFloat("movement_speed_crouch")           * BS;
	movement_speed_fast             = g_settings->getFloat("movement_speed_fast")             * BS;
	movement_speed_climb            = g_settings->getFloat("movement_speed_climb")            * BS;
	movement_speed_jump             = g_settings->getFloat("movement_speed_jump")             * BS;
	movement_liquid_fluidity        = g_settings->getFloat("movement_liquid_fluidity")        * BS;
	movement_liquid_fluidity_smooth = g_settings->getFloat("movement_liquid_fluidity_smooth") * BS;
	movement_liquid_sink            = g_settings->getFloat("movement_liquid_sink")            * BS;
	movement_gravity                = g_settings->getFloat("movement_gravity")                * BS;
}

void RemotePlayer::save(std::string savedir, IGameDef *gamedef)
{
	/*
	 * We have to open all possible player files in the players directory
	 * and check their player names because some file systems are not
	 * case-sensitive and player names are case-sensitive.
	 */

	// A player to deserialize files into to check their names
	RemotePlayer testplayer("", gamedef->idef());

	savedir += DIR_DELIM;
	std::string path = savedir + m_name;
	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES; i++) {
		if (!fs::PathExists(path)) {
			// Open file and serialize
			std::ostringstream ss(std::ios_base::binary);
			serialize(ss);
			if (!fs::safeWriteToFile(path, ss.str())) {
				infostream << "Failed to write " << path << std::endl;
			}
			setModified(false);
			return;
		}
		// Open file and deserialize
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good()) {
			infostream << "Failed to open " << path << std::endl;
			return;
		}
		testplayer.deSerialize(is, path, NULL);
		is.close();
		if (strcmp(testplayer.getName(), m_name) == 0) {
			// Open file and serialize
			std::ostringstream ss(std::ios_base::binary);
			serialize(ss);
			if (!fs::safeWriteToFile(path, ss.str())) {
				infostream << "Failed to write " << path << std::endl;
			}
			setModified(false);
			return;
		}
		path = savedir + m_name + itos(i);
	}

	infostream << "Didn't find free file for player " << m_name << std::endl;
}

void RemotePlayer::serializeExtraAttributes(std::string &output)
{
	assert(m_sao);
	Json::Value json_root;
	const PlayerAttributes &attrs = m_sao->getExtendedAttributes();
	for (PlayerAttributes::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
		json_root[(*it).first] = (*it).second;
	}

	Json::FastWriter writer;
	output = writer.write(json_root);
	m_sao->setExtendedAttributeModified(false);
}


void RemotePlayer::deSerialize(std::istream &is, const std::string &playername,
		PlayerSAO *sao)
{
	Settings args;

	if (!args.parseConfigLines(is, "PlayerArgsEnd")) {
		throw SerializationError("PlayerArgsEnd of player " + playername + " not found!");
	}

	m_dirty = true;
	//args.getS32("version"); // Version field value not used
	const std::string &name = args.get("name");
	strlcpy(m_name, name.c_str(), PLAYERNAME_SIZE);

	if (sao) {
		try {
			sao->setHPRaw(args.getS32("hp"));
		} catch(SettingNotFoundException &e) {
			sao->setHPRaw(PLAYER_MAX_HP);
		}

		try {
			sao->setBasePosition(args.getV3F("position"));
		} catch (SettingNotFoundException &e) {}

		try {
			sao->setPitch(args.getFloat("pitch"));
		} catch (SettingNotFoundException &e) {}
		try {
			sao->setYaw(args.getFloat("yaw"));
		} catch (SettingNotFoundException &e) {}

		try {
			sao->setBreath(args.getS32("breath"), false);
		} catch (SettingNotFoundException &e) {}

		try {
			const std::string &extended_attributes = args.get("extended_attributes");
			Json::Reader reader;
			Json::Value attr_root;
			reader.parse(extended_attributes, attr_root);

			const Json::Value::Members attr_list = attr_root.getMemberNames();
			for (Json::Value::Members::const_iterator it = attr_list.begin();
					it != attr_list.end(); ++it) {
				Json::Value attr_value = attr_root[*it];
				sao->setExtendedAttribute(*it, attr_value.asString());
			}
		} catch (SettingNotFoundException &e) {}
	}

	inventory.deSerialize(is);

	if (inventory.getList("craftpreview") == NULL) {
		// Convert players without craftpreview
		inventory.addList("craftpreview", 1);

		bool craftresult_is_preview = true;
		if(args.exists("craftresult_is_preview"))
			craftresult_is_preview = args.getBool("craftresult_is_preview");
		if(craftresult_is_preview)
		{
			// Clear craftresult
			inventory.getList("craftresult")->changeItem(0, ItemStack());
		}
	}
}

void RemotePlayer::serialize(std::ostream &os)
{
	// Utilize a Settings object for storing values
	Settings args;
	args.setS32("version", 1);
	args.set("name", m_name);

	// This should not happen
	assert(m_sao);
	args.setS32("hp", m_sao->getHP());
	args.setV3F("position", m_sao->getBasePosition());
	args.setFloat("pitch", m_sao->getPitch());
	args.setFloat("yaw", m_sao->getYaw());
	args.setS32("breath", m_sao->getBreath());

	std::string extended_attrs = "";
	serializeExtraAttributes(extended_attrs);
	args.set("extended_attributes", extended_attrs);

	args.writeLines(os);

	os<<"PlayerArgsEnd\n";

	inventory.serialize(os);
}

const RemotePlayerChatResult RemotePlayer::canSendChatMessage()
{
	// Rate limit messages
	u32 now = time(NULL);
	float time_passed = now - m_last_chat_message_sent;
	m_last_chat_message_sent = now;

	// If this feature is disabled
	if (m_setting_chat_message_limit_per_10sec <= 0.0) {
		return RPLAYER_CHATRESULT_OK;
	}

	m_chat_message_allowance += time_passed * (m_setting_chat_message_limit_per_10sec / 8.0f);
	if (m_chat_message_allowance > m_setting_chat_message_limit_per_10sec) {
		m_chat_message_allowance = m_setting_chat_message_limit_per_10sec;
	}

	if (m_chat_message_allowance < 1.0f) {
		infostream << "Player " << m_name
				<< " chat limited due to excessive message amount." << std::endl;

		// Kick player if flooding is too intensive
		m_message_rate_overhead++;
		if (m_message_rate_overhead > RemotePlayer::m_setting_chat_message_limit_trigger_kick) {
			return RPLAYER_CHATRESULT_KICK;
		}

		return RPLAYER_CHATRESULT_FLOODING;
	}

	// Reinit message overhead
	if (m_message_rate_overhead > 0) {
		m_message_rate_overhead = 0;
	}

	m_chat_message_allowance -= 1.0f;
	return RPLAYER_CHATRESULT_OK;
}
