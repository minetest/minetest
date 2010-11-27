/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

/*
	Debug stuff
*/

#ifndef DEBUG_HEADER
#define DEBUG_HEADER

#include <stdio.h>
#include <jmutex.h>
#include <jmutexautolock.h>
#include <iostream>
#include "common_irrlicht.h"

/*
	Compatibility stuff
*/

#if (defined(WIN32) || defined(_WIN32_WCE))
typedef DWORD threadid_t;
#define __NORETURN __declspec(noreturn)
#define __FUNCTION_NAME __FUNCTION__
#else
typedef pthread_t threadid_t;
#define __NORETURN __attribute__ ((__noreturn__))
#define __FUNCTION_NAME __PRETTY_FUNCTION__
#endif

inline threadid_t get_current_thread_id()
{
#if (defined(WIN32) || defined(_WIN32_WCE))
	return GetCurrentThreadId();
#else
	return pthread_self();
#endif
}

/*
	Debug output
*/

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
				fwrite(&c, 1, 1, g_debugstreams[i]);
			//TODO: Is this slow?
			fflush(g_debugstreams[i]);
		}
		
		return c;
	}
	int xsputn(const char *s, int n)
	{
		for(int i=0; i<DEBUGSTREAM_COUNT; i++)
		{
			if(g_debugstreams[i] == stderr && m_disable_stderr)
				continue;
			if(g_debugstreams[i] != NULL)
				fwrite(s, 1, n, g_debugstreams[i]);
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
	
	threadid_t threadid;
	char stack[DEBUG_STACK_SIZE][DEBUG_STACK_TEXT_SIZE];
	int stack_i; // Points to the lowest empty position
	int stack_max_i; // Highest i that was seen
};

extern core::map<threadid_t, DebugStack*> g_debug_stacks;
extern JMutex g_debug_stacks_mutex;

extern void debug_stacks_init();
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

#define DSTACK(...)\
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


#endif // DEBUG_HEADER


