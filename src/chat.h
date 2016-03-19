/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef CHAT_HEADER
#define CHAT_HEADER

#include "irrlichttypes.h"
#include <string>
#include <vector>
#include <list>


/// Message in chat console, may be split into multiple lines.
struct ChatMessage {
	ChatMessage(const std::wstring &name, const std::wstring &text);

	// Name of sending player, or empty if sent by server
	std::wstring name;

	std::wstring text;
	time_t timestamp;  // Wall time
	u64 time_raw;  // Monotonic time in milliseconds
};


/// Single line in chat console, wrapped to console width
struct ChatLine {
	ChatLine() {}
	ChatLine(bool first, const std::wstring &text) :
		first(first), text(text) {}
	bool first;
	std::wstring text;
};


class ChatBuffer {
public:
	ChatBuffer(size_t scrollback, bool add_ts=true);

	// Append chat line
	// Removes oldest chat line if scrollback size is reached
	void addLine(const std::wstring &name, const std::wstring &text);

	// Remove all chat lines
	void clear();

	// Get number of lines currently in buffer.
	size_t getLineCount() const { return m_unformatted.size(); }
	// Get scrollback size, maximum number of lines in buffer.
	size_t getScrollback() const { return m_scrollback; }
	// Get reference to i-th chat line.
	const ChatMessage& getLine(size_t index) const;

	// Delete oldest N chat lines.
	void deleteOldest(size_t count);
	// Delete lines older than maxAge seconds.
	void deleteOlderThan(float age);

	// Get number of columns, 0 if reformat has not been called yet.
	u32 getColumns() const { return m_cols; }
	// Get number of rows, 0 if reformat has not been called yet.
	u32 getRows() const { return m_rows; }
	// Update console size and reformat all formatted lines.
	void reformat(u32 cols, u32 rows);
	// Get formatted line for a given row (0 is top of screen).
	// Only valid after reformat has been called at least once
	bool getFormattedLine(size_t row, ChatLine *line) const;
	// Scrolling in formatted buffer (relative)
	// positive rows == scroll up, negative rows == scroll down
	void scroll(ssize_t rows) { scrollAbsolute(m_scroll + rows); }
	// Scrolling in formatted buffer (absolute)
	void scrollAbsolute(ssize_t scroll);
	// Scroll to bottom of buffer (newest)
	void scrollBottom() { m_scroll = getBottomScrollPos(); }
	// Scroll to top of buffer (oldest)
	void scrollTop() { m_scroll = getTopScrollPos(); }

	/// Format a chat line for the current number of columns.
	/// Appends the formatted lines to m_formatted.
	/// @return the number of formatted lines added.
	size_t formatChatLine(const ChatMessage &line);

protected:
	static std::string formatTimestamp(const time_t *time);

	ssize_t getTopScrollPos() const
	{
		return (m_rows != 0 && m_formatted.size() <= m_rows) ?
				(ssize_t)m_formatted.size() - (ssize_t)m_rows : 0;
	}
	ssize_t getBottomScrollPos() const
	{
		return m_rows != 0 ? (ssize_t)m_formatted.size() - (ssize_t)m_rows : 0;
	}

private:
	// Scrollback size
	size_t m_scrollback;
	// Array of unformatted chat messages
	std::vector<ChatMessage> m_unformatted;

	// Number of character columns in console
	u32 m_cols;
	// Number of character rows in console
	u32 m_rows;
	// Scroll position (console's top line index into m_formatted)
	ssize_t m_scroll;
	// Array of formatted lines
	std::vector<ChatLine> m_formatted;
	// Whether to prefix a timestamp to messages
	bool m_add_ts;
};

class ChatPrompt {
public:
	ChatPrompt(const std::wstring &prompt, u32 history_limit);

	// Input character or string
	void input(wchar_t ch);
	void input(const std::wstring &str);

	// Add a string to the history
	void addToHistory(const std::wstring &line);

	// Get current line
	std::wstring getLine() const { return m_line; }

	// Get section of line that is currently selected
	std::wstring getSelection() const
		{ return m_line.substr(m_cursor, m_cursor_len); }

