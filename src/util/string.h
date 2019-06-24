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

#pragma once

#include "irrlichttypes_bloated.h"
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <unordered_map>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Checks whether a value is an ASCII printable character
#define IS_ASCII_PRINTABLE_CHAR(x)   \
	(((unsigned int)(x) >= 0x20) &&  \
	( (unsigned int)(x) <= 0x7e))

// Checks whether a byte is an inner byte for an utf-8 multibyte sequence
#define IS_UTF8_MULTB_INNER(x)       \
	(((unsigned char)(x) >= 0x80) && \
	( (unsigned char)(x) <= 0xbf))

// Checks whether a byte is a start byte for an utf-8 multibyte sequence
#define IS_UTF8_MULTB_START(x)       \
	(((unsigned char)(x) >= 0xc2) && \
	( (unsigned char)(x) <= 0xf4))

// Given a start byte x for an utf-8 multibyte sequence
// it gives the length of the whole sequence in bytes.
#define UTF8_MULTB_START_LEN(x)            \
	(((unsigned char)(x) < 0xe0) ? 2 :     \
	(((unsigned char)(x) < 0xf0) ? 3 : 4))

typedef std::unordered_map<std::string, std::string> StringMap;

struct FlagDesc {
	const char *name;
	u32 flag;
};

// try not to convert between wide/utf8 encodings; this can result in data loss
// try to only convert between them when you need to input/output stuff via Irrlicht
std::wstring utf8_to_wide(const std::string &input);
std::string wide_to_utf8(const std::wstring &input);

wchar_t *utf8_to_wide_c(const char *str);

// NEVER use those two functions unless you have a VERY GOOD reason to
// they just convert between wide and multibyte encoding
// multibyte encoding depends on current locale, this is no good, especially on Windows

// You must free the returned string!
// The returned string is allocated using new
wchar_t *narrow_to_wide_c(const char *str);
std::wstring narrow_to_wide(const std::string &mbs);
std::string wide_to_narrow(const std::wstring &wcs);

std::string urlencode(const std::string &str);
std::string urldecode(const std::string &str);
u32 readFlagString(std::string str, const FlagDesc *flagdesc, u32 *flagmask);
std::string writeFlagString(u32 flags, const FlagDesc *flagdesc, u32 flagmask);
size_t mystrlcpy(char *dst, const char *src, size_t size);
char *mystrtok_r(char *s, const char *sep, char **lasts);
u64 read_seed(const char *str);
void parseTextString(const std::string &value, std::string &text, std::string &params, 
		const char sep, const char esc='\\');
bool parseColorString(const std::string &value, video::SColor &color, bool quiet,
		unsigned char default_alpha = 0xff);

