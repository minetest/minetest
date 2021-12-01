/*
Minetest
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include <cassert>
#include "convert_json.h"
#include "database-files.h"
#include "remoteplayer.h"
#include "settings.h"
#include "porting.h"
#include "filesys.h"
#include "server/player_sao.h"
#include "util/string.h"

// !!! WARNING !!!
// This backend is intended to be used on Minetest 0.4.16 only for the transition backend
// for player files

PlayerDatabaseFiles::PlayerDatabaseFiles(const std::string &savedir) : m_savedir(savedir)
{
	fs::CreateDir(m_savedir);
}

void PlayerDatabaseFiles::deSerialize(RemotePlayer *p, std::istream &is,
	 const std::string &playername, PlayerSAO *sao)
{
	Settings args("PlayerArgsEnd");

	if (!args.parseConfigLines(is)) {
		throw SerializationError("PlayerArgsEnd of player " + playername + " not found!");
	}

	p->m_dirty = true;
	//args.getS32("version"); // Version field value not used
	const std::string &name = args.get("name");
	strlcpy(p->m_name, name.c_str(), PLAYERNAME_SIZE);

	if (sao) {
		try {
			sao->setHPRaw(args.getU16("hp"));
		} catch(SettingNotFoundException &e) {
			sao->setHPRaw(PLAYER_MAX_HP_DEFAULT);
		}

		try {
			sao->setBasePosition(args.getV3F("position"));
		} catch (SettingNotFoundException &e) {}

		try {
			sao->setLookPitch(args.getFloat("pitch"));
		} catch (SettingNotFoundException &e) {}
		try {
			sao->setPlayerYaw(args.getFloat("yaw"));
		} catch (SettingNotFoundException &e) {}

		try {
			sao->setBreath(args.getU16("breath"), false);
		} catch (SettingNotFoundException &e) {}

		try {
			const std::string &extended_attributes = args.get("extended_attributes");
			std::istringstream iss(extended_attributes);
			Json::CharReaderBuilder builder;
			builder.settings_["collectComments"] = false;
			std::string errs;

			Json::Value attr_root;
			Json::parseFromStream(builder, iss, &attr_root, &errs);

			const Json::Value::Members attr_list = attr_root.getMemberNames();
			for (const auto &it : attr_list) {
				Json::Value attr_value = attr_root[it];
				sao->getMeta().setString(it, attr_value.asString());
			}
			sao->getMeta().setModified(false);
		} catch (SettingNotFoundException &e) {}
	}

	try {
		p->inventory.deSerialize(is);
	} catch (SerializationError &e) {
		errorstream << "Failed to deserialize player inventory. player_name="
			<< name << " " << e.what() << std::endl;
	}

	if (!p->inventory.getList("craftpreview") && p->inventory.getList("craftresult")) {
		// Convert players without craftpreview
		p->inventory.addList("craftpreview", 1);

		bool craftresult_is_preview = true;
		if(args.exists("craftresult_is_preview"))
			craftresult_is_preview = args.getBool("craftresult_is_preview");
		if(craftresult_is_preview)
		{
			// Clear craftresult
			p->inventory.getList("craftresult")->changeItem(0, ItemStack());
		}
	}
}

void PlayerDatabaseFiles::serialize(RemotePlayer *p, std::ostream &os)
{
	// Utilize a Settings object for storing values
	Settings args("PlayerArgsEnd");
	args.setS32("version", 1);
	args.set("name", p->m_name);

	PlayerSAO *sao = p->getPlayerSAO();
	// This should not happen
	sanity_check(sao);
	args.setU16("hp", sao->getHP());
	args.setV3F("position", sao->getBasePosition());
	args.setFloat("pitch", sao->getLookPitch());
	args.setFloat("yaw", sao->getRotation().Y);
	args.setU16("breath", sao->getBreath());

	std::string extended_attrs;
	{
		// serializeExtraAttributes
		Json::Value json_root;

		const StringMap &attrs = sao->getMeta().getStrings();
		for (const auto &attr : attrs) {
			json_root[attr.first] = attr.second;
		}

		extended_attrs = fastWriteJson(json_root);
	}
	args.set("extended_attributes", extended_attrs);

	args.writeLines(os);

	p->inventory.serialize(os);
}

void PlayerDatabaseFiles::savePlayer(RemotePlayer *player)
{
	fs::CreateDir(m_savedir);

	std::string savedir = m_savedir + DIR_DELIM;
	std::string path = savedir + player->getName();
	bool path_found = false;
	RemotePlayer testplayer("", NULL);

	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES && !path_found; i++) {
		if (!fs::PathExists(path)) {
			path_found = true;
			continue;
		}

		// Open and deserialize file to check player name
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good()) {
			errorstream << "Failed to open " << path << std::endl;
			return;
		}

		deSerialize(&testplayer, is, path, NULL);
		is.close();
		if (strcmp(testplayer.getName(), player->getName()) == 0) {
			path_found = true;
			continue;
		}

		path = savedir + player->getName() + itos(i);
	}

	if (!path_found) {
		errorstream << "Didn't find free file for player " << player->getName()
				<< std::endl;
		return;
	}

	// Open and serialize file
	std::ostringstream ss(std::ios_base::binary);
	serialize(player, ss);
	if (!fs::safeWriteToFile(path, ss.str())) {
		infostream << "Failed to write " << path << std::endl;
	}

	player->onSuccessfulSave();
}

bool PlayerDatabaseFiles::removePlayer(const std::string &name)
{
	std::string players_path = m_savedir + DIR_DELIM;
	std::string path = players_path + name;

	RemotePlayer temp_player("", NULL);
	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES; i++) {
		// Open file and deserialize
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good())
			continue;

		deSerialize(&temp_player, is, path, NULL);
		is.close();

		if (temp_player.getName() == name) {
			fs::DeleteSingleFileOrEmptyDirectory(path);
			return true;
		}

		path = players_path + name + itos(i);
	}

	return false;
}

bool PlayerDatabaseFiles::loadPlayer(RemotePlayer *player, PlayerSAO *sao)
{
	std::string players_path = m_savedir + DIR_DELIM;
	std::string path = players_path + player->getName();

	const std::string player_to_load = player->getName();
	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES; i++) {
		// Open file and deserialize
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good())
			continue;

		deSerialize(player, is, path, sao);
		is.close();

		if (player->getName() == player_to_load)
			return true;

		path = players_path + player_to_load + itos(i);
	}

	infostream << "Player file for player " << player_to_load << " not found" << std::endl;
	return false;
}

void PlayerDatabaseFiles::listPlayers(std::vector<std::string> &res)
{
	std::vector<fs::DirListNode> files = fs::GetDirListing(m_savedir);
	// list files into players directory
	for (std::vector<fs::DirListNode>::const_iterator it = files.begin(); it !=
		files.end(); ++it) {
		// Ignore directories
		if (it->dir)
			continue;

		const std::string &filename = it->name;
		std::string full_path = m_savedir + DIR_DELIM + filename;
		std::ifstream is(full_path.c_str(), std::ios_base::binary);
		if (!is.good())
			continue;

		RemotePlayer player(filename.c_str(), NULL);
		// Null env & dummy peer_id
		PlayerSAO playerSAO(NULL, &player, 15789, false);

		deSerialize(&player, is, "", &playerSAO);
		is.close();

		res.emplace_back(player.getName());
	}
}

AuthDatabaseFiles::AuthDatabaseFiles(const std::string &savedir) : m_savedir(savedir)
{
	readAuthFile();
}

bool AuthDatabaseFiles::getAuth(const std::string &name, AuthEntry &res)
{
	const auto res_i = m_auth_list.find(name);
	if (res_i == m_auth_list.end()) {
		return false;
	}
	res = res_i->second;
	return true;
}

bool AuthDatabaseFiles::saveAuth(const AuthEntry &authEntry)
{
	m_auth_list[authEntry.name] = authEntry;

	// save entire file
	return writeAuthFile();
}

bool AuthDatabaseFiles::createAuth(AuthEntry &authEntry)
{
	m_auth_list[authEntry.name] = authEntry;

	// save entire file
	return writeAuthFile();
}

bool AuthDatabaseFiles::deleteAuth(const std::string &name)
{
	if (!m_auth_list.erase(name)) {
		// did not delete anything -> hadn't existed
		return false;
	}
	return writeAuthFile();
}

void AuthDatabaseFiles::listNames(std::vector<std::string> &res)
{
	res.clear();
	res.reserve(m_auth_list.size());
	for (const auto &res_pair : m_auth_list) {
		res.push_back(res_pair.first);
	}
}

void AuthDatabaseFiles::reload()
{
	readAuthFile();
}

bool AuthDatabaseFiles::readAuthFile()
{
	std::string path = m_savedir + DIR_DELIM + "auth.txt";
	std::ifstream file(path, std::ios::binary);
	if (!file.good()) {
		return false;
	}
	m_auth_list.clear();
	while (file.good()) {
		std::string line;
		std::getline(file, line);
		std::vector<std::string> parts = str_split(line, ':');
		if (parts.size() < 3) // also: empty line at end
			continue;
		const std::string &name = parts[0];
		const std::string &password = parts[1];
		std::vector<std::string> privileges = str_split(parts[2], ',');
		s64 last_login = parts.size() > 3 ? atol(parts[3].c_str()) : 0;

		m_auth_list[name] = {
				1,
				name,
				password,
				privileges,
				last_login,
		};
	}
	return true;
}

bool AuthDatabaseFiles::writeAuthFile()
{
	std::string path = m_savedir + DIR_DELIM + "auth.txt";
	std::ostringstream output(std::ios_base::binary);
	for (const auto &auth_i : m_auth_list) {
		const AuthEntry &authEntry = auth_i.second;
		output << authEntry.name << ":" << authEntry.password << ":";
		output << str_join(authEntry.privileges, ",");
		output << ":" << authEntry.last_login;
		output << std::endl;
	}
	if (!fs::safeWriteToFile(path, output.str())) {
		infostream << "Failed to write " << path << std::endl;
		return false;
	}
	return true;
}

ModMetadataDatabaseFiles::ModMetadataDatabaseFiles(const std::string &savedir):
	m_storage_dir(savedir + DIR_DELIM + "mod_storage")
{
}

bool ModMetadataDatabaseFiles::getModEntries(const std::string &modname, StringMap *storage)
{
	Json::Value *meta = getOrCreateJson(modname);
	if (!meta)
		return false;

	const Json::Value::Members attr_list = meta->getMemberNames();
	for (const auto &it : attr_list) {
		Json::Value attr_value = (*meta)[it];
		(*storage)[it] = attr_value.asString();
	}

	return true;
}

bool ModMetadataDatabaseFiles::setModEntry(const std::string &modname,
	const std::string &key, const std::string &value)
{
	Json::Value *meta = getOrCreateJson(modname);
	if (!meta)
		return false;

	(*meta)[key] = Json::Value(value);
	m_modified.insert(modname);

	return true;
}

bool ModMetadataDatabaseFiles::removeModEntry(const std::string &modname,
		const std::string &key)
{
	Json::Value *meta = getOrCreateJson(modname);
	if (!meta)
		return false;

	Json::Value removed;
	if (meta->removeMember(key, &removed)) {
		m_modified.insert(modname);
		return true;
	}
	return false;
}

void ModMetadataDatabaseFiles::beginSave()
{
}

void ModMetadataDatabaseFiles::endSave()
{
	if (!fs::CreateAllDirs(m_storage_dir)) {
		errorstream << "ModMetadataDatabaseFiles: Unable to save. '" << m_storage_dir
				<< "' tree cannot be created." << std::endl;
		return;
	}

	for (auto it = m_modified.begin(); it != m_modified.end();) {
		const std::string &modname = *it;

		if (!fs::PathExists(m_storage_dir)) {
			if (!fs::CreateAllDirs(m_storage_dir)) {
				errorstream << "ModMetadataDatabaseFiles[" << modname
						<< "]: Unable to save. '" << m_storage_dir
						<< "' tree cannot be created." << std::endl;
				++it;
				continue;
			}
		} else if (!fs::IsDir(m_storage_dir)) {
			errorstream << "ModMetadataDatabaseFiles[" << modname << "]: Unable to save. '"
					<< m_storage_dir << "' is not a directory." << std::endl;
			++it;
			continue;
		}

		const Json::Value &json = m_mod_meta[modname];

		if (!fs::safeWriteToFile(m_storage_dir + DIR_DELIM + modname, fastWriteJson(json))) {
			errorstream << "ModMetadataDatabaseFiles[" << modname
					<< "]: failed write file." << std::endl;
			++it;
			continue;
		}

		it = m_modified.erase(it);
	}
}

void ModMetadataDatabaseFiles::listMods(std::vector<std::string> *res)
{
	// List in-memory metadata first.
	for (const auto &pair : m_mod_meta) {
		res->push_back(pair.first);
	}

	// List other metadata present in the filesystem.
	for (const auto &entry : fs::GetDirListing(m_storage_dir)) {
		if (!entry.dir && m_mod_meta.count(entry.name) == 0)
			res->push_back(entry.name);
	}
}

Json::Value *ModMetadataDatabaseFiles::getOrCreateJson(const std::string &modname)
{
	auto found = m_mod_meta.find(modname);
	if (found == m_mod_meta.end()) {
		fs::CreateAllDirs(m_storage_dir);

		Json::Value meta(Json::objectValue);

		std::string path = m_storage_dir + DIR_DELIM + modname;
		if (fs::PathExists(path)) {
			std::ifstream is(path.c_str(), std::ios_base::binary);

			Json::CharReaderBuilder builder;
			builder.settings_["collectComments"] = false;
			std::string errs;

			if (!Json::parseFromStream(builder, is, &meta, &errs)) {
				errorstream << "ModMetadataDatabaseFiles[" << modname
						<< "]: failed read data (Json decoding failure). Message: "
						<< errs << std::endl;
				return nullptr;
			}
		}

		return &(m_mod_meta[modname] = meta);
	} else {
		return &found->second;
	}
}
