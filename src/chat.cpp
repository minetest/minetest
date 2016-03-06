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

#include "chat.h"
#include "debug.h"
#include "strfnd.h"
#include "util/string.h"
#include "util/numeric.h"
#include "settings.h"
#include "porting.h"
#include <cctype>
#include <sstream>


ChatMessage::ChatMessage(const std::wstring &name, const std::wstring &text) :
	name(name),
	text(text),
	timestamp(time(NULL)),
	time_raw(porting::getTimeMs())
{}


ChatBuffer::ChatBuffer(size_t scrollback, bool add_ts):
	m_scrollback(scrollback),
	m_unformatted(),
	m_cols(0),
	m_rows(0),
	m_scroll(0),
	m_formatted(),
	m_add_ts(add_ts)
{
	if (m_scrollback == 0)
		m_scrollback = 1;
}


void ChatBuffer::addLine(const std::wstring &name, const std::wstring &text)
{
	ChatMessage msg(name, text);
	m_unformatted.push_back(msg);

	if (m_rows > 0) {
		// m_formatted is valid and must be kept valid
		bool scrolled_at_bottom = (m_scroll == getBottomScrollPos());
		u32 num_added = formatChatLine(msg);
		if (scrolled_at_bottom)
			m_scroll += num_added;
	}

	// Limit number of lines by m_scrollback
	if (m_unformatted.size() > m_scrollback) {
		deleteOldest(m_unformatted.size() - m_scrollback);
	}
}

void ChatBuffer::clear()
{
	m_unformatted.clear();
	m_formatted.clear();
	m_scroll = 0;
}


const ChatMessage& ChatBuffer::getLine(size_t index) const
{
	assert(index < m_unformatted.size());
	return m_unformatted[index];
}


void ChatBuffer::deleteOldest(size_t count)
{
	size_t del_unformatted = 0;
	size_t del_formatted = 0;

	while (count > 0 && del_unformatted < m_unformatted.size()) {
		++del_unformatted;

		// keep m_formatted in sync
		if (del_formatted < m_formatted.size()) {
			sanity_check(m_formatted[del_formatted].first);
			++del_formatted;
			while (del_formatted < m_formatted.size() &&
					!m_formatted[del_formatted].first)
				++del_formatted;
		}

		--count;
	}

	m_unformatted.erase(m_unformatted.begin(), m_unformatted.begin() + del_unformatted);
	m_formatted.erase(m_formatted.begin(), m_formatted.begin() + del_formatted);
}


void ChatBuffer::deleteOlderThan(float age)
{
	u64 oldest = porting::getTimeMs() - (age * 1000);
	size_t count = 0;
	while (count < m_unformatted.size() && m_unformatted[count].time_raw < oldest)
		++count;
	deleteOldest(count);
}


void ChatBuffer::reformat(u32 cols, u32 rows)
{
	if (cols == 0 || rows == 0) {
		// Clear formatted buffer
		m_cols = 0;
		m_rows = 0;
		m_scroll = 0;
		m_formatted.clear();
	} else if (cols == m_cols && rows == m_rows) {
		return;
	}
	// TODO: Avoid reformatting ALL lines (even invisible ones)
	//       each time the console size changes?

	// Find out the scroll position in *unformatted* lines
	size_t scroll_unformatted = 0;
	size_t scroll_formatted = 0;
	bool at_bottom = (m_scroll == getBottomScrollPos());

	if (!at_bottom) {
		for (ssize_t i = 0; i < m_scroll; ++i) {
			if (m_formatted[i].first)
				++scroll_unformatted;
		}
	}

	// If number of columns change, reformat everything
	if (cols != m_cols) {
		m_cols = cols;
		m_formatted.clear();
		for (size_t i = 0; i < m_unformatted.size(); ++i) {
			if (i == scroll_unformatted)
				scroll_formatted = m_formatted.size();
			formatChatLine(m_unformatted[i]);
		}
	}

	m_rows = rows;

	// Restore the scroll position
	if (at_bottom) {
		scrollBottom();
	} else {
		scrollAbsolute(scroll_formatted);
	}
}


bool ChatBuffer::getFormattedLine(size_t row, ChatLine *line) const
{
	ssize_t index = m_scroll + (ssize_t)row;
	if (index < 0 || index >= (ssize_t)m_formatted.size())
		return false;
	*line = m_formatted[index];
	return true;
}


