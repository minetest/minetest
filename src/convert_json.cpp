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

#include <vector>
#include <iostream>
#include <sstream>

#include "convert_json.h"
#include "mods.h"
#include "config.h"
#include "log.h"
#include "main.h" // for g_settings
#include "settings.h"
#include "httpfetch.h"
#include "porting.h"

Json::Value fetchJsonValue(const std::string &url,
		std::vector<std::string> *extra_headers)
{
	HTTPFetchRequest fetch_request;
	HTTPFetchResult fetch_result;
	fetch_request.url = url;
	fetch_request.caller = HTTPFETCH_SYNC;

	if (extra_headers != NULL)
		fetch_request.extra_headers = *extra_headers;

	httpfetch_sync(fetch_request, fetch_result);

	if (!fetch_result.succeeded) {
		return Json::Value();
	}
	Json::Value root;
	Json::Reader reader;
	std::istringstream stream(fetch_result.data);

	if (!reader.parse(stream, root)) {
		errorstream << "URL: " << url << std::endl;
		errorstream << "Failed to parse json data " << reader.getFormattedErrorMessages();
		errorstream << "data: \"" << fetch_result.data << "\"" << std::endl;
		return Json::Value();
	}

	return root;
}

std::vector<ModStoreMod>    readModStoreList(Json::Value& modlist) {
		std::vector<ModStoreMod> retval;

	if (modlist.isArray()) {
		for (unsigned int i = 0; i < modlist.size(); i++)
		{
			ModStoreMod toadd;
			toadd.valid = true;

			//id
			if (modlist[i]["id"].asString().size()) {
				std::string id_raw = modlist[i]["id"].asString();
				char* endptr = 0;
				int numbervalue = strtol(id_raw.c_str(),&endptr,10);

				if ((id_raw != "") && (*endptr == 0)) {
					toadd.id = numbervalue;
				}
				else {
					errorstream << "readModStoreList: missing id" << std::endl;
					toadd.valid = false;
				}
			}
			else {
				errorstream << "readModStoreList: missing id" << std::endl;
				toadd.valid = false;
			}

			//title
			if (modlist[i]["title"].asString().size()) {
				toadd.title = modlist[i]["title"].asString();
			}
			else {
				errorstream << "readModStoreList: missing title" << std::endl;
				toadd.valid = false;
			}

			//basename
			if (modlist[i]["basename"].asString().size()) {
				toadd.basename = modlist[i]["basename"].asString();
			}
			else {
				errorstream << "readModStoreList: missing basename" << std::endl;
				toadd.valid = false;
			}

			//author

			//rating

			//version

			if (toadd.valid) {
				retval.push_back(toadd);
			}
		}
	}
	return retval;
}