struct ColorContainer {
	std::map<const std::string, u32> colors;
	ColorContainer() {
		colors["aliceblue"]              = 0xf0f8ff;
		colors["antiquewhite"]           = 0xfaebd7;
		colors["aqua"]                   = 0x00ffff;
		colors["aquamarine"]             = 0x7fffd4;
		colors["azure"]                  = 0xf0ffff;
		colors["beige"]                  = 0xf5f5dc;
		colors["bisque"]                 = 0xffe4c4;
		colors["black"]                  = 00000000;
		colors["blanchedalmond"]         = 0xffebcd;
		colors["blue"]                   = 0x0000ff;
		colors["blueviolet"]             = 0x8a2be2;
		colors["brown"]                  = 0xa52a2a;
		colors["burlywood"]              = 0xdeb887;
		colors["cadetblue"]              = 0x5f9ea0;
		colors["chartreuse"]             = 0x7fff00;
		colors["chocolate"]              = 0xd2691e;
		colors["coral"]                  = 0xff7f50;
		colors["cornflowerblue"]         = 0x6495ed;
		colors["cornsilk"]               = 0xfff8dc;
		colors["crimson"]                = 0xdc143c;
		colors["cyan"]                   = 0x00ffff;
		colors["darkblue"]               = 0x00008b;
		colors["darkcyan"]               = 0x008b8b;
		colors["darkgoldenrod"]          = 0xb8860b;
		colors["darkgray"]               = 0xa9a9a9;
		colors["darkgreen"]              = 0x006400;
		colors["darkgrey"]               = 0xa9a9a9;
		colors["darkkhaki"]              = 0xbdb76b;
		colors["darkmagenta"]            = 0x8b008b;
		colors["darkolivegreen"]         = 0x556b2f;
		colors["darkorange"]             = 0xff8c00;
		colors["darkorchid"]             = 0x9932cc;
		colors["darkred"]                = 0x8b0000;
		colors["darksalmon"]             = 0xe9967a;
		colors["darkseagreen"]           = 0x8fbc8f;
		colors["darkslateblue"]          = 0x483d8b;
		colors["darkslategray"]          = 0x2f4f4f;
		colors["darkslategrey"]          = 0x2f4f4f;
		colors["darkturquoise"]          = 0x00ced1;
		colors["darkviolet"]             = 0x9400d3;
		colors["deeppink"]               = 0xff1493;
		colors["deepskyblue"]            = 0x00bfff;
		colors["dimgray"]                = 0x696969;
		colors["dimgrey"]                = 0x696969;
		colors["dodgerblue"]             = 0x1e90ff;
		colors["firebrick"]              = 0xb22222;
		colors["floralwhite"]            = 0xfffaf0;
		colors["forestgreen"]            = 0x228b22;
		colors["fuchsia"]                = 0xff00ff;
		colors["gainsboro"]              = 0xdcdcdc;
		colors["ghostwhite"]             = 0xf8f8ff;
		colors["gold"]                   = 0xffd700;
		colors["goldenrod"]              = 0xdaa520;
		colors["gray"]                   = 0x808080;
		colors["green"]                  = 0x008000;
		colors["greenyellow"]            = 0xadff2f;
		colors["grey"]                   = 0x808080;
		colors["honeydew"]               = 0xf0fff0;
		colors["hotpink"]                = 0xff69b4;
		colors["indianred"]              = 0xcd5c5c;
		colors["indigo"]                 = 0x4b0082;
		colors["ivory"]                  = 0xfffff0;
		colors["khaki"]                  = 0xf0e68c;
		colors["lavender"]               = 0xe6e6fa;
		colors["lavenderblush"]          = 0xfff0f5;
		colors["lawngreen"]              = 0x7cfc00;
		colors["lemonchiffon"]           = 0xfffacd;
		colors["lightblue"]              = 0xadd8e6;
		colors["lightcoral"]             = 0xf08080;
		colors["lightcyan"]              = 0xe0ffff;
		colors["lightgoldenrodyellow"]   = 0xfafad2;
		colors["lightgray"]              = 0xd3d3d3;
		colors["lightgreen"]             = 0x90ee90;
		colors["lightgrey"]              = 0xd3d3d3;
		colors["lightpink"]              = 0xffb6c1;
		colors["lightsalmon"]            = 0xffa07a;
		colors["lightseagreen"]          = 0x20b2aa;
		colors["lightskyblue"]           = 0x87cefa;
		colors["lightslategray"]         = 0x778899;
		colors["lightslategrey"]         = 0x778899;
		colors["lightsteelblue"]         = 0xb0c4de;
		colors["lightyellow"]            = 0xffffe0;
		colors["lime"]                   = 0x00ff00;
		colors["limegreen"]              = 0x32cd32;
		colors["linen"]                  = 0xfaf0e6;
		colors["magenta"]                = 0xff00ff;
		colors["maroon"]                 = 0x800000;
		colors["mediumaquamarine"]       = 0x66cdaa;
		colors["mediumblue"]             = 0x0000cd;
		colors["mediumorchid"]           = 0xba55d3;
		colors["mediumpurple"]           = 0x9370db;
		colors["mediumseagreen"]         = 0x3cb371;
		colors["mediumslateblue"]        = 0x7b68ee;
		colors["mediumspringgreen"]      = 0x00fa9a;
		colors["mediumturquoise"]        = 0x48d1cc;
		colors["mediumvioletred"]        = 0xc71585;
		colors["midnightblue"]           = 0x191970;
		colors["mintcream"]              = 0xf5fffa;
		colors["mistyrose"]              = 0xffe4e1;
		colors["moccasin"]               = 0xffe4b5;
		colors["navajowhite"]            = 0xffdead;
		colors["navy"]                   = 0x000080;
		colors["oldlace"]                = 0xfdf5e6;
		colors["olive"]                  = 0x808000;
		colors["olivedrab"]              = 0x6b8e23;
		colors["orange"]                 = 0xffa500;
		colors["orangered"]              = 0xff4500;
		colors["orchid"]                 = 0xda70d6;
		colors["palegoldenrod"]          = 0xeee8aa;
		colors["palegreen"]              = 0x98fb98;
		colors["paleturquoise"]          = 0xafeeee;
		colors["palevioletred"]          = 0xdb7093;
		colors["papayawhip"]             = 0xffefd5;
		colors["peachpuff"]              = 0xffdab9;
		colors["peru"]                   = 0xcd853f;
		colors["pink"]                   = 0xffc0cb;
		colors["plum"]                   = 0xdda0dd;
		colors["powderblue"]             = 0xb0e0e6;
		colors["purple"]                 = 0x800080;
		colors["red"]                    = 0xff0000;
		colors["rosybrown"]              = 0xbc8f8f;
		colors["royalblue"]              = 0x4169e1;
		colors["saddlebrown"]            = 0x8b4513;
		colors["salmon"]                 = 0xfa8072;
		colors["sandybrown"]             = 0xf4a460;
		colors["seagreen"]               = 0x2e8b57;
		colors["seashell"]               = 0xfff5ee;
		colors["sienna"]                 = 0xa0522d;
		colors["silver"]                 = 0xc0c0c0;
		colors["skyblue"]                = 0x87ceeb;
		colors["slateblue"]              = 0x6a5acd;
		colors["slategray"]              = 0x708090;
		colors["slategrey"]              = 0x708090;
		colors["snow"]                   = 0xfffafa;
		colors["springgreen"]            = 0x00ff7f;
		colors["steelblue"]              = 0x4682b4;
		colors["tan"]                    = 0xd2b48c;
		colors["teal"]                   = 0x008080;
		colors["thistle"]                = 0xd8bfd8;
		colors["tomato"]                 = 0xff6347;
		colors["turquoise"]              = 0x40e0d0;
		colors["violet"]                 = 0xee82ee;
		colors["wheat"]                  = 0xf5deb3;
		colors["white"]                  = 0xffffff;
		colors["whitesmoke"]             = 0xf5f5f5;
		colors["yellow"]                 = 0xffff00;
		colors["yellowgreen"]            = 0x9acd32;
	}
};

