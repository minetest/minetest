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
#include "catch.h"

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
}
