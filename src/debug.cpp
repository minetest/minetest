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


#include "porting.h"
#include "debug.h"
#include "exceptions.h"
#include "threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <map>
#include "jthread/jmutex.h"
#include "jthread/jmutexautolock.h"
#include "config.h"

#ifdef _MSC_VER
	#include <dbghelp.h>
	#include "version.h"
	#include "filesys.h"
#endif

/*
	Debug output
*/

#define DEBUGSTREAM_COUNT 2

FILE *g_debugstreams[DEBUGSTREAM_COUNT] = {stderr, NULL};

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

void debugstreams_init(bool disable_stderr, const char *filename)
{
	if(disable_stderr)
		g_debugstreams[0] = NULL;
	else
		g_debugstreams[0] = stderr;

	if(filename)
		g_debugstreams[1] = fopen(filename, "a");

	if(g_debugstreams[1])
	{
		fprintf(g_debugstreams[1], "\n\n-------------\n");
		fprintf(g_debugstreams[1],     "  Separator  \n");
		fprintf(g_debugstreams[1],     "-------------\n\n");
	}
}

void debugstreams_deinit()
{
	if(g_debugstreams[1] != NULL)
		fclose(g_debugstreams[1]);
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
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_VERBOSE, PROJECT_NAME, "%s", s);
#endif
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

Debugbuf debugbuf(false);
std::ostream dstream(&debugbuf);
Debugbuf debugbuf_no_stderr(true);
std::ostream dstream_no_stderr(&debugbuf_no_stderr);
Nullstream dummyout;

/*
	Assert
*/

void assert_fail(const char *assertion, const char *file,
		unsigned int line, const char *function)
{
	DEBUGPRINT("\nIn thread %lx:\n"
			"%s:%u: %s: Assertion '%s' failed.\n",
			(unsigned long)get_current_thread_id(),
			file, line, function, assertion);

	debug_stacks_print();

	if(g_debugstreams[1])
		fclose(g_debugstreams[1]);

	abort();
}

/*
	DebugStack
*/

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

DebugStack::DebugStack(threadid_t id)
{
	threadid = id;
	stack_i = 0;
	stack_max_i = 0;
	memset(stack, 0, DEBUG_STACK_SIZE*DEBUG_STACK_TEXT_SIZE);
}

void DebugStack::print(FILE *file, bool everything)
{
	fprintf(file, "DEBUG STACK FOR THREAD %lx:\n",
			(unsigned long)threadid);

	for(int i=0; i<stack_max_i; i++)
	{
		if(i == stack_i && everything == false)
			break;

		if(i < stack_i)
			fprintf(file, "#%d  %s\n", i, stack[i]);
		else
			fprintf(file, "(Leftover data: #%d  %s)\n", i, stack[i]);
	}

	if(stack_i == DEBUG_STACK_SIZE)
		fprintf(file, "Probably overflown.\n");
}

void DebugStack::print(std::ostream &os, bool everything)
{
	os<<"DEBUG STACK FOR THREAD "<<(unsigned long)threadid<<": "<<std::endl;

	for(int i=0; i<stack_max_i; i++)
	{
		if(i == stack_i && everything == false)
			break;

		if(i < stack_i)
			os<<"#"<<i<<"  "<<stack[i]<<std::endl;
		else
			os<<"(Leftover data: #"<<i<<"  "<<stack[i]<<")"<<std::endl;
	}

	if(stack_i == DEBUG_STACK_SIZE)
		os<<"Probably overflown."<<std::endl;
}

std::map<threadid_t, DebugStack*> g_debug_stacks;
JMutex g_debug_stacks_mutex;

void debug_stacks_init()
{
}

void debug_stacks_print_to(std::ostream &os)
{
	JMutexAutoLock lock(g_debug_stacks_mutex);

	os<<"Debug stacks:"<<std::endl;

	for(std::map<threadid_t, DebugStack*>::iterator
			i = g_debug_stacks.begin();
			i != g_debug_stacks.end(); ++i)
	{
		i->second->print(os, false);
	}
}

void debug_stacks_print()
{
	JMutexAutoLock lock(g_debug_stacks_mutex);

	DEBUGPRINT("Debug stacks:\n");

	for(std::map<threadid_t, DebugStack*>::iterator
			i = g_debug_stacks.begin();
			i != g_debug_stacks.end(); ++i)
	{
		DebugStack *stack = i->second;

		for(int i=0; i<DEBUGSTREAM_COUNT; i++)
		{
			if(g_debugstreams[i] != NULL)
				stack->print(g_debugstreams[i], true);
		}
	}
}

