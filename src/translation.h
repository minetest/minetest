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

#pragma once

#include "util/string.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

class Translations;
#ifndef SERVER
extern Translations *g_client_translations;
std::string get_client_language_code();
#endif

template<typename T> static std::vector<T> parse_language_list(const T& language)
{
	std::vector<T> list;
	size_t begin = 0, next;
	while ((next = language.find(':', begin)) != std::wstring::npos)
	{
		T entry = language.substr(begin, next-begin);
		if (!entry.empty())
			list.push_back(entry);
		begin = next+1;
	}
	T last = language.substr(begin);
	if (!last.empty())
		list.push_back(last);
	return list;
}

class Translations
{
public:
	Translations();

	void loadTranslation(const std::wstring &language, const std::string &data);
	inline void loadTranslation(const std::string &data) {
		loadTranslation(getPreferredPrimaryLanguage(), data);
	}
	void clear();

	const std::wstring &getTranslation(const std::vector<std::wstring> &languages,
			const std::wstring &textdomain, const std::wstring &s) const;
	inline const std::wstring &getTranslation(
			const std::wstring &textdomain, const std::wstring &s) const {
		return getTranslation(m_preferred_languages, textdomain, s);
	}

	inline const std::wstring getPreferredPrimaryLanguage() const {
		if (m_preferred_languages.empty())
			return L"";
		return m_preferred_languages[0];
	}
	inline const std::vector<std::wstring> &getPreferredLanguages() const {
		return m_preferred_languages;
	}
	inline void setPreferredLanguages(const std::vector<std::wstring> &languages) {
		m_preferred_languages = languages;
	}
	inline void setPreferredLanguages(const std::wstring &languages) {
		setPreferredLanguages(parse_language_list(languages));
	}
	inline void setPreferredLanguages(const std::string &languages) {
		setPreferredLanguages(utf8_to_wide(languages));
	}

private:
	std::vector<std::wstring> m_preferred_languages;
	std::shared_ptr<std::unordered_map<std::wstring, std::wstring>> m_translations;
};
