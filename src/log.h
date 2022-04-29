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
#include "util/basic_macros.h"
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
		is_tty = isatty(fileno(stdout));
#else
		is_tty = false;
#endif
	}

	void logRaw(LogLevel lev, const std::string &line);

private:
	std::ostream &m_stream;
	bool is_tty;
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
		m_buffer = std::queue<std::string>();
	}

	bool empty() const
	{
		return m_buffer.empty();
	}

	std::string get()
	{
		if (empty())
			return "";
		std::string s = m_buffer.front();
		m_buffer.pop();
		return s;
	}

private:
	std::queue<std::string> m_buffer;
	Logger &m_logger;
};

#ifdef __ANDROID__
class AndroidLogOutput : public ICombinedLogOutput {
	public:
		void logRaw(LogLevel lev, const std::string &line);
};
#endif

class LogTarget {
public:
	// Must be thread-safe. These can be called from any thread.
	virtual bool hasOutput() = 0;
	virtual void log(const std::string &buf) = 0;
};


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


class LogStream {
public:
	LogStream() = delete;
	DISABLE_CLASS_COPY(LogStream);

	LogStream(LogTarget &target) :
		m_target(target),
		m_buffer(*this),
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
	class StringBuffer : public std::streambuf {
	public:
		StringBuffer(LogStream &p) : parent(p) {
			buffer_index = 0;
		}

		int overflow(int c) {
			push_back(c);
			return c;
		}

		void push_back(char c) {
			if (c == '\n' || c == '\r') {
				if (buffer_index)
					parent.internalFlush(std::string(buffer, buffer_index));
				buffer_index = 0;
			} else {
				buffer[buffer_index++] = c;
				if (buffer_index >= BUFFER_LENGTH) {
					parent.internalFlush(std::string(buffer, buffer_index));
					buffer_index = 0;
				}
			}
		}

	        std::streamsize xsputn(const char *s, std::streamsize n) {
			for (int i = 0; i < n; ++i)
				push_back(s[i]);
			return n;
		}

	private:
		// 10 streams per thread x (256 + overhead) ~ 3K per thread
		static const int BUFFER_LENGTH = 256;
		LogStream &parent;
		char buffer[BUFFER_LENGTH];
	        int buffer_index;
	};

	class DummyBuffer : public std::streambuf {
		int overflow(int c) {
			return c;
		}
	        std::streamsize xsputn(const char *s, std::streamsize n) {
			return n;
		}
	};
	LogTarget &m_target;
	StringBuffer m_buffer;
	DummyBuffer m_dummy_buffer;
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