void ChatBuffer::scrollAbsolute(ssize_t scroll)
{
	ssize_t top = getTopScrollPos();
	ssize_t bottom = getBottomScrollPos();
	m_scroll = MYMIN(MYMAX(scroll, top), bottom);
}


std::string ChatBuffer::formatTimestamp(const time_t *time)
{
	static std::string ts_format = g_settings->get(
			"console_timestamp_format");
	if (ts_format.empty())
		return "";

#if defined(__STDC_LIB_EXT1__)
	// C11 localtime_s
	tm tms;
	const tm *tms_p = localtime_s(time, &tms);
#elif _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _BSD_SOURCE || _SVID_SOURCE || _POSIX_SOURCE
	// POSIX localtime_r
	tm tms;
	const tm *tms_p = localtime_r(time, &tms);
#else
	// Thread-unsafe C89 localtime (safe in MSVC)
	const tm *tms_p = localtime(time);
#endif
	if (tms_p == NULL)
		return "<localtime failed>";

	char ts_frag[64];
	size_t size = strftime(ts_frag, sizeof(ts_frag),
			ts_format.c_str(), tms_p);

	return std::string(ts_frag, size);
}


size_t ChatBuffer::formatChatLine(const ChatMessage &msg)
{
	// Wstring so we get length in codepoints and don't break codepoints
	std::wstring msg_str;

	// Format the timestamp
	if (m_add_ts) {
		std::string ts = formatTimestamp(&msg.timestamp);
		if (!ts.empty()) {
			msg_str += narrow_to_wide(ts);
			msg_str += L' ';
		}
	}

	// Format the sender name
	if (!msg.name.empty()) {
		msg_str += L"<";
		msg_str += msg.name;
		msg_str += L"> ";
	}

	// Choose an indentation level
	unsigned indentation = 4;
	if (m_cols < 16) {
		// Too cramped for indentation
		indentation = 0;
	} else if (msg_str.size() <= m_cols / 2) {
		// Prefixes shorter than about half the console width
		indentation = msg_str.size();
	}

	msg_str += msg.text;

	// If it fits on one line just add the line
	if (msg_str.size() <= m_cols) {
		m_formatted.push_back(ChatLine(true, msg_str));
		return 1;
	}

	const std::wstring indent_str = std::wstring(indentation, L' ');
	const size_t pre_size = m_formatted.size();

	bool first = true;
	while (indentation + msg_str.size() > m_cols) {
		size_t cutoff = first ? m_cols : m_cols - indentation;

		// Wrap at last whitespace if we can
		size_t i = cutoff;
		while (i > 0 && !std::isspace(msg_str[--i]))
			{}
		// If we're on the first line make sure there's at least one
		// character in the message before the cutoff point so the
		// first line isn't empty (except for the timestamp and name).
		// The first line is allowed to be empty if we have a smaller
		// indentation length though (because in that case the next
		// line might have space to split nicely).  Otherwise just
		// check that we've actually found a whitespace to wrap at.
		if (first ? i > indentation : i != 0)
			cutoff = i;

		std::wstring next_line = first ? L"" : indent_str;
		next_line += msg_str.substr(0, cutoff);
		msg_str.erase(msg_str.begin(), msg_str.begin() + cutoff);
		msg_str = trim(msg_str);
		m_formatted.push_back(ChatLine(first, next_line));

		first = false;
	}
	m_formatted.push_back(ChatLine(false, indent_str + msg_str));

	return m_formatted.size() - pre_size;
}


ChatPrompt::ChatPrompt(const std::wstring &prompt, u32 history_limit):
	m_prompt(prompt),
	m_line(L""),
	m_history(),
	m_history_index(0),
	m_history_limit(history_limit),
	m_cols(0),
	m_view(0),
	m_cursor(0),
	m_cursor_len(0),
	m_nick_completion_start(0),
	m_nick_completion_end(0)
{
}


void ChatPrompt::input(wchar_t ch)
{
	m_line.insert(m_cursor, 1, ch);
	m_cursor++;
	clampView();
	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
}


void ChatPrompt::input(const std::wstring &str)
{
	m_line.insert(m_cursor, str);
	m_cursor += str.size();
	clampView();
	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
}


void ChatPrompt::addToHistory(const std::wstring &line)
{
	if (!line.empty())
		m_history.push_back(line);
	if (m_history.size() > m_history_limit)
		m_history.erase(m_history.begin());
	m_history_index = m_history.size();
}


