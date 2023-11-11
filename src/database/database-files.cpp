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

#include "database-files.h"
#include "convert_json.h"
#include "remoteplayer.h"
#include "settings.h"
#include "porting.h"
#include "filesys.h"
#include "server/player_sao.h"
#include "util/string.h"
#include <cassert>

ModStorageDatabaseFiles::ModStorageDatabaseFiles(const std::string &savedir):
	m_storage_dir(savedir + DIR_DELIM + "mod_storage")
{
}

void ModStorageDatabaseFiles::getModEntries(const std::string &modname, StringMap *storage)
{
	Json::Value *meta = getOrCreateJson(modname);
	if (!meta)
		return;

	const Json::Value::Members attr_list = meta->getMemberNames();
	for (const auto &it : attr_list) {
		Json::Value attr_value = (*meta)[it];
		(*storage)[it] = attr_value.asString();
	}
}

void ModStorageDatabaseFiles::getModKeys(const std::string &modname,
		std::vector<std::string> *storage)
{
	Json::Value *meta = getOrCreateJson(modname);
	if (!meta)
		return;

	std::vector<std::string> keys = meta->getMemberNames();
	storage->reserve(storage->size() + keys.size());
	for (std::string &key : keys)
		storage->push_back(std::move(key));
}

bool ModStorageDatabaseFiles::getModEntry(const std::string &modname,
	const std::string &key, std::string *value)
{
	Json::Value *meta = getOrCreateJson(modname);
	if (!meta)
		return false;

	if (meta->isMember(key)) {
		*value = (*meta)[key].asString();
		return true;
	}
	return false;
}

bool ModStorageDatabaseFiles::hasModEntry(const std::string &modname, const std::string &key)
{
	Json::Value *meta = getOrCreateJson(modname);
	return meta && meta->isMember(key);
}

bool ModStorageDatabaseFiles::setModEntry(const std::string &modname,
	const std::string &key, const std::string &value)
{
	Json::Value *meta = getOrCreateJson(modname);
	if (!meta)
		return false;

	(*meta)[key] = Json::Value(value);
	m_modified.insert(modname);

	return true;
}

bool ModStorageDatabaseFiles::removeModEntry(const std::string &modname,
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

bool ModStorageDatabaseFiles::removeModEntries(const std::string &modname)
{
	Json::Value *meta = getOrCreateJson(modname);
	if (!meta || meta->empty())
		return false;

	meta->clear();
	m_modified.insert(modname);
	return true;
}

void ModStorageDatabaseFiles::beginSave()
{
}

void ModStorageDatabaseFiles::endSave()
{
	if (m_modified.empty())
		return;

	if (!fs::CreateAllDirs(m_storage_dir)) {
		errorstream << "ModStorageDatabaseFiles: Unable to save. '"
				<< m_storage_dir << "' cannot be created." << std::endl;
		return;
	}
	if (!fs::IsDir(m_storage_dir)) {
		errorstream << "ModStorageDatabaseFiles: Unable to save. '"
				<< m_storage_dir << "' is not a directory." << std::endl;
		return;
	}

	for (auto it = m_modified.begin(); it != m_modified.end();) {
		const std::string &modname = *it;

		const Json::Value &json = m_mod_storage[modname];

		if (!fs::safeWriteToFile(m_storage_dir + DIR_DELIM + modname, fastWriteJson(json))) {
			errorstream << "ModStorageDatabaseFiles[" << modname
					<< "]: failed to write file." << std::endl;
			++it;
			continue;
		}

		it = m_modified.erase(it);
	}
}

void ModStorageDatabaseFiles::listMods(std::vector<std::string> *res)
{
	// List in-memory metadata first.
	for (const auto &pair : m_mod_storage) {
		res->push_back(pair.first);
	}

	// List other metadata present in the filesystem.
	for (const auto &entry : fs::GetDirListing(m_storage_dir)) {
		if (!entry.dir && m_mod_storage.count(entry.name) == 0)
			res->push_back(entry.name);
	}
}

Json::Value *ModStorageDatabaseFiles::getOrCreateJson(const std::string &modname)
{
	auto found = m_mod_storage.find(modname);
	if (found != m_mod_storage.end())
		return &found->second;

	Json::Value meta(Json::objectValue);

	std::string path = m_storage_dir + DIR_DELIM + modname;
	if (fs::PathExists(path)) {
		std::ifstream is(path.c_str(), std::ios_base::binary);

		Json::CharReaderBuilder builder;
		builder.settings_["collectComments"] = false;
		std::string errs;

		if (!Json::parseFromStream(builder, is, &meta, &errs)) {
			errorstream << "ModStorageDatabaseFiles[" << modname
					<< "]: failed to decode data: " << errs << std::endl;
			return nullptr;
		}
	}

	return &(m_mod_storage[modname] = std::move(meta));
}
