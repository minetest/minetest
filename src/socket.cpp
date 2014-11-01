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

#include "socket.h"

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include <iomanip>
#include "util/string.h"
#include "util/numeric.h"
#include "constants.h"
#include "debug.h"
#include "settings.h"
#include "log.h"
#include "main.h" // for g_settings

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	// Without this some of the network functions are not found on mingw
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#ifdef _MSC_VER
		#pragma comment(lib, "ws2_32.lib")
	#endif
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
typedef int socket_t;
#endif

// Set to true to enable verbose debug output
bool socket_enable_debug_output = false;

bool g_sockets_initialized = false;

// Initialize sockets
void sockets_init()
{
#ifdef _WIN32
	// Windows needs sockets to be initialized before use
	WSADATA WsaData;
	if(WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR)
		throw SocketException("WSAStartup failed");
#endif
	g_sockets_initialized = true;
}

void sockets_cleanup()
{
#ifdef _WIN32
	// On Windows, cleanup sockets after use
	WSACleanup();
#endif
}

/*
	Address
*/

Address::Address()
{
	m_addr_family = 0;
	memset(&m_address, 0, sizeof(m_address));
	m_port = 0;
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

Address::Address(const IPv6AddressBytes * ipv6_bytes, u16 port)
{
	memset(&m_address, 0, sizeof(m_address));
	setAddress(ipv6_bytes);
	setPort(port);
}

// Equality (address family, address and port must be equal)
bool Address::operator==(Address &address)
{
	if(address.m_addr_family != m_addr_family || address.m_port != m_port)
		return false;
	else if(m_addr_family == AF_INET)
	{
		return m_address.ipv4.sin_addr.s_addr ==
		       address.m_address.ipv4.sin_addr.s_addr;
	}
	else if(m_addr_family == AF_INET6)
	{
		return memcmp(m_address.ipv6.sin6_addr.s6_addr,
		              address.m_address.ipv6.sin6_addr.s6_addr, 16) == 0;
	}
	else
		return false;
}

bool Address::operator!=(Address &address)
{
	return !(*this == address);
}

void Address::Resolve(const char *name)
{
	if (!name || name[0] == 0) {
		if (m_addr_family == AF_INET) {
			setAddress((u32) 0);
		} else if (m_addr_family == AF_INET6) {
			setAddress((IPv6AddressBytes*) 0);
		}
		return;
	}

	struct addrinfo *resolved, hints;
	memset(&hints, 0, sizeof(hints));
	
	// Setup hints
	hints.ai_socktype = 0;
	hints.ai_protocol = 0;
	hints.ai_flags    = 0;
	if(g_settings->getBool("enable_ipv6"))
	{
		// AF_UNSPEC allows both IPv6 and IPv4 addresses to be returned
		hints.ai_family = AF_UNSPEC;
	}
	else
	{
		hints.ai_family = AF_INET;
	}
	
	// Do getaddrinfo()
	int e = getaddrinfo(name, NULL, &hints, &resolved);
	if(e != 0)
		throw ResolveError(gai_strerror(e));

	// Copy data
	if(resolved->ai_family == AF_INET)
	{
		struct sockaddr_in *t = (struct sockaddr_in *) resolved->ai_addr;
		m_addr_family = AF_INET;
		m_address.ipv4 = *t;
	}
	else if(resolved->ai_family == AF_INET6)
	{
		struct sockaddr_in6 *t = (struct sockaddr_in6 *) resolved->ai_addr;
		m_addr_family = AF_INET6;
		m_address.ipv6 = *t;
	}
	else
	{
		freeaddrinfo(resolved);
		throw ResolveError("");
	}
	freeaddrinfo(resolved);
}

// IP address -> textual representation
std::string Address::serializeString() const
{
// windows XP doesnt have inet_ntop, maybe use better func
#ifdef _WIN32
	if(m_addr_family == AF_INET)
	{
		u8 a, b, c, d;
		u32 addr;
		addr = ntohl(m_address.ipv4.sin_addr.s_addr);
		a = (addr & 0xFF000000) >> 24;
		b = (addr & 0x00FF0000) >> 16;
		c = (addr & 0x0000FF00) >> 8;
		d = (addr & 0x000000FF);
		return itos(a) + "." + itos(b) + "." + itos(c) + "." + itos(d);
	}
	else if(m_addr_family == AF_INET6)
	{
		std::ostringstream os;
		for(int i = 0; i < 16; i += 2)
		{
			u16 section =
			(m_address.ipv6.sin6_addr.s6_addr[i] << 8) |
			(m_address.ipv6.sin6_addr.s6_addr[i + 1]);
			os << std::hex << section;
			if(i < 14)
				os << ":";
		}
		return os.str();
	}
	else
		return std::string("");
#else
	char str[INET6_ADDRSTRLEN];
	if (inet_ntop(m_addr_family, (m_addr_family == AF_INET) ? (void*)&(m_address.ipv4.sin_addr) : (void*)&(m_address.ipv6.sin6_addr), str, INET6_ADDRSTRLEN) == NULL) {
		return std::string("");
	}
	return std::string(str);
#endif
}

struct sockaddr_in Address::getAddress() const
{
	return m_address.ipv4; // NOTE: NO PORT INCLUDED, use getPort()
}

struct sockaddr_in6 Address::getAddress6() const
{
	return m_address.ipv6; // NOTE: NO PORT INCLUDED, use getPort()
}

u16 Address::getPort() const
{
	return m_port;
}

int Address::getFamily() const
{
	return m_addr_family;
}

bool Address::isIPv6() const
{
	return m_addr_family == AF_INET6;
}

bool Address::isZero() const
{
	if (m_addr_family == AF_INET) {
		return m_address.ipv4.sin_addr.s_addr == 0;
	} else if (m_addr_family == AF_INET6) {
		static const char zero[16] = {0};
		return memcmp(m_address.ipv6.sin6_addr.s6_addr,
		              zero, 16) == 0;
	}
	return false;
}

void Address::setAddress(u32 address)
{
	m_addr_family = AF_INET;
	m_address.ipv4.sin_family = AF_INET;
	m_address.ipv4.sin_addr.s_addr = htonl(address);
}

void Address::setAddress(u8 a, u8 b, u8 c, u8 d)
{
	m_addr_family = AF_INET;
	m_address.ipv4.sin_family = AF_INET;
	u32 addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
	m_address.ipv4.sin_addr.s_addr = addr;
}

void Address::setAddress(const IPv6AddressBytes * ipv6_bytes)
{
	m_addr_family = AF_INET6;
	m_address.ipv6.sin6_family = AF_INET6;
	if(ipv6_bytes)
		memcpy(m_address.ipv6.sin6_addr.s6_addr, ipv6_bytes->bytes, 16);
	else
		memset(m_address.ipv6.sin6_addr.s6_addr, 0, 16);
}

void Address::setPort(u16 port)
{
	m_port = port;
}

void Address::print(std::ostream *s) const
{
	if(m_addr_family == AF_INET6)
	{
		(*s) << "[" << serializeString() << "]:" << m_port;
	}
	else
	{
		(*s) << serializeString() << ":" << m_port;
	}
}

/*
	UDPSocket
*/

UDPSocket::UDPSocket(bool ipv6)
{
	if(g_sockets_initialized == false)
		throw SocketException("Sockets not initialized");

	// Use IPv6 if specified
	m_addr_family = ipv6 ? AF_INET6 : AF_INET;
	m_handle = socket(m_addr_family, SOCK_DGRAM, IPPROTO_UDP);
	
	if(socket_enable_debug_output)
	{
		dstream << "UDPSocket(" << (int) m_handle
		        << ")::UDPSocket(): ipv6 = "
		        << (ipv6 ? "true" : "false")
		        << std::endl;
	}

	if(m_handle <= 0)
	{
		throw SocketException("Failed to create socket");
	}

	setTimeoutMs(0);
}

UDPSocket::~UDPSocket()
{
	if(socket_enable_debug_output)
	{
		dstream << "UDPSocket( " << (int) m_handle << ")::~UDPSocket()"
		        << std::endl;
	}

#ifdef _WIN32
	closesocket(m_handle);
#else
	close(m_handle);
#endif
}

void UDPSocket::Bind(Address addr)
{
	if(socket_enable_debug_output)
	{
		dstream << "UDPSocket(" << (int) m_handle << ")::Bind(): "
		        << addr.serializeString() << ":"
		        << addr.getPort() << std::endl;
	}

	if (addr.getFamily() != m_addr_family)
	{
		char errmsg[] = "Socket and bind address families do not match";
		errorstream << "Bind failed: " << errmsg << std::endl;
		throw SocketException(errmsg);
	}

	if(m_addr_family == AF_INET6)
	{
		struct sockaddr_in6 address;
		memset(&address, 0, sizeof(address));

		address             = addr.getAddress6();
		address.sin6_family = AF_INET6;
		address.sin6_port   = htons(addr.getPort());

		if(bind(m_handle, (const struct sockaddr *) &address,
				sizeof(struct sockaddr_in6)) < 0)
		{
			dstream << (int) m_handle << ": Bind failed: "
			        << strerror(errno) << std::endl;
			throw SocketException("Failed to bind socket");
		}
	}
	else
	{
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));

		address                 = addr.getAddress();
		address.sin_family      = AF_INET;
		address.sin_port        = htons(addr.getPort());

		if(bind(m_handle, (const struct sockaddr *) &address,
		        sizeof(struct sockaddr_in)) < 0)
		{
			dstream << (int) m_handle << ": Bind failed: "
			        << strerror(errno) << std::endl;
			throw SocketException("Failed to bind socket");
		}
	}
}

