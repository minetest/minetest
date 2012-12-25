/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include <sstream>
#include <algorithm>

#include "main.h" // for g_settings
#include "settings.h"
#include "serverlist.h"
#include "filesys.h"
#include "porting.h"
#include "log.h"
#if USE_CURL
#include <curl/curl.h>
#endif

namespace ServerList
{
std::string getFilePath()
{
	std::string serverlist_file = g_settings->get("serverlist_file");

	std::string rel_path = std::string("client") + DIR_DELIM
		+ "serverlist" + DIR_DELIM
		+ serverlist_file;
	std::string path = porting::path_share + DIR_DELIM + rel_path;
	return path;
}

std::vector<ServerListSpec> getLocal()
{
	std::string path = ServerList::getFilePath();
	std::string liststring;
	if(fs::PathExists(path))
	{
		std::ifstream istream(path.c_str(), std::ios::binary);
		if(istream.is_open())
		{
			std::ostringstream ostream;
			ostream << istream.rdbuf();
			liststring = ostream.str();
			istream.close();
		}
	}

	return ServerList::deSerialize(liststring);
}


#if USE_CURL

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


std::vector<ServerListSpec> getOnline()
{
	std::string liststring;
	CURL *curl;

	curl = curl_easy_init();
	if (curl)
	{
		CURLcode res;

		curl_easy_setopt(curl, CURLOPT_URL, g_settings->get("serverlist_url").c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ServerList::WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &liststring);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			errorstream<<"Serverlist at url "<<g_settings->get("serverlist_url")<<" not found (internet connection?)"<<std::endl;
		curl_easy_cleanup(curl);
	}

	return ServerList::deSerialize(liststring);
}

#endif

/*
	Delete a server fromt he local favorites list
*/
bool deleteEntry (ServerListSpec server)
{
	std::vector<ServerListSpec> serverlist = ServerList::getLocal();
	for(unsigned i = 0; i < serverlist.size(); i++)
	{
		if  (serverlist[i].address == server.address
		&&   serverlist[i].port    == server.port)
		{
			serverlist.erase(serverlist.begin() + i);
		}
	}

	std::string path = ServerList::getFilePath();
	std::ofstream stream (path.c_str());
	if (stream.is_open())
	{
		stream<<ServerList::serialize(serverlist);
		return true;
	}
	return false;
}

/*
	Insert a server to the local favorites list
*/
bool insert (ServerListSpec server)
{
	// Remove duplicates
	ServerList::deleteEntry(server);

	std::vector<ServerListSpec> serverlist = ServerList::getLocal();

	// Insert new server at the top of the list
	serverlist.insert(serverlist.begin(), server);

	std::string path = ServerList::getFilePath();
	std::ofstream stream (path.c_str());
	if (stream.is_open())
	{
		stream<<ServerList::serialize(serverlist);
	}

	return false;
}

std::vector<ServerListSpec> deSerialize(std::string liststring)
{
	std::vector<ServerListSpec> serverlist;
	std::istringstream stream(liststring);
	std::string line;
	while (std::getline(stream, line))
	{
		std::transform(line.begin(), line.end(),line.begin(), ::toupper);
		if (line == "[SERVER]")
		{
			ServerListSpec thisserver;
			std::getline(stream, thisserver.name);
			std::getline(stream, thisserver.address);
			std::getline(stream, thisserver.port);
			std::getline(stream, thisserver.description);
			serverlist.push_back(thisserver);
		}
	}
	return serverlist;
}

std::string serialize(std::vector<ServerListSpec> serverlist)
{
	std::string liststring;
	for(std::vector<ServerListSpec>::iterator i = serverlist.begin(); i != serverlist.end(); i++)
	{
		liststring += "[server]\n";
		liststring += i->name + "\n";
		liststring += i->address + "\n";
		liststring += i->port + "\n";
		liststring += i->description + "\n";
		liststring += "\n";
	}
	return liststring;
}

} //namespace ServerList
