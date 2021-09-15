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

#include <exception>
#include <string>


class BaseException : public std::exception
{
public:
	BaseException(const std::string &s) throw(): m_s(s) {}
	~BaseException() throw() = default;

	virtual const char * what() const throw()
	{
		return m_s.c_str();
	}
protected:
	std::string m_s;
};

class AlreadyExistsException : public BaseException {
public:
	AlreadyExistsException(const std::string &s): BaseException(s) {}
};

class VersionMismatchException : public BaseException {
public:
	VersionMismatchException(const std::string &s): BaseException(s) {}
};

class FileNotGoodException : public BaseException {
public:
	FileNotGoodException(const std::string &s): BaseException(s) {}
};

class DatabaseException : public BaseException {
public:
	DatabaseException(const std::string &s): BaseException(s) {}
};

class SerializationError : public BaseException {
public:
	SerializationError(const std::string &s): BaseException(s) {}
};

class PacketError : public BaseException {
public:
	PacketError(const std::string &s): BaseException(s) {}
};

class SettingNotFoundException : public BaseException {
public:
	SettingNotFoundException(const std::string &s): BaseException(s) {}
};

class ItemNotFoundException : public BaseException {
public:
	ItemNotFoundException(const std::string &s): BaseException(s) {}
};

class ServerError : public BaseException {
public:
	ServerError(const std::string &s): BaseException(s) {}
};

class ClientStateError : public BaseException {
public:
	ClientStateError(const std::string &s): BaseException(s) {}
};

class PrngException : public BaseException {
public:
	PrngException(const std::string &s): BaseException(s) {}
};

class ModError : public BaseException {
public:
	ModError(const std::string &s): BaseException(s) {}
};


/*
	Some "old-style" interrupts:
*/

class InvalidNoiseParamsException : public BaseException {
public:
	InvalidNoiseParamsException():
		BaseException("One or more noise parameters were invalid or require "
			"too much memory")
	{}

	InvalidNoiseParamsException(const std::string &s):
		BaseException(s)
	{}
};

class InvalidPositionException : public BaseException
{
public:
	InvalidPositionException():
		BaseException("Somebody tried to get/set something in a nonexistent position.")
	{}
	InvalidPositionException(const std::string &s):
		BaseException(s)
	{}
};

class IllegalObjectActivationException : public BaseException
{
public:
	IllegalObjectActivationException():
		BaseException("Someone tried to activate objects when doing so was not permitted.")
	{}

	IllegalObjectActivationException(const std::string &s):
		BaseException(s)
	{}
};