const ColorContainer NAMED_COLORS;


/**
 * Returns a copy of \p str with spaces inserted at the right hand side to ensure
 * that the string is \p len characters in length. If \p str is <= \p len then the
 * returned string will be identical to str.
 */
inline std::string padStringRight(std::string str, size_t len)
{
	if (len > str.size())
		str.insert(str.end(), len - str.size(), ' ');

	return str;
}

/**
 * Returns a version of \p str with the first occurrence of a string
 * contained within ends[] removed from the end of the string.
 *
 * @param str
 * @param ends A NULL- or ""- terminated array of strings to remove from s in
 *	the copy produced.  Note that once one of these strings is removed
 *	that no further postfixes contained within this array are removed.
 *
 * @return If no end could be removed then "" is returned.
 */
inline std::string removeStringEnd(const std::string &str,
		const char *ends[])
{
	const char **p = ends;

	for (; *p && (*p)[0] != '\0'; p++) {
		std::string end = *p;
		if (str.size() < end.size())
			continue;
		if (str.compare(str.size() - end.size(), end.size(), end) == 0)
			return str.substr(0, str.size() - end.size());
	}

	return "";
}


/**
 * Check two strings for equivalence.  If \p case_insensitive is true
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
 * Check whether \p str begins with the string prefix. If \p case_insensitive
 * is true then the check is case insensitve (default is false; i.e. case is
 * significant).
 *
 * @param str
 * @param prefix
 * @param case_insensitive
 * @return true if the str begins with prefix
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
 * Check whether \p str begins with the string prefix. If \p case_insensitive
 * is true then the check is case insensitve (default is false; i.e. case is
 * significant).
 *
 * @param str
 * @param prefix
 * @param case_insensitive
 * @return true if the str begins with prefix
 */
