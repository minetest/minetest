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
#include "numeric.h"
#include "log.h"

#include <sstream>
#include <iomanip>

#include "../sha1.h"
#include "../base64.h"
#include "../hex.h"
#include "../porting.h"

#ifdef __ANDROID__
const wchar_t* wide_chars =
	L" !\"#$%&'()*+,-./0123456789:;<=>?@"
	L"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
	L"abcdefghijklmnopqrstuvwxyz{|}~";

int wctomb(char *s, wchar_t wc)
{
	for (unsigned int j = 0; j < (sizeof(wide_chars)/sizeof(wchar_t));j++) {
		if (wc == wide_chars[j]) {
			*s = (char) (j+32);
			return 1;
		}
		else if (wc == L'\n') {
			*s = '\n';
			return 1;
		}
	}
	return -1;
}

int mbtowc(wchar_t *pwc, const char *s, size_t n)
{
	std::wstring intermediate = narrow_to_wide(s);

	if (intermediate.length() > 0) {
		*pwc = intermediate[0];
		return 1;
	}
	else {
		return -1;
	}
}

std::wstring narrow_to_wide(const std::string& mbs) {
	size_t wcl = mbs.size();

	std::wstring retval = L"";

	for (unsigned int i = 0; i < wcl; i++) {
		if (((unsigned char) mbs[i] >31) &&
		 ((unsigned char) mbs[i] < 127)) {

			retval += wide_chars[(unsigned char) mbs[i] -32];
		}
		//handle newline
		else if (mbs[i] == '\n') {
			retval += L'\n';
		}
	}

	return retval;
}
#else

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

#endif

#ifdef __ANDROID__
std::string wide_to_narrow(const std::wstring& wcs) {
	size_t mbl = wcs.size()*4;

	std::string retval = "";
	for (unsigned int i = 0; i < wcs.size(); i++) {
		wchar_t char1 = (wchar_t) wcs[i];

		if (char1 == L'\n') {
			retval += '\n';
			continue;
		}

		for (unsigned int j = 0; j < wcslen(wide_chars);j++) {
			wchar_t char2 = (wchar_t) wide_chars[j];

			if (char1 == char2) {
				char toadd = (j+32);
				retval += toadd;
				break;
			}
		}
	}

	return retval;
}
#else
std::string wide_to_narrow(const std::wstring& wcs)
{
	size_t mbl = wcs.size()*4;
	SharedBuffer<char> mbs(mbl+1);
	size_t l = wcstombs(*mbs, wcs.c_str(), mbl);
	if(l == (size_t)(-1)) {
		return "Character conversion failed!";
	}
	else
		mbs[l] = 0;
	return *mbs;
}

#endif

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

