/*
Minetest
Copyright (C) 2013-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "exceptions.h"

namespace con
{
/*
	Exceptions
*/
class NotFoundException : public BaseException
{
public:
	NotFoundException(const char *s) : BaseException(s) {}
};

class PeerNotFoundException : public BaseException
{
public:
	PeerNotFoundException(const char *s) : BaseException(s) {}
};

class ConnectionException : public BaseException
{
public:
	ConnectionException(const char *s) : BaseException(s) {}
};

class ConnectionBindFailed : public BaseException
{
public:
	ConnectionBindFailed(const char *s) : BaseException(s) {}
};

class InvalidIncomingDataException : public BaseException
{
public:
	InvalidIncomingDataException(const char *s) : BaseException(s) {}
};

class NoIncomingDataException : public BaseException
{
public:
	NoIncomingDataException(const char *s) : BaseException(s) {}
};

class ProcessedSilentlyException : public BaseException
{
public:
	ProcessedSilentlyException(const char *s) : BaseException(s) {}
};

class ProcessedQueued : public BaseException
{
public:
	ProcessedQueued(const char *s) : BaseException(s) {}
};

class IncomingDataCorruption : public BaseException
{
public:
	IncomingDataCorruption(const char *s) : BaseException(s) {}
};
}

class SocketException : public BaseException
{
public:
	SocketException(const std::string &s) : BaseException(s) {}
};

class ResolveError : public BaseException
{
public:
	ResolveError(const std::string &s) : BaseException(s) {}
};

class SendFailedException : public BaseException
{
public:
	SendFailedException(const std::string &s) : BaseException(s) {}
};
