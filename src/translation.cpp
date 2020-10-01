/*
Minetest
Copyright (C) 2017 Nore, Nathanaëlle Courant <nore@mesecons.net>

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

#include "translation.h"
#include "log.h"
#include "util/hex.h"
#include "util/string.h"
#include "gettext.h"
#include <unordered_map>


#ifndef SERVER
// Client translations
static Translations client_translations;
Translations *g_client_translations = &client_translations;
#endif


void Translations::clear()
{
	m_translations.clear();
	m_plural_translations.clear();
}

const std::wstring &Translations::getTranslation(
		const std::wstring &textdomain, const std::wstring &s) const
{
	std::wstring key = textdomain + L"|"; key.append(s);
	auto it = m_translations.find(key);
	if (it != m_translations.end())
		return it->second;
	return s;
}

const std::wstring &Translations::getPluralTranslation(
		const std::wstring &textdomain, const std::wstring &s, unsigned long int number) const
{
	std::wstring key = textdomain + L"|"; key.append(s);

	/*~ DO NOT TRANSLATE THIS LITERALLY!
	This is a special string which needs to be translated to the translation plural identifier.

	If you edit the po file directly, you should see the following:

		msgid "0"
		msgid_plural "1"
		msgstr[0] ...
		...
		msgstr[n] ...

	You then need to translate msgstr[i] to the string containing the integer i, like this:

		msgstr[0] "0"


	If you edit the po file using weblate or another po file editor, the translations should normally
	be 0, 1, ... in this order. In case of doubt, the translations for each case should match the value
	returned by the formula for Plural-Forms. */
	std::string ns = ngettext("0", "1", number);
	unsigned int n = 0;
	static bool warned = false;
	if (ns.length() == 1 && '0' <= ns[0] && ns[0] <= '9') {
		n = ns[0] - '0';
	} else if (!warned) {
		warned = true;
		errorstream << "Translations: plurals are not translated correctly in the current language; got \""
		            << ns << "\" instead of the integer selecting which plural to use" << std::endl;
	}

  auto it = m_plural_translations.find(key);
  if (it != m_plural_translations.end()) {
	  const std::vector<std::wstring> &v = it->second;
	  if (n < v.size()) {
		  return v[n];
	  }
  }
  return s;
}


void Translations::addTranslation(
		const std::wstring &textdomain, const std::wstring &original, const std::wstring &translated)
{
	std::wstring key = textdomain + L"|"; key.append(original);
	if (!translated.empty()) {
		m_translations.emplace(std::move(key), std::move(translated));
	}
}

void Translations::addPluralTranslation(
		const std::wstring &textdomain, const std::wstring &original, std::vector<std::wstring> &translated)
{
	std::wstring key = textdomain + L"|"; key.append(original);
	m_plural_translations.emplace(std::move(key), std::move(translated));
}


void Translations::loadTrTranslation(const std::string &data)
{
	std::istringstream is(data);
	std::string textdomain_narrow;
	std::wstring textdomain;
	std::string line;

	while (is.good()) {
		std::getline(is, line);
		// Trim last character if file was using a \r\n line ending
		if (line.length () > 0 && line[line.length() - 1] == '\r')
			line.resize(line.length() - 1);

		if (str_starts_with(line, "# textdomain:")) {
			auto parts = str_split(line, ':');
			if (parts.size() < 2) {
				errorstream << "Invalid textdomain translation line \"" << line
						<< "\"" << std::endl;
				continue;
			}
			textdomain_narrow = trim(parts[1]);
			textdomain = utf8_to_wide(textdomain_narrow);
		}
		if (line.empty() || line[0] == '#')
			continue;

		std::wstring wline = utf8_to_wide(line);
		if (wline.empty())
			continue;

		// Read line
		// '=' marks the key-value pair, but may be escaped by an '@'.
		// '\n' may also be escaped by '@'.
		// All other escapes are preserved.

		size_t i = 0;
		std::wostringstream word1, word2;
		while (i < wline.length() && wline[i] != L'=') {
			if (wline[i] == L'@') {
				if (i + 1 < wline.length()) {
					if (wline[i + 1] == L'=') {
						word1.put(L'=');
					} else if (wline[i + 1] == L'n') {
						word1.put(L'\n');
					} else {
						word1.put(L'@');
						word1.put(wline[i + 1]);
					}
					i += 2;
				} else {
					// End of line, go to the next one.
					word1.put(L'\n');
					if (!is.good()) {
						break;
					}
					i = 0;
					std::getline(is, line);
					wline = utf8_to_wide(line);
				}
			} else {
				word1.put(wline[i]);
				i++;
			}
		}

		if (i == wline.length()) {
			errorstream << "Malformed translation line \"" << line << "\""
			            << " in text domain " << textdomain_narrow << std::endl;
			continue;
		}
		i++;

		while (i < wline.length()) {
			if (wline[i] == L'@') {
				if (i + 1 < wline.length()) {
					if (wline[i + 1] == L'=') {
						word2.put(L'=');
					} else if (wline[i + 1] == L'n') {
						word2.put(L'\n');
					} else {
						word2.put(L'@');
						word2.put(wline[i + 1]);
					}
					i += 2;
				} else {
					// End of line, go to the next one.
					word2.put(L'\n');
					if (!is.good()) {
						break;
					}
					i = 0;
					std::getline(is, line);
					wline = utf8_to_wide(line);
				}
			} else {
				word2.put(wline[i]);
				i++;
			}
		}

		addTranslation(textdomain, word1.str(), word2.str());
	}
}


