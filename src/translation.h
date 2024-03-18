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

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

class Translations;
#ifndef SERVER
extern Translations *g_client_translations;
#endif

class Translations
{
public:
	Translations();

	void loadTranslation(const std::wstring &language, const std::string &data);
	void loadTranslation(const std::string &language, const std::string &data);
	void loadTranslation(const std::string &data);
	void clear();

	const std::wstring &getTranslation(const std::vector<std::wstring> &languages,
			const std::wstring &textdomain, const std::wstring &s) const;
	const std::wstring &getTranslation(
			const std::wstring &textdomain, const std::wstring &s) const;

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
	void setPreferredLanguages(const std::wstring &languages);
	void setPreferredLanguages(const std::string &languages);

private:
	std::vector<std::wstring> m_preferred_languages;
	std::shared_ptr<std::unordered_map<std::wstring, std::wstring>> m_translations;
};
