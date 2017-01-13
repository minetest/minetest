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
	// Without this some of the network functions are not found on mingw
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
typedef SOCKET socket_t;
typedef int socklen_t;
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
typedef int socket_t;
#endif


#include <ostream>
#include <string.h>
#include "irrlichttypes.h"
#include "exceptions.h"

extern bool socket_enable_debug_output;

struct SocketException : public BaseException {
	SocketException(const std::string &s) : BaseException(s) {}
};
struct ResolveError : public BaseException {
	ResolveError(const std::string &s) : BaseException(s) {}
};
struct SendFailedException : public BaseException {
	SendFailedException(const std::string &s) : BaseException(s) {}
};

union SockAddrUnion {
	sockaddr_in v4;
	sockaddr_in6 v6;
};

class Address
{
public:
	Address() : family(0), port(0) { setZero(); }
	Address(u32 host, u16 port) : port(port) { setHost(host); }
	Address(u8 a, u8 b, u8 c, u8 d, u16 port) : port(port)
		{ setHost(a, b, c, d); }
	Address(const u8 bytes[16], u16 port) : port(port) { setHost(bytes); }

	bool operator == (const Address &other);
	bool operator != (const Address &other) { return !(*this == other); }

	// May throw ResolveError (address is unchanged in this case)
	void resolve(const std::string &name);

	socklen_t getSocketAddress(SockAddrUnion*) const;
	u16 getPort() const { return port; }
	int getFamily() const { return family; }
	bool isIPv6() const { return family == AF_INET6; }
	bool isZero() const;
	bool isLoopback() const;

	void setHost(u32 host);
	void setHost(u8 a, u8 b, u8 c, u8 d)
		{ setHost((a << 24) | (b << 16) | (c << 8) | d); }
	void setHost(const u8 bytes[16]);
	void setHost(const SockAddrUnion &addr, int family=0);
	void setLoopback(int family=0);
	void setZero() { memset(&host, 0, sizeof(host)); }
	void setPort(u16 p) { port = p; }
	void setFamily(int f) { family = f; }

	void serialize(std::ostream &os) const;
	std::string serialize() const;
	std::string serializeHost() const;
	void deserialize(const std::string &str);

private:
	int family;
	u16 port;
	union {
		in_addr  v4;
		in6_addr v6;
	} host;
};

class UDPSocket
{
public:
	UDPSocket() : m_handle(0) {}
	~UDPSocket();
	bool init(const Address&, bool except = true);
	bool bind(bool except = true);

	void send(const Address&, const void *data, int size);
	// Returns -1 if there is no data
	int receive(Address&, void *data, int size);

	socket_t getHandle() const { return m_handle; }
	const Address &getAddress() const { return m_address; }

private:
	void updateAddress();
	bool setNonBlocking();

	socket_t m_handle;
	Address m_address;
};

#endif
