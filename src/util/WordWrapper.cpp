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

#include "WordWrapper.h"

#include <irrString.h>
#include "gettext.h"

static const wchar_t SOFT_HYPHEN = 0x00AD;

void WordWrapper::wrap(std::vector<EnrichedString> &output, std::vector<s32> *line_starts,
		const EnrichedString &text, const core::rect<s32> &bounds,
		s32 line_height, bool allow_newlines) const
{
	s32 avail_width = bounds.getWidth();

	// Ellipsis text
	const wchar_t *cstr_ellipsis = wgettext("…");
	const std::wstring ellipsis = cstr_ellipsis;
	delete[] cstr_ellipsis;
	s32 ellipsis_width = getTextWidth(ellipsis);

	// Stores the confirmed words in the current line.
	EnrichedString line;

	// The current width of `line` in pixels.
	s32 line_width = 0;

	// The whitespace characters between `line` and `word`.
	EnrichedString whitespace;

	// The current non-whitespace characters being accumulated.
	// This may end up on this line, wrapped, or truncated.
	EnrichedString word;

	// The last starting index of a line, used for `line_starts`.
	s32 last_line_start = 0;

	// Whether this is the last line that can be rendered in `bounds`.
	bool is_last_line = false;

	const s32 size = text.size();
	core::rect<s32> line_bounds = bounds;
	line_bounds.LowerRightCorner.Y = line_bounds.UpperLeftCorner.Y + line_height;


	for (s32 i = 0; i < size; ++i) {
		wchar_t c = text.getString()[i];

		bool is_newline = false;
		if (c == L'\r' || c == L'\n') {
			if (allow_newlines)
				is_newline = true;
			else
				c = L' ';
		}

		const bool is_whitespace = c == L' ' || c == 0 || c == SOFT_HYPHEN;
		if (!is_whitespace && !is_newline)
			word.addChar(text, i);
		if (!is_whitespace && !is_newline && i != size - 1)
			continue;

		// Handle word finished
		if (!word.empty()) {
			const s32 whitespace_width = getTextWidth(whitespace.getString());
			const s32 word_width = getTextWidth(word.getString());

			if (line.empty() && word_width > avail_width) {

			} else if (line_width > 0 && line_width + whitespace_width + word_width >
							      avail_width) {
				if (is_last_line) {
					line.addAtEndNoColor(ellipsis);
					output.push_back(line);
					if (line_starts)
						line_starts->push_back(last_line_start);
					return;
				}

				if (!whitespace.empty() &&
						whitespace.getString()[0] == SOFT_HYPHEN)
					whitespace = EnrichedString(L"-") +
						     whitespace.substr(1,
								     whitespace.size() -
										     1);

				// break to next line
				line += whitespace;
				whitespace.clear();
				output.push_back(line);
				line_width = 0;
				line.clear();

				if (line_starts)
					line_starts->push_back(last_line_start);

				if (is_whitespace || is_newline)
					last_line_start = i - (s32)word.size();
				else
					last_line_start = i - (s32)word.size() + 1;

				i--; // Do character again

				line_bounds.UpperLeftCorner.Y += line_height;
				line_bounds.LowerRightCorner.Y += line_height;
				is_last_line = line_bounds.LowerRightCorner.Y +
							       line_height >
					       bounds.LowerRightCorner.Y;
				if (is_last_line)
					avail_width -= ellipsis_width;

				is_newline = false;
			} else if (!is_newline) {
				// add word to line
				line += whitespace;
				line += word;
				line_width += whitespace_width + word_width;

				word.clear();
				whitespace.clear();
			}
		}

		// Handle explicit newline
		if (is_newline) {
			bool is_crlf = c == L'\r' && i < size - 1 && text.getString()[i + 1] == L'\n';
			if (is_crlf)
				i++;

			line += whitespace;
			line += word;

			word.clear();
			whitespace.clear();

			if (is_last_line) {
				if (i < size - 1)
					line.addCharNoColor(L'…');
				output.push_back(line);
				if (line_starts)
					line_starts->push_back(last_line_start);
				return;
			}

			output.push_back(line);
			if (line_starts)
				line_starts->push_back(last_line_start);
			last_line_start = i + 1;

			line.clear();
			line_width = 0;

			line_bounds.UpperLeftCorner.Y += line_height;
			line_bounds.LowerRightCorner.Y += line_height;
			is_last_line = line_bounds.LowerRightCorner.Y +
				line_height >
				bounds.LowerRightCorner.Y;
			if (is_last_line)
				avail_width -= getTextWidth(L"…");
		} else if (is_whitespace) {
			whitespace.addChar(text, i);
		}
	}

	if (!whitespace.empty() || !word.empty()) {
		line += whitespace;
		line += word;
	}

	if (!line.empty()) {
		output.push_back(line);
		if (line_starts)
			line_starts->push_back(last_line_start);
	}
}
