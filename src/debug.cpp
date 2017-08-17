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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <thread>
#include "threading/mutex_auto_lock.h"
#include "config.h"

#ifdef _MSC_VER
	#include <dbghelp.h>
	#include "version.h"
	#include "filesys.h"
#endif

#if USE_CURSES
	#include "terminal_chat_console.h"
#endif

/*
	Assert
*/

void sanity_check_fn(const char *assertion, const char *file,
		unsigned int line, const char *function)
{
#if USE_CURSES
	g_term_console.stopAndWaitforThread();
#endif

	errorstream << std::endl << "In thread " << std::hex
		<< std::this_thread::get_id() << ":" << std::endl;
	errorstream << file << ":" << line << ": " << function
		<< ": An engine assumption '" << assertion << "' failed." << std::endl;

	debug_stacks_print_to(errorstream);

	abort();
}

void fatal_error_fn(const char *msg, const char *file,
		unsigned int line, const char *function)
{
#if USE_CURSES
	g_term_console.stopAndWaitforThread();
#endif

	errorstream << std::endl << "In thread " << std::hex
		<< std::this_thread::get_id() << ":" << std::endl;
	errorstream << file << ":" << line << ": " << function
		<< ": A fatal error occured: " << msg << std::endl;

	debug_stacks_print_to(errorstream);

	abort();
}

/*
	DebugStack
*/

struct DebugStack
{
	DebugStack(std::thread::id id);
	void print(FILE *file, bool everything);
	void print(std::ostream &os, bool everything);

	std::thread::id thread_id;
	char stack[DEBUG_STACK_SIZE][DEBUG_STACK_TEXT_SIZE];
	int stack_i; // Points to the lowest empty position
	int stack_max_i; // Highest i that was seen
};

DebugStack::DebugStack(std::thread::id id)
{
	thread_id = id;
	stack_i = 0;
	stack_max_i = 0;
	memset(stack, 0, DEBUG_STACK_SIZE*DEBUG_STACK_TEXT_SIZE);
}

void DebugStack::print(FILE *file, bool everything)
{
	std::ostringstream os;
	os << thread_id;
	fprintf(file, "DEBUG STACK FOR THREAD %s:\n",
		os.str().c_str());

	for (int i = 0; i < stack_max_i; i++) {
		if (i == stack_i && !everything)
			break;

		if (i < stack_i)
			fprintf(file, "#%d  %s\n", i, stack[i]);
		else
			fprintf(file, "(Leftover data: #%d  %s)\n", i, stack[i]);
	}

	if (stack_i == DEBUG_STACK_SIZE)
		fprintf(file, "Probably overflown.\n");
}

void DebugStack::print(std::ostream &os, bool everything)
{
	os<<"DEBUG STACK FOR THREAD "<<thread_id<<": "<<std::endl;

	for(int i = 0; i < stack_max_i; i++) {
		if(i == stack_i && !everything)
			break;

		if (i < stack_i)
			os<<"#"<<i<<"  "<<stack[i]<<std::endl;
		else
			os<<"(Leftover data: #"<<i<<"  "<<stack[i]<<")"<<std::endl;
	}

	if (stack_i == DEBUG_STACK_SIZE)
		os<<"Probably overflown."<<std::endl;
}

std::map<std::thread::id, DebugStack*> g_debug_stacks;
std::mutex g_debug_stacks_mutex;

void debug_stacks_print_to(std::ostream &os)
{
	MutexAutoLock lock(g_debug_stacks_mutex);

	os<<"Debug stacks:"<<std::endl;

	for (auto it : g_debug_stacks) {
		it.second->print(os, false);
	}
}

void debug_stacks_print()
{
	debug_stacks_print_to(errorstream);
}

DebugStacker::DebugStacker(const char *text)
{
	std::thread::id thread_id = std::this_thread::get_id();

	MutexAutoLock lock(g_debug_stacks_mutex);

	auto n = g_debug_stacks.find(thread_id);
	if (n != g_debug_stacks.end()) {
		m_stack = n->second;
	} else {
		/*DEBUGPRINT("Creating new debug stack for thread %x\n",
				(unsigned int)thread_id);*/
		m_stack = new DebugStack(thread_id);
		g_debug_stacks[thread_id] = m_stack;
	}

	if (m_stack->stack_i >= DEBUG_STACK_SIZE) {
		m_overflowed = true;
	} else {
		m_overflowed = false;

		snprintf(m_stack->stack[m_stack->stack_i],
				DEBUG_STACK_TEXT_SIZE, "%s", text);
		m_stack->stack_i++;
		if (m_stack->stack_i > m_stack->stack_max_i)
			m_stack->stack_max_i = m_stack->stack_i;
	}
}

DebugStacker::~DebugStacker()
{
	MutexAutoLock lock(g_debug_stacks_mutex);

	if (m_overflowed)
		return;

	m_stack->stack_i--;

	if (m_stack->stack_i == 0) {
		std::thread::id thread_id = m_stack->thread_id;
		/*DEBUGPRINT("Deleting debug stack for thread %x\n",
				(unsigned int)thread_id);*/
		delete m_stack;
		g_debug_stacks.erase(thread_id);
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

	std::string dumpfile = porting::path_user + DIR_DELIM PROJECT_NAME ".dmp";

	std::string version_str(PROJECT_NAME " ");
	version_str += g_version_hash;

	HANDLE hFile = CreateFileA(dumpfile.c_str(), GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto minidump_failed;

	if (SetEndOfFile(hFile) == FALSE)
		goto minidump_failed;

	mdei.ClientPointers	   = NULL;
	mdei.ExceptionPointers = pExceptInfo;
	mdei.ThreadId		   = GetCurrentThreadId();

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

