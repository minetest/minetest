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

#ifndef UTIL_STRING_HEADER
#define UTIL_STRING_HEADER

#include "irrlichttypes_bloated.h"
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <cctype>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

struct FlagDesc {
	const char *name;
	u32 flag;
};

std::wstring narrow_to_wide(const std::string& mbs);
std::string wide_to_narrow(const std::wstring& wcs);

static inline std::string padStringRight(std::string s, size_t len)
{
	if(len > s.size())
		s.insert(s.end(), len - s.size(), ' ');
	return s;
}

// ends: NULL- or ""-terminated array of strings
// Returns "" if no end could be removed.
static inline std::string removeStringEnd(const std::string &s, const char *ends[])
{
	const char **p = ends;
	for(; (*p) && (*p)[0] != '\0'; p++){
		std::string end = *p;
		if(s.size() < end.size())
			continue;
		if(s.substr(s.size()-end.size(), end.size()) == end)
			return s.substr(0, s.size() - end.size());
	}
	return "";
}

// Tests if two strings are equal, optionally case insensitive
inline bool str_equal(const std::wstring& s1, const std::wstring& s2,
		bool case_insensitive = false)
{
	if(case_insensitive)
	{
		if(s1.size() != s2.size())
			return false;
		for(size_t i = 0; i < s1.size(); ++i)
			if(tolower(s1[i]) != tolower(s2[i]))
				return false;
		return true;
	}
	else
	{
		return s1 == s2;
	}
}

// Tests if the second string is a prefix of the first, optionally case insensitive
inline bool str_starts_with(const std::wstring& str, const std::wstring& prefix,
		bool case_insensitive = false)
{
	if(str.size() < prefix.size())
		return false;
	if(case_insensitive)
	{
		for(size_t i = 0; i < prefix.size(); ++i)
			if(tolower(str[i]) != tolower(prefix[i]))
				return false;
	}
	else
	{
		for(size_t i = 0; i < prefix.size(); ++i)
			if(str[i] != prefix[i])
				return false;
	}
	return true;
}

// Split a string using the given delimiter. Returns a vector containing
// the component parts.
inline std::vector<std::wstring> str_split(const std::wstring &str, wchar_t delimiter)
{
	std::vector<std::wstring> parts;
	std::wstringstream sstr(str);
	std::wstring part;
	while(std::getline(sstr, part, delimiter))
		parts.push_back(part);
	return parts;
}

inline std::vector<std::string> str_split(const std::string &str, char delimiter) {

	std::vector<std::string> parts;
	std::stringstream sstr(str);
	std::string part;
	while(std::getline(sstr, part, delimiter))
		parts.push_back(part);
	return parts;
}

inline std::string lowercase(const std::string &s)
{
	std::string s2;
	for(size_t i=0; i<s.size(); i++)
	{
		char c = s[i];
		if(c >= 'A' && c <= 'Z')
			c -= 'A' - 'a';
		s2 += c;
	}
	return s2;
}

inline std::string trim(const std::string &s)
{
	size_t front = 0;
	while(s[front] == ' '    ||
	      s[front] == '\t'   ||
	      s[front] == '\r'   ||
	      s[front] == '\n'
	     )
		++front;

	size_t back = s.size();
	while(back > front &&
	      (s[back-1] == ' '  ||
	       s[back-1] == '\t' ||
	       s[back-1] == '\r' ||
	       s[back-1] == '\n'
	      )
	     )
		--back;

	return s.substr(front, back - front);
}

inline bool is_yes(const std::string &s)
{
	std::string s2 = lowercase(trim(s));
	if(s2 == "y" || s2 == "yes" || s2 == "true" || atoi(s2.c_str()) != 0)
		return true;
	return false;
}

inline s32 mystoi(const std::string &s, s32 min, s32 max)
{
	s32 i = atoi(s.c_str());
	if(i < min)
		i = min;
	if(i > max)
		i = max;
	return i;
}

inline s64 stoi64(const std::string &s) {
	std::stringstream tmp(s);
	s64 t;
	tmp >> t;
	return t;
}

