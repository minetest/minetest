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
#include <stdlib.h>
#include "threading/mutex.h"
#include "threading/mutex_auto_lock.h"
#include "config.h"
#include <cstring>
#include <map>
#include <sstream>

#ifdef _MSC_VER
	#include "version.h"
	#include "filesys.h"
	#include <dbghelp.h>
#endif

const std::string fe_prefix = "A fatal error occurred: ";

void fatal_error_fn(const std::string &msg, const char *file,
		unsigned int line, const char *function)
{
	std::ostringstream os;
	os << "At " << file << ':' << line << ": " << function
		<< ": " << msg << std::endl;
	debug_stacks_print_to(os);
	errorstream << std::endl << os;

#ifdef __ANDROID__
	__android_log_assert(msg.c_str(), PROJECT_NAME, "%s", os.str().c_str());
#endif
	abort();
}


// DebugStack

class DebugStack
{
public:
	DebugStack(threadid_t id) :
		thread_id(id),
		stack_i(0),
		stack_max_i(0)
	{
		memset(stack, 0, sizeof(stack));
	}
	void print(std::ostream &os, bool everything);
private:
	friend class DebugStacker;
	friend void debug_stacks_print_to(std::ostream&);

	threadid_t thread_id;
	char stack[DEBUG_STACK_SIZE][DEBUG_STACK_TEXT_SIZE];
	unsigned int stack_i; // Points to the lowest empty position
	unsigned int stack_max_i; // Highest i that was seen

	static std::map<threadid_t, DebugStack*> instances;
	static Mutex instances_mutex;
};
std::map<threadid_t, DebugStack*> DebugStack::instances;
Mutex DebugStack::instances_mutex;


void DebugStack::print(std::ostream &os, bool everything)
{
	os << "DEBUG STACK FOR THREAD " << thread_id << ": " << std::endl;

	for (unsigned i = 0; i < stack_max_i; ++i) {
		if (i == stack_i && !everything)
			break;

		if (i < stack_i)
			os << "#" << i << "  " << stack[i] << std::endl;
		else
			os << "(Leftover data: #" << i << "  " << stack[i] << ")" << std::endl;
	}

	if (stack_i == DEBUG_STACK_SIZE)
		os << "Probably overflown." << std::endl;
}


void debug_stacks_print_to(std::ostream &os)
{
	MutexAutoLock lock(DebugStack::instances_mutex);

	os << "Debug stacks:" << std::endl;

	for (std::map<threadid_t, DebugStack*>::iterator
			it = DebugStack::instances.begin();
			it != DebugStack::instances.end(); ++it) {
		it->second->print(os, false);
	}
}


DebugStacker::DebugStacker(const char *text)
{
	threadid_t thread_id = get_current_thread_id();

	MutexAutoLock lock(DebugStack::instances_mutex);

	std::map<threadid_t, DebugStack*>::iterator n =
			DebugStack::instances.find(thread_id);
	if (n != DebugStack::instances.end()) {
		m_stack = n->second;
	} else {
		m_stack = new DebugStack(thread_id);
		DebugStack::instances[thread_id] = m_stack;
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
	if (m_overflowed)
		return;

	m_stack->stack_i--;

	if (m_stack->stack_i == 0) {
		MutexAutoLock lock(DebugStack::instances_mutex);
		threadid_t thread_id = m_stack->thread_id;
		delete m_stack;
		DebugStack::instances.erase(thread_id);
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

void debug_set_exception_handler()
{
	SetUnhandledExceptionFilter(Win32ExceptionHandler);
}

#endif  // _MSC_VER