std::string urlencode(std::string str)
{
	// Encodes non-unreserved URI characters by a percent sign
	// followed by two hex digits. See RFC 3986, section 2.3.
	static const char url_hex_chars[] = "0123456789ABCDEF";
	std::ostringstream oss(std::ios::binary);
	for (u32 i = 0; i < str.size(); i++) {
		unsigned char c = str[i];
		if (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~')
			oss << c;
		else
			oss << "%"
				<< url_hex_chars[(c & 0xf0) >> 4]
				<< url_hex_chars[c & 0x0f];
	}
	return oss.str();
}

std::string urldecode(std::string str)
{
	// Inverse of urlencode
	std::ostringstream oss(std::ios::binary);
	for (u32 i = 0; i < str.size(); i++) {
		unsigned char highvalue, lowvalue;
		if (str[i] == '%' &&
				hex_digit_decode(str[i+1], highvalue) &&
				hex_digit_decode(str[i+2], lowvalue)) {
			oss << (char) ((highvalue << 4) | lowvalue);
			i += 2;
		}
		else
			oss << str[i];
	}
	return oss.str();
}

u32 readFlagString(std::string str, const FlagDesc *flagdesc, u32 *flagmask)
{
	u32 result = 0, mask = 0;
	char *s = &str[0];
	char *flagstr, *strpos = NULL;

	while ((flagstr = strtok_r(s, ",", &strpos))) {
		s = NULL;

		while (*flagstr == ' ' || *flagstr == '\t')
			flagstr++;

		bool flagset = true;
		if (!strncasecmp(flagstr, "no", 2)) {
			flagset = false;
			flagstr += 2;
		}

		for (int i = 0; flagdesc[i].name; i++) {
			if (!strcasecmp(flagstr, flagdesc[i].name)) {
				mask |= flagdesc[i].flag;
				if (flagset)
					result |= flagdesc[i].flag;
				break;
			}
		}
	}

	if (flagmask)
		*flagmask = mask;

	return result;
}

std::string writeFlagString(u32 flags, const FlagDesc *flagdesc, u32 flagmask)
{
	std::string result;

	for (int i = 0; flagdesc[i].name; i++) {
		if (flagmask & flagdesc[i].flag) {
			if (!(flags & flagdesc[i].flag))
				result += "no";

			result += flagdesc[i].name;
			result += ", ";
		}
	}

	size_t len = result.length();
	if (len >= 2)
		result.erase(len - 2, 2);

	return result;
}

size_t mystrlcpy(char *dst, const char *src, size_t size)
{
	size_t srclen  = strlen(src) + 1;
	size_t copylen = MYMIN(srclen, size);

	if (copylen > 0) {
		memcpy(dst, src, copylen);
		dst[copylen - 1] = '\0';
	}

	return srclen;
}

char *mystrtok_r(char *s, const char *sep, char **lasts)
{
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

u64 read_seed(const char *str)
{
	char *endptr;
	u64 num;
	
	if (str[0] == '0' && str[1] == 'x')
		num = strtoull(str, &endptr, 16);
	else
		num = strtoull(str, &endptr, 10);
		
	if (*endptr)
		num = murmur_hash_64_ua(str, (int)strlen(str), 0x1337);
		
	return num;
}

bool parseColorString(const std::string &value, video::SColor &color, bool quiet)
{
	const char *hexpattern = NULL;
	video::SColor outcolor(255, 255, 255, 255);

	if (value[0] == '#') {
		if (value.size() == 9)
			hexpattern = "#RRGGBBAA";
		else if (value.size() == 7)
			hexpattern = "#RRGGBB";
		else if (value.size() == 5)
			hexpattern = "#RGBA";
		else if (value.size() == 4)
			hexpattern = "#RGB";
	}

	if (!hexpattern)
		goto fail;

	assert(strlen(hexpattern) == value.size());
	for (size_t pos = 0; pos < value.size(); ++pos) {
		// '#' in the pattern means skip that character
		if (hexpattern[pos] == '#')
			continue;

		// Else assume hexpattern[pos] is one of 'R' 'G' 'B' 'A'
		// Read one or two digits, depending on hexpattern
		unsigned char c1, c2;
		if (hexpattern[pos+1] == hexpattern[pos]) {
			// Two digits, e.g. hexpattern == "#RRGGBB"
			if (!hex_digit_decode(value[pos], c1) ||
				   !hex_digit_decode(value[pos+1], c2))
				goto fail;
			++pos;
		} else {
			// One digit, e.g. hexpattern == "#RGB"
			if (!hex_digit_decode(value[pos], c1))
				goto fail;
			c2 = c1;
		}
		u32 colorpart = ((c1 & 0x0f) << 4) | (c2 & 0x0f);

		// Update outcolor with newly read color part
		if (hexpattern[pos] == 'R')
			outcolor.setRed(colorpart);
		else if (hexpattern[pos] == 'G')
			outcolor.setGreen(colorpart);
		else if (hexpattern[pos] == 'B')
			outcolor.setBlue(colorpart);
		else if (hexpattern[pos] == 'A')
			outcolor.setAlpha(colorpart);
	}

	color = outcolor;
	return true;

fail:
	if (!quiet)
		errorstream << "Invalid color: \"" << value << "\"" << std::endl;
	return false;
}
