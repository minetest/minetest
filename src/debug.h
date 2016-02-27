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

#ifndef DEBUG_HEADER
#define DEBUG_HEADER

#include <iostream>
#include <exception>
#include <assert.h>
#include "gettime.h"
#include "log.h"

#if (defined(WIN32) || defined(_WIN32_WCE))
	#define WIN32_LEAN_AND_MEAN
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#include <windows.h>
	#ifdef _MSC_VER
		#include <eh.h>
	#endif
	#define NORETURN __declspec(noreturn)
	#define FUNCTION_NAME __FUNCTION__
#else
	#define NORETURN __attribute__ ((__noreturn__))
	#define FUNCTION_NAME __PRETTY_FUNCTION__
#endif

// Whether to catch all std::exceptions.
// When "catching", the program will abort with an error message.
// In debug mode, leave these for the debugger and don't catch them.
#ifdef NDEBUG
	#define CATCH_UNHANDLED_EXCEPTIONS 1
#else
	#define CATCH_UNHANDLED_EXCEPTIONS 0
#endif

/* Abort program execution immediately
 */
NORETURN extern void fatal_error_fn(
		const char *msg, const char *file,
		unsigned int line, const char *function);

#define FATAL_ERROR(msg) \
	fatal_error_fn((msg), __FILE__, __LINE__, FUNCTION_NAME)

#define FATAL_ERROR_IF(expr, msg) \
	((expr) \
	? fatal_error_fn((msg), __FILE__, __LINE__, FUNCTION_NAME) \
	: (void)(0))

/*
	sanity_check()
	Equivalent to assert() but persists in Release builds (i.e. when NDEBUG is
	defined)
*/

NORETURN extern void sanity_check_fn(
		const char *assertion, const char *file,
		unsigned int line, const char *function);

#define SANITY_CHECK(expr) \
	((expr) \
	? (void)(0) \
	: sanity_check_fn(#expr, __FILE__, __LINE__, FUNCTION_NAME))

#define sanity_check(expr) SANITY_CHECK(expr)


void debug_set_exception_handler();

/*
	DebugStack
*/

#define DEBUG_STACK_SIZE 50
#define DEBUG_STACK_TEXT_SIZE 300

extern void debug_stacks_init();
extern void debug_stacks_print_to(std::ostream &os);
extern void debug_stacks_print();

struct DebugStack;
class DebugStacker
{
public:
	DebugStacker(const char *text);
	~DebugStacker();

private:
	DebugStack *m_stack;
	bool m_overflowed;
};

#define DSTACK(msg) \
	DebugStacker __debug_stacker(msg);

#define DSTACKF(...) \
	char __buf[DEBUG_STACK_TEXT_SIZE];                   \
	snprintf(__buf, DEBUG_STACK_TEXT_SIZE, __VA_ARGS__); \
	DebugStacker __debug_stacker(__buf);

/*
	These should be put into every thread
*/

#if CATCH_UNHANDLED_EXCEPTIONS == 1
	#define BEGIN_DEBUG_EXCEPTION_HANDLER try {
	#define END_DEBUG_EXCEPTION_HANDLER                        \
		} catch (std::exception &e) {                          \
			errorstream << "An unhandled exception occurred: " \
				<< e.what() << std::endl;                      \
			FATAL_ERROR(e.what());                             \
		}
#else
	// Dummy ones
	#define BEGIN_DEBUG_EXCEPTION_HANDLER
	#define END_DEBUG_EXCEPTION_HANDLER
#endif

#endif // DEBUG_HEADER


