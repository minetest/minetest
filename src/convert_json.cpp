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

#if USE_CURL
#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

#endif

Json::Value                 fetchJsonValue(const std::string url,
													struct curl_slist *chunk) {
#if USE_CURL
	std::string liststring;
	CURL *curl;

	curl = curl_easy_init();
	if (curl)
	{
		CURLcode res;

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &liststring);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, g_settings->getS32("curl_timeout"));
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, g_settings->getS32("curl_timeout"));

		if (chunk != 0)
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			errorstream<<"Jsonreader: "<< url <<" not found (" << curl_easy_strerror(res) << ")" <<std::endl;
		curl_easy_cleanup(curl);
	}

	Json::Value root;
	Json::Reader reader;
	std::istringstream stream(liststring);
	if (!liststring.size()) {
		return Json::Value();
	}

	if (!reader.parse( stream, root ) )
	{
		errorstream << "URL: " << url << std::endl;
		errorstream << "Failed to parse json data " << reader.getFormattedErrorMessages();
		errorstream << "data: \"" << liststring << "\"" << std::endl;
		return Json::Value();
	}

	if (root.isArray()) {
		return root;
	}
	if ((root["list"].isArray())) {
		return root["list"];
	}
	else {
		return root;
	}
#endif
	return Json::Value();
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
				const char* id_raw = modlist[i]["id"].asString().c_str();
				char* endptr = 0;
				int numbervalue = strtol(id_raw,&endptr,10);

				if ((*id_raw != 0) && (*endptr == 0)) {
					toadd.id = numbervalue;
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
				const char* id_raw = details["version_set"][i]["id"].asString().c_str();
				char* endptr = 0;
				int numbervalue = strtol(id_raw,&endptr,10);

				if ((*id_raw != 0) && (*endptr == 0)) {
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
		errorstream << "readModStoreModDetails: not a single version specified!" << std::endl;
		retval.valid = false;
	}

	//categories
	if (details["categories"].isObject()) {
		for (unsigned int i = 0; i < details["categories"].size(); i++) {
			ModStoreCategoryInfo toadd;

			if (details["categories"][i]["id"].asString().size()) {

				const char* id_raw = details["categories"][i]["id"].asString().c_str();
				char* endptr = 0;
				int numbervalue = strtol(id_raw,&endptr,10);

				if ((*id_raw != 0) && (*endptr == 0)) {
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

			const char* id_raw = details["author"]["id"].asString().c_str();
			char* endptr = 0;
			int numbervalue = strtol(id_raw,&endptr,10);

			if ((*id_raw != 0) && (*endptr == 0)) {
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

			const char* id_raw = details["license"]["id"].asString().c_str();
			char* endptr = 0;
			int numbervalue = strtol(id_raw,&endptr,10);

			if ((*id_raw != 0) && (*endptr == 0)) {
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

			const char* id_raw = details["titlepic"]["id"].asString().c_str();
			char* endptr = 0;
			int numbervalue = strtol(id_raw,&endptr,10);

			if ((*id_raw != 0) && (*endptr == 0)) {
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

			const char* mod_raw = details["titlepic"]["mod"].asString().c_str();
			char* endptr = 0;
			int numbervalue = strtol(mod_raw,&endptr,10);

			if ((*mod_raw != 0) && (*endptr == 0)) {
				retval.titlepic.mod = numbervalue;
			}
		}
	}

	//id
	if (details["id"].asString().size()) {

		const char* id_raw = details["id"].asString().c_str();
		char* endptr = 0;
		int numbervalue = strtol(id_raw,&endptr,10);

		if ((*id_raw != 0) && (*endptr == 0)) {
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

		const char* id_raw = details["rating"].asString().c_str();
		char* endptr = 0;
		float numbervalue = strtof(id_raw,&endptr);

		if ((*id_raw != 0) && (*endptr == 0)) {
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
