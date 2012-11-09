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

#ifndef SOCKET_HEADER
#define SOCKET_HEADER

#include <ostream>
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

class Address
{
public:
	Address();
	Address(unsigned int address, unsigned short port);
	Address(unsigned int a, unsigned int b,
			unsigned int c, unsigned int d,
			unsigned short port);
	bool operator==(Address &address);
	bool operator!=(Address &address);
	void Resolve(const char *name);
	unsigned int getAddress() const;
	unsigned short getPort() const;
	void setAddress(unsigned int address);
	void setAddress(unsigned int a, unsigned int b,
			unsigned int c, unsigned int d);
	void setPort(unsigned short port);
	void print(std::ostream *s) const;
	void print() const;
	std::string serializeString() const;
private:
	unsigned int m_address;
	unsigned short m_port;
};

class UDPSocket
{
public:
	UDPSocket();
	~UDPSocket();
	void Bind(unsigned short port);
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
};

#endif

