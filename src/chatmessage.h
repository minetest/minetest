/*
Minetest
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include <string>
#include <ctime>

enum ChatMessageType
{
	CHATMESSAGE_TYPE_RAW = 0,
	CHATMESSAGE_TYPE_NORMAL = 1,
	CHATMESSAGE_TYPE_ANNOUNCE = 2,
	CHATMESSAGE_TYPE_SYSTEM = 3,
	CHATMESSAGE_TYPE_MAX = 4,
};

struct ChatMessage
{
	ChatMessage(const std::wstring &m = L"") : message(m) {}

	ChatMessage(ChatMessageType t, const std::wstring &m, const std::wstring &s = L"",
			std::time_t ts = std::time(0)) :
			type(t),
			message(m), sender(s), timestamp(ts)
	{
	}

	ChatMessageType type = CHATMESSAGE_TYPE_RAW;
	std::wstring message = L"";
	std::wstring sender = L"";
	std::time_t timestamp = std::time(0);
};