ModStoreModDetails          readModStoreModDetails(Json::Value& details) {

	ModStoreModDetails retval;

	retval.valid = true;

	//version set
	if (details["version_set"].isArray()) {
		for (unsigned int i = 0; i < details["version_set"].size(); i++)
		{
			ModStoreVersionEntry toadd;

			if (details["version_set"][i]["id"].asString().size()) {
				std::string id_raw = details["version_set"][i]["id"].asString();
				char* endptr = 0;
				int numbervalue = strtol(id_raw.c_str(),&endptr,10);

				if ((id_raw != "") && (*endptr == 0)) {
					toadd.id = numbervalue;
				}
			}
			else {
				errorstream << "readModStoreModDetails: missing version_set id" << std::endl;
				retval.valid = false;
			}

			//date
			if (details["version_set"][i]["date"].asString().size()) {
				toadd.date = details["version_set"][i]["date"].asString();
			}

			//file
			if (details["version_set"][i]["file"].asString().size()) {
				toadd.file = details["version_set"][i]["file"].asString();
			}
			else {
				errorstream << "readModStoreModDetails: missing version_set file" << std::endl;
				retval.valid = false;
			}

			//approved

			//mtversion

			if( retval.valid ) {
				retval.versions.push_back(toadd);
			}
			else {
				break;
			}
		}
	}

	if (retval.versions.size() < 1) {
		infostream << "readModStoreModDetails: not a single version specified!" << std::endl;
		retval.valid = false;
	}

	//categories
	if (details["categories"].isObject()) {
		for (unsigned int i = 0; i < details["categories"].size(); i++) {
			ModStoreCategoryInfo toadd;

			if (details["categories"][i]["id"].asString().size()) {

				std::string id_raw = details["categories"][i]["id"].asString();
				char* endptr = 0;
				int numbervalue = strtol(id_raw.c_str(),&endptr,10);

				if ((id_raw != "") && (*endptr == 0)) {
					toadd.id = numbervalue;
				}
			}
			else {
				errorstream << "readModStoreModDetails: missing categories id" << std::endl;
				retval.valid = false;
			}
			if (details["categories"][i]["title"].asString().size()) {
				toadd.name = details["categories"][i]["title"].asString();
			}
			else {
				errorstream << "readModStoreModDetails: missing categories title" << std::endl;
				retval.valid = false;
			}

			if( retval.valid ) {
				retval.categories.push_back(toadd);
			}
			else {
				break;
			}
		}
	}

	//author
	if (details["author"].isObject()) {
		if (details["author"]["id"].asString().size()) {

			std::string id_raw = details["author"]["id"].asString();
			char* endptr = 0;
			int numbervalue = strtol(id_raw.c_str(),&endptr,10);

			if ((id_raw != "") && (*endptr == 0)) {
				retval.author.id = numbervalue;
			}
			else {
				errorstream << "readModStoreModDetails: missing author id (convert)" << std::endl;
				retval.valid = false;
			}
		}
		else {
			errorstream << "readModStoreModDetails: missing author id" << std::endl;
			retval.valid = false;
		}

		if (details["author"]["username"].asString().size()) {
			retval.author.username = details["author"]["username"].asString();
		}
		else {
			errorstream << "readModStoreModDetails: missing author username" << std::endl;
			retval.valid = false;
		}
	}
	else {
		errorstream << "readModStoreModDetails: missing author" << std::endl;
		retval.valid = false;
	}

	//license
	if (details["license"].isObject()) {
		if (details["license"]["id"].asString().size()) {

			std::string id_raw = details["license"]["id"].asString();
			char* endptr = 0;
			int numbervalue = strtol(id_raw.c_str(),&endptr,10);

			if ((id_raw != "") && (*endptr == 0)) {
				retval.license.id = numbervalue;
			}
		}
		else {
			errorstream << "readModStoreModDetails: missing license id" << std::endl;
			retval.valid = false;
		}

		if (details["license"]["short"].asString().size()) {
			retval.license.shortinfo = details["license"]["short"].asString();
		}
		else {
			errorstream << "readModStoreModDetails: missing license short" << std::endl;
			retval.valid = false;
		}

		if (details["license"]["link"].asString().size()) {
			retval.license.url = details["license"]["link"].asString();
		}

	}

	//titlepic
	if (details["titlepic"].isObject()) {
		if (details["titlepic"]["id"].asString().size()) {

			std::string id_raw = details["titlepic"]["id"].asString();
			char* endptr = 0;
			int numbervalue = strtol(id_raw.c_str(),&endptr,10);

			if ((id_raw != "") && (*endptr == 0)) {
				retval.titlepic.id = numbervalue;
			}
		}

		if (details["titlepic"]["file"].asString().size()) {
			retval.titlepic.file = details["titlepic"]["file"].asString();
		}

		if (details["titlepic"]["desc"].asString().size()) {
			retval.titlepic.description = details["titlepic"]["desc"].asString();
		}

		if (details["titlepic"]["mod"].asString().size()) {

			std::string mod_raw = details["titlepic"]["mod"].asString();
			char* endptr = 0;
			int numbervalue = strtol(mod_raw.c_str(),&endptr,10);

			if ((mod_raw != "") && (*endptr == 0)) {
				retval.titlepic.mod = numbervalue;
			}
		}
	}

	//id
	if (details["id"].asString().size()) {

		std::string id_raw = details["id"].asString();
		char* endptr = 0;
		int numbervalue = strtol(id_raw.c_str(),&endptr,10);

		if ((id_raw != "") && (*endptr == 0)) {
			retval.id = numbervalue;
		}
	}
	else {
		errorstream << "readModStoreModDetails: missing id" << std::endl;
		retval.valid = false;
	}

	//title
	if (details["title"].asString().size()) {
		retval.title = details["title"].asString();
	}
	else {
		errorstream << "readModStoreModDetails: missing title" << std::endl;
		retval.valid = false;
	}

	//basename
	if (details["basename"].asString().size()) {
		retval.basename = details["basename"].asString();
	}
	else {
		errorstream << "readModStoreModDetails: missing basename" << std::endl;
		retval.valid = false;
	}

	//description
	if (details["desc"].asString().size()) {
		retval.description = details["desc"].asString();
	}

	//repository
	if (details["replink"].asString().size()) {
		retval.repository = details["replink"].asString();
	}

	//value
	if (details["rating"].asString().size()) {

		std::string id_raw = details["rating"].asString();
		char* endptr = 0;
		float numbervalue = strtof(id_raw.c_str(),&endptr);

		if ((id_raw != "") && (*endptr == 0)) {
			retval.rating = numbervalue;
		}
	}
	else {
		retval.rating = 0.0;
	}

	//depends
	if (details["depends"].isArray()) {
		//TODO
	}

	//softdepends
	if (details["softdep"].isArray()) {
		//TODO
	}

	//screenshot url
	if (details["screenshot_url"].asString().size()) {
		retval.screenshot_url = details["screenshot_url"].asString();
	}

	return retval;
}