DebugStacker::DebugStacker(const char *text)
{
	threadid_t threadid = get_current_thread_id();

	JMutexAutoLock lock(g_debug_stacks_mutex);

	std::map<threadid_t, DebugStack*>::iterator n;
	n = g_debug_stacks.find(threadid);
	if(n != g_debug_stacks.end())
	{
		m_stack = n->second;
	}
	else
	{
		/*DEBUGPRINT("Creating new debug stack for thread %x\n",
				(unsigned int)threadid);*/
		m_stack = new DebugStack(threadid);
		g_debug_stacks[threadid] = m_stack;
	}

	if(m_stack->stack_i >= DEBUG_STACK_SIZE)
	{
		m_overflowed = true;
	}
	else
	{
		m_overflowed = false;

		snprintf(m_stack->stack[m_stack->stack_i],
				DEBUG_STACK_TEXT_SIZE, "%s", text);
		m_stack->stack_i++;
		if(m_stack->stack_i > m_stack->stack_max_i)
			m_stack->stack_max_i = m_stack->stack_i;
	}
}

DebugStacker::~DebugStacker()
{
	JMutexAutoLock lock(g_debug_stacks_mutex);

	if(m_overflowed == true)
		return;

	m_stack->stack_i--;

	if(m_stack->stack_i == 0)
	{
		threadid_t threadid = m_stack->threadid;
		/*DEBUGPRINT("Deleting debug stack for thread %x\n",
				(unsigned int)threadid);*/
		delete m_stack;
		g_debug_stacks.erase(threadid);
	}
}

#ifdef _MSC_VER

const char *Win32ExceptionCodeToString(DWORD exception_code)
{
	switch (exception_code) {
	case EXCEPTION_ACCESS_VIOLATION:
		return "Access violation";
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		return "Misaligned data access";
	case EXCEPTION_BREAKPOINT:
		return "Breakpoint reached";
	case EXCEPTION_SINGLE_STEP:
		return "Single debug step";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		return "Array access out of bounds";
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		return "Denormal floating point operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		return "Floating point division by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:
		return "Inaccurate floating point result";
	case EXCEPTION_FLT_INVALID_OPERATION:
		return "Invalid floating point operation";
	case EXCEPTION_FLT_OVERFLOW:
		return "Floating point exponent overflow";
	case EXCEPTION_FLT_STACK_CHECK:
		return "Floating point stack overflow or underflow";
	case EXCEPTION_FLT_UNDERFLOW:
		return "Floating point exponent underflow";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		return "Integer division by zero";
	case EXCEPTION_INT_OVERFLOW:
		return "Integer overflow";
	case EXCEPTION_PRIV_INSTRUCTION:
		return "Privileged instruction executed";
	case EXCEPTION_IN_PAGE_ERROR:
		return "Could not access or load page";
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		return "Illegal instruction encountered";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		return "Attempted to continue after fatal exception";
	case EXCEPTION_STACK_OVERFLOW:
		return "Stack overflow";
	case EXCEPTION_INVALID_DISPOSITION:
		return "Invalid disposition returned to the exception dispatcher";
	case EXCEPTION_GUARD_PAGE:
		return "Attempted guard page access";
	case EXCEPTION_INVALID_HANDLE:
		return "Invalid handle";
	}

	return "Unknown exception";
}

long WINAPI Win32ExceptionHandler(struct _EXCEPTION_POINTERS *pExceptInfo)
{
	char buf[512];
	MINIDUMP_EXCEPTION_INFORMATION mdei;
	MINIDUMP_USER_STREAM_INFORMATION mdusi;
	MINIDUMP_USER_STREAM mdus;
	bool minidump_created = false;
	std::string version_str("Minetest ");

	std::string dumpfile = porting::path_user + DIR_DELIM "minetest.dmp";

	HANDLE hFile = CreateFileA(dumpfile.c_str(), GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto minidump_failed;

	if (SetEndOfFile(hFile) == FALSE)
		goto minidump_failed;

	mdei.ClientPointers	   = NULL;
	mdei.ExceptionPointers = pExceptInfo;
	mdei.ThreadId		   = GetCurrentThreadId();

	version_str += minetest_version_hash;

	mdus.Type       = CommentStreamA;
	mdus.BufferSize = version_str.size();
	mdus.Buffer     = (PVOID)version_str.c_str();

	mdusi.UserStreamArray = &mdus;
	mdusi.UserStreamCount = 1;

	if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
			MiniDumpNormal, &mdei, &mdusi, NULL) == FALSE)
		goto minidump_failed;

	minidump_created = true;

minidump_failed:

	CloseHandle(hFile);

	DWORD excode = pExceptInfo->ExceptionRecord->ExceptionCode;
	_snprintf(buf, sizeof(buf),
		" >> === FATAL ERROR ===\n"
		" >> %s (Exception 0x%08X) at 0x%p\n",
		Win32ExceptionCodeToString(excode), excode,
		pExceptInfo->ExceptionRecord->ExceptionAddress);
	dstream << buf;

	if (minidump_created)
		dstream << " >> Saved dump to " << dumpfile << std::endl;
	else
		dstream << " >> Failed to save dump" << std::endl;

	return EXCEPTION_EXECUTE_HANDLER;
}

#endif

void debug_set_exception_handler()
{
#ifdef _MSC_VER
	SetUnhandledExceptionFilter(Win32ExceptionHandler);
#endif
}

