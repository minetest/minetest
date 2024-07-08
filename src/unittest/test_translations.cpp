/*
Minetest
Copyright (C) 2024 y5nw <y5nw@protonmail.com>

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
#include "filesys.h"
#include "content/subgames.h"
#include "catch.h"

#define TEXTDOMAIN_PO L"translation_po"
#define TEST_PO_NAME "translation_po.de.po"

TEST_CASE("test translations")
{
	SECTION("Plural-Forms function for translations")
	{
		auto form = GettextPluralForm::parseHeaderLine(L"Plural-Forms: nplurals=3; plural= (n-1+1)<=1 ? n||1?0:1 : 1?(!!2):2;");
		CHECK(form);
		CHECK((*form)(0) == 0);
		CHECK((*form)(1) == 0);
		CHECK((*form)(2) == 1);
	}

	SECTION("PO file parser")
	{
		Translations translations;
		auto gamespec = findSubgame("devtest");
		CHECK(gamespec.isValid());
		auto popath = gamespec.gamemods_path + (DIR_DELIM "testtranslations" DIR_DELIM "locale" DIR_DELIM TEST_PO_NAME);
		std::string content;
		CHECK(fs::ReadFile(popath, content));
		translations.loadTranslation(TEST_PO_NAME, content);

		CHECK(translations.size() == 4);
		CHECK(translations.getTranslation(TEXTDOMAIN_PO, L"foo") == L"bar");
		CHECK(translations.getTranslation(TEXTDOMAIN_PO, L"Untranslated") == L"Untranslated");
		CHECK(translations.getTranslation(TEXTDOMAIN_PO, L"Fuzzy") == L"Fuzzy");
		CHECK(translations.getTranslation(TEXTDOMAIN_PO, L"Multi\\line\nstring") == L"Multi\\\"li\\ne\nresult");
		CHECK(translations.getTranslation(TEXTDOMAIN_PO, L"Wrong order") == L"Wrong order");
		CHECK(translations.getPluralTranslation(TEXTDOMAIN_PO, L"Plural form", 1) == L"Singular result");
		CHECK(translations.getPluralTranslation(TEXTDOMAIN_PO, L"Singular form", 0) == L"Plural result");
		CHECK(translations.getTranslation(L"context", L"With context") == L"Has context");
	}
}