std::wstring Translations::unescapeC(const std::wstring &str) {
	// Process escape sequences in str as if it were a C string
	std::wstring result;
	size_t i = 0;
	while (i < str.length()) {
		if (str[i] != L'\\') {
			result.push_back(str[i]);
			i++;
			continue;
		}
		i++;
		if (i == str.length()) {
			errorstream << "Unfinished escape sequence at the end of \"" << wide_to_utf8(str) << "\"" << std::endl;
			break;
		}
		switch (str[i]) {
			// From https://en.wikipedia.org/wiki/Escape_sequences_in_C#Table_of_escape_sequences
			case L'a': result.push_back(L'\a'); break;
			case L'b': result.push_back(L'\b'); break;
			case L'e': result.push_back(L'\x1b'); break;
			case L'f': result.push_back(L'\f'); break;
			case L'n': result.push_back(L'\n'); break;
			case L'r': result.push_back(L'\r'); break;
			case L't': result.push_back(L'\t'); break;
			case L'v': result.push_back(L'\v'); break;
			case L'\\': result.push_back(L'\\'); break;
			case L'\'': result.push_back(L'\''); break;
			case L'"': result.push_back(L'"'); break;
			case L'?': result.push_back(L'?'); break;
			case L'0': case L'1': case L'2': case L'3': case L'4': case L'5': case L'6': case L'7': {
				size_t j = 0;
				wchar_t c = 0;
				for (; j < 3 && i+j < str.length() && L'0' <= str[i+j] && str[i+j] <= L'7'; j++) {
					c = c * 8 + (str[i+j] - L'0');
				}
				if (c <= 0xff) {
					result.push_back(c);
				}
				i += j;
				continue;
			}
			case L'x': {
				i++;
				if (i >= str.length()) {
					errorstream << "Unfinished escape sequence at the end of \"" << wide_to_utf8(str) << "\"" << std::endl;
				}
				char32_t c = 0;
				size_t j = 0;
				unsigned char v;
				for (; i+j < str.length() && hex_digit_decode((char)str[i+j], v); j++) {
					c = c * 16 + v;
				}
				if (j == 0) {
					errorstream << "Invalid escape sequence \\x, ignoring" << std::endl;
					continue;
				}
				// If character fits in 16 bits and is not part of surrogate pair, insert it.
				// Otherwise, silently drop it: this is valid since \x escape sequences with
				// values above 0xff are implementation-defined
				if ((c < 0xd800) || (0xe000 <= c && c <= 0xffff)) {
					result.push_back(c);
				}
				i += j;
				continue;
			}
			case L'u': {
				i++;
				if (i + 4 > str.length()) {
					errorstream << "Unfinished escape sequence at the end of \"" << wide_to_utf8(str) << "\"" << std::endl;
				}
				char16_t c = 0;
				bool ok = true;
				for (size_t j = 0; j < 4; j++) {
					unsigned char v;
					if (str[i+j] <= 0xff && hex_digit_decode((char)str[i+j], v)) {
						c = c * 16 + v;
					} else {
						errorstream << "Invalid unicode escape sequence \"\\u" << wide_to_utf8(str.substr(i, 4)) << "\", ignoring" << std::endl;
						ok = false;
						break;
					}
				}
				if (ok) {
					wide_add_codepoint(result, c);
				}
				i += 4;
				continue;
			}
			case L'U': {
				i++;
				if (i + 8 > str.length()) {
					errorstream << "Unfinished escape sequence at the end of \"" << wide_to_utf8(str) << "\"" << std::endl;
				}
				char32_t c = 0;
				bool ok = true;
				for (size_t j = 0; j < 8; j++) {
					unsigned char v;
					if (str[i+j] <= 0xff && hex_digit_decode((char)str[i+j], v)) {
						c = c * 16 + v;
					} else {
						errorstream << "Invalid unicode escape sequence \"\\U" << wide_to_utf8(str.substr(i, 8)) << "\", ignoring" << std::endl;
						ok = false;
						break;
					}
				}
				if (ok) {
					wide_add_codepoint(result, c);
				}
				i += 8;
				continue;
			}
			default: {
				errorstream << "Unknown escape sequence \"\\" << str[i] << "\", ignoring" << std::endl;
				break;
			}
		}
		i++;
	}
	return result;
}

