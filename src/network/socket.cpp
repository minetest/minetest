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

#ifndef _WIN32
	#include <sys/types.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <arpa/inet.h>
#endif

// Windows defines setsockopt with a char* fourth parameter instead of the
// POSIX void*, but Cygwin fixes this.
#if defined(_WIN32) && !defined(__CYGWIN__)
	#define setsockopt(a, b, c, d, e) \
		setsockopt(a, b, c, reinterpret_cast<const char*>(d), e)
#endif

// Set to true to enable verbose debug output
bool socket_enable_debug_output = false;


// Windows needs sockets to be initialized before use
#ifdef _WIN32
static bool g_sockets_initialized = false;

static inline void cleanup_sockets() { WSACleanup(); }

static inline void init_sockets()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data) != NO_ERROR)
		throw SocketException("Failed to initialize Windows sockets!");
	atexit(cleanup_sockets);
	g_sockets_initialized = true;
}
#endif



// Address

// Equality (family, host, and port must be equal)
bool Address::operator == (const Address &other)
{
	if (other.family != family || other.port != port)
		return false;

	switch (family) {
	case AF_INET: return host.v4.s_addr == other.host.v4.s_addr;
	case AF_INET6: return memcmp(host.v6.s6_addr,
			other.host.v6.s6_addr, 16) == 0;
	default:
		return false;
	}
}

void Address::resolve(const std::string &name)
{
	if (name.empty())
		return setZero();

	struct addrinfo *resolved, hints = {0};

	// Setup hints
	// AF_UNSPEC allows both IPv6 and IPv4 addresses to be returned
	hints.ai_family = g_settings->getBool("enable_ipv6") ?
			AF_UNSPEC : AF_INET;

	// Do getaddrinfo()
	int err = getaddrinfo(name.c_str(), NULL, &hints, &resolved);
	if (err)
		throw ResolveError(gai_strerror(err));

	// Copy data
	switch (resolved->ai_family) {
	case AF_INET:
		host.v4 = reinterpret_cast<sockaddr_in *>(resolved->ai_addr)->sin_addr;
		break;
	case AF_INET6:
		host.v6 = reinterpret_cast<sockaddr_in6*>(resolved->ai_addr)->sin6_addr;
		break;
	default:
		freeaddrinfo(resolved);
		throw ResolveError("Unknown address family");
	}
	family = resolved->ai_family;
	freeaddrinfo(resolved);
}

void Address::deserialize(const std::string &str)
{
	std::istringstream is(str);
	bool bracketed = (is.peek() == '[');

	if (bracketed)
		is.ignore();

	std::string addr;
	std::getline(is, addr, bracketed ? ']' : ':');
	resolve(addr);

	if (bracketed)
		is.ignore();

	is >> port;
}

std::string Address::serializeHost() const
{
	// Windows XP doesn't have inet_ntop, so use getnameinfo
	char str[INET6_ADDRSTRLEN];
	SockAddrUnion addr;
	int addr_sz = getSocketAddress(&addr);
	if (int err = getnameinfo(reinterpret_cast<const sockaddr *>(&(addr)),
			addr_sz, str, INET6_ADDRSTRLEN,
			NULL, 0, NI_NUMERICHOST))
		throw SerializationError(
			std::string("Unable to serialize address: ") +
				gai_strerror(err));
	return str;
}

void Address::serialize(std::ostream &os) const
{
	if (isIPv6())
		os << '[';
	os << serializeHost();
	if (isIPv6())
		os << ']';
	os << ':' << std::dec << port;
}

std::string Address::serialize() const
{
	std::ostringstream os;
	serialize(os);
	return os.str();
}

bool Address::isZero() const
{
	switch (family) {
	case AF_INET: return host.v4.s_addr == 0;
	case AF_INET6: return memcmp(&host.v6, &in6addr_any, 16) == 0;
	}
	return false;
}

socklen_t Address::getSocketAddress(SockAddrUnion *addr) const
{
	memset(addr, 0, sizeof(*addr));
	switch (family) {
	case AF_INET:
		addr->v4.sin_family = family;
		addr->v4.sin_port = htons(port);
		addr->v4.sin_addr = host.v4;
		return sizeof(addr->v4);
	case AF_INET6:
		addr->v6.sin6_family = family;
		addr->v6.sin6_port = htons(port);
		addr->v6.sin6_addr = host.v6;
		return sizeof(addr->v6);
	}
	return 0;
}

bool Address::isLoopback() const
{
	switch (family) {
	case AF_INET: return ntohl(host.v4.s_addr) == INADDR_LOOPBACK;
	case AF_INET6: return memcmp(&host.v6, &in6addr_loopback, 16) == 0;
	}
	return false;
}

void Address::setLoopback(int family)
{
	if (family)
		this->family = family;
	switch (this->family) {
	case AF_INET:
		host.v4.s_addr = htonl(INADDR_LOOPBACK);
		break;
	case AF_INET6:
		host.v6 = in6addr_loopback;
		break;
	}
}

void Address::setHost(const SockAddrUnion &addr, int family)
{
	if (family)
		this->family = family;
	switch (this->family) {
	case AF_INET:
		port = ntohs(addr.v4.sin_port);
		host.v4 = addr.v4.sin_addr;
		break;
	case AF_INET6:
		port = ntohs(addr.v6.sin6_port);
		host.v6 = addr.v6.sin6_addr;
		break;
	}
}

void Address::setHost(u32 addr)
{
	family = AF_INET;
	host.v4.s_addr = htonl(addr);
}

void Address::setHost(const u8 bytes[16])
{
	family = AF_INET6;
	if (bytes)
		memcpy(host.v6.s6_addr, bytes, 16);
	else
		setZero();
}


