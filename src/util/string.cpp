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

#include "hex.h"
#include "porting.h"
#include "translation.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <iomanip>
#include <unordered_map>

#ifndef _WIN32
	#include <iconv.h>
#else
	#define _WIN32_WINNT 0x0501
	#include <windows.h>
#endif

#ifdef __NetBSD__
	#include <sys/param.h>
	#if __NetBSD_Version__ <= 999001500
		#define BSD_ICONV_USED
	#endif
#elif defined(_ICONV_H_) && (defined(__FreeBSD__) || defined(__OpenBSD__) || \
	defined(__DragonFly__))
	#define BSD_ICONV_USED
#endif

#ifndef _WIN32

static bool convert(const char *to, const char *from, char *outbuf,
		size_t *outbuf_size, char *inbuf, size_t inbuf_size)
{
	iconv_t cd = iconv_open(to, from);

#ifdef BSD_ICONV_USED
	const char *inbuf_ptr = inbuf;
#else
	char *inbuf_ptr = inbuf;
#endif
	char *outbuf_ptr = outbuf;

	size_t *inbuf_left_ptr = &inbuf_size;

	const size_t old_outbuf_size = *outbuf_size;
	size_t old_size = inbuf_size;
	while (inbuf_size > 0) {
		iconv(cd, &inbuf_ptr, inbuf_left_ptr, &outbuf_ptr, outbuf_size);
		if (inbuf_size == old_size) {
			iconv_close(cd);
			return false;
		}
		old_size = inbuf_size;
	}

	iconv_close(cd);
	*outbuf_size = old_outbuf_size - *outbuf_size;
	return true;
}

#ifdef __ANDROID__
// On Android iconv disagrees how big a wchar_t is for whatever reason
const char *DEFAULT_ENCODING = "UTF-32LE";
#elif defined(__NetBSD__)
	// NetBSD does not allow "WCHAR_T" as a charset input to iconv.
	#include <sys/endian.h>
	#if BYTE_ORDER == BIG_ENDIAN
	const char *DEFAULT_ENCODING = "UTF-32BE";
	#else
	const char *DEFAULT_ENCODING = "UTF-32LE";
	#endif
#else
const char *DEFAULT_ENCODING = "WCHAR_T";
#endif

std::wstring utf8_to_wide(const std::string &input)
{
	const size_t inbuf_size = input.length();
	// maximum possible size, every character is sizeof(wchar_t) bytes
	size_t outbuf_size = input.length() * sizeof(wchar_t);

	char *inbuf = new char[inbuf_size]; // intentionally NOT null-terminated
	memcpy(inbuf, input.c_str(), inbuf_size);
	std::wstring out;
	out.resize(outbuf_size / sizeof(wchar_t));

#if defined(__ANDROID__) || defined(__NetBSD__)
	SANITY_CHECK(sizeof(wchar_t) == 4);
#endif

	char *outbuf = reinterpret_cast<char*>(&out[0]);
	if (!convert(DEFAULT_ENCODING, "UTF-8", outbuf, &outbuf_size, inbuf, inbuf_size)) {
		infostream << "Couldn't convert UTF-8 string 0x" << hex_encode(input)
			<< " into wstring" << std::endl;
		delete[] inbuf;
		return L"<invalid UTF-8 string>";
	}
	delete[] inbuf;

	out.resize(outbuf_size / sizeof(wchar_t));
	return out;
}

std::string wide_to_utf8(const std::wstring &input)
{
	const size_t inbuf_size = input.length() * sizeof(wchar_t);
	// maximum possible size: utf-8 encodes codepoints using 1 up to 4 bytes
	size_t outbuf_size = input.length() * 4;

	char *inbuf = new char[inbuf_size]; // intentionally NOT null-terminated
	memcpy(inbuf, input.c_str(), inbuf_size);
	std::string out;
	out.resize(outbuf_size);

	if (!convert("UTF-8", DEFAULT_ENCODING, &out[0], &outbuf_size, inbuf, inbuf_size)) {
		infostream << "Couldn't convert wstring 0x" << hex_encode(inbuf, inbuf_size)
			<< " into UTF-8 string" << std::endl;
		delete[] inbuf;
		return "<invalid wide string>";
	}
	delete[] inbuf;

	out.resize(outbuf_size);
	return out;
}

