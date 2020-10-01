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
#include <map>
#include <string>
#include <vector>

class Translations;
#ifndef SERVER
extern Translations *g_client_translations;
#endif

class Translations
{
public:
	void loadTranslation(const std::string &filename, const std::string &data);
	void clear();
	const std::wstring &getTranslation(
			const std::wstring &textdomain, const std::wstring &s) const;
	const std::wstring &getPluralTranslation(const std::wstring &textdomain,
			const std::wstring &s, unsigned long int number) const;

private:
	std::unordered_map<std::wstring, std::wstring> m_translations;
	std::unordered_map<std::wstring, std::vector<std::wstring>> m_plural_translations;

	void addTranslation(const std::wstring &textdomain, const std::wstring &original,
			const std::wstring &translated);
	void addPluralTranslation(const std::wstring &textdomain,
			const std::wstring &original,
			std::vector<std::wstring> &translated);
	std::wstring unescapeC(const std::wstring &str);
	void loadPoEntry(const std::map<std::wstring, std::wstring> &entry);
	void loadMoEntry(const std::string &orignal, const std::string &translated);
	void loadTrTranslation(const std::string &data);
	void loadPoTranslation(const std::string &data);
	void loadMoTranslation(const std::string &data);
};
