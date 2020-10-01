/*
Minetest
Copyright (C) 2017 Nore, Nathanaël Courant <nore@mesecons.net>

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
#include "util/string.h"
#include <unordered_map>


#ifndef SERVER
// Client translations
Translations client_translations;
Translations *g_client_translations = &client_translations;
#endif


void Translations::clear()
{
	m_translations.clear();
}

const std::wstring &Translations::getTranslation(
		const std::wstring &textdomain, const std::wstring &s)
{
	std::wstring key = textdomain + L"|" + s;
	try {
		return m_translations.at(key);
	} catch (const std::out_of_range &) {
		verbosestream << "Translations: can't find translation for string \""
		              << wide_to_utf8(s) << "\" in textdomain \""
		              << wide_to_utf8(textdomain) << "\"" << std::endl;
		// Silence that warning in the future
		m_translations[key] = s;
		return s;
	}
}

void Translations::loadTrTranslation(const std::string &data)
{
	std::istringstream is(data);
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
			textdomain = utf8_to_wide(trim(parts[1]));
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
			            << std::endl;
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

		std::wstring oword1 = word1.str(), oword2 = word2.str();
		if (!oword2.empty()) {
			std::wstring translation_index = textdomain + L"|";
			translation_index.append(oword1);
			m_translations[translation_index] = oword2;
		} else {
			infostream << "Ignoring empty translation for \""
				<< wide_to_utf8(oword1) << "\"" << std::endl;
		}
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
			case L'0' ... L'7': {
				size_t j = 0;
				wchar_t c = 0;
				for (; j < 3 && i+j < str.length() && L'0' <= str[i+j] && str[i+j] <= L'7'; j++) {
					c = c * 8 + (str[i+j] - L'0');
				}
				if (c <= 0xff)
					result.push_back(c);
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
				if ((c < 0xd800) || (0xe000 <= c && c <= 0xffff)) result.push_back(c);
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
				if (ok) wide_add_codepoint(result, c);
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
				if (ok) wide_add_codepoint(result, c);
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
	std::wstring translation_index;
	auto ctx = entry.find(L"msgctx");
	if (ctx != entry.end()) {
		translation_index = ctx->second + L"|";
	} else {
		translation_index = L"|";
	}
	translation_index.append(entry.at(L"msgid"));

	auto plural = entry.find(L"msgid_plural");
	if (plural == entry.end()) {
		auto translated = entry.find(L"msgstr");
		if (translated == entry.end()) {
			errorstream << "Could not load translation: entry for msgid \"" << wide_to_utf8(entry.at(L"msgid")) << "\" does not contain a msgstr field" << std::endl;
			return;
		}
		m_translations[translation_index] = translated->second;
	} else {
		errorstream << "Translations with plurals are unsupported for now" << std::endl;
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

void Translations::loadTranslation(const std::string &filename, const std::string &data) {
	const char *trExtension[] = { ".tr", NULL };
	const char *poExtension[] = { ".po", NULL };
	if (!removeStringEnd(filename, trExtension).empty()) {
		loadTrTranslation(data);
		return;
	} else if (!removeStringEnd(filename, poExtension).empty()) {
		loadPoTranslation(data);
		return;
	} else {
		errorstream << "loadTranslation called with invalid filename: \"" << filename << "\"" << std::endl;
		return;
	}
}