#else // _WIN32

std::wstring utf8_to_wide(const std::string &input)
{
	size_t outbuf_size = input.size() + 1;
	wchar_t *outbuf = new wchar_t[outbuf_size];
	memset(outbuf, 0, outbuf_size * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, input.c_str(), input.size(),
		outbuf, outbuf_size);
	std::wstring out(outbuf);
	delete[] outbuf;
	return out;
}

std::string wide_to_utf8(const std::wstring &input)
{
	size_t outbuf_size = (input.size() + 1) * 6;
	char *outbuf = new char[outbuf_size];
	memset(outbuf, 0, outbuf_size);
	WideCharToMultiByte(CP_UTF8, 0, input.c_str(), input.size(),
		outbuf, outbuf_size, NULL, NULL);
	std::string out(outbuf);
	delete[] outbuf;
	return out;
}

#endif // _WIN32

wchar_t *utf8_to_wide_c(const char *str)
{
	std::wstring ret = utf8_to_wide(std::string(str));
	size_t len = ret.length();
	wchar_t *ret_c = new wchar_t[len + 1];
	memcpy(ret_c, ret.c_str(), (len + 1) * sizeof(wchar_t));
	return ret_c;
}


std::string urlencode(const std::string &str)
{
	// Encodes non-unreserved URI characters by a percent sign
	// followed by two hex digits. See RFC 3986, section 2.3.
	static const char url_hex_chars[] = "0123456789ABCDEF";
	std::ostringstream oss(std::ios::binary);
	for (unsigned char c : str) {
		if (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~') {
			oss << c;
		} else {
			oss << "%"
				<< url_hex_chars[(c & 0xf0) >> 4]
				<< url_hex_chars[c & 0x0f];
		}
	}
	return oss.str();
}

std::string urldecode(const std::string &str)
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
		} else {
			oss << str[i];
		}
	}
	return oss.str();
}

