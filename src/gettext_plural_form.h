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

#pragma once
#include <string_view>
#include <memory>

// Note that this only implements a subset of C expressions. See:
// https://git.savannah.gnu.org/gitweb/?p=gettext.git;a=blob;f=gettext-runtime/intl/plural.y
class GettextPluralForm {
public:
	using NumT = unsigned long;
	using Ptr = std::shared_ptr<GettextPluralForm>;
	virtual NumT operator()(const NumT) const = 0;
	virtual operator bool() const { return true; }
	virtual ~GettextPluralForm() {};

	static GettextPluralForm::Ptr parse(const std::wstring_view &str);
	static GettextPluralForm::Ptr parseHeaderLine(const std::wstring_view &str);
};
