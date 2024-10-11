// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "util/basic_macros.h"
#include "util/stream.h"

/*
 * LogTarget
 *
 * This is the interface that sits between the LogStreams and the global logger.
 * Primarily used to route streams to log levels, but could also enable other
 * custom behavior.
 *
 */
class LogTarget {
public:
	// Must be thread-safe. These can be called from any thread.
	virtual bool hasOutput() = 0;
	virtual void log(std::string_view buf) = 0;
};


/*
 * StreamProxy
 *
 * An ostream-like object that can proxy to a real ostream or do nothing,
 * depending on how it is configured. See LogStream below.
 *
 */
class StreamProxy {
public:
	StreamProxy(std::ostream *os) : m_os(os) { }

	template<typename T>
	StreamProxy& operator<<(T&& arg) {
		if (m_os) {
			*m_os << std::forward<T>(arg);
		}
		return *this;
	}

	StreamProxy& operator<<(std::ostream& (*manip)(std::ostream&)) {
		if (m_os) {
			*m_os << manip;
		}
		return *this;
	}

private:
	std::ostream *m_os;
};


/*
 * LogStream
 *
 * The public interface for log streams (infostream, verbosestream, etc).
 *
 * LogStream minimizes the work done when a given stream is off. (meaning
 * it has no output targets, so it goes to /dev/null)
 *
 * For example, consider:
 *
 *     verbosestream << "hello world" << 123 << std::endl;
 *
 * The compiler evaluates this as:
 *
 *   (((verbosestream << "hello world") << 123) << std::endl)
 *      ^                            ^
 *
 * If `verbosestream` is on, the innermost expression (marked by ^) will return
 * a StreamProxy that forwards to a real ostream, that feeds into the logger.
 * However, if `verbosestream` is off, it will return a StreamProxy that does
 * nothing on all later operations. Specifically, CPU time won't be wasted
 * writing "hello world" and 123 into a buffer, or formatting the log entry.
 *
 * It is also possible to directly check if the stream is on/off:
 *
 *   if (verbosestream) {
 *       auto data = ComputeExpensiveDataForTheLog();
 *       verbosestream << data << endl;
 *   }
 *
*/

class LogStream {
public:
	LogStream() = delete;
	DISABLE_CLASS_COPY(LogStream);

	LogStream(LogTarget &target) :
		m_target(target),
		m_buffer(std::bind(&LogStream::internalFlush, this, std::placeholders::_1)),
		m_dummy_buffer(),
		m_stream(&m_buffer),
		m_dummy_stream(&m_dummy_buffer),
		m_proxy(&m_stream),
		m_dummy_proxy(nullptr) { }

	template<typename T>
	StreamProxy& operator<<(T&& arg) {
		StreamProxy& sp = m_target.hasOutput() ? m_proxy : m_dummy_proxy;
		sp << std::forward<T>(arg);
		return sp;
	}

	StreamProxy& operator<<(std::ostream& (*manip)(std::ostream&)) {
		StreamProxy& sp = m_target.hasOutput() ? m_proxy : m_dummy_proxy;
		sp << manip;
		return sp;
	}

	operator bool() {
		return m_target.hasOutput();
	}

	void internalFlush(std::string_view buf) {
		m_target.log(buf);
	}

	operator std::ostream&() {
		return m_target.hasOutput() ? m_stream : m_dummy_stream;
	}

private:
	// 10 streams per thread x (256 + overhead) ~ 3K per thread
	static const int BUFFER_LENGTH = 256;
	LogTarget &m_target;
	StringStreamBuffer<BUFFER_LENGTH> m_buffer;
	DummyStreamBuffer m_dummy_buffer;
	std::ostream m_stream;
	std::ostream m_dummy_stream;
	StreamProxy m_proxy;
	StreamProxy m_dummy_proxy;

};

/*
 * By making the streams thread_local, each thread has its own
 * private buffer. Two or more threads can write to the same stream
 * simultaneously (lock-free), and there won't be any interference.
 *
 * The finished lines are sent to a LogTarget which is a global (not thread-local)
 * object, and from there relayed to g_logger. The final writes are serialized
 * by the mutex in g_logger.
*/

extern thread_local LogStream dstream;
extern thread_local LogStream rawstream;  // Writes directly to all LL_NONE log outputs with no prefix.
extern thread_local LogStream errorstream;
extern thread_local LogStream warningstream;
extern thread_local LogStream actionstream;
extern thread_local LogStream infostream;
extern thread_local LogStream verbosestream;
extern thread_local LogStream tracestream;
// TODO: Search/replace these with verbose/tracestream
extern thread_local LogStream derr_con;
extern thread_local LogStream dout_con;

#define TRACESTREAM(x) do {	\
	if (tracestream) { 	\
		tracestream x;	\
	}			\
} while (0)
