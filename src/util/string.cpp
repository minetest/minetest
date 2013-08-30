/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "string.h"
#include "pointer.h"

#include "../sha1.h"
#include "../base64.h"
#include "../porting.h"

std::wstring narrow_to_wide(const std::string& mbs)
{
	size_t wcl = mbs.size();
	Buffer<wchar_t> wcs(wcl+1);
	size_t l = mbstowcs(*wcs, mbs.c_str(), wcl);
	if(l == (size_t)(-1))
		return L"<invalid multibyte string>";
	wcs[l] = 0;
	return *wcs;
}

std::string wide_to_narrow(const std::wstring& wcs)
{
	size_t mbl = wcs.size()*4;
	SharedBuffer<char> mbs(mbl+1);
	size_t l = wcstombs(*mbs, wcs.c_str(), mbl);
	if(l == (size_t)(-1))
		mbs[0] = 0;
	else
		mbs[l] = 0;
	return *mbs;
}

// Get an sha-1 hash of the player's name combined with
// the password entered. That's what the server uses as
// their password. (Exception : if the password field is
// blank, we send a blank password - this is for backwards
// compatibility with password-less players).
std::string translatePassword(std::string playername, std::wstring password)
{
	if(password.length() == 0)
		return "";

	std::string slt = playername + wide_to_narrow(password);
	SHA1 sha1;
	sha1.addBytes(slt.c_str(), slt.length());
	unsigned char *digest = sha1.getDigest();
	std::string pwd = base64_encode(digest, 20);
	free(digest);
	return pwd;
}

size_t curl_write_data(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::ostringstream *stream = (std::ostringstream*)userdata;
    size_t count = size * nmemb;
    stream->write(ptr, count);
    return count;
}

u32 readFlagString(std::string str, FlagDesc *flagdesc) {
	u32 result = 0;
	char *s = &str[0];
	char *flagstr, *strpos = NULL;
	
	while ((flagstr = strtok_r(s, ",", &strpos))) {
		s = NULL;
		
		while (*flagstr == ' ' || *flagstr == '\t')
			flagstr++;
		
		for (int i = 0; flagdesc[i].name; i++) {
			if (!strcasecmp(flagstr, flagdesc[i].name)) {
				result |= flagdesc[i].flag;
				break;
			}
		}
	}
	
	return result;
}

std::string writeFlagString(u32 flags, FlagDesc *flagdesc) {
	std::string result;
	
	for (int i = 0; flagdesc[i].name; i++) {
		if (flags & flagdesc[i].flag) {
			result += flagdesc[i].name;
			result += ", ";
		}
	}
	
	size_t len = result.length();
	if (len >= 2)
		result.erase(len - 2, 2);
	
	return result;
}

char *mystrtok_r(char *s, const char *sep, char **lasts) {
	char *t;

	if (!s)
		s = *lasts;

	while (*s && strchr(sep, *s))
		s++;

	if (!*s)
		return NULL;

	t = s;
	while (*t) {
		if (strchr(sep, *t)) {
			*t++ = '\0';
			break;
		}
		t++;
	}
	
	*lasts = t;
	return s;
}