void UDPSocket::Send(const Address & destination, const void * data, int size)
{
	bool dumping_packet = false; // for INTERNET_SIMULATOR

	if(INTERNET_SIMULATOR)
		dumping_packet = (myrand() % INTERNET_SIMULATOR_PACKET_LOSS == 0);

	if(socket_enable_debug_output)
	{
		// Print packet destination and size
		dstream << (int) m_handle << " -> ";
		destination.print(&dstream);
		dstream << ", size=" << size;
		
		// Print packet contents
		dstream << ", data=";
		for(int i = 0; i < size && i < 20; i++)
		{
			if(i % 2 == 0)
				dstream << " ";
			unsigned int a = ((const unsigned char *) data)[i];
			dstream << std::hex << std::setw(2) << std::setfill('0')
				<< a;
		}
		
		if(size > 20)
			dstream << "...";
		
		if(dumping_packet)
			dstream << " (DUMPED BY INTERNET_SIMULATOR)";
		
		dstream << std::endl;
	}

	if(dumping_packet)
	{
		// Lol let's forget it
		dstream << "UDPSocket::Send(): "
				   "INTERNET_SIMULATOR: dumping packet."
				<< std::endl;
		return;
	}

	if(destination.getFamily() != m_addr_family)
		throw SendFailedException("Address family mismatch");

	int sent;
	if(m_addr_family == AF_INET6)
	{
		struct sockaddr_in6 address = destination.getAddress6();
		address.sin6_port = htons(destination.getPort());
		sent = sendto(m_handle, (const char *) data, size,
			0, (struct sockaddr *) &address, sizeof(struct sockaddr_in6));
	}
	else
	{
		struct sockaddr_in address = destination.getAddress();
		address.sin_port = htons(destination.getPort());
		sent = sendto(m_handle, (const char *) data, size,
			0, (struct sockaddr *) &address, sizeof(struct sockaddr_in));
	}

	if(sent != size)
	{
		throw SendFailedException("Failed to send packet");
	}
}