u32 readFlagString(std::string str, const FlagDesc *flagdesc, u32 *flagmask)
{
	u32 result = 0;
	u32 mask = 0;
	char *s = &str[0];
	char *flagstr;
	char *strpos = nullptr;

	while ((flagstr = strtok_r(s, ",", &strpos))) {
		s = nullptr;

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
		return nullptr;

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

static bool parseHexColorString(const std::string &value, video::SColor &color,
		unsigned char default_alpha)
{
	u8 components[] = {0x00, 0x00, 0x00, default_alpha}; // R,G,B,A

	size_t len = value.size();
	bool short_form;

	if (len == 9 || len == 7) // #RRGGBBAA or #RRGGBB
		short_form = false;
	else if (len == 5 || len == 4) // #RGBA or #RGB
		short_form = true;
	else
		return false;

	for (size_t pos = 1, cc = 0; pos < len; pos++, cc++) {
		if (short_form) {
			u8 d;
			if (!hex_digit_decode(value[pos], d))
				return false;

			components[cc] = (d & 0xf) << 4 | (d & 0xf);
		} else {
			u8 d1, d2;
			if (!hex_digit_decode(value[pos], d1) ||
					!hex_digit_decode(value[pos+1], d2))
				return false;

			components[cc] = (d1 & 0xf) << 4 | (d2 & 0xf);
			pos++; // skip the second digit -- it's already used
		}
	}

	color.setRed(components[0]);
	color.setGreen(components[1]);
	color.setBlue(components[2]);
	color.setAlpha(components[3]);

	return true;
}

const static std::unordered_map<std::string, u32> s_named_colors = {
	{"aliceblue",            0xf0f8ff},
	{"antiquewhite",         0xfaebd7},
	{"aqua",                 0x00ffff},
	{"aquamarine",           0x7fffd4},
	{"azure",                0xf0ffff},
	{"beige",                0xf5f5dc},
	{"bisque",               0xffe4c4},
	{"black",                00000000},
	{"blanchedalmond",       0xffebcd},
	{"blue",                 0x0000ff},
	{"blueviolet",           0x8a2be2},
	{"brown",                0xa52a2a},
	{"burlywood",            0xdeb887},
	{"cadetblue",            0x5f9ea0},
	{"chartreuse",           0x7fff00},
	{"chocolate",            0xd2691e},
	{"coral",                0xff7f50},
	{"cornflowerblue",       0x6495ed},
	{"cornsilk",             0xfff8dc},
	{"crimson",              0xdc143c},
	{"cyan",                 0x00ffff},
	{"darkblue",             0x00008b},
	{"darkcyan",             0x008b8b},
	{"darkgoldenrod",        0xb8860b},
	{"darkgray",             0xa9a9a9},
	{"darkgreen",            0x006400},
	{"darkgrey",             0xa9a9a9},
	{"darkkhaki",            0xbdb76b},
	{"darkmagenta",          0x8b008b},
	{"darkolivegreen",       0x556b2f},
	{"darkorange",           0xff8c00},
	{"darkorchid",           0x9932cc},
	{"darkred",              0x8b0000},
	{"darksalmon",           0xe9967a},
	{"darkseagreen",         0x8fbc8f},
	{"darkslateblue",        0x483d8b},
	{"darkslategray",        0x2f4f4f},
	{"darkslategrey",        0x2f4f4f},
	{"darkturquoise",        0x00ced1},
	{"darkviolet",           0x9400d3},
	{"deeppink",             0xff1493},
	{"deepskyblue",          0x00bfff},
	{"dimgray",              0x696969},
	{"dimgrey",              0x696969},
	{"dodgerblue",           0x1e90ff},
	{"firebrick",            0xb22222},
	{"floralwhite",          0xfffaf0},
	{"forestgreen",          0x228b22},
	{"fuchsia",              0xff00ff},
	{"gainsboro",            0xdcdcdc},
	{"ghostwhite",           0xf8f8ff},
	{"gold",                 0xffd700},
	{"goldenrod",            0xdaa520},
	{"gray",                 0x808080},
	{"green",                0x008000},
	{"greenyellow",          0xadff2f},
	{"grey",                 0x808080},
	{"honeydew",             0xf0fff0},
	{"hotpink",              0xff69b4},
	{"indianred",            0xcd5c5c},
	{"indigo",               0x4b0082},
	{"ivory",                0xfffff0},
	{"khaki",                0xf0e68c},
	{"lavender",             0xe6e6fa},
	{"lavenderblush",        0xfff0f5},
	{"lawngreen",            0x7cfc00},
	{"lemonchiffon",         0xfffacd},
	{"lightblue",            0xadd8e6},
	{"lightcoral",           0xf08080},
	{"lightcyan",            0xe0ffff},
	{"lightgoldenrodyellow", 0xfafad2},
	{"lightgray",            0xd3d3d3},
	{"lightgreen",           0x90ee90},
	{"lightgrey",            0xd3d3d3},
	{"lightpink",            0xffb6c1},
	{"lightsalmon",          0xffa07a},
	{"lightseagreen",        0x20b2aa},
	{"lightskyblue",         0x87cefa},
	{"lightslategray",       0x778899},
	{"lightslategrey",       0x778899},
	{"lightsteelblue",       0xb0c4de},
	{"lightyellow",          0xffffe0},
	{"lime",                 0x00ff00},
	{"limegreen",            0x32cd32},
	{"linen",                0xfaf0e6},
	{"magenta",              0xff00ff},
	{"maroon",               0x800000},
	{"mediumaquamarine",     0x66cdaa},
	{"mediumblue",           0x0000cd},
	{"mediumorchid",         0xba55d3},
	{"mediumpurple",         0x9370db},
	{"mediumseagreen",       0x3cb371},
	{"mediumslateblue",      0x7b68ee},
	{"mediumspringgreen",    0x00fa9a},
	{"mediumturquoise",      0x48d1cc},
	{"mediumvioletred",      0xc71585},
	{"midnightblue",         0x191970},
	{"mintcream",            0xf5fffa},
	{"mistyrose",            0xffe4e1},
	{"moccasin",             0xffe4b5},
	{"navajowhite",          0xffdead},
	{"navy",                 0x000080},
	{"oldlace",              0xfdf5e6},
	{"olive",                0x808000},
	{"olivedrab",            0x6b8e23},
	{"orange",               0xffa500},
	{"orangered",            0xff4500},
	{"orchid",               0xda70d6},
	{"palegoldenrod",        0xeee8aa},
	{"palegreen",            0x98fb98},
	{"paleturquoise",        0xafeeee},
	{"palevioletred",        0xdb7093},
	{"papayawhip",           0xffefd5},
	{"peachpuff",            0xffdab9},
	{"peru",                 0xcd853f},
	{"pink",                 0xffc0cb},
	{"plum",                 0xdda0dd},
	{"powderblue",           0xb0e0e6},
	{"purple",               0x800080},
	{"red",                  0xff0000},
	{"rosybrown",            0xbc8f8f},
	{"royalblue",            0x4169e1},
	{"saddlebrown",          0x8b4513},
	{"salmon",               0xfa8072},
	{"sandybrown",           0xf4a460},
	{"seagreen",             0x2e8b57},
	{"seashell",             0xfff5ee},
	{"sienna",               0xa0522d},
	{"silver",               0xc0c0c0},
	{"skyblue",              0x87ceeb},
	{"slateblue",            0x6a5acd},
	{"slategray",            0x708090},
	{"slategrey",            0x708090},
	{"snow",                 0xfffafa},
	{"springgreen",          0x00ff7f},
	{"steelblue",            0x4682b4},
	{"tan",                  0xd2b48c},
	{"teal",                 0x008080},
	{"thistle",              0xd8bfd8},
	{"tomato",               0xff6347},
	{"turquoise",            0x40e0d0},
	{"violet",               0xee82ee},
	{"wheat",                0xf5deb3},
	{"white",                0xffffff},
	{"whitesmoke",           0xf5f5f5},
	{"yellow",               0xffff00},
	{"yellowgreen",          0x9acd32}
};

static bool parseNamedColorString(const std::string &value, video::SColor &color)
{
	std::string color_name;
	std::string alpha_string;

	/* If the string has a # in it, assume this is the start of a specified
	 * alpha value (if it isn't the string is invalid and the error will be
	 * caught later on, either because the color name won't be found or the
	 * alpha value will fail conversion)
	 */
	size_t alpha_pos = value.find('#');
	if (alpha_pos != std::string::npos) {
		color_name = value.substr(0, alpha_pos);
		alpha_string = value.substr(alpha_pos + 1);
	} else {
		color_name = value;
	}

	color_name = lowercase(color_name);

	auto it = s_named_colors.find(color_name);
	if (it == s_named_colors.end())
		return false;

	u32 color_temp = it->second;

	/* An empty string for alpha is ok (none of the color table entries
	 * have an alpha value either). Color strings without an alpha specified
	 * are interpreted as fully opaque
	 */
	if (!alpha_string.empty()) {
		if (alpha_string.size() == 1) {
			u8 d;
			if (!hex_digit_decode(alpha_string[0], d))
				return false;

			color_temp |= ((d & 0xf) << 4 | (d & 0xf)) << 24;
		} else if (alpha_string.size() == 2) {
			u8 d1, d2;
			if (!hex_digit_decode(alpha_string[0], d1)
					|| !hex_digit_decode(alpha_string[1], d2))
				return false;

			color_temp |= ((d1 & 0xf) << 4 | (d2 & 0xf)) << 24;
		} else {
			return false;
		}
	} else {
		color_temp |= 0xff << 24; // Fully opaque
	}

	color = video::SColor(color_temp);

	return true;
}

bool parseColorString(const std::string &value, video::SColor &color, bool quiet,
		unsigned char default_alpha)
{
	bool success;

	if (value[0] == '#')
		success = parseHexColorString(value, color, default_alpha);
	else
		success = parseNamedColorString(value, color);

	if (!success && !quiet)
		errorstream << "Invalid color: \"" << value << "\"" << std::endl;

	return success;
}

void str_replace(std::string &str, char from, char to)
{
	std::replace(str.begin(), str.end(), from, to);
}

/* Translated strings have the following format:
 * \x1bT marks the beginning of a translated string
 * \x1bE marks its end
 *
 * \x1bF marks the beginning of an argument, and \x1bE its end.
 *
 * Arguments are *not* translated, as they may contain escape codes.
 * Thus, if you want a translated argument, it should be inside \x1bT/\x1bE tags as well.
 *
 * This representation is chosen so that clients ignoring escape codes will
 * see untranslated strings.
 *
 * For instance, suppose we have a string such as "@1 Wool" with the argument "White"
 * The string will be sent as "\x1bT\x1bF\x1bTWhite\x1bE\x1bE Wool\x1bE"
 * To translate this string, we extract what is inside \x1bT/\x1bE tags.
 * When we notice the \x1bF tag, we recursively extract what is there up to the \x1bE end tag,
 * translating it as well.
 * We get the argument "White", translated, and create a template string with "@1" instead of it.
 * We finally get the template "@1 Wool" that was used in the beginning, which we translate
 * before filling it again.
 */

void translate_all(const std::wstring &s, size_t &i,
		Translations *translations, std::wstring &res);

void translate_string(const std::wstring &s, Translations *translations,
		const std::wstring &textdomain, size_t &i, std::wstring &res)
{
	std::wostringstream output;
	std::vector<std::wstring> args;
	int arg_number = 1;
	while (i < s.length()) {
		// Not an escape sequence: just add the character.
		if (s[i] != '\x1b') {
			output.put(s[i]);
			// The character is a literal '@'; add it twice
			// so that it is not mistaken for an argument.
			if (s[i] == L'@')
				output.put(L'@');
			++i;
			continue;
		}

		// We have an escape sequence: locate it and its data
		// It is either a single character, or it begins with '('
		// and extends up to the following ')', with '\' as an escape character.
		++i;
		size_t start_index = i;
		size_t length;
		if (i == s.length()) {
			length = 0;
		} else if (s[i] == L'(') {
			++i;
			++start_index;
			while (i < s.length() && s[i] != L')') {
				if (s[i] == L'\\')
					++i;
				++i;
			}
			length = i - start_index;
			++i;
			if (i > s.length())
				i = s.length();
		} else {
			++i;
			length = 1;
		}
		std::wstring escape_sequence(s, start_index, length);

		// The escape sequence is now reconstructed.
		std::vector<std::wstring> parts = split(escape_sequence, L'@');
		if (parts[0] == L"E") {
			// "End of translation" escape sequence. We are done locating the string to translate.
			break;
		} else if (parts[0] == L"F") {
			// "Start of argument" escape sequence.
			// Recursively translate the argument, and add it to the argument list.
			// Add an "@n" instead of the argument to the template to translate.
			if (arg_number >= 10) {
				errorstream << "Ignoring too many arguments to translation" << std::endl;
				std::wstring arg;
				translate_all(s, i, translations, arg);
				args.push_back(arg);
				continue;
			}
			output.put(L'@');
			output << arg_number;
			++arg_number;
			std::wstring arg;
			translate_all(s, i, translations, arg);
			args.push_back(arg);
		} else {
			// This is an escape sequence *inside* the template string to translate itself.
			// This should not happen, show an error message.
			errorstream << "Ignoring escape sequence '"
				<< wide_to_utf8(escape_sequence) << "' in translation" << std::endl;
		}
	}

	std::wstring toutput;
	// Translate the template.
	if (translations != nullptr)
		toutput = translations->getTranslation(
				textdomain, output.str());
	else
		toutput = output.str();

	// Put back the arguments in the translated template.
	std::wostringstream result;
	size_t j = 0;
	while (j < toutput.length()) {
		// Normal character, add it to output and continue.
		if (toutput[j] != L'@' || j == toutput.length() - 1) {
			result.put(toutput[j]);
			++j;
			continue;
		}

		++j;
		// Literal escape for '@'.
		if (toutput[j] == L'@') {
			result.put(L'@');
			++j;
			continue;
		}

		// Here we have an argument; get its index and add the translated argument to the output.
		int arg_index = toutput[j] - L'1';
		++j;
		if (0 <= arg_index && (size_t)arg_index < args.size()) {
			result << args[arg_index];
		} else {
			// This is not allowed: show an error message
			errorstream << "Ignoring out-of-bounds argument escape sequence in translation" << std::endl;
		}
	}
	res = result.str();
}

void translate_all(const std::wstring &s, size_t &i,
		Translations *translations, std::wstring &res)
{
	std::wostringstream output;
	while (i < s.length()) {
		// Not an escape sequence: just add the character.
		if (s[i] != '\x1b') {
			output.put(s[i]);
			++i;
			continue;
		}

		// We have an escape sequence: locate it and its data
		// It is either a single character, or it begins with '('
		// and extends up to the following ')', with '\' as an escape character.
		size_t escape_start = i;
		++i;
		size_t start_index = i;
		size_t length;
		if (i == s.length()) {
			length = 0;
		} else if (s[i] == L'(') {
			++i;
			++start_index;
			while (i < s.length() && s[i] != L')') {
				if (s[i] == L'\\') {
					++i;
				}
				++i;
			}
			length = i - start_index;
			++i;
			if (i > s.length())
				i = s.length();
		} else {
			++i;
			length = 1;
		}
		std::wstring escape_sequence(s, start_index, length);

		// The escape sequence is now reconstructed.
		std::vector<std::wstring> parts = split(escape_sequence, L'@');
		if (parts[0] == L"E") {
			// "End of argument" escape sequence. Exit.
			break;
		} else if (parts[0] == L"T") {
			// Beginning of translated string.
			std::wstring textdomain;
			if (parts.size() > 1)
				textdomain = parts[1];
			std::wstring translated;
			translate_string(s, translations, textdomain, i, translated);
			output << translated;
		} else {
			// Another escape sequence, such as colors. Preserve it.
			output << std::wstring(s, escape_start, i - escape_start);
		}
	}

	res = output.str();
}

// Translate string server side
std::wstring translate_string(const std::wstring &s, Translations *translations)
{
	size_t i = 0;
	std::wstring res;
	translate_all(s, i, translations, res);
	return res;
}

// Translate string client side
std::wstring translate_string(const std::wstring &s)
{
#ifdef SERVER
	return translate_string(s, nullptr);
#else
	return translate_string(s, g_client_translations);
#endif
}

static const std::array<std::wstring, 22> disallowed_dir_names = {
	// Problematic filenames from here:
	// https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#file-and-directory-names
	L"CON",
	L"PRN",
	L"AUX",
	L"NUL",
	L"COM1",
	L"COM2",
	L"COM3",
	L"COM4",
	L"COM5",
	L"COM6",
	L"COM7",
	L"COM8",
	L"COM9",
	L"LPT1",
	L"LPT2",
	L"LPT3",
	L"LPT4",
	L"LPT5",
	L"LPT6",
	L"LPT7",
	L"LPT8",
	L"LPT9",
};

/**
 * List of characters that are blacklisted from created directories
 */
static const std::wstring disallowed_path_chars = L"<>:\"/\\|?*.";

/**
 * Sanitize the name of a new directory. This consists of two stages:
 * 1. Check for 'reserved filenames' that can't be used on some filesystems
 *	and add a prefix to them
 * 2. Remove 'unsafe' characters from the name by replacing them with '_'
 */
std::string sanitizeDirName(const std::string &str, const std::string &optional_prefix)
{
	std::wstring safe_name = utf8_to_wide(str);

	for (std::wstring disallowed_name : disallowed_dir_names) {
		if (str_equal(safe_name, disallowed_name, true)) {
			safe_name = utf8_to_wide(optional_prefix) + safe_name;
			break;
		}
	}

	for (unsigned long i = 0; i < safe_name.length(); i++) {
		bool is_valid = true;

		// Unlikely, but control characters should always be blacklisted
		if (safe_name[i] < 32) {
			is_valid = false;
		} else if (safe_name[i] < 128) {
			is_valid = disallowed_path_chars.find_first_of(safe_name[i])
					== std::wstring::npos;
		}

		if (!is_valid)
			safe_name[i] = '_';
	}

	return wide_to_utf8(safe_name);
}


void safe_print_string(std::ostream &os, const std::string &str)
{
	std::ostream::fmtflags flags = os.flags();
	os << std::hex;
	for (const char c : str) {
		if (IS_ASCII_PRINTABLE_CHAR(c) || IS_UTF8_MULTB_START(c) ||
				IS_UTF8_MULTB_INNER(c) || c == '\n' || c == '\t') {
			os << c;
		} else {
			os << '<' << std::setw(2) << (int)c << '>';
		}
	}
	os.setf(flags);
}
