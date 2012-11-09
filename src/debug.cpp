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


#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

/*
	Debug output
*/

FILE *g_debugstreams[DEBUGSTREAM_COUNT] = {stderr, NULL};

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
			"%s:%d: %s: Assertion '%s' failed.\n",
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

core::map<threadid_t, DebugStack*> g_debug_stacks;
JMutex g_debug_stacks_mutex;

void debug_stacks_init()
{
	g_debug_stacks_mutex.Init();
}

void debug_stacks_print_to(std::ostream &os)
{
	JMutexAutoLock lock(g_debug_stacks_mutex);

	os<<"Debug stacks:"<<std::endl;

	for(core::map<threadid_t, DebugStack*>::Iterator
			i = g_debug_stacks.getIterator();
			i.atEnd() == false; i++)
	{
		DebugStack *stack = i.getNode()->getValue();
		stack->print(os, false);
	}
}

void debug_stacks_print()
{
	JMutexAutoLock lock(g_debug_stacks_mutex);

	DEBUGPRINT("Debug stacks:\n");

	for(core::map<threadid_t, DebugStack*>::Iterator
			i = g_debug_stacks.getIterator();
			i.atEnd() == false; i++)
	{
		DebugStack *stack = i.getNode()->getValue();

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

	core::map<threadid_t, DebugStack*>::Node *n;
	n = g_debug_stacks.find(threadid);
	if(n != NULL)
	{
		m_stack = n->getValue();
	}
	else
	{
		/*DEBUGPRINT("Creating new debug stack for thread %x\n",
				(unsigned int)threadid);*/
		m_stack = new DebugStack(threadid);
		g_debug_stacks.insert(threadid, m_stack);
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
		g_debug_stacks.remove(threadid);
	}
}


#ifdef _MSC_VER
#if CATCH_UNHANDLED_EXCEPTIONS == 1
void se_trans_func(unsigned int u, EXCEPTION_POINTERS* pExp)
{
	dstream<<"In trans_func.\n";
	if(u == EXCEPTION_ACCESS_VIOLATION)
	{
		PEXCEPTION_RECORD r = pExp->ExceptionRecord;
		dstream<<"Access violation at "<<r->ExceptionAddress
				<<" write?="<<r->ExceptionInformation[0]
				<<" address="<<r->ExceptionInformation[1]
				<<std::endl;
		throw FatalSystemException
		("Access violation");
	}
	if(u == EXCEPTION_STACK_OVERFLOW)
	{
		throw FatalSystemException
		("Stack overflow");
	}
	if(u == EXCEPTION_ILLEGAL_INSTRUCTION)
	{
		throw FatalSystemException
		("Illegal instruction");
	}
}
#endif
#endif



