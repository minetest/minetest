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
#include "version.h"
#include "settings.h"
#include "serverlist.h"
#include "filesys.h"
#include "log.h"
#include "network/networkprotocol.h"
#include <json/json.h>
#include "convert_json.h"
#include "httpfetch.h"

namespace ServerList
{
#if USE_CURL
void sendAnnounce(AnnounceAction action,
		const u16 port,
		const std::vector<std::string> &clients_names,
		const double uptime,
		const u32 game_time,
		const float lag,
		const std::string &gameid,
		const std::string &mg_name,
		const std::vector<ModSpec> &mods,
		bool dedicated)
{
	static const char *aa_names[] = {"start", "update", "delete"};
	Json::Value server;
	server["action"] = aa_names[action];
	server["port"] = port;
	if (g_settings->exists("server_address")) {
		server["address"] = g_settings->get("server_address");
	}
	if (action != AA_DELETE) {
		bool strict_checking = g_settings->getBool("strict_protocol_version_checking");
		server["name"]         = g_settings->get("server_name");
		server["description"]  = g_settings->get("server_description");
		server["version"]      = g_version_string;
		server["proto_min"]    = strict_checking ? LATEST_PROTOCOL_VERSION : SERVER_PROTOCOL_VERSION_MIN;
		server["proto_max"]    = strict_checking ? LATEST_PROTOCOL_VERSION : SERVER_PROTOCOL_VERSION_MAX;
		server["url"]          = g_settings->get("server_url");
		server["creative"]     = g_settings->getBool("creative_mode");
		server["damage"]       = g_settings->getBool("enable_damage");
		server["password"]     = g_settings->getBool("disallow_empty_password");
		server["pvp"]          = g_settings->getBool("enable_pvp");
		server["uptime"]       = (int) uptime;
		server["game_time"]    = game_time;
		server["clients"]      = (int) clients_names.size();
		server["clients_max"]  = g_settings->getU16("max_users");
		server["clients_list"] = Json::Value(Json::arrayValue);
		for (const std::string &clients_name : clients_names) {
			server["clients_list"].append(clients_name);
		}
		if (!gameid.empty())
			server["gameid"] = gameid;
	}

	if (action == AA_START) {
		server["dedicated"]         = dedicated;
		server["rollback"]          = g_settings->getBool("enable_rollback_recording");
		server["mapgen"]            = mg_name;
		server["privs"]             = g_settings->get("default_privs");
		server["can_see_far_names"] = g_settings->getS16("player_transfer_distance") <= 0;
		server["mods"]              = Json::Value(Json::arrayValue);
		for (const ModSpec &mod : mods) {
			server["mods"].append(mod.name);
		}
	} else if (action == AA_UPDATE) {
		if (lag)
			server["lag"] = lag;
	}

	if (action == AA_START) {
		actionstream << "Announcing " << aa_names[action] << " to " <<
			g_settings->get("serverlist_url") << std::endl;
	} else {
		infostream << "Announcing " << aa_names[action] << " to " <<
			g_settings->get("serverlist_url") << std::endl;
	}

	HTTPFetchRequest fetch_request;
	fetch_request.caller = HTTPFETCH_PRINT_ERR;
	fetch_request.url = g_settings->get("serverlist_url") + std::string("/announce");
	fetch_request.method = HTTP_POST;
	fetch_request.fields["json"] = fastWriteJson(server);
	fetch_request.multipart = true;
	httpfetch_async(fetch_request);
}
#endif

} // namespace ServerList