int UDPSocket::Receive(Address & sender, void * data, int size)
{
	// Return on timeout
	if(WaitData(m_timeout_ms) == false)
	{
		return -1;
	}

	int received;
	if(m_addr_family == AF_INET6)
	{
		struct sockaddr_in6 address;
		memset(&address, 0, sizeof(address));
		socklen_t address_len = sizeof(address);

		received = recvfrom(m_handle, (char *) data,
				size, 0, (struct sockaddr *) &address, &address_len);

		if(received < 0)
			return -1;

		u16 address_port = ntohs(address.sin6_port);
		IPv6AddressBytes bytes;
		memcpy(bytes.bytes, address.sin6_addr.s6_addr, 16);
		sender = Address(&bytes, address_port);
	}
	else
	{
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));

		socklen_t address_len = sizeof(address);

		received = recvfrom(m_handle, (char *) data,
				size, 0, (struct sockaddr *) &address, &address_len);

		if(received < 0)
			return -1;

		u32 address_ip = ntohl(address.sin_addr.s_addr);
		u16 address_port = ntohs(address.sin_port);

		sender = Address(address_ip, address_port);
	}

	if(socket_enable_debug_output)
	{
		// Print packet sender and size
		dstream << (int) m_handle << " <- ";
		sender.print(&dstream);
		dstream << ", size=" << received;
		
		// Print packet contents
		dstream << ", data=";
		for(int i = 0; i < received && i < 20; i++)
		{
			if(i % 2 == 0)
				dstream << " ";
			unsigned int a = ((const unsigned char *) data)[i];
			dstream << std::hex << std::setw(2) << std::setfill('0')
				<< a;
		}
		if(received > 20)
			dstream << "...";
		
		dstream << std::endl;
	}

	return received;
}

int UDPSocket::GetHandle()
{
	return m_handle;
}

void UDPSocket::setTimeoutMs(int timeout_ms)
{
	m_timeout_ms = timeout_ms;
}

bool UDPSocket::WaitData(int timeout_ms)
{
	fd_set readset;
	int result;

	// Initialize the set
	FD_ZERO(&readset);
	FD_SET(m_handle, &readset);

	// Initialize time out struct
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = timeout_ms * 1000;

	// select()
	result = select(m_handle+1, &readset, NULL, NULL, &tv);

	if(result == 0)
		return false;
	else if(result < 0 && (errno == EINTR || errno == EBADF))
		// N.B. select() fails when sockets are destroyed on Connection's dtor
		// with EBADF.  Instead of doing tricky synchronization, allow this
		// thread to exit but don't throw an exception.
		return false;
	else if(result < 0)
	{
		dstream << (int) m_handle << ": Select failed: "
		        << strerror(errno) << std::endl;

#ifdef _WIN32
		int e = WSAGetLastError();
		dstream << (int) m_handle << ": WSAGetLastError()="
		        << e << std::endl;
		if(e == 10004 /* = WSAEINTR */ || e == 10009 /*WSAEBADF*/)
		{
			dstream << "WARNING: Ignoring WSAEINTR/WSAEBADF." << std::endl;
			return false;
		}
#endif

		throw SocketException("Select failed");
	}
	else if(FD_ISSET(m_handle, &readset) == false)
	{
		// No data
		return false;
	}
	
	// There is data
	return true;
}
