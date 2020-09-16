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

#pragma once

#include <unordered_map>
#include <string>

class Translations;
#ifndef SERVER
extern Translations *g_client_translations;
#endif

class Translations
{
public:
	void loadTranslation(const std::string &data);
	void clear();
	const std::wstring &getTranslation(
			const std::wstring &textdomain, const std::wstring &s);

private:
	std::unordered_map<std::wstring, std::wstring> m_translations;
};