void ChatPrompt::clear()
{
	m_line.clear();
	m_view = 0;
	m_cursor = 0;
	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
}


std::wstring ChatPrompt::replace(const std::wstring &line)
{
	std::wstring old_line = m_line;
	m_line =  line;
	m_view = m_cursor = line.size();
	clampView();
	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
	return old_line;
}


void ChatPrompt::historyPrev()
{
	if (m_history_index > 0) {
		--m_history_index;
		replace(m_history[m_history_index]);
	}
}


void ChatPrompt::historyNext()
{
	if (m_history_index + 1 >= m_history.size()) {
		m_history_index = m_history.size();
		replace(L"");
	} else {
		++m_history_index;
		replace(m_history[m_history_index]);
	}
}


void ChatPrompt::nickCompletion(const std::list<std::string>& names, bool backwards)
{
	// Two cases:
	// (a) m_nick_completion_start == m_nick_completion_end == 0
	//     Then no previous nick completion is active.
	//     Get the word around the cursor and replace with any nick
	//     that has that word as a prefix.
	// (b) else, continue a previous nick completion.
	//     m_nick_completion_start..m_nick_completion_end are the
	//     interval where the originally used prefix was. Cycle
	//     through the list of completions of that prefix.
	u32 prefix_start = m_nick_completion_start;
	u32 prefix_end = m_nick_completion_end;
	bool initial = (prefix_end == 0);
	if (initial) {
		// no previous nick completion is active
		prefix_start = prefix_end = m_cursor;
		while (prefix_start > 0 && !isspace(m_line[prefix_start-1]))
			--prefix_start;
		while (prefix_end < m_line.size() && !isspace(m_line[prefix_end]))
			++prefix_end;
		if (prefix_start == prefix_end)
			return;
	}
	std::wstring prefix = m_line.substr(prefix_start, prefix_end - prefix_start);

	// find all names that start with the selected prefix
	std::vector<std::wstring> completions;
	for (std::list<std::string>::const_iterator
			i = names.begin();
			i != names.end(); ++i) {
		if (str_starts_with(narrow_to_wide(*i), prefix, true)) {
			std::wstring completion = narrow_to_wide(*i);
			if (prefix_start == 0)
				completion += L": ";
			completions.push_back(completion);
		}
	}
	if (completions.empty())
		return;

	// find a replacement string and the word that will be replaced
	u32 word_end = prefix_end;
	u32 replacement_index = 0;
	if (!initial) {
		while (word_end < m_line.size() && !isspace(m_line[word_end]))
			++word_end;
		std::wstring word = m_line.substr(prefix_start, word_end - prefix_start);

		// cycle through completions
		for (u32 i = 0; i < completions.size(); ++i) {
			if (str_equal(word, completions[i], true)) {
				if (backwards)
					replacement_index = i + completions.size() - 1;
				else
					replacement_index = i + 1;
				replacement_index %= completions.size();
				break;
			}
		}
	}
	std::wstring replacement = completions[replacement_index];
	if (word_end < m_line.size() && isspace(word_end))
		++word_end;

	// replace existing word with replacement word,
	// place the cursor at the end and record the completion prefix
	m_line.replace(prefix_start, word_end - prefix_start, replacement);
	m_cursor = prefix_start + replacement.size();
	clampView();
	m_nick_completion_start = prefix_start;
	m_nick_completion_end = prefix_end;
}


void ChatPrompt::reformat(u32 cols)
{
	if (cols <= m_prompt.size()) {
		m_cols = 0;
		m_view = m_cursor;
	} else {
		ssize_t length = m_line.size();
		bool was_at_end = (m_view + m_cols >= length + 1);
		m_cols = cols - m_prompt.size();
		if (was_at_end)
			m_view = length;
		clampView();
	}
}


std::wstring ChatPrompt::getVisiblePortion() const
{
	return m_prompt + m_line.substr(m_view, m_cols);
}


s32 ChatPrompt::getVisibleCursorPosition() const
{
	return m_cursor - m_view + m_prompt.size();
}


