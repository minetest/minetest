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

void Translations::loadTranslation(const std::string &data)
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
			textdomain = utf8_to_wide(trim(str_split(line, ':')[1]));
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
		if (oword2.empty()) {
			oword2 = oword1;
			errorstream << "Ignoring empty translation for \""
			            << wide_to_utf8(oword1) << "\"" << std::endl;
		}

		std::wstring translation_index = textdomain + L"|";
		translation_index.append(oword1);
		m_translations[translation_index] = oword2;
	}
}
