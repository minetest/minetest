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

#ifndef LOG_HEADER
#define LOG_HEADER

#include <map>
#include <queue>
#include <string>
#include <fstream>
#include "threads.h"


namespace logging {

/* Use this for logging everything.
 * If you need to explicitly print something, use cout or cerr.
 */

class ILogOutput;


// TODO: Use C++11 enum class
enum LogLevel {
	LL_NONE, // Special level that is always printed
	LL_ERROR,
	LL_WARNING,
	LL_ACTION,  // In-game actions
	LL_INFO,
	LL_VERBOSE,
	LL_MAX,
};

LogLevel stringToLevel(const std::string &name);

class Logger {
public:
	void addOutput(ILogOutput *out) { addOutputMaxLevel(out, LL_MAX); }
	void addOutput(ILogOutput *, LogLevel);
	void addOutputMaxLevel(ILogOutput *, LogLevel);
	void removeOutput(ILogOutput *);
	void setLevelSilenced(LogLevel, bool);

	void registerThread(const std::string &name);
	void deregisterThread();

	void log(LogLevel, const std::string &text);
	// Logs without a prefix
	void logRaw(LogLevel, const std::string &text);

	void setTraceEnabled(bool x) { trace_enabled = x; }
	bool getTraceEnabled() { return trace_enabled; }

private:
	void logToSystem(LogLevel, const std::string &text);
	void logToOutputs(LogLevel, const std::string &text);

	const std::string getLevelLabel(LogLevel lev);
	const std::string getThreadName();

	typedef std::vector<ILogOutput*> OutputList;

	OutputList outputs[LL_MAX];
	// Should implement atomic loads and stores (even though it's only
	// written to when one thread has access currently).
	// Works on all known architectures (x86, ARM, MIPS).
	volatile bool silenced_levels[LL_MAX];
	std::map<threadid_t, std::string> thread_names;
	mutable Mutex mutex;
	bool trace_enabled;
};


class ILogOutput
{
public:
	virtual void log(const std::string &line) = 0;
};


class StreamLogOutput : public ILogOutput
{
public:
	StreamLogOutput(std::ostream &stream) : stream(stream) {}
	void log(const std::string &line) { stream << line << std::endl; }
private:
	std::ostream &stream;
};


class FileLogOutput : public ILogOutput
{
public:
	void open(const std::string &filename);
	void log(const std::string &line) { stream << line << std::endl; }

private:
	std::ofstream stream;
};


class LogOutputBuffer : public ILogOutput
{
public:
	LogOutputBuffer(Logger &logger, LogLevel lev) : logger(logger)
		{ logger.addOutput(this, lev); }
	~LogOutputBuffer() { logger.removeOutput(this); }

	virtual void log(const std::string &line) { buffer.push(line); }
	bool empty() { return buffer.empty(); }

	std::string get()
	{
		if (empty())
			return "";
		std::string s = buffer.front();
		buffer.pop();
		return s;
	}
private:
	std::queue<std::string> buffer;
	Logger &logger;
};


extern StreamLogOutput stdout_output;
extern StreamLogOutput stderr_output;
extern std::ostream null_stream;

extern std::ostream *dout_con_ptr;
extern std::ostream *derr_con_ptr;
extern std::ostream *dout_server_ptr;
extern std::ostream *dout_client_ptr;

#ifndef SERVER
extern std::ostream *derr_client_ptr;
extern std::ostream *derr_server_ptr;
#endif

}; // namespace logging

extern logging::Logger g_logger;

// Writes directly to all LL_NONE log outputs for g_logger with no prefix.
extern std::ostream rawstream;

extern std::ostream errorstream;
extern std::ostream warningstream;
extern std::ostream actionstream;
extern std::ostream infostream;
extern std::ostream verbosestream;
extern std::ostream dstream;


#define TRACEDO(x) { if (g_logger.getTraceEnabled()) { x; } }
#define TRACESTREAM(x) TRACEDO(verbosestream x)

#define dout_con (*logging::dout_con_ptr)
#define derr_con (*logging::derr_con_ptr)
#define dout_server (*logging::dout_server_ptr)
#define derr_server (*logging::derr_server_ptr)

#ifndef SERVER
	#define dout_client (*logging::dout_client_ptr)
	#define derr_client (*logging::derr_client_ptr)
#endif


#endif
