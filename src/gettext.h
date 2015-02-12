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

#ifndef GETTEXT_HEADER
#define GETTEXT_HEADER

#include "config.h" // for USE_GETTEXT

#if USE_GETTEXT
#include <libintl.h>
#else
#define gettext(String) String
#endif

#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#ifdef _MSC_VER
void init_gettext(const char *path, const std::string &configured_language, int argc, char** argv);
#else
void init_gettext(const char *path, const std::string &configured_language);
#endif

extern const wchar_t *narrow_to_wide_c(const char *mbs);
extern std::wstring narrow_to_wide(const std::string &mbs);

// You must free the returned string!
inline const wchar_t *wgettext(const char *str)
{
	return narrow_to_wide_c(gettext(str));
}

// Gettext under MSVC needs this strange way. Just don't ask...
inline std::wstring wstrgettext(const std::string &text)
{
	const wchar_t *tmp = wgettext(text.c_str());
	std::wstring retval = (std::wstring)tmp;
	delete[] tmp;
	return retval;
}

inline std::string strgettext(const std::string &text)
{
	return gettext(text.c_str());
}

#endif
