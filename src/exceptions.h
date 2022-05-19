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

#include <cstdarg>
#include <exception>
#include <string>
#include <util/string.h>

class BaseException : public std::exception
{
public:
	BaseException(const std::string &s) noexcept : m_s(s) {}
	BaseException(std::string &&s) noexcept : m_s(std::move(s)) {}

	// Supports printf-like exception creation, example:
	//   throw MyException("Invalid value: %d", value);
	PRINTF_ATTR(2, 3)
	explicit BaseException(const char *format, ...) noexcept
	{
		va_list ap;
		va_start(ap, format);
		m_s = VStringPrintf(format, ap);
		va_end(ap);
	}

	virtual const char * what() const noexcept
	{
		return m_s.c_str();
	}
protected:
	std::string m_s;
};

/*
 * Clang supports C++11 Constructor Delegation with printf-style constructors,
 * but GCC doesn't. So this macro is required for now.
 */
#define DEFINE_EXCEPTION_BASE(ClassName, EXTRA_CONSTRUCTORS) \
	class ClassName : public BaseException { \
	public: \
		ClassName(const std::string &s) noexcept : BaseException(s) { } \
		ClassName(std::string &&s) noexcept : BaseException(std::move(s)) { } \
		PRINTF_ATTR(2, 3) \
		explicit ClassName(const char *format, ...) noexcept : \
			BaseException(std::string()) \
		{ \
			va_list ap; \
			va_start(ap, format); \
			m_s = VStringPrintf(format, ap); \
			va_end(ap); \
		} \
		EXTRA_CONSTRUCTORS \
	};

#define DEFINE_EXCEPTION(ClassName) \
	DEFINE_EXCEPTION_BASE(ClassName,)

// Includes a default constructor with predefined text.
#define DEFINE_EXCEPTION_DEFTEXT(ClassName, DefaultText) \
	DEFINE_EXCEPTION_BASE(ClassName, \
		ClassName() : BaseException(std::string(DefaultText)) { } \
	)


DEFINE_EXCEPTION(AlreadyExistsException)
DEFINE_EXCEPTION(VersionMismatchException)
DEFINE_EXCEPTION(FileNotGoodException)
DEFINE_EXCEPTION(DatabaseException)
DEFINE_EXCEPTION(SerializationError)
DEFINE_EXCEPTION(PacketError)
DEFINE_EXCEPTION(SettingNotFoundException)
DEFINE_EXCEPTION(ItemNotFoundException)
DEFINE_EXCEPTION(ServerError)
DEFINE_EXCEPTION(ClientStateError)
DEFINE_EXCEPTION(PrngException)
DEFINE_EXCEPTION(ModError)

DEFINE_EXCEPTION_DEFTEXT(
	InvalidNoiseParamsException,
	"One or more noise parameters were invalid or require too much memory")

DEFINE_EXCEPTION_DEFTEXT(
	InvalidPositionException,
	"Somebody tried to get/set something in a nonexistent position.")