void ChatPrompt::cursorOperation(CursorOp op, CursorOpDir dir, CursorOpScope scope)
{
	s32 old_cursor = m_cursor;
	s32 new_cursor = m_cursor;

	s32 length = m_line.size();
	s32 increment = (dir == CURSOROP_DIR_RIGHT) ? 1 : -1;

	switch (scope) {
	case CURSOROP_SCOPE_CHARACTER:
		new_cursor += increment;
		break;
	case CURSOROP_SCOPE_WORD:
		if (dir == CURSOROP_DIR_RIGHT) {
			// skip one word to the right
			while (new_cursor < length && isspace(m_line[new_cursor]))
				new_cursor++;
			while (new_cursor < length && !isspace(m_line[new_cursor]))
				new_cursor++;
			while (new_cursor < length && isspace(m_line[new_cursor]))
				new_cursor++;
		} else {
			// skip one word to the left
			while (new_cursor >= 1 && isspace(m_line[new_cursor - 1]))
				new_cursor--;
			while (new_cursor >= 1 && !isspace(m_line[new_cursor - 1]))
				new_cursor--;
		}
		break;
	case CURSOROP_SCOPE_LINE:
		new_cursor += increment * length;
		break;
	case CURSOROP_SCOPE_SELECTION:
		break;
	}

	new_cursor = MYMAX(MYMIN(new_cursor, length), 0);

	switch (op) {
	case CURSOROP_MOVE:
		m_cursor = new_cursor;
		m_cursor_len = 0;
		break;
	case CURSOROP_DELETE:
		if (m_cursor_len > 0) { // Delete selected text first
			m_line.erase(m_cursor, m_cursor_len);
		} else {
			m_cursor = MYMIN(new_cursor, old_cursor);
			m_line.erase(m_cursor, abs(new_cursor - old_cursor));
		}
		m_cursor_len = 0;
		break;
	case CURSOROP_SELECT:
		if (scope == CURSOROP_SCOPE_LINE) {
			m_cursor = 0;
			m_cursor_len = length;
		} else {
			m_cursor = MYMIN(new_cursor, old_cursor);
			m_cursor_len += abs(new_cursor - old_cursor);
			m_cursor_len = MYMIN(m_cursor_len, length - m_cursor);
		}
		break;
	}

	clampView();

	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
}


void ChatPrompt::clampView()
{
	s32 length = m_line.size();
	if (length + 1 <= m_cols) {
		m_view = 0;
	} else {
		m_view = MYMIN(m_view, length + 1 - m_cols);
		m_view = MYMIN(m_view, m_cursor);
		m_view = MYMAX(m_view, m_cursor - m_cols + 1);
		m_view = MYMAX(m_view, 0);
	}
}


ChatBackend::ChatBackend(bool add_ts):
	m_console_buffer(500, add_ts),
	m_recent_buffer(6),
	m_prompt(L"]", 500)
{}


void ChatBackend::addMessage(const std::wstring &name, const std::wstring &text)
{
	// Note: A message may consist of multiple lines, for example the MOTD.
	WStrfnd fnd(text);
	while (!fnd.atend()) {
		std::wstring line = fnd.next(L"\n");
		m_console_buffer.addLine(name, line);
		m_recent_buffer.addLine(name, line);
	}
}


void ChatBackend::addUnparsedMessage(const std::wstring &message)
{
	// TODO: Remove the need to parse chat messages client-side, by sending
	// separate name and text fields in TOCLIENT_CHAT_MESSAGE.

	if (message.size() < 2 || message[0] != L'<') {
		// Probably a server message.
		addMessage(L"", message);
		return;
	}
	size_t closing = message.find_first_of(L'>', 1);
	if (closing == std::wstring::npos ||
			closing + 2 > message.size() ||
			message[closing+1] != L' ') {
		// Parse failure
		addMessage(L"", message);
		return;
	}
	addMessage(message.substr(1, closing - 1), message.substr(closing + 2));
}


std::wstring ChatBackend::getRecentChat()
{
	std::wostringstream stream;
	for (size_t i = 0; i < m_recent_buffer.getLineCount(); ++i) {
		const ChatMessage &line = m_recent_buffer.getLine(i);
		if (i != 0)
			stream << L"\n";
		if (!line.name.empty())
			stream << L"<" << line.name << L"> ";
		stream << line.text;
	}
	return stream.str();
}


void ChatBackend::reformat(u32 cols, u32 rows)
{
	m_console_buffer.reformat(cols, rows);

	// No need to reformat m_recent_buffer, its formatted lines
	// are not used.

	m_prompt.reformat(cols);
}


void ChatBackend::step()
{
	m_recent_buffer.deleteOlderThan(60);
}

