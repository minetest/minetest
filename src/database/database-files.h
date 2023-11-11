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

#pragma once

#include "database.h"
#include <unordered_map>
#include <unordered_set>
#include <json/json.h> // for Json::Value

class ModStorageDatabaseFiles : public ModStorageDatabase
{
public:
	ModStorageDatabaseFiles(const std::string &savedir);
	virtual ~ModStorageDatabaseFiles() = default;

	virtual void getModEntries(const std::string &modname, StringMap *storage);
	virtual void getModKeys(const std::string &modname, std::vector<std::string> *storage);
	virtual bool getModEntry(const std::string &modname,
		const std::string &key, std::string *value);
	virtual bool hasModEntry(const std::string &modname, const std::string &key);
	virtual bool setModEntry(const std::string &modname,
		const std::string &key, const std::string &value);
	virtual bool removeModEntry(const std::string &modname, const std::string &key);
	virtual bool removeModEntries(const std::string &modname);
	virtual void listMods(std::vector<std::string> *res);

	virtual void beginSave();
	virtual void endSave();

private:
	Json::Value *getOrCreateJson(const std::string &modname);
	bool writeJson(const std::string &modname, const Json::Value &json);

	std::string m_storage_dir;
	std::unordered_map<std::string, Json::Value> m_mod_storage;
	std::unordered_set<std::string> m_modified;
};
