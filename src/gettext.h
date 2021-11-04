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

#pragma once

#include "config.h" // for USE_GETTEXT
#include <string>
#include "porting.h"

#if USE_GETTEXT
	#include <libintl.h>
#else
	// In certain environments, some standard headers like <iomanip>
	// and <locale> include libintl.h. If libintl.h is included after
	// we define our gettext macro below, this causes a syntax error
	// at the declaration of the gettext function in libintl.h.
	// Fix this by including such a header before defining the macro.
	// See issue #4446.
	// Note that we can't include libintl.h directly since we're in
	// the USE_GETTEXT=0 case and can't assume that gettext is installed.
	#include <locale>

	#define gettext(String) String
#endif

#define _(String) gettext(String)
#define gettext_noop(String) (String)
#define N_(String) gettext_noop((String))

void init_gettext(const char *path, const std::string &configured_language,
	int argc, char *argv[]);

extern wchar_t *utf8_to_wide_c(const char *str);

// You must free the returned string!
// The returned string is allocated using new
inline const wchar_t *wgettext(const char *str)
{
	// We must check here that is not an empty string to avoid trying to translate it
	return str[0] ? utf8_to_wide_c(gettext(str)) : utf8_to_wide_c("");
}

inline std::string strgettext(const std::string &text)
{
	return text.empty() ? "" : gettext(text.c_str());
}

/**
 * Returns translated string with format args applied
 *
 * @tparam Args Template parameter for format args
 * @param src Translation source string
 * @param args Variable format args
 * @return translated string
 */
template <typename ...Args>
inline std::wstring fwgettext(const char *src, Args&&... args)
{
	wchar_t buf[255];
	const wchar_t* str = wgettext(src);
	swprintf(buf, sizeof(buf) / sizeof(wchar_t), str, std::forward<Args>(args)...);
	delete[] str;
	return std::wstring(buf);
}

/**
 * Returns translated string with format args applied
 *
 * @tparam Args Template parameter for format args
 * @param format Translation source string
 * @param args Variable format args
 * @return translated string.
 */
template <typename ...Args>
inline std::string fmtgettext(const char *format, Args&&... args)
{
	std::string buf;
	std::size_t buf_size = 256;
	buf.resize(buf_size);

	format = gettext(format);

	int len = porting::mt_snprintf(&buf[0], buf_size, format, std::forward<Args>(args)...);
	if (len <= 0) throw std::runtime_error("gettext format error: " + std::string(format));
	if ((size_t)len >= buf.size()) {
		buf.resize(len+1); // extra null byte
		porting::mt_snprintf(&buf[0], buf.size(), format, std::forward<Args>(args)...);
	}
	buf.resize(len); // remove null bytes

	return buf;
}
