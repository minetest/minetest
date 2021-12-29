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

#pragma once

#include <ostream>
#include <cstring>
#include "address.h"
#include "irrlichttypes.h"
#include "networkexceptions.h"

extern bool socket_enable_debug_output;

void sockets_init();
void sockets_cleanup();

class UDPSocket
{
public:
	UDPSocket() = default;

	UDPSocket(bool ipv6);
	~UDPSocket();
	void Bind(Address addr);

	bool init(bool ipv6, bool noExceptions = false);

	void Send(const Address &destination, const void *data, int size);
	// Returns -1 if there is no data
	int Receive(Address &sender, void *data, int size);
	int GetHandle(); // For debugging purposes only
	void setTimeoutMs(int timeout_ms);
	// Returns true if there is data, false if timeout occurred
	bool WaitData(int timeout_ms);

private:
	int m_handle;
	int m_timeout_ms;
	int m_addr_family;
};
