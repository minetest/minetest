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
#include "content/mods.h"
#include "config.h"
#include "log.h"
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
	std::istringstream stream(fetch_result.data);

	Json::CharReaderBuilder builder;
	builder.settings_["collectComments"] = false;
	std::string errs;

	if (!Json::parseFromStream(builder, stream, &root, &errs)) {
		errorstream << "URL: " << url << std::endl;
		errorstream << "Failed to parse json data " << errs << std::endl;
		if (fetch_result.data.size() > 100) {
			errorstream << "Data (" << fetch_result.data.size()
				<< " bytes) printed to warningstream." << std::endl;
			warningstream << "data: \"" << fetch_result.data << "\"" << std::endl;
		} else {
			errorstream << "data: \"" << fetch_result.data << "\"" << std::endl;
		}
		return Json::Value();
	}

	return root;
}

void fastWriteJson(const Json::Value &value, std::ostream &to)
{
	Json::StreamWriterBuilder builder;
	builder["indentation"] = "";
	std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	writer->write(value, &to);
}

std::string fastWriteJson(const Json::Value &value)
{
	std::ostringstream oss;
	fastWriteJson(value, oss);
	return oss.str();
}
