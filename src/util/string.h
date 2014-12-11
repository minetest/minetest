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
std::string translatePassword(std::string playername, std::wstring password);
std::string urlencode(std::string str);
std::string urldecode(std::string str);
u32 readFlagString(std::string str, const FlagDesc *flagdesc, u32 *flagmask);
std::string writeFlagString(u32 flags, const FlagDesc *flagdesc, u32 flagmask);
size_t mystrlcpy(char *dst, const char *src, size_t size);
char *mystrtok_r(char *s, const char *sep, char **lasts);
u64 read_seed(const char *str);
bool parseColorString(const std::string &value, video::SColor &color, bool quiet);


/**
 * Returns a copy of s with spaces inserted at the right hand side to ensure
 *			that the string is len characters in length. If s is <= len then the
 *			returned string will be identical to s.
 */
inline std::string padStringRight(std::string s, size_t len)
{
	if (len > s.size())
		s.insert(s.end(), len - s.size(), ' ');

	return s;
}

/**
 * Returns a version of the string s with the first occurrence of a string
 * contained within ends[] removed from the end of the string.
 *
 * @param s
 * @param ends A NULL- or ""- terminated array of strings to remove from s in
 *			the copy produced. Note that once one of these strings is removed
 *			that no further postfixes contained within this array are removed.
 *
 * @return If no end could be removed then "" is returned
 */
inline std::string removeStringEnd(const std::string &s,
		const char *ends[])
{
	const char **p = ends;

	for (; *p && (*p)[0] != '\0'; p++) {
		std::string end = *p;
		if (s.size() < end.size())
			continue;
		if (s.compare(s.size() - end.size(), end.size(), end) == 0)
			return s.substr(0, s.size() - end.size());
	}

	return "";
}


/**
 * Check two strings for equivalence. If case_insensitive is true
 * then the case of the strings is ignored (default is false).
 *
 * @param s1
 * @param s2
 * @param case_insensitive
 * @return true if the strings match
 */
template <typename T>
inline bool str_equal(const std::basic_string<T> &s1,
		const std::basic_string<T> &s2,
		bool case_insensitive = false)
{
	if (!case_insensitive)
		return s1 == s2;

	if (s1.size() != s2.size())
		return false;

	for (size_t i = 0; i < s1.size(); ++i)
		if(tolower(s1[i]) != tolower(s2[i]))
			return false;

	return true;
}


/**
 * Check whether str begins with the string prefix. If the argument
 *			case_insensitive == true then the check is case insensitve (default
 *			is false; i.e. case is significant).
 *
 * @param str
 * @param prefix
 * @param case_insensitive
 * @return	true if the str begins with prefix
 */
template <typename T>
inline bool str_starts_with(const std::basic_string<T> &str,
		const std::basic_string<T> &prefix,
		bool case_insensitive = false)
{
	if (str.size() < prefix.size())
		return false;

	if (!case_insensitive)
		return str.compare(0, prefix.size(), prefix) == 0;

	for (size_t i = 0; i < prefix.size(); ++i)
		if (tolower(str[i]) != tolower(prefix[i]))
			return false;
	return true;
}


/**
 * Splits a string into its component parts separated by the character
 * delimiter.
 *
 * @return an std::vector<std::basic_string<T> > of the component parts
 */
template <typename T>
inline std::vector<std::basic_string<T> > str_split(
		const std::basic_string<T> &str,
		T delimiter)
{
	std::vector<std::basic_string<T> > parts;
	std::basic_stringstream<T> sstr(str);
	std::basic_string<T> part;

	while (std::getline(sstr, part, delimiter))
		parts.push_back(part);

	return parts;
}


/**
 * Return a copy of s converted to all lowercase characters
 * @param s
 */
inline std::string lowercase(const std::string &s)
{
	std::string s2;

	s2.reserve(s.size());

	for (size_t i = 0; i < s.size(); i++)
		s2 += tolower(s[i]);

	return s2;
}


/**
 * Returns a copy of s with leading and trailing whitespace removed.
 * @param s
 */
inline std::string trim(const std::string &s)
{
	size_t front = 0;

	while (std::isspace(s[front]))
		++front;

	size_t back = s.size();
	while (back > front && std::isspace(s[back - 1]))
		--back;

	return s.substr(front, back - front);
}


/**
 * Returns true if s should be regarded as (bool) true. Leading and trailing
 *			whitespace are ignored; case is ignored. Values that will return
 *			true are "y", "yes", "true" and any number that != 0.
 * @param s
 */
inline bool is_yes(const std::string &s)
{
	std::string s2 = lowercase(trim(s));

	return s2 == "y" || s2 == "yes" || s2 == "true" || atoi(s2.c_str()) != 0;
}


/**
 * Converts the string s to a signed 32-bit integer. The converted value is
 * constrained so that min <= value <= max.
 *
 * @see atoi(3) for limitations
 *
 * @param s
 * @param min Range minimum
 * @param max Range maximum
 * @return The value converted to a signed 32-bit integer and constrained
 *			within the range defined by min and max (inclusive)
 */