void Translations::loadPoEntry(const std::map<std::wstring, std::wstring> &entry) {
	// Process an entry from a PO file and add it to the translation table
	// Assumes that entry[L"msgid"] is always defined
	std::wstring textdomain;
	auto ctx = entry.find(L"msgctxt");
	if (ctx != entry.end()) {
		textdomain = ctx->second;
	} else {
		textdomain = L"";
	}
	std::wstring original = entry.at(L"msgid");

	auto plural = entry.find(L"msgid_plural");
	if (plural == entry.end()) {
		auto translated = entry.find(L"msgstr");
		if (translated == entry.end()) {
			errorstream << "Could not load translation: entry for msgid \"" << wide_to_utf8(original) << "\" does not contain a msgstr field" << std::endl;
			return;
		}
		addTranslation(textdomain, original, translated->second);
	} else {
		std::vector<std::wstring> translations;
		for (int i = 0; ; i++) {
			auto translated = entry.find(L"msgstr[" + std::to_wstring(i) + L"]");
			if (translated == entry.end()) break;
			translations.push_back(translated->second);
		}
		if (translations.size() == 0) {
			errorstream << "Could not load translation: entry for msgid\"" << wide_to_utf8(original) << "\" does not contain a msgstr[0] field" << std::endl;
			return;
		}
		addPluralTranslation(textdomain, original, translations);
		addPluralTranslation(textdomain, plural->second, translations);
	}
}

void Translations::loadPoTranslation(const std::string &data) {
	std::istringstream is(data);
	std::string line;
	std::map<std::wstring, std::wstring> last_entry;
	std::wstring last_key;

	while (is.good()) {
		std::getline(is, line);
		// Trim last character if file was using a \r\n line ending
		if (line.length () > 0 && line[line.length() - 1] == '\r')
			line.resize(line.length() - 1);

		if (line.empty() || line[0] == '#')
			continue;

		std::wstring wline = utf8_to_wide(line);
		// Defend against some possibly malformed utf8 string, which
		// is empty after converting to wide string
		if (wline.empty())
			continue;

		if (wline[0] == L'"') {
			// Continuation of previous line
			if (wline.length() < 2 || wline[wline.length()-1] != L'"') {
				errorstream << "Unable to parse po file line: " << line << std::endl;
				continue;
			}

			if (last_key == L"") {
				errorstream << "Unable to parse po file: continuation of non-existant previous line" << std::endl;
				continue;
			}

			std::wstring s = unescapeC(wline.substr(1, wline.length() - 2));
			last_entry[last_key].append(s);
			continue;
		}

		std::size_t pos = wline.find(' ');
		if (pos == std::wstring::npos || wline.length() < pos+3 || wline[pos+1] != L'"' || wline[wline.length() - 1] != L'"') {
			errorstream << "Unable to parse po file line: " << line << std::endl;
			continue;			
		}
		std::wstring prefix = wline.substr(0, pos);
		std::wstring s = unescapeC(wline.substr(pos+2, wline.length()-pos-3));

		if (prefix == L"msgctxt" || (prefix == L"msgid" && last_entry.find(L"msgid") != last_entry.end())) {
			if (last_entry.find(L"msgid") != last_entry.end()) {
				loadPoEntry(last_entry);
				last_entry.clear();
			} else if (!last_entry.empty()) {
				errorstream << "Unable to parse po file: previous entry has no \"msgid\" field but is not empty" << std::endl;
				last_entry.clear();
			}
		}
		if (last_entry.find(prefix) != last_entry.end()) {
			errorstream << "Unable to parse po file: Key \"" << wide_to_utf8(prefix) << "\" was already present in previous entry" << std::endl;
			continue;
		}
		last_key = prefix;
		last_entry[prefix] = s;
	}

	if (last_entry.find(L"msgid") != last_entry.end()) {
		loadPoEntry(last_entry);
	} else if (!last_entry.empty()) {
		errorstream << "Unable to parse po file: Last entry has no \"msgid\" field" << std::endl;
	}
}

