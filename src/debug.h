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

#ifndef DEBUG_HEADER
#define DEBUG_HEADER

#include <stdio.h>
#include <jmutex.h>
#include <jmutexautolock.h>
#include <iostream>
#include "irrlichttypes.h"
#include <irrMap.h>
#include "threads.h"
#include "gettime.h"
#include "exceptions.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#ifdef _MSC_VER
		#include <eh.h>
	#endif
#else
#endif

// Whether to catch all std::exceptions.
// Assert will be called on such an event.
// In debug mode, leave these for the debugger and don't catch them.
#ifdef NDEBUG
	#define CATCH_UNHANDLED_EXCEPTIONS 1
#else
	#define CATCH_UNHANDLED_EXCEPTIONS 0
#endif

/*
	Debug output
*/

#define DTIME (getTimestamp()+": ")

#define DEBUGSTREAM_COUNT 2

extern FILE *g_debugstreams[DEBUGSTREAM_COUNT];

extern void debugstreams_init(bool disable_stderr, const char *filename);
extern void debugstreams_deinit();

#define DEBUGPRINT(...)\
{\
	for(int i=0; i<DEBUGSTREAM_COUNT; i++)\
	{\
		if(g_debugstreams[i] != NULL){\
			fprintf(g_debugstreams[i], __VA_ARGS__);\
			fflush(g_debugstreams[i]);\
		}\
	}\
}

class Debugbuf : public std::streambuf
{
public:
	Debugbuf(bool disable_stderr)
	{
		m_disable_stderr = disable_stderr;
	}

	int overflow(int c)
	{
		for(int i=0; i<DEBUGSTREAM_COUNT; i++)
		{
			if(g_debugstreams[i] == stderr && m_disable_stderr)
				continue;
			if(g_debugstreams[i] != NULL)
				(void)fwrite(&c, 1, 1, g_debugstreams[i]);
			//TODO: Is this slow?
			fflush(g_debugstreams[i]);
		}
		
		return c;
	}
	std::streamsize xsputn(const char *s, std::streamsize n)
	{
		for(int i=0; i<DEBUGSTREAM_COUNT; i++)
		{
			if(g_debugstreams[i] == stderr && m_disable_stderr)
				continue;
			if(g_debugstreams[i] != NULL)
				(void)fwrite(s, 1, n, g_debugstreams[i]);
			//TODO: Is this slow?
			fflush(g_debugstreams[i]);
		}

		return n;
	}
	
private:
	bool m_disable_stderr;
};

// This is used to redirect output to /dev/null
class Nullstream : public std::ostream {
public:
	Nullstream():
		std::ostream(0)
	{
	}
private:
};

extern Debugbuf debugbuf;
extern std::ostream dstream;
extern std::ostream dstream_no_stderr;
extern Nullstream dummyout;

/*
	Assert
*/

__NORETURN extern void assert_fail(
		const char *assertion, const char *file,
		unsigned int line, const char *function);

#define ASSERT(expr)\
	((expr)\
	? (void)(0)\
	: assert_fail(#expr, __FILE__, __LINE__, __FUNCTION_NAME))

#define assert(expr) ASSERT(expr)

/*
	DebugStack
*/

#define DEBUG_STACK_SIZE 50
#define DEBUG_STACK_TEXT_SIZE 300

struct DebugStack
{
	DebugStack(threadid_t id);
	void print(FILE *file, bool everything);
	void print(std::ostream &os, bool everything);
	
	threadid_t threadid;
	char stack[DEBUG_STACK_SIZE][DEBUG_STACK_TEXT_SIZE];
	int stack_i; // Points to the lowest empty position
	int stack_max_i; // Highest i that was seen
};

extern core::map<threadid_t, DebugStack*> g_debug_stacks;
extern JMutex g_debug_stacks_mutex;

extern void debug_stacks_init();
extern void debug_stacks_print_to(std::ostream &os);
extern void debug_stacks_print();

class DebugStacker
{
public:
	DebugStacker(const char *text);
	~DebugStacker();

private:
	DebugStack *m_stack;
	bool m_overflowed;
};

#define DSTACK(msg)\
	DebugStacker __debug_stacker(msg);

#define DSTACKF(...)\
	char __buf[DEBUG_STACK_TEXT_SIZE];\
	snprintf(__buf,\
			DEBUG_STACK_TEXT_SIZE, __VA_ARGS__);\
	DebugStacker __debug_stacker(__buf);

/*
	Packet counter
*/

class PacketCounter
{
public:
	PacketCounter()
	{
	}

	void add(u16 command)
	{
		core::map<u16, u16>::Node *n = m_packets.find(command);
		if(n == NULL)
		{
			m_packets[command] = 1;
		}
		else
		{
			n->setValue(n->getValue()+1);
		}
	}

	void clear()
	{
		for(core::map<u16, u16>::Iterator
				i = m_packets.getIterator();
				i.atEnd() == false; i++)
		{
			i.getNode()->setValue(0);
		}
	}

	void print(std::ostream &o)
	{
		for(core::map<u16, u16>::Iterator
				i = m_packets.getIterator();
				i.atEnd() == false; i++)
		{
			o<<"cmd "<<i.getNode()->getKey()
					<<" count "<<i.getNode()->getValue()
					<<std::endl;
		}
	}

private:
	// command, count
	core::map<u16, u16> m_packets;
};

/*
	These should be put into every thread
*/

#if CATCH_UNHANDLED_EXCEPTIONS == 1
	#define BEGIN_PORTABLE_DEBUG_EXCEPTION_HANDLER try{
	#define END_PORTABLE_DEBUG_EXCEPTION_HANDLER(logstream)\
		}catch(std::exception &e){\
			logstream<<"ERROR: An unhandled exception occurred: "\
					<<e.what()<<std::endl;\
			assert(0);\
		}
	#ifdef _WIN32 // Windows
		#ifdef _MSC_VER // MSVC
void se_trans_func(unsigned int, EXCEPTION_POINTERS*);

class FatalSystemException : public BaseException
{
public:
	FatalSystemException(const char *s):
		BaseException(s)
	{}
};
			#define BEGIN_DEBUG_EXCEPTION_HANDLER \
				BEGIN_PORTABLE_DEBUG_EXCEPTION_HANDLER\
				_set_se_translator(se_trans_func);

			#define END_DEBUG_EXCEPTION_HANDLER(logstream) \
				END_PORTABLE_DEBUG_EXCEPTION_HANDLER(logstream)
		#else // Probably mingw
			#define BEGIN_DEBUG_EXCEPTION_HANDLER\
				BEGIN_PORTABLE_DEBUG_EXCEPTION_HANDLER
			#define END_DEBUG_EXCEPTION_HANDLER(logstream)\
				END_PORTABLE_DEBUG_EXCEPTION_HANDLER(logstream)
		#endif
	#else // Posix
		#define BEGIN_DEBUG_EXCEPTION_HANDLER\
			BEGIN_PORTABLE_DEBUG_EXCEPTION_HANDLER
		#define END_DEBUG_EXCEPTION_HANDLER(logstream)\
			END_PORTABLE_DEBUG_EXCEPTION_HANDLER(logstream)
	#endif
#else
	// Dummy ones
	#define BEGIN_DEBUG_EXCEPTION_HANDLER
	#define END_DEBUG_EXCEPTION_HANDLER(logstream)
#endif

#endif // DEBUG_HEADER