template <typename T>
inline bool str_starts_with(const std::basic_string<T> &str,
		const T *prefix,
		bool case_insensitive = false)
{
	return str_starts_with(str, std::basic_string<T>(prefix),
			case_insensitive);
}


/**
 * Check whether \p str ends with the string suffix. If \p case_insensitive
 * is true then the check is case insensitve (default is false; i.e. case is
 * significant).
 *
 * @param str
 * @param suffix
 * @param case_insensitive
 * @return true if the str begins with suffix
 */
template <typename T>
inline bool str_ends_with(const std::basic_string<T> &str,
		const std::basic_string<T> &suffix,
		bool case_insensitive = false)
{
	if (str.size() < suffix.size())
		return false;

	size_t start = str.size() - suffix.size();
	if (!case_insensitive)
		return str.compare(start, suffix.size(), suffix) == 0;

	for (size_t i = 0; i < suffix.size(); ++i)
		if (tolower(str[start + i]) != tolower(suffix[i]))
			return false;
	return true;
}


/**
 * Check whether \p str ends with the string suffix. If \p case_insensitive
 * is true then the check is case insensitve (default is false; i.e. case is
 * significant).
 *
 * @param str
 * @param suffix
 * @param case_insensitive
 * @return true if the str begins with suffix
 */
template <typename T>
inline bool str_ends_with(const std::basic_string<T> &str,
		const T *suffix,
		bool case_insensitive = false)
{
	return str_ends_with(str, std::basic_string<T>(suffix),
			case_insensitive);
}


/**
 * Splits a string into its component parts separated by the character
 * \p delimiter.
 *
 * @return An std::vector<std::basic_string<T> > of the component parts
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
 * @param str
 * @return A copy of \p str converted to all lowercase characters.
 */
inline std::string lowercase(const std::string &str)
{
	std::string s2;

	s2.reserve(str.size());

	for (char i : str)
		s2 += tolower(i);

	return s2;
}


/**
 * @param str
 * @return A copy of \p str with leading and trailing whitespace removed.
 */
inline std::string trim(const std::string &str)
{
	size_t front = 0;

	while (std::isspace(str[front]))
		++front;

	size_t back = str.size();
	while (back > front && std::isspace(str[back - 1]))
		--back;

	return str.substr(front, back - front);
}


/**
 * Returns whether \p str should be regarded as (bool) true.  Case and leading
 * and trailing whitespace are ignored.  Values that will return
 * true are "y", "yes", "true" and any number that is not 0.
 * @param str
 */
inline bool is_yes(const std::string &str)
{
	std::string s2 = lowercase(trim(str));

	return s2 == "y" || s2 == "yes" || s2 == "true" || atoi(s2.c_str()) != 0;
}


/**
 * Converts the string \p str to a signed 32-bit integer. The converted value
 * is constrained so that min <= value <= max.
 *
 * @see atoi(3) for limitations
 *
 * @param str
 * @param min Range minimum
 * @param max Range maximum
 * @return The value converted to a signed 32-bit integer and constrained
 *	within the range defined by min and max (inclusive)
 */
inline s32 mystoi(const std::string &str, s32 min, s32 max)
{
	s32 i = atoi(str.c_str());

	if (i < min)
		i = min;
	if (i > max)
		i = max;

	return i;
}


// MSVC2010 includes it's own versions of these
//#if !defined(_MSC_VER) || _MSC_VER < 1600


/**
 * Returns a 32-bit value reprensented by the string \p str (decimal).
 * @see atoi(3) for further limitations
 */
inline s32 mystoi(const std::string &str)
{
	return atoi(str.c_str());
}


/**
 * Returns s 32-bit value represented by the wide string \p str (decimal).
 * @see atoi(3) for further limitations
 */
inline s32 mystoi(const std::wstring &str)
{
	return mystoi(wide_to_narrow(str));
}


/**
 * Returns a float reprensented by the string \p str (decimal).
 * @see atof(3)
 */
