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

#include <atomic>
#include <map>
#include <queue>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#if !defined(_WIN32)  // POSIX
	#include <unistd.h>
#endif
#include "threading/mutex_auto_lock.h"
#include "util/basic_macros.h"
#include "util/stream.h"
#include "irrlichttypes.h"

class ILogOutput;

enum LogLevel {
	LL_NONE, // Special level that is always printed
	LL_ERROR,
	LL_WARNING,
	LL_ACTION,  // In-game actions
	LL_INFO,
	LL_VERBOSE,
	LL_TRACE,
	LL_MAX,
};

enum LogColor {
	LOG_COLOR_NEVER,
	LOG_COLOR_ALWAYS,
	LOG_COLOR_AUTO,
};

typedef u8 LogLevelMask;
#define LOGLEVEL_TO_MASKLEVEL(x) (1 << x)

class Logger {
public:
	void addOutput(ILogOutput *out);
	void addOutput(ILogOutput *out, LogLevel lev);
	void addOutputMasked(ILogOutput *out, LogLevelMask mask);
	void addOutputMaxLevel(ILogOutput *out, LogLevel lev);
	LogLevelMask removeOutput(ILogOutput *out);
	void setLevelSilenced(LogLevel lev, bool silenced);

	void registerThread(const std::string &name);
	void deregisterThread();

	void log(LogLevel lev, const std::string &text);
	// Logs without a prefix
	void logRaw(LogLevel lev, const std::string &text);

	static LogLevel stringToLevel(const std::string &name);
	static const std::string getLevelLabel(LogLevel lev);

	bool hasOutput(LogLevel level) {
		return m_has_outputs[level].load(std::memory_order_relaxed);
	}

	static LogColor color_mode;

private:
	void logToOutputsRaw(LogLevel, const std::string &line);
	void logToOutputs(LogLevel, const std::string &combined,
		const std::string &time, const std::string &thread_name,
		const std::string &payload_text);

	const std::string getThreadName();

	std::vector<ILogOutput *> m_outputs[LL_MAX];
	std::atomic<bool> m_has_outputs[LL_MAX];

	// Should implement atomic loads and stores (even though it's only
	// written to when one thread has access currently).
	// Works on all known architectures (x86, ARM, MIPS).
	volatile bool m_silenced_levels[LL_MAX];
	std::map<std::thread::id, std::string> m_thread_names;
	mutable std::mutex m_mutex;
};

class ILogOutput {
public:
	virtual void logRaw(LogLevel, const std::string &line) = 0;
	virtual void log(LogLevel, const std::string &combined,
		const std::string &time, const std::string &thread_name,
		const std::string &payload_text) = 0;
};

class ICombinedLogOutput : public ILogOutput {
public:
	void log(LogLevel lev, const std::string &combined,
		const std::string &time, const std::string &thread_name,
		const std::string &payload_text)
	{
		logRaw(lev, combined);
	}
};

class StreamLogOutput : public ICombinedLogOutput {
public:
	StreamLogOutput(std::ostream &stream) :
		m_stream(stream)
	{
#if !defined(_WIN32)
		if (&stream == &std::cout)
			is_tty = isatty(STDOUT_FILENO);
		else if (&stream == &std::cerr)
			is_tty = isatty(STDERR_FILENO);
#endif
	}

	void logRaw(LogLevel lev, const std::string &line);

private:
	std::ostream &m_stream;
	bool is_tty = false;
};

class FileLogOutput : public ICombinedLogOutput {
public:
	void setFile(const std::string &filename, s64 file_size_max);

	void logRaw(LogLevel lev, const std::string &line)
	{
		m_stream << line << std::endl;
	}

private:
	std::ofstream m_stream;
};

class LogOutputBuffer : public ICombinedLogOutput {
public:
	LogOutputBuffer(Logger &logger) :
		m_logger(logger)
	{
		updateLogLevel();
	};

	virtual ~LogOutputBuffer()
	{
		m_logger.removeOutput(this);
	}

	void updateLogLevel();

	void logRaw(LogLevel lev, const std::string &line);

	void clear()
	{
		MutexAutoLock lock(m_buffer_mutex);
		m_buffer = std::queue<std::string>();
	}

	bool empty() const
	{
		MutexAutoLock lock(m_buffer_mutex);
		return m_buffer.empty();
	}

	std::string get()
	{
		MutexAutoLock lock(m_buffer_mutex);
		if (m_buffer.empty())
			return "";
		std::string s = std::move(m_buffer.front());
		m_buffer.pop();
		return s;
	}

private:
	// g_logger serializes calls to logRaw() with a mutex, but that
	// doesn't prevent get() / clear() from being called on top of it.
	// This mutex prevents that.
	mutable std::mutex m_buffer_mutex;
	std::queue<std::string> m_buffer;
	Logger &m_logger;
};

#ifdef __ANDROID__
class AndroidLogOutput : public ICombinedLogOutput {
public:
	void logRaw(LogLevel lev, const std::string &line);
};
#endif

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
	virtual void log(const std::string &buf) = 0;
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

	void internalFlush(const std::string &buf) {
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

#ifdef __ANDROID__
extern AndroidLogOutput stdout_output;
extern AndroidLogOutput stderr_output;
#else
extern StreamLogOutput stdout_output;
extern StreamLogOutput stderr_output;
#endif

extern Logger g_logger;

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
