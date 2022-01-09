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

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <ostream>
#include <cstring>
#include "irrlichttypes.h"
#include "networkexceptions.h"

struct IPv6AddressBytes
{
	u8 bytes[16];
	IPv6AddressBytes() { memset(bytes, 0, 16); }
};

class Address
{
public:
	Address();
	Address(u32 address, u16 port);
	Address(u8 a, u8 b, u8 c, u8 d, u16 port);
	Address(const IPv6AddressBytes *ipv6_bytes, u16 port);

	bool operator==(const Address &address);
	bool operator!=(const Address &address) { return !(*this == address); }

	struct in_addr getAddress() const;
	struct in6_addr getAddress6() const;
	u16 getPort() const;
	int getFamily() const { return m_addr_family; }
	bool isIPv6() const { return m_addr_family == AF_INET6; }
	bool isZero() const;
	void print(std::ostream *s) const;
	std::string serializeString() const;
	bool isLocalhost() const;

	// Resolve() may throw ResolveError (address is unchanged in this case)
	void Resolve(const char *name);

	void setAddress(u32 address);
	void setAddress(u8 a, u8 b, u8 c, u8 d);
	void setAddress(const IPv6AddressBytes *ipv6_bytes);
	void setPort(u16 port);

private:
	unsigned short m_addr_family = 0;
	union
	{
		struct in_addr ipv4;
		struct in6_addr ipv6;
	} m_address;
	u16 m_port = 0; // Port is separate from sockaddr structures
};
