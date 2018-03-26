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

#include <map>
#include <queue>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#if !defined(_WIN32)  // POSIX
	#include <unistd.h>
#endif
#include "irrlichttypes.h"

class ILogOutput;

enum LogLevel {
	LL_NONE, // Special level that is always printed
	LL_ERROR,
	LL_WARNING,
	LL_ACTION,  // In-game actions
	LL_INFO,
	LL_VERBOSE,
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

	void setTraceEnabled(bool enable) { m_trace_enabled = enable; }
	bool getTraceEnabled() { return m_trace_enabled; }

	static LogLevel stringToLevel(const std::string &name);
	static const std::string getLevelLabel(LogLevel lev);

	static LogColor color_mode;

private:
	void logToOutputsRaw(LogLevel, const std::string &line);
	void logToOutputs(LogLevel, const std::string &combined,
		const std::string &time, const std::string &thread_name,
		const std::string &payload_text);

	const std::string getThreadName();

	std::vector<ILogOutput *> m_outputs[LL_MAX];

	// Should implement atomic loads and stores (even though it's only
	// written to when one thread has access currently).
	// Works on all known architectures (x86, ARM, MIPS).
	volatile bool m_silenced_levels[LL_MAX];
	std::map<std::thread::id, std::string> m_thread_names;
	mutable std::mutex m_mutex;
	bool m_trace_enabled;
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

	void logRaw(LogLevel lev, const std::string &line)
	{
		bool colored_message = (Logger::color_mode == LOG_COLOR_ALWAYS) ||
			(Logger::color_mode == LOG_COLOR_AUTO && is_tty);
		if (colored_message)
			switch (lev) {
			case LL_ERROR:
				// error is red
				m_stream << "\033[91m";
				break;
			case LL_WARNING:
				// warning is yellow
				m_stream << "\033[93m";
				break;
			case LL_INFO:
				// info is a bit dark
				m_stream << "\033[37m";
				break;
			case LL_VERBOSE:
				// verbose is darker than info
				m_stream << "\033[2m";
				break;
			default:
				// action is white
				colored_message = false;
			}

		m_stream << line << std::endl;

		if (colored_message)
			// reset to white color
			m_stream << "\033[0m";
	}

private:
	std::ostream &m_stream;
	bool is_tty;
};

class FileLogOutput : public ICombinedLogOutput {
public:
	void open(const std::string &filename);

	void logRaw(LogLevel lev, const std::string &line)
	{
		m_stream << line << std::endl;
	}

private:
	std::ofstream m_stream;
};

class LogOutputBuffer : public ICombinedLogOutput {
public:
	LogOutputBuffer(Logger &logger, LogLevel lev) :
		m_logger(logger)
	{
		m_logger.addOutput(this, lev);
	}

	~LogOutputBuffer()
	{
		m_logger.removeOutput(this);
	}

	void logRaw(LogLevel lev, const std::string &line)
	{
		m_buffer.push(line);
	}

	bool empty()
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


extern StreamLogOutput stdout_output;
extern StreamLogOutput stderr_output;
extern std::ostream null_stream;

extern std::ostream *dout_con_ptr;
extern std::ostream *derr_con_ptr;
extern std::ostream *dout_server_ptr;
extern std::ostream *derr_server_ptr;

#ifndef SERVER
extern std::ostream *dout_client_ptr;
extern std::ostream *derr_client_ptr;
#endif

extern Logger g_logger;

// Writes directly to all LL_NONE log outputs for g_logger with no prefix.
extern std::ostream rawstream;

extern std::ostream errorstream;
extern std::ostream warningstream;
extern std::ostream actionstream;
extern std::ostream infostream;
extern std::ostream verbosestream;
extern std::ostream dstream;

#define TRACEDO(x) do {               \
	if (g_logger.getTraceEnabled()) { \
		x;                            \
	}                                 \
} while (0)

#define TRACESTREAM(x) TRACEDO(verbosestream x)

#define dout_con (*dout_con_ptr)
#define derr_con (*derr_con_ptr)
#define dout_server (*dout_server_ptr)

#ifndef SERVER
	#define dout_client (*dout_client_ptr)
#endif
