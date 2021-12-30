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

#include "address.h"

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <iomanip>
#include "network/networkexceptions.h"
#include "util/string.h"
#include "util/numeric.h"
#include "constants.h"
#include "debug.h"
#include "settings.h"
#include "log.h"

#ifdef _WIN32
// Without this some of the network functions are not found on mingw
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define LAST_SOCKET_ERR() WSAGetLastError()
typedef SOCKET socket_t;
typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#define LAST_SOCKET_ERR() (errno)
typedef int socket_t;
#endif

/*
	Address
*/

Address::Address()
{
	memset(&m_address, 0, sizeof(m_address));
}

Address::Address(u32 address, u16 port)
{
	memset(&m_address, 0, sizeof(m_address));
	setAddress(address);
	setPort(port);
}

Address::Address(u8 a, u8 b, u8 c, u8 d, u16 port)
{
	memset(&m_address, 0, sizeof(m_address));
	setAddress(a, b, c, d);
	setPort(port);
}

Address::Address(const IPv6AddressBytes *ipv6_bytes, u16 port)
{
	memset(&m_address, 0, sizeof(m_address));
	setAddress(ipv6_bytes);
	setPort(port);
}

// Equality (address family, IP and port must be equal)
bool Address::operator==(const Address &other)
{
	if (other.m_addr_family != m_addr_family || other.m_port != m_port)
		return false;

	if (m_addr_family == AF_INET) {
		return m_address.ipv4.s_addr == other.m_address.ipv4.s_addr;
	}

	if (m_addr_family == AF_INET6) {
		return memcmp(m_address.ipv6.s6_addr,
				other.m_address.ipv6.s6_addr, 16) == 0;
	}

	return false;
}

void Address::Resolve(const char *name)
{
	if (!name || name[0] == 0) {
		if (m_addr_family == AF_INET)
			setAddress(static_cast<u32>(0));
		else if (m_addr_family == AF_INET6)
			setAddress(static_cast<IPv6AddressBytes*>(nullptr));
		return;
	}

	struct addrinfo *resolved, hints;
	memset(&hints, 0, sizeof(hints));

	// Setup hints
	if (g_settings->getBool("enable_ipv6")) {
		// AF_UNSPEC allows both IPv6 and IPv4 addresses to be returned
		hints.ai_family = AF_UNSPEC;
	} else {
		hints.ai_family = AF_INET;
	}

	// Do getaddrinfo()
	int e = getaddrinfo(name, NULL, &hints, &resolved);
	if (e != 0)
		throw ResolveError(gai_strerror(e));

	// Copy data
	if (resolved->ai_family == AF_INET) {
		struct sockaddr_in *t = (struct sockaddr_in *)resolved->ai_addr;
		m_addr_family = AF_INET;
		m_address.ipv4 = t->sin_addr;
	} else if (resolved->ai_family == AF_INET6) {
		struct sockaddr_in6 *t = (struct sockaddr_in6 *)resolved->ai_addr;
		m_addr_family = AF_INET6;
		m_address.ipv6 = t->sin6_addr;
	} else {
		m_addr_family = 0;
	}
	freeaddrinfo(resolved);
}

// IP address -> textual representation
std::string Address::serializeString() const
{
// windows XP doesnt have inet_ntop, maybe use better func
#ifdef _WIN32
	if (m_addr_family == AF_INET) {
		return inet_ntoa(m_address.ipv4);
	} else if (m_addr_family == AF_INET6) {
		std::ostringstream os;
		os << std::hex;
		for (int i = 0; i < 16; i += 2) {
			u16 section = (m_address.ipv6.s6_addr[i] << 8) |
					(m_address.ipv6.s6_addr[i + 1]);
			os << section;
			if (i < 14)
				os << ":";
		}
		return os.str();
	} else {
		return "";
	}
#else
	char str[INET6_ADDRSTRLEN];
	if (inet_ntop(m_addr_family, (void*) &m_address, str, sizeof(str)) == nullptr)
		return "";
	return str;
#endif
}

struct in_addr Address::getAddress() const
{
	return m_address.ipv4;
}

struct in6_addr Address::getAddress6() const
{
	return m_address.ipv6;
}

u16 Address::getPort() const
{
	return m_port;
}

bool Address::isZero() const
{
	if (m_addr_family == AF_INET) {
		return m_address.ipv4.s_addr == 0;
	}

	if (m_addr_family == AF_INET6) {
		static const char zero[16] = {0};
		return memcmp(m_address.ipv6.s6_addr, zero, 16) == 0;
	}

	return false;
}

void Address::setAddress(u32 address)
{
	m_addr_family = AF_INET;
	m_address.ipv4.s_addr = htonl(address);
}

void Address::setAddress(u8 a, u8 b, u8 c, u8 d)
{
	u32 addr = (a << 24) | (b << 16) | (c << 8) | d;
	setAddress(addr);
}

void Address::setAddress(const IPv6AddressBytes *ipv6_bytes)
{
	m_addr_family = AF_INET6;
	if (ipv6_bytes)
		memcpy(m_address.ipv6.s6_addr, ipv6_bytes->bytes, 16);
	else
		memset(m_address.ipv6.s6_addr, 0, 16);
}

void Address::setPort(u16 port)
{
	m_port = port;
}

void Address::print(std::ostream *s) const
{
	if (m_addr_family == AF_INET6)
		*s << "[" << serializeString() << "]:" << m_port;
	else if (m_addr_family == AF_INET)
		*s << serializeString() << ":" << m_port;
	else
		*s << "(undefined)";
}

bool Address::isLocalhost() const
{
	if (isIPv6()) {
		static const u8 localhost_bytes[] = {
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
		static const u8 mapped_ipv4_localhost[] = {
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 0};

		auto addr = m_address.ipv6.s6_addr;

		return memcmp(addr, localhost_bytes, 16) == 0 ||
			memcmp(addr, mapped_ipv4_localhost, 13) == 0;
	}

	auto addr = ntohl(m_address.ipv4.s_addr);
	return (addr >> 24) == 0x7f;
}
