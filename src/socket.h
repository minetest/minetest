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

#ifndef SOCKET_HEADER
#define SOCKET_HEADER

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

#include <ostream>
#include <string.h>
#include "irrlichttypes.h"
#include "exceptions.h"

extern bool socket_enable_debug_output;

class SocketException : public BaseException
{
public:
	SocketException(const char *s):
		BaseException(s)
	{
	}
};

class ResolveError : public BaseException
{
public:
	ResolveError(const char *s):
		BaseException(s)
	{
	}
};

class SendFailedException : public BaseException
{
public:
	SendFailedException(const char *s):
		BaseException(s)
	{
	}
};

void sockets_init();
void sockets_cleanup();

class IPv6AddressBytes
{
public:
	u8 bytes[16];
	IPv6AddressBytes() { memset(bytes, 0, 16); }
};

class Address
{
public:
	Address();
	Address(u32 address, u16 port);
	Address(u8 a, u8 b, u8 c, u8 d, u16 port);
	Address(const IPv6AddressBytes * ipv6_bytes, u16 port);
	bool operator==(Address &address);
	bool operator!=(Address &address);
	// Resolve() may throw ResolveError (address is unchanged in this case)
	void Resolve(const char *name);
	struct sockaddr_in getAddress() const;
	unsigned short getPort() const;
	void setAddress(u32 address);
	void setAddress(u8 a, u8 b, u8 c, u8 d);
	void setAddress(const IPv6AddressBytes * ipv6_bytes);
	struct sockaddr_in6 getAddress6() const;
	int getFamily() const;
	bool isIPv6() const;
	bool isZero() const;
	void setPort(unsigned short port);
	void print(std::ostream *s) const;
	std::string serializeString() const;
private:
	unsigned int m_addr_family;
	union
	{
		struct sockaddr_in  ipv4;
		struct sockaddr_in6 ipv6;
	} m_address;
	u16 m_port; // Port is separate from sockaddr structures
};

class UDPSocket
{
public:
	UDPSocket(bool ipv6);
	~UDPSocket();
	void Bind(Address addr);
	//void Close();
	//bool IsOpen();
	void Send(const Address & destination, const void * data, int size);
	// Returns -1 if there is no data
	int Receive(Address & sender, void * data, int size);
	int GetHandle(); // For debugging purposes only
	void setTimeoutMs(int timeout_ms);
	// Returns true if there is data, false if timeout occurred
	bool WaitData(int timeout_ms);
private:
	int m_handle;
	int m_timeout_ms;
	int m_addr_family;
};

#endif

