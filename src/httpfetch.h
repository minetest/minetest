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

#ifndef HTTPFETCH_HEADER
#define HTTPFETCH_HEADER

#include <string>
#include <vector>
#include <map>
#include "config.h"

// Can be used in place of "caller" in asynchronous transfers to discard result
// (used as default value of "caller")
#define HTTPFETCH_DISCARD 0
#define HTTPFETCH_SYNC 1

struct HTTPFetchRequest
{
	std::string url;

	// Identifies the caller (for asynchronous requests)
	// Ignored by httpfetch_sync
	unsigned long caller;

	// Some number that identifies the request
	// (when the same caller issues multiple httpfetch_async calls)
	unsigned long request_id;

	// Timeout for the whole transfer, in milliseconds
	long timeout;

	// Timeout for the connection phase, in milliseconds
	long connect_timeout;

	// Indicates if this is multipart/form-data or
	// application/x-www-form-urlencoded.  POST-only.
	bool multipart;

	// POST fields.  Fields are escaped properly.
	// If this is empty a GET request is done instead.
	std::map<std::string, std::string> post_fields;

	// Raw POST data, overrides post_fields.
	std::string post_data;

	// If not empty, should contain entries such as "Accept: text/html"
	std::vector<std::string> extra_headers;

	//useragent to use
	std::string useragent;

	HTTPFetchRequest();
};

struct HTTPFetchResult
{
	bool succeeded;
	bool timeout;
	long response_code;
	std::string data;
	// The caller and request_id from the corresponding HTTPFetchRequest.
	unsigned long caller;
	unsigned long request_id;

	HTTPFetchResult()
	{
		succeeded = false;
		timeout = false;
		response_code = 0;
		data = "";
		caller = HTTPFETCH_DISCARD;
		request_id = 0;
	}

	HTTPFetchResult(const HTTPFetchRequest &fetch_request)
	{
		succeeded = false;
		timeout = false;
		response_code = 0;
		data = "";
		caller = fetch_request.caller;
		request_id = fetch_request.request_id;
	}

};

// Initializes the httpfetch module
void httpfetch_init(int parallel_limit);

// Stops the httpfetch thread and cleans up resources
void httpfetch_cleanup();

// Starts an asynchronous HTTP fetch request
void httpfetch_async(const HTTPFetchRequest &fetch_request);

// If any fetch for the given caller ID is complete, removes it from the
// result queue, sets the fetch result and returns true. Otherwise returns false.
bool httpfetch_async_get(unsigned long caller, HTTPFetchResult &fetch_result);

// Allocates a caller ID for httpfetch_async
// Not required if you want to set caller = HTTPFETCH_DISCARD
unsigned long httpfetch_caller_alloc();

// Frees a caller ID allocated with httpfetch_caller_alloc
// Note: This can be expensive, because the httpfetch thread is told
// to stop any ongoing fetches for the given caller.
void httpfetch_caller_free(unsigned long caller);

// Performs a synchronous HTTP request. This blocks and therefore should
// only be used from background threads.
void httpfetch_sync(const HTTPFetchRequest &fetch_request,
		HTTPFetchResult &fetch_result);


#endif // !HTTPFETCH_HEADER
