/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "constants.h"
#include "debug.h"
#include "config.h"
#include "settings.h"
#include "main.h" // for g_settings
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include "util/string.h"
#include "util/numeric.h"

bool socket_enable_debug_output = false;
#define DP socket_enable_debug_output
// This is prepended to everything printed here
#define DPS ""

bool g_sockets_initialized = false;

void sockets_init()
{
#ifdef _WIN32
	WSADATA WsaData;
	if(WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR)
		throw SocketException("WSAStartup failed");
#else
#endif
	g_sockets_initialized = true;
}

void sockets_cleanup()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

Address::Address()
{
#if USE_IPV6
	m_addr_family = 0;
#endif
	memset(&m_address, 0, sizeof m_address);
	m_port = 0;
}

Address::Address(unsigned int address, unsigned short port)
{
#if USE_IPV6
	m_addr_family = AF_INET;
#endif
	m_address.ipv4.sin_family = AF_INET;
	m_address.ipv4.sin_addr.s_addr = htonl(address);
	m_port = port;
}

Address::Address(unsigned int a, unsigned int b,
		unsigned int c, unsigned int d,
		unsigned short port)
{
#if USE_IPV6
	m_addr_family = AF_INET;
#endif
	m_address.ipv4.sin_family = AF_INET;
	m_address.ipv4.sin_addr.s_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
	m_port = port;
}

#if USE_IPV6
Address::Address(const unsigned char * ipv6_bytes, unsigned short port)
{
	m_addr_family = AF_INET6;
	m_address.ipv6.sin6_family = AF_INET6;
	if(ipv6_bytes)
		memcpy(m_address.ipv6.sin6_addr.s6_addr, ipv6_bytes, 16);
	else
		memset(m_address.ipv6.sin6_addr.s6_addr, 0, 16);
	m_port = port;
}
#endif

bool Address::operator==(Address &address)
{
#if USE_IPV6
	if(address.m_addr_family != m_addr_family || address.m_port != m_port)
		return false;
	else if(m_addr_family == AF_INET)
	{
		return m_address.ipv4.sin_addr.s_addr == address.m_address.ipv4.sin_addr.s_addr;
	}
	else if(m_addr_family == AF_INET6)
	{
		return memcmp(m_address.ipv6.sin6_addr.s6_addr,
			      address.m_address.ipv6.sin6_addr.s6_addr, 16) == 0;
	}
	else
		return false;
#else
	return address.m_port == m_port &&
		m_address.ipv4.sin_addr.s_addr == address.m_address.ipv4.sin_addr.s_addr;
#endif
}

bool Address::operator!=(Address &address)
{
	return !(*this == address);
}

void Address::Resolve(const char *name)
{
#if USE_IPV6
	struct addrinfo *resolved, hints;
	hints.ai_socktype = 0;
	hints.ai_protocol = 0;
	hints.ai_flags    = 0;
	if(g_settings->getBool("enable_ipv6"))
	{
		hints.ai_family = AF_UNSPEC;
	}
	else
	{
		hints.ai_family = AF_INET;
	}
	int e = getaddrinfo(name, NULL, &hints, &resolved);
	if(e != 0)
		throw ResolveError("");

	if(resolved->ai_family == AF_INET)
	{
		struct sockaddr_in *t = (struct sockaddr_in*)resolved->ai_addr;
		m_addr_family = AF_INET;
		m_address.ipv4 = *t;
	}
	else if(resolved->ai_family == AF_INET6)
	{
		struct sockaddr_in6 *t = (struct sockaddr_in6*)resolved->ai_addr;
		m_addr_family = AF_INET6;
		m_address.ipv6 = *t;
	}
	else
	{
		freeaddrinfo(resolved);
		throw ResolveError("");
	}
	freeaddrinfo(resolved);
#else
	struct addrinfo * resolved;
	int e = getaddrinfo(name, NULL, NULL, &resolved);
	if(e != 0)
		throw ResolveError("");
	m_address.ipv4 = *((struct sockaddr_in *) resolved->ai_addr);
	freeaddrinfo(resolved);
#endif
}

