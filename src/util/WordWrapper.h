/*
Minetest
Copyright (C) 2021 rubenwardy <rw@rubenwardy>

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

#include <functional>
#include <rect.h>

#include "enriched_string.h"
#include "irrlichttypes.h"

class WordWrapper
{
	const std::function<s32(const std::wstring &)> getTextWidth;

public:
	/**
	 * @param getTextWidth Function to calculate the length of a string in pixels.
	 */
	explicit WordWrapper(
			const std::function<s32(const std::wstring &)> &getTextWidth) :
			getTextWidth(getTextWidth)
	{
	}

	/**
	 * Wraps `text` to fit into `bounds`, splitting at whitespace or unicode soft
	 * hyphens.
	 *
	 * If `text` exceeds the bounds vertically, it will be truncated with an ellipsis.
	 *
	 * @param output Should be passed empty.
	 * @param text Text to wrap.
	 * @param bounds Text target bounds.
	 * @param line_height The height of a line, including spacing.
	 * @param allow_newlines Whether explicit new lines (\n\r etc) are allowed. If
	 * not, they will be interpreted as spaces.
	 */
	void wrap(std::vector<EnrichedString> &output, std::vector<s32> *line_starts,
			const EnrichedString &text, const core::rect<s32> &bounds,
			s32 line_height, bool allow_newlines = true) const;
};
