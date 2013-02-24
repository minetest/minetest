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

#include <string>

/*
	Use this for logging everything.
	
	If you need to explicitly print something, use dstream or cout or cerr.
*/

enum LogMessageLevel {
	LMT_ERROR, /* Something failed ("invalid map data on disk, block (2,2,1)") */
	LMT_ACTION, /* In-game actions ("celeron55 placed block at (12,4,-5)") */
	LMT_INFO, /* More deep info ("saving map on disk (only_modified=true)") */
	LMT_VERBOSE, /* Flood-style ("loaded block (2,2,2) from disk") */
	LMT_NUM_VALUES,
};

class ILogOutput
{
public:
	/* line: Full line with timestamp, level and thread */
	virtual void printLog(const std::string &line){};
	/* line: Full line with timestamp, level and thread */
	virtual void printLog(const std::string &line, enum LogMessageLevel lev){};
	/* line: Only actual printed text */
	virtual void printLog(enum LogMessageLevel lev, const std::string &line){};
};

void log_add_output(ILogOutput *out, enum LogMessageLevel lev);
void log_add_output_maxlev(ILogOutput *out, enum LogMessageLevel lev);
void log_add_output_all_levs(ILogOutput *out);
void log_remove_output(ILogOutput *out);

void log_register_thread(const std::string &name);
void log_deregister_thread();

void log_printline(enum LogMessageLevel lev, const std::string &text);

#define LOGLINEF(lev, ...)\
{\
	char buf[10000];\
	snprintf(buf, 10000, __VA_ARGS__);\
	log_printline(lev, buf);\
}

extern std::ostream errorstream;
extern std::ostream actionstream;
extern std::ostream infostream;
extern std::ostream verbosestream;

extern bool log_trace_level_enabled;

#define TRACESTREAM(x){ if(log_trace_level_enabled) verbosestream x; }
#define TRACEDO(x){ if(log_trace_level_enabled){ x ;} }

#endif

