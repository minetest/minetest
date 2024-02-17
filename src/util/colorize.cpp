/*
Part of Minetest
Copyright (C) 2024 rubenwardy

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "colorize.h"

#ifdef USE_CURL

#include <curl/urlapi.h>
#include "log.h"
#include "string.h"
#include <sstream>

bool colorize_url(std::string &out, const std::string &url) {
	// Forbid escape codes in URL
	if (url.find('\x1b') != std::string::npos) {
		out = "Unable to open URL as it contains escape codes";
		return false;
	}

	CURLU *h = curl_url();
	auto rc = curl_url_set(h, CURLUPART_URL, url.c_str(), 0);
	if (rc != CURLUE_OK) {
		out = "Unable to open URL as it is not valid";
		curl_url_cleanup(h);
		return false;
	}

	char *fragment, *host, *password, *path, *port, *query, *scheme, *user, *zoneid;
	curl_url_get(h, CURLUPART_FRAGMENT, &fragment, 0);
	curl_url_get(h, CURLUPART_HOST, &host, 0);
	curl_url_get(h, CURLUPART_PASSWORD, &password, 0);
	curl_url_get(h, CURLUPART_PATH, &path, 0);
	curl_url_get(h, CURLUPART_PORT, &port, 0);
	curl_url_get(h, CURLUPART_QUERY, &query, 0);
	curl_url_get(h, CURLUPART_SCHEME, &scheme, 0);
	curl_url_get(h, CURLUPART_USER, &user, 0);
	curl_url_get(h, CURLUPART_ZONEID, &zoneid, 0);

	std::ostringstream os;

	std::string_view red = COLOR_CODE("#faa");
	std::string_view white = COLOR_CODE("#fff");
	std::string_view grey = COLOR_CODE("#aaa");

	os << grey << scheme << "://";
	if (user)
		os << user;
	if (password)
		os << ":" << password;
	if (user || password)
		os << "@";

	os << white;
	bool was_alphanum = true;
	std::string host_s = host;
	for (size_t i = 0; i < host_s.size(); i++) {
		char c = host_s[i];
		bool is_alphanum = !IS_UTF8_MULTB_INNER(c) && (isalnum(c) || ispunct(c));
		if (is_alphanum == was_alphanum) {
			// skip
		} else if (is_alphanum) {
			os << white;
		} else {
			os << red;
		}
		was_alphanum = is_alphanum;

		if (is_alphanum) {
			os << c;
		} else {
			os << "%" << std::setfill('0') << std::setw(2) << std::hex
			   << (static_cast<unsigned int>(c) & 0xff);
		}
	}


	os << grey;
	if (port != NULL)
		os << ":" << port;
	os << path;
	if (query != NULL)
		os << "?" << query;
	if (fragment != NULL)
		os << "#" << fragment;

	curl_url_cleanup(h);
	out = os.str();
	return true;
}

#endif