void Translations::loadMoEntry(const std::string &original, const std::string &translated) {
	std::wstring textdomain = L"";
	size_t found;
	std::string noriginal = original;
	found = original.find('\x04'); // EOT character
	if (found != std::string::npos) {
		textdomain = utf8_to_wide(original.substr(0, found));
		noriginal = original.substr(found + 1);
	}

	found = noriginal.find('\0');
	if (found != std::string::npos) {
		std::vector<std::wstring> translations = str_split(utf8_to_wide(translated), L'\0');
		addPluralTranslation(textdomain, utf8_to_wide(noriginal.substr(0, found)), translations);
		addPluralTranslation(textdomain, utf8_to_wide(noriginal.substr(found + 1)), translations);
	} else {
		addTranslation(textdomain, utf8_to_wide(noriginal), utf8_to_wide(translated));
	}
}

inline u32 readVarEndian(bool is_be, const char *data)
{
	if (is_be) {
		return
			((u32)(unsigned char)data[0] << 24) | ((u32)(unsigned char)data[1] << 16) |
			((u32)(unsigned char)data[2] <<  8) | ((u32)(unsigned char)data[3] <<  0);
	} else {
		return
			((u32)(unsigned char)data[0] <<  0) | ((u32)(unsigned char)data[1] <<  8) |
			((u32)(unsigned char)data[2] << 16) | ((u32)(unsigned char)data[3] << 24);
	}
}

void Translations::loadMoTranslation(const std::string &data) {
	size_t length = data.length();
	const char* cdata = data.c_str();

	if (length < 20) {
		errorstream << "Ignoring too short mo file" << std::endl;
		return;
	}

	u32 magic = readVarEndian(false, cdata);
	bool is_be;
	if (magic == 0x950412de) {
		is_be = false;
	} else if (magic == 0xde120495) {
		is_be = true;
	} else {
		errorstream << "Bad magic number for mo file: 0x" << hex_encode(cdata, 4) << std::endl;
		return;
	}

	u32 revision = readVarEndian(is_be, cdata + 4);
	if (revision != 0) {
		errorstream << "Unknown revision " << revision << " for mo file" << std::endl;
		return;
	}

	u32 nstring = readVarEndian(is_be, cdata + 8);
	u32 original_offset = readVarEndian(is_be, cdata + 12);
	u32 translated_offset = readVarEndian(is_be, cdata + 16);

	if (length < original_offset + 8 * nstring || length < translated_offset + 8 * nstring) {
		errorstream << "Ignoring truncated mo file" << std::endl;
		return;
	}

	for (u32 i = 0; i < nstring; i++) {
		u32 original_len = readVarEndian(is_be, cdata + original_offset + 8 * i);
		u32 original_off = readVarEndian(is_be, cdata + original_offset + 8 * i + 4);
		u32 translated_len = readVarEndian(is_be, cdata + translated_offset + 8 * i);
		u32 translated_off = readVarEndian(is_be, cdata + translated_offset + 8 * i + 4);
		
		if (length < original_off + original_len || length < translated_off + translated_len) {
			errorstream << "Ignoring translation out of mo file" << std::endl;
			continue;
		}

		const std::string original(cdata + original_off, original_len);
		const std::string translated(cdata + translated_off, translated_len);

		loadMoEntry(original, translated);
	}
	
	return;
}

void Translations::loadTranslation(const std::string &filename, const std::string &data) {
	const char *trExtension[] = { ".tr", NULL };
	const char *poExtension[] = { ".po", NULL };
	const char *moExtension[] = { ".mo", NULL };
	if (!removeStringEnd(filename, trExtension).empty()) {
		loadTrTranslation(data);
	} else if (!removeStringEnd(filename, poExtension).empty()) {
		loadPoTranslation(data);
	} else if (!removeStringEnd(filename, moExtension).empty()) {
		loadMoTranslation(data);
	} else {
		errorstream << "loadTranslation called with invalid filename: \"" << filename << "\"" << std::endl;
	}
}