std::string Address::serializeString() const
{
#if USE_IPV6
	if(m_addr_family == AF_INET)
	{
		unsigned int a, b, c, d, addr;
		addr = ntohl(m_address.ipv4.sin_addr.s_addr);
		a = (addr & 0xFF000000)>>24;
		b = (addr & 0x00FF0000)>>16;
		c = (addr & 0x0000FF00)>>8;
		d = (addr & 0x000000FF);
		return itos(a)+"."+itos(b)+"."+itos(c)+"."+itos(d);
	}
	else if(m_addr_family == AF_INET6)
	{
		std::ostringstream os;
		for(int i = 0; i < 16; i += 2)
		{
			unsigned short section =
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
	unsigned int a, b, c, d, addr;
	addr = ntohl(m_address.ipv4.sin_addr.s_addr);
	a = (addr & 0xFF000000)>>24;
	b = (addr & 0x00FF0000)>>16;
	c = (addr & 0x0000FF00)>>8;
	d = (addr & 0x000000FF);
	return itos(a)+"."+itos(b)+"."+itos(c)+"."+itos(d);
#endif
}

struct sockaddr_in Address::getAddress() const
{
	return m_address.ipv4; // NOTE: NO PORT INCLUDED, use getPort()
}

#if USE_IPV6
struct sockaddr_in6 Address::getAddress6() const
{
	return m_address.ipv6; // NOTE: NO PORT INCLUDED, use getPort()
}
#endif

unsigned short Address::getPort() const
{
	return m_port;
}

#if USE_IPV6
int Address::getFamily() const
{
	return m_addr_family;
}

bool Address::isIPv6() const
{
	return m_addr_family == AF_INET6;
}
#endif

void Address::setAddress(unsigned int address)
{
#if USE_IPV6
	m_addr_family = AF_INET;
#endif
	m_address.ipv4.sin_family = AF_INET;
	m_address.ipv4.sin_addr.s_addr = htonl(address);
}

void Address::setAddress(unsigned int a, unsigned int b,
		unsigned int c, unsigned int d)
{
#if USE_IPV6
	m_addr_family = AF_INET;
#endif
	m_address.ipv4.sin_family = AF_INET;
	m_address.ipv4.sin_addr.s_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
	dstream << m_address.ipv4.sin_addr.s_addr << std::endl;
}

#if USE_IPV6
void Address::setAddress(const unsigned char * ipv6_bytes)
{
	m_addr_family = AF_INET6;
	m_address.ipv6.sin6_family = AF_INET6;
	if(ipv6_bytes)
		memcpy(m_address.ipv6.sin6_addr.s6_addr, ipv6_bytes, 16);
	else
		memset(m_address.ipv6.sin6_addr.s6_addr, 0, 16);
}
#endif

void Address::setPort(unsigned short port)
{
	m_port = port;
}

void Address::print(std::ostream *s) const
{
#if USE_IPV6
	if(m_addr_family == AF_INET6)
	{
		(*s) << "[" << serializeString() << "]:" << m_port;
	}
	else
	{
		(*s) << serializeString() << ":" << m_port;
	}
#else
	(*s) << serializeString() << ":" << m_port;
#endif
}

void Address::print() const
{
	print(&dstream);
}

// Yes, this is a preprocessor mess, but whatever . . . .
#if USE_IPV6
UDPSocket::UDPSocket(bool ipv6)
#else
UDPSocket::UDPSocket()
#endif
{
	if(g_sockets_initialized == false)
		throw SocketException("Sockets not initialized");

#if USE_IPV6
	// Use IPv6 if specified
	m_addr_family = ipv6 ? AF_INET6 : AF_INET;
	m_handle = socket(m_addr_family, SOCK_DGRAM, IPPROTO_UDP);
	if(DP)
	{
		dstream<<DPS<<"UDPSocket("<<(int)m_handle<<")::UDPSocket(): ipv6 = "
			<<(g_settings->getBool("enable_ipv6") ? "true" : "false")<<std::endl;
	}
#else
	// IPv4-only code
	m_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(DP)
		dstream<<DPS<<"UDPSocket("<<(int)m_handle<<")::UDPSocket()"<<std::endl;
#endif

	if(m_handle <= 0)
	{
		throw SocketException("Failed to create socket");
	}

/*#ifdef _WIN32
	DWORD nonblocking = 0;
	if(ioctlsocket(m_handle, FIONBIO, &nonblocking) != 0)
	{
		throw SocketException("Failed set non-blocking mode");
	}
#else
	int nonblocking = 0;
	if(fcntl(m_handle, F_SETFL, O_NONBLOCK, nonblocking) == -1)
	{
		throw SocketException("Failed set non-blocking mode");
	}
#endif*/

	setTimeoutMs(0);
}

UDPSocket::~UDPSocket()
{
	if(DP)
	dstream<<DPS<<"UDPSocket("<<(int)m_handle<<")::~UDPSocket()"<<std::endl;

#ifdef _WIN32
	closesocket(m_handle);
#else
	close(m_handle);
#endif
}

void UDPSocket::Bind(unsigned short port)
{
	if(DP)
	dstream<<DPS<<"UDPSocket("<<(int)m_handle
			<<")::Bind(): port="<<port<<std::endl;

#if USE_IPV6
	if(m_addr_family == AF_INET6)
	{
		sockaddr_in6 address;
		address.sin6_family = AF_INET6;
		address.sin6_addr = in6addr_any;
		address.sin6_port = htons(port);
		if(bind(m_handle, (const sockaddr*)&address, sizeof(sockaddr_in6)) < 0)
		{
#ifndef DISABLE_ERRNO
			dstream<<(int)m_handle<<": Bind failed: "<<strerror(errno)<<std::endl;
#endif // !DISABLE_ERRNO
			throw SocketException("Failed to bind socket");
		}
	}
	else
	{
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port);
		if(bind(m_handle, (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
		{
#ifndef DISABLE_ERRNO
			dstream<<(int)m_handle<<": Bind failed: "<<strerror(errno)<<std::endl;
#endif // !DISABLE_ERRNO
			throw SocketException("Failed to bind socket");
		}
	}
#else
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if(bind(m_handle, (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
	{
#ifndef DISABLE_ERRNO
		dstream<<(int)m_handle<<": Bind failed: "<<strerror(errno)<<std::endl;
#endif // !DISABLE_ERRNO
		throw SocketException("Failed to bind socket");
	}
#endif // USE_IPV6
}

void UDPSocket::Send(const Address & destination, const void * data, int size)
{
	bool dumping_packet = false;
	if(INTERNET_SIMULATOR)
		dumping_packet = (myrand()%10==0); //easy
		//dumping_packet = (myrand()%4==0); // hard

	if(DP){
		/*dstream<<DPS<<"UDPSocket("<<(int)m_handle
				<<")::Send(): destination=";*/
		dstream<<DPS;
		dstream<<(int)m_handle<<" -> ";
		destination.print();
		dstream<<", size="<<size<<", data=";
		for(int i=0; i<size && i<20; i++){
			if(i%2==0) DEBUGPRINT(" ");
			unsigned int a = ((const unsigned char*)data)[i];
			DEBUGPRINT("%.2X", a);
		}
		if(size>20)
			dstream<<"...";
		if(dumping_packet)
			dstream<<" (DUMPED BY INTERNET_SIMULATOR)";
		dstream<<std::endl;
	}
	else if(dumping_packet)
	{
		// Lol let's forget it
		dstream<<"UDPSocket::Send(): "
				"INTERNET_SIMULATOR: dumping packet."
				<<std::endl;
	}

	if(dumping_packet)
		return;

#if USE_IPV6
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
#else
	struct sockaddr_in address = destination.getAddress();
	address.sin_port = htons(destination.getPort());
	int sent = sendto(m_handle, (const char *) data, size,
		      0, (struct sockaddr *) &address, sizeof(struct sockaddr_in));
#endif

	if(sent != size)
	{
		throw SendFailedException("Failed to send packet");
	}
}

int UDPSocket::Receive(Address & sender, void * data, int size)
{
	if(WaitData(m_timeout_ms) == false)
	{
		return -1;
	}

#if USE_IPV6
	int received;
	if(m_addr_family == AF_INET6)
	{
		sockaddr_in6 address;
		socklen_t address_len = sizeof(address);

		received = recvfrom(m_handle, (char*) data,
				size, 0, (sockaddr*)&address, &address_len);

		if(received < 0)
			return -1;

		unsigned short address_port = ntohs(address.sin6_port);
		sender = Address(address.sin6_addr.s6_addr, address_port);
	}
	else
	{
		sockaddr_in address;
		socklen_t address_len = sizeof(address);

		received = recvfrom(m_handle, (char*)data,
				size, 0, (sockaddr*)&address, &address_len);

		if(received < 0)
			return -1;

		unsigned int address_ip = ntohl(address.sin_addr.s_addr);
		unsigned short address_port = ntohs(address.sin_port);

		sender = Address(address_ip, address_port);
	}
#else
	sockaddr_in address;
	socklen_t address_len = sizeof(address);
	
	int received = recvfrom(m_handle, (char*)data,
			size, 0, (sockaddr*)&address, &address_len);

	if(received < 0)
		return -1;
	
	unsigned int address_ip = ntohl(address.sin_addr.s_addr);
	unsigned short address_port = ntohs(address.sin_port);
	
	sender = Address(address_ip, address_port);
#endif

	if(DP){
		//dstream<<DPS<<"UDPSocket("<<(int)m_handle<<")::Receive(): sender=";
		dstream<<DPS<<(int)m_handle<<" <- ";
		sender.print();
		//dstream<<", received="<<received<<std::endl;
		dstream<<", size="<<received<<", data=";
		for(int i=0; i<received && i<20; i++){
			if(i%2==0) DEBUGPRINT(" ");
			unsigned int a = ((const unsigned char*)data)[i];
			DEBUGPRINT("%.2X", a);
		}
		if(received>20)
			dstream<<"...";
		dstream<<std::endl;
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

	if(result == 0){
		// Timeout
		/*dstream<<"Select timed out (timeout_ms="
				<<timeout_ms<<")"<<std::endl;*/
		return false;
	}
	else if(result < 0 && errno == EINTR){
		return false;
	}
	else if(result < 0){
		// Error
#ifndef DISABLE_ERRNO
		dstream<<(int)m_handle<<": Select failed: "<<strerror(errno)<<std::endl;
#endif
#ifdef _WIN32
		int e = WSAGetLastError();
		dstream<<(int)m_handle<<": WSAGetLastError()="<<e<<std::endl;
		if(e == 10004 /*=WSAEINTR*/)
		{
			dstream<<"WARNING: Ignoring WSAEINTR."<<std::endl;
			return false;
		}
#endif
		throw SocketException("Select failed");
	}
	else if(FD_ISSET(m_handle, &readset) == false){
		// No data
		//dstream<<"Select reported no data in m_handle"<<std::endl;
		return false;
	}
	
	// There is data
	//dstream<<"Select reported data in m_handle"<<std::endl;
	return true;
}