inline s32 mystoi(const std::string &s, s32 min, s32 max)
{
	s32 i = atoi(s.c_str());

	if (i < min)
		i = min;
	if (i > max)
		i = max;

	return i;
}


/**
 * Returns a 64-bit value reprensented by the string s (decimal).
 */
inline s64 stoi64(const std::string &s)
{
	std::stringstream tmp(s);
	s64 t;
	tmp >> t;
	return t;
}

// MSVC2010 includes it's own versions of these
//#if !defined(_MSC_VER) || _MSC_VER < 1600


/**
 * Returns a 32-bit value reprensented by the string s (decimal).
 *
 * @see atoi(3) for further limitations
 */
inline s32 mystoi(const std::string &s)
{
	return atoi(s.c_str());
}


/**
 * Returns a 32-bit value reprensented by the wide string s (decimal).
 *
 * @see atoi(3) for further limitations
 */
inline s32 mystoi(const std::wstring &s)
{
	return mystoi(wide_to_narrow(s));
}


/**
 * Returns a float reprensented by the string s (decimal).
 *
 * @see atof(3)
 */
inline float mystof(const std::string &s)
{
	return atof(s.c_str());
}

//#endif

#define stoi mystoi
#define stof mystof

// TODO: Replace with C++11 std::to_string.

/// Returns a string representing the value v
template <typename T>
inline std::string to_string(T v)
{
	std::ostringstream o;
	o << v;
	return o.str();
}

/// Returns a string representing the decimal value of the 32-bit value i
inline std::string itos(s32 i) { return to_string(i); }
/// Returns a string representing the decimal value of the 64-bit value i
inline std::string i64tos(s64 i) { return to_string(i); }
/// Returns a string representing the decimal value of the float value f
inline std::string ftos(float f) { return to_string(f); }


/**
 * Replace all occurrences of `pattern` in `str` with `replacement`
 *
 * @param str String to replace pattern with replacement within
 * @param pattern The pattern to replace
 * @param replacement What to replace the pattern with
 */
inline void str_replace(std::string &str, const std::string &pattern,
		const std::string &replacement)
{
	std::string::size_type start = str.find(pattern, 0);
	while (start != str.npos) {
		str.replace(start, pattern.size(), replacement);
		start = str.find(pattern, start + replacement.size());
	}
}


/**
 * Replace all occurrances of the character `from` in `str` with `to`.
 *
 * @param str The string to (potentially) modify
 * @param from The character in str to replace
 * @param to The replacement character
 */
void str_replace(std::string &str, char from, char to);


/**
 * Check that a string only contains whitelisted characters. This is the
 * opposite of string_allowed_blacklist().
 *
 * @param s The string to be checked.
 * @param allowed_chars A string containing permitted characters.
 * @return true if the string is allowed, otherwise false.
 *
 * @see string_allowed_blacklist()
 */
inline bool string_allowed(const std::string &s, const std::string &allowed_chars)
{
	return s.find_first_not_of(allowed_chars) == s.npos;
}


/**
 * Check that a string contains no blacklisted characters. This is the
 * opposite of string_allowed().
 *
 * @param s The string to be checked.
 * @param blacklisted_chars A string containing prohibited characters.
 * @return true if the string is allowed, otherwise false.

 * @see string_allowed()
 */
inline bool string_allowed_blacklist(const std::string &s,
		const std::string &blacklisted_chars)
{
	return s.find_first_of(blacklisted_chars) == s.npos;
}


/**
 * Create a string based on 'from' where a newline is forcefully inserted every
 * 'row_len' characters.
 *
 * @note This function does not honour word wraps and blindy inserts a newline
 *			every row_len characters whether it breaks a word or not. It is
 *			intended to be used, for example, showing paths in the GUI
 *
 * @param from The string to be wrapped into rows.
 * @param row_len The row length (in characters).
 * @return A new string with the wrapping applied.
 */
inline std::string wrap_rows(const std::string &from,
		unsigned row_len)
{
	std::string to;

	for (size_t i = 0; i < from.size(); i++) {
		if (i != 0 && i % row_len == 0)
			to += '\n';
		to += from[i];
	}

	return to;
}


/**
 * Removes all \\ from a string that had been escaped (FormSpec strings)
 *
 */
template <typename T>
inline std::basic_string<T> unescape_string(std::basic_string<T> &s)
{
	std::basic_string<T> res;
	
	for (size_t i = 0; i < s.length(); i++) {
		if (s[i] == '\\')
			i++;
		res += s[i];
	}
	
	return res;
}


/**
 * Checks that all characters in to_check are a decimal digits
 *
 * @param to_check
 * @return true if to_check is not empty and all characters in tocheck are
 *			decimal digits, otherwise false
 */
inline bool is_number(const std::string &to_check)
{
	for (size_t i = 0; i < to_check.size(); i++)
		if (!std::isdigit(to_check[i]))
			return false;

	return !to_check.empty();
}


/**
 * Returns a C-string, either "true" or "false", corresponding to v
 *
 * @return If v == true, then "true" is returned, otherwise "false"
 */
inline const char *bool_to_cstr(bool v)
{
	return v ? "true" : "false";
}

#endif
