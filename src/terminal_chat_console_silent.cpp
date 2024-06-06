/*
Minetest
Copyright (C) 2015 est31 <MTest31@outlook.com>

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

#include <inttypes.h>
#include "config.h"
#if USE_CURSES
#include "version.h"
#include "terminal_chat_console_silent.h"
#include "porting.h"
#include "settings.h"
#include "util/numeric.h"
#include "util/string.h"
#include "chat_interface.h"
#include <fcntl.h>

TerminalChatConsoleSilent g_term_console_silent;

void TerminalChatConsoleSilent::initOfCurses()
{
	// Make cin non-blocking.
	int flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
}

void TerminalChatConsoleSilent::deInitOfCurses()
{
	// no-op
}

void *TerminalChatConsoleSilent::run()
{
	BEGIN_DEBUG_EXCEPTION_HANDLER

	// Inform the server of our nick
	m_chat_interface->command_queue.push_back(
		new ChatEventNick(CET_NICK_ADD, m_nick));

	{
		// Ensures that curses is deinitialized even on an exception being thrown
		CursesInitHelper helper(this);

		while (!stopRequested()) {

			if (stopRequested())
				break;

			step();

			// 10 / 1000 means you can only run 100 commands per second, the horror.
			// If you are running 100 commands per second, you might want to 
			// redesign the program interfacing with the server.
			// This keeps the thread from maxing out the cpu core.
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			
		}
	}

	if (m_kill_requested)
		*m_kill_requested = true;


	END_DEBUG_EXCEPTION_HANDLER

	return NULL;
}

void TerminalChatConsoleSilent::typeChatMessage(const std::wstring &msg)
{
	// Discard empty line
	if (msg.empty())
		return;

	// Send to server
	m_chat_interface->command_queue.push_back(
		new ChatEventChat(m_nick, msg));

	// Print if its a command (gets eaten by server otherwise)
	if (msg[0] == L'/') {
		m_chat_backend.addMessage(L"", (std::wstring)L"Issued command: " + msg);
	}
}

void TerminalChatConsoleSilent::step()
{

	// C++ hello world comes back for round 2.
	std::string str;
	std::getline(std::cin, str);

	// The string is not null terminated so we cannot use length().
	if (str.length() > 0) {

		std::wstring test(str.begin(), str.end());

		typeChatMessage(test);
	}

	// Needs to be outside check.
	std::cin.clear();
}


void TerminalChatConsoleSilent::stopAndWaitforThread()
{
	clearKillStatus();
	stop();
	wait();
}

#endif