	// Clear the current line
	void clear();

	// Replace the current line with the given text
	std::wstring replace(const std::wstring &line);

	// Select previous command from history
	void historyPrev();
	// Select next command from history
	void historyNext();

	// Nick completion
	void nickCompletion(const std::list<std::string>& names, bool backwards);

	// Update console size and reformat the visible portion of the prompt
	void reformat(u32 cols);
	// Get visible portion of the prompt.
	std::wstring getVisiblePortion() const;
	// Get cursor position (relative to visible portion). -1 if invalid
	s32 getVisibleCursorPosition() const;
	// Get length of cursor selection
	s32 getCursorLength() const { return m_cursor_len; }

	// Cursor operations
	enum CursorOp {
		CURSOROP_MOVE,
		CURSOROP_SELECT,
		CURSOROP_DELETE
	};

	// Cursor operation direction
	enum CursorOpDir {
		CURSOROP_DIR_LEFT,
		CURSOROP_DIR_RIGHT
	};

	// Cursor operation scope
	enum CursorOpScope {
		CURSOROP_SCOPE_CHARACTER,
		CURSOROP_SCOPE_WORD,
		CURSOROP_SCOPE_LINE,
		CURSOROP_SCOPE_SELECTION
	};

	// Cursor operation
	// op specifies whether it's a move or delete operation
	// dir specifies whether the operation goes left or right
	// scope specifies how far the operation will reach (char/word/line)
	// Examples:
	//   cursorOperation(CURSOROP_MOVE, CURSOROP_DIR_RIGHT, CURSOROP_SCOPE_LINE)
	//     moves the cursor to the end of the line.
	//   cursorOperation(CURSOROP_DELETE, CURSOROP_DIR_LEFT, CURSOROP_SCOPE_WORD)
	//     deletes the word to the left of the cursor.
	void cursorOperation(CursorOp op, CursorOpDir dir, CursorOpScope scope);

protected:
	// set m_view to ensure that 0 <= m_view <= m_cursor < m_view + m_cols
	// if line can be fully shown, set m_view to zero
	// else, also ensure m_view <= m_line.size() + 1 - m_cols
	void clampView();

private:
	// Prompt prefix
	std::wstring m_prompt;
	// Currently edited line
	std::wstring m_line;
	// History buffer
	std::vector<std::wstring> m_history;
	// History index (0 <= m_history_index <= m_history.size())
	u32 m_history_index;
	// Maximum number of history entries
	u32 m_history_limit;

	// Number of columns excluding columns reserved for the prompt
	s32 m_cols;
	// Start of visible portion (index into m_line)
	s32 m_view;
	// Cursor (index into m_line)
	s32 m_cursor;
	// Cursor length (length of selected portion of line)
	s32 m_cursor_len;

	// Last nick completion start (index into m_line)
	s32 m_nick_completion_start;
	// Last nick completion start (index into m_line)
	s32 m_nick_completion_end;
};


class ChatBackend {
public:
	ChatBackend(bool add_ts=true);

	// Add chat message
	void addMessage(const std::wstring &name, const std::wstring &text);
	// Parse chat message in the form "<name> text" and add it
	void addUnparsedMessage(const std::wstring &line);

	ChatBuffer& getConsoleBuffer() { return m_console_buffer; }
	ChatBuffer& getRecentBuffer() { return m_recent_buffer; }
	ChatPrompt& getPrompt() { return m_prompt; }

	/// Return concatenation of all recent messages
	std::wstring getRecentChat();

	// Reformat all buffers
	void reformat(u32 cols, u32 rows);

	// Clear all recent messages
	void clearRecentChat() { m_recent_buffer.clear(); }

	// Scrolling
	void scroll(s32 rows) { m_console_buffer.scroll(rows); }
	void scrollPageUp() { scroll(-(s32)m_console_buffer.getRows()); }
	void scrollPageDown() { scroll(m_console_buffer.getRows()); }

	void step();

private:
	ChatBuffer m_console_buffer;
	ChatBuffer m_recent_buffer;
	ChatPrompt m_prompt;
};

#endif