// MSVC2010 includes it's own versions of these
//#if !defined(_MSC_VER) || _MSC_VER < 1600

inline s32 mystoi(const std::string &s)
{
	return atoi(s.c_str());
}

inline s32 mystoi(const std::wstring &s)
{
	return atoi(wide_to_narrow(s).c_str());
}

inline float mystof(const std::string &s)
{
	// This crap causes a segfault in certain cases on MinGW
	/*float f;
	std::istringstream ss(s);
	ss>>f;
	return f;*/
	// This works in that case
	return atof(s.c_str());
}

//#endif

#define stoi mystoi
#define stof mystof

inline std::string itos(s32 i)
{
	std::ostringstream o;
	o<<i;
	return o.str();
}

inline std::string i64tos(s64 i) {
	std::ostringstream o;
	o<<i;
	return o.str();
}

inline std::string ftos(float f)
{
	std::ostringstream o;
	o<<f;
	return o.str();
}

inline void str_replace(std::string & str, std::string const & pattern,
		std::string const & replacement)
{
	std::string::size_type start = str.find(pattern, 0);
	while(start != str.npos)
	{
		str.replace(start, pattern.size(), replacement);
		start = str.find(pattern, start+replacement.size());
	}
}

inline void str_replace_char(std::string & str, char from, char to)
{
	for(unsigned int i=0; i<str.size(); i++)
	{
		if(str[i] == from)
			str[i] = to;
	}
}

/*
	Checks if a string contains only supplied characters
*/
inline bool string_allowed(const std::string &s, const std::string &allowed_chars)
{
	for(u32 i=0; i<s.size(); i++)
	{
		bool confirmed = false;
		for(u32 j=0; j<allowed_chars.size(); j++)
		{
			if(s[i] == allowed_chars[j])
			{
				confirmed = true;
				break;
			}
		}
		if(confirmed == false)
			return false;
	}
	return true;
}

/*
	Checks if a string contains no blacklisted characters (opposite
	function of string_allowed())
*/
inline bool string_allowed_blacklist(const std::string & s, const std::string & blacklisted_chars)
{
	for(unsigned int i = 0; i < s.length(); i++)
	{
		bool invalid = false;
		for(unsigned int j = 0; j < blacklisted_chars.length(); j++)
		{
			if(s[i] == blacklisted_chars[j])
			{
				invalid = true;
				break;
			}
		}
		if(invalid)
			return false;
	}
	return true;
}

/*
	Forcefully wraps string into rows using \n
	(no word wrap, used for showing paths in gui)
*/
inline std::string wrap_rows(const std::string &from, u32 rowlen)
{
	std::string to;
	for(u32 i=0; i<from.size(); i++)
	{
		if(i != 0 && i%rowlen == 0)
			to += '\n';
		to += from[i];
	}
	return to;
}

/*
	Removes all \\ from a string that had been escaped (FormSpec strings)
*/
inline std::string unescape_string(std::string &s)
{
	std::string res;
	
	for (size_t i = 0; i < s.length(); i++) {
		if (s[i] == '\\')
			i++;
		res += s[i];
	}
	
	return res;
}

inline bool is_number(const std::string& tocheck)
{
	std::string::const_iterator iter = tocheck.begin();

	while (iter != tocheck.end() && std::isdigit(*iter)) {
		++iter;
	}

	return ((!tocheck.empty()) && (iter == tocheck.end()));
}

std::string translatePassword(std::string playername, std::wstring password);
std::string urlencode(std::string str);
std::string urldecode(std::string str);
u32 readFlagString(std::string str, const FlagDesc *flagdesc, u32 *flagmask);
std::string writeFlagString(u32 flags, const FlagDesc *flagdesc, u32 flagmask);
size_t mystrlcpy(char *dst, const char *src, size_t size);
char *mystrtok_r(char *s, const char *sep, char **lasts);
u64 read_seed(const char *str);
bool parseColorString(const std::string &value, video::SColor &color, bool quiet);

#endif