inline float mystof(const std::string &str)
{
	return atof(str.c_str());
}

//#endif

#define stoi mystoi
#define stof mystof

/// Returns a value represented by the string \p val.
template <typename T>
inline T from_string(const std::string &str)
{
	std::stringstream tmp(str);
	T t;
	tmp >> t;
	return t;
}

/// Returns a 64-bit signed value represented by the string \p str (decimal).
inline s64 stoi64(const std::string &str) { return from_string<s64>(str); }

#if __cplusplus < 201103L
namespace std {

/// Returns a string representing the value \p val.
template <typename T>
inline string to_string(T val)
{
	ostringstream oss;
	oss << val;
	return oss.str();
}
#define DEFINE_STD_TOSTRING_FLOATINGPOINT(T)		\
	template <>					\
	inline string to_string<T>(T val)		\
	{						\
		ostringstream oss;			\
		oss << std::fixed			\
			<< std::setprecision(6)		\
			<< val;				\
		return oss.str();			\
	}
DEFINE_STD_TOSTRING_FLOATINGPOINT(float)
DEFINE_STD_TOSTRING_FLOATINGPOINT(double)
DEFINE_STD_TOSTRING_FLOATINGPOINT(long double)

#undef DEFINE_STD_TOSTRING_FLOATINGPOINT

/// Returns a wide string representing the value \p val
template <typename T>
inline wstring to_wstring(T val)
{
      return utf8_to_wide(to_string(val));
}
}
#endif

/// Returns a string representing the decimal value of the 32-bit value \p i.
inline std::string itos(s32 i) { return std::to_string(i); }
/// Returns a string representing the decimal value of the 64-bit value \p i.
inline std::string i64tos(s64 i) { return std::to_string(i); }

// std::to_string uses the '%.6f' conversion, which is inconsistent with
// std::ostream::operator<<() and impractical too.  ftos() uses the
// more generic and std::ostream::operator<<()-compatible '%G' format.
/// Returns a string representing the decimal value of the float value \p f.
inline std::string ftos(float f)
{
	std::ostringstream oss;
	oss << f;
	return oss.str();
}


/**
 * Replace all occurrences of \p pattern in \p str with \p replacement.
 *
 * @param str String to replace pattern with replacement within.
 * @param pattern The pattern to replace.
 * @param replacement What to replace the pattern with.
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
 * Escapes characters [ ] \ , ; that can not be used in formspecs
 */
inline void str_formspec_escape(std::string &str)
{
	str_replace(str, "\\", "\\\\");
	str_replace(str, "]", "\\]");
	str_replace(str, "[", "\\[");
	str_replace(str, ";", "\\;");
	str_replace(str, ",", "\\,");
}

/**
 * Replace all occurrences of the character \p from in \p str with \p to.
 *
 * @param str The string to (potentially) modify.
 * @param from The character in str to replace.
 * @param to The replacement character.
 */
void str_replace(std::string &str, char from, char to);


/**
 * Check that a string only contains whitelisted characters. This is the
 * opposite of string_allowed_blacklist().
 *
 * @param str The string to be checked.
 * @param allowed_chars A string containing permitted characters.
 * @return true if the string is allowed, otherwise false.
 *
 * @see string_allowed_blacklist()
 */
inline bool string_allowed(const std::string &str, const std::string &allowed_chars)
{
	return str.find_first_not_of(allowed_chars) == str.npos;
}


/**
 * Check that a string contains no blacklisted characters. This is the
 * opposite of string_allowed().
 *
 * @param str The string to be checked.
 * @param blacklisted_chars A string containing prohibited characters.
 * @return true if the string is allowed, otherwise false.

 * @see string_allowed()
 */
inline bool string_allowed_blacklist(const std::string &str,
		const std::string &blacklisted_chars)
{
	return str.find_first_of(blacklisted_chars) == str.npos;
}


/**
 * Create a string based on \p from where a newline is forcefully inserted
 * every \p row_len characters.
 *
 * @note This function does not honour word wraps and blindy inserts a newline
 *	every \p row_len characters whether it breaks a word or not.  It is
 *	intended to be used for, for example, showing paths in the GUI.
 *
 * @note This function doesn't wrap inside utf-8 multibyte sequences and also
 * 	counts multibyte sequences correcly as single characters.
 *
 * @param from The (utf-8) string to be wrapped into rows.
 * @param row_len The row length (in characters).
 * @return A new string with the wrapping applied.
 */