// UDPSocket

bool UDPSocket::init(const Address &address, bool except)
{
#ifdef _WIN32
	if (!g_sockets_initialized)
		init_sockets();
#endif
	m_address = address;
	m_handle = socket(address.getFamily(), SOCK_DGRAM, IPPROTO_UDP);

	if (socket_enable_debug_output) {
		dstream << "UDPSocket::init: handle: " << (int) m_handle << std::endl;
	}

	if (m_handle <= 0) {
		if (!except)
			return false;
		throw SocketException(
			std::string("Failed to create socket: ") +
			strerror(errno));
	}

	if (!setNonBlocking()) {
		if (!except)
			return false;
		throw SocketException(
			std::string("Failed to set socket non-blocking: ") +
			strerror(errno));
	}

	return bind(except);
}

UDPSocket::~UDPSocket()
{
	if (socket_enable_debug_output) {
		dstream << "UDPSocket(" << (int) m_handle << ")::~UDPSocket()"
		        << std::endl;
	}

	//delete m_paired;

#ifdef _WIN32
	closesocket(m_handle);
#else
	close(m_handle);
#endif
}


bool UDPSocket::bind(bool except)
{
	if (socket_enable_debug_output) {
		dstream << "UDPSocket::bind(" << (int) m_handle << ", "
			<< m_address.serialize() << ')' << std::endl;
	}

#ifdef IPV6_V6ONLY
	if (m_address.isIPv6()) {
		int on = 1;
		if (setsockopt(m_handle, IPPROTO_IPV6, IPV6_V6ONLY,
				&on, sizeof(on))) {
			std::string errstr = strerror(errno);
			errorstream << (int) m_handle << ": Setting IPv6 socket ";
			m_address.serialize(errorstream);
			errorstream << " IPv6-only failed: " << errstr << std::endl;
			if (!except)
				return false;
			throw SocketException("Failed to set IPv6 socket IPv6-only: " + errstr);
		}
	}
#endif

	SockAddrUnion addr;
	socklen_t sl = m_address.getSocketAddress(&addr);

	if (::bind(m_handle, reinterpret_cast<const sockaddr *>(&addr), sl)) {
		std::string errstr = strerror(errno);
		errorstream << (int) m_handle << ": Bind to ";
		m_address.serialize(errorstream);
		errorstream << " failed: " << errstr << std::endl;
		if (!except)
			return false;
		throw SocketException("Failed to bind socket: " + errstr);
	}

	if (m_address.getPort() == 0)
		updateAddress();
	return true;
}

void UDPSocket::updateAddress()
{
	SockAddrUnion addr = {0};
	socklen_t sl = sizeof(addr);
	getsockname(m_handle, reinterpret_cast<sockaddr *>(&addr), &sl);
	m_address.setHost(addr);
}

void UDPSocket::send(const Address &destination, const void *data, int size)
{
	bool dumping_packet = false; // for INTERNET_SIMULATOR

	if (INTERNET_SIMULATOR)
		dumping_packet = myrand() % INTERNET_SIMULATOR_PACKET_LOSS == 0;

	if (socket_enable_debug_output) {
		// Print packet destination and size
		dstream << (int)m_handle << " -> ";
		destination.serialize(dstream);
		dstream << ", size=" << size;

		// Print packet contents
		dstream << ", data=" << std::hex;
		for (int i = 0; i < size && i < 20; i++) {
			if (i && i % 2 == 0)
				dstream << " ";
			unsigned int a = ((const unsigned char *) data)[i];
			dstream << std::setw(2) << std::setfill('0') << a;
		}

		if (size > 20)
			dstream << "...";

		if (dumping_packet)
			dstream << " (DUMPED BY INTERNET_SIMULATOR)";

		dstream << std::dec << std::endl;
	}

	if (dumping_packet) {
		// LOL, let's forget it
		dstream << "UDPSocket::Send(): INTERNET_SIMULATOR: dumping packet."
				<< std::endl;
		return;
	}

	if (destination.getFamily() != m_address.getFamily())
		throw SendFailedException("Address family mismatch");

	SockAddrUnion addr;
	socklen_t sl = destination.getSocketAddress(&addr);

	int sent = sendto(m_handle, static_cast<const char *>(data), size, 0,
			reinterpret_cast<sockaddr *>(&addr), sl);

	if (sent != size)
		throw SendFailedException("Failed to send packet");
}

int UDPSocket::receive(Address &sender, void *data, int size)
{
	SockAddrUnion addr = {0};
	socklen_t sl = sizeof(addr);

	int received = recvfrom(m_handle, (char *) data, size, 0,
		reinterpret_cast<sockaddr *>(&addr), &sl);

	if (received < 0)
		return -1;

	sender.setHost(addr, m_address.getFamily());

	if (socket_enable_debug_output) {
		// Print packet sender and size
		dstream << (int) m_handle << " <- ";
		sender.serialize(dstream);
		dstream << ", size=" << received;

		// Print packet contents
		dstream << ", data=" << std::hex;
		for (int i = 0; i < received && i < 20; i++) {
			if (i && i % 2 == 0)
				dstream << " ";
			unsigned int a = ((const unsigned char *) data)[i];
			dstream << std::setw(2) << std::setfill('0') << a;
		}
		if (received > 20)
			dstream << "...";

		dstream << std::dec << std::endl;
	}

	return received;
}

bool UDPSocket::setNonBlocking()
{
#ifdef WIN32
	unsigned long mode = 1;
	return ioctlsocket(m_handle, FIONBIO, &mode) == 0;
#else
	int flags = fcntl(m_handle, F_GETFL, 0);
	if (flags < 0)
		return false;
	return fcntl(m_handle, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}