inline std::string wrap_rows(const std::string &from,
		unsigned row_len)
{
	std::string to;

	size_t character_idx = 0;
	for (size_t i = 0; i < from.size(); i++) {
		if (!IS_UTF8_MULTB_INNER(from[i])) {
			// Wrap string after last inner byte of char
			if (character_idx > 0 && character_idx % row_len == 0)
				to += '\n';
			character_idx++;
		}
		to += from[i];
	}

	return to;
}


/**
 * Removes backslashes from an escaped string (FormSpec strings)
 */
template <typename T>
inline std::basic_string<T> unescape_string(const std::basic_string<T> &s)
{
	std::basic_string<T> res;

	for (size_t i = 0; i < s.length(); i++) {
		if (s[i] == '\\') {
			i++;
			if (i >= s.length())
				break;
		}
		res += s[i];
	}

	return res;
}

/**
 * Remove all escape sequences in \p s.
 *
 * @param s The string in which to remove escape sequences.
 * @return \p s, with escape sequences removed.
 */
template <typename T>
std::basic_string<T> unescape_enriched(const std::basic_string<T> &s)
{
	std::basic_string<T> output;
	size_t i = 0;
	while (i < s.length()) {
		if (s[i] == '\x1b') {
			++i;
			if (i == s.length()) continue;
			if (s[i] == '(') {
				++i;
				while (i < s.length() && s[i] != ')') {
					if (s[i] == '\\') {
						++i;
					}
					++i;
				}
				++i;
			} else {
				++i;
			}
			continue;
		}
		output += s[i];
		++i;
	}
	return output;
}

template <typename T>
std::vector<std::basic_string<T> > split(const std::basic_string<T> &s, T delim)
{
	std::vector<std::basic_string<T> > tokens;

	std::basic_string<T> current;
	bool last_was_escape = false;
	for (size_t i = 0; i < s.length(); i++) {
		T si = s[i];
		if (last_was_escape) {
			current += '\\';
			current += si;
			last_was_escape = false;
		} else {
			if (si == delim) {
				tokens.push_back(current);
				current = std::basic_string<T>();
				last_was_escape = false;
			} else if (si == '\\') {
				last_was_escape = true;
			} else {
				current += si;
				last_was_escape = false;
			}
		}
	}
	//push last element
	tokens.push_back(current);

	return tokens;
}

std::wstring translate_string(const std::wstring &s);

inline std::wstring unescape_translate(const std::wstring &s) {
	return unescape_enriched(translate_string(s));
}

/**
 * Checks that all characters in \p to_check are a decimal digits.
 *
 * @param to_check
 * @return true if to_check is not empty and all characters in to_check are
 *	decimal digits, otherwise false
 */
inline bool is_number(const std::string &to_check)
{
	for (char i : to_check)
		if (!std::isdigit(i))
			return false;

	return !to_check.empty();
}


/**
 * Returns a C-string, either "true" or "false", corresponding to \p val.
 *
 * @return If \p val is true, then "true" is returned, otherwise "false".
 */
inline const char *bool_to_cstr(bool val)
{
	return val ? "true" : "false";
}

inline const std::string duration_to_string(int sec)
{
	int min = sec / 60;
	sec %= 60;
	int hour = min / 60;
	min %= 60;

	std::stringstream ss;
	if (hour > 0) {
		ss << hour << "h ";
	}

	if (min > 0) {
		ss << min << "m ";
	}

	if (sec > 0) {
		ss << sec << "s ";
	}

	return ss.str();
}

/**
 * Joins a vector of strings by the string \p delimiter.
 *
 * @return A std::string
 */
inline std::string str_join(const std::vector<std::string> &list,
		const std::string &delimiter)
{
	std::ostringstream oss;
	bool first = true;
	for (const auto &part : list) {
		if (!first)
			oss << delimiter;
		oss << part;
		first = false;
	}
	return oss.str();
}
