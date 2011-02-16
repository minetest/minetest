/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
	Random portability stuff

	See comments in porting.h
*/

#include "porting.h"
#include "config.h"
#include "debug.h"

namespace porting
{

/*
	Signal handler (grabs Ctrl-C on POSIX systems)
*/

bool g_killed = false;

bool * signal_handler_killstatus(void)
{
	return &g_killed;
}

#if !defined(_WIN32) // POSIX
	#include <signal.h>

void sigint_handler(int sig)
{
	if(g_killed == false)
	{
		dstream<<DTIME<<"INFO: sigint_handler(): "
				<<"Ctrl-C pressed, shutting down."<<std::endl;
		
		dstream<<DTIME<<"INFO: siging_handler(): "
				<<"Printing debug stacks"<<std::endl;
		debug_stacks_print();

		g_killed = true;
	}
	else
	{
		(void)signal(SIGINT, SIG_DFL);
	}
}

void signal_handler_init(void)
{
	dstream<<"signal_handler_init()"<<std::endl;
	(void)signal(SIGINT, sigint_handler);
}

#else // _WIN32

void signal_handler_init(void)
{
	// No-op
}

#endif

/*
	Path mangler
*/

std::string path_data = "../data";
std::string path_userdata = "../";

void pathRemoveFile(char *path, char delim)
{
	// Remove filename and path delimiter
	int i;
	for(i = strlen(path)-1; i>=0; i--)
	{
		if(path[i] == delim)
			break;
	}
	path[i] = 0;
}

void initializePaths()
{
#ifdef RUN_IN_PLACE
	/*
		Use relative paths if RUN_IN_PLACE
	*/

	dstream<<"Using relative paths (RUN_IN_PLACE)"<<std::endl;

	/*
		Windows
	*/
	#if defined(_WIN32)
		#include <windows.h>

	const DWORD buflen = 1000;
	char buf[buflen];
	DWORD len;
	
	// Find path of executable and set path_data relative to it
	len = GetModuleFileName(GetModuleHandle(NULL), buf, buflen);
	assert(len < buflen);
	pathRemoveFile(buf, '\\');

	// Use "./bin/../data"
	path_data = std::string(buf) + "/../data";
		
	// Use "./bin/../"
	path_userdata = std::string(buf) + "/../";

	/*
		Linux
	*/
	#elif defined(linux)
		#include <unistd.h>
	
	char buf[BUFSIZ];
	memset(buf, 0, BUFSIZ);
	// Get path to executable
	readlink("/proc/self/exe", buf, BUFSIZ-1);
	
	pathRemoveFile(buf, '/');

	// Use "./bin/../data"
	path_data = std::string(buf) + "/../data";
		
	// Use "./bin/../"
	path_userdata = std::string(buf) + "/../";
	
	/*
		OS X
	*/
	#elif defined(__APPLE__)
	
	//TODO: Get path of executable. This assumes working directory is bin/
	dstream<<"WARNING: Relative path not properly supported on OS X"
			<<std::endl;
	path_data = std::string("../data");
	path_userdata = std::string("../");

	#endif

#else // RUN_IN_PLACE

	/*
		Use platform-specific paths otherwise
	*/

	dstream<<"Using system-wide paths (NOT RUN_IN_PLACE)"<<std::endl;

	/*
		Windows
	*/
	#if defined(_WIN32)
		#include <windows.h>

	const DWORD buflen = 1000;
	char buf[buflen];
	DWORD len;
	
	// Find path of executable and set path_data relative to it
	len = GetModuleFileName(GetModuleHandle(NULL), buf, buflen);
	assert(len < buflen);
	pathRemoveFile(buf, '\\');
	
	// Use "./bin/../data"
	path_data = std::string(buf) + "/../data";
	//path_data = std::string(buf) + "/../share/" + APPNAME;
		
	// Use "C:\Documents and Settings\user\Application Data\<APPNAME>"
	len = GetEnvironmentVariable("APPDATA", buf, buflen);
	assert(len < buflen);
	path_userdata = std::string(buf) + "/" + APPNAME;

	/*
		Linux
	*/
	#elif defined(linux)
		#include <unistd.h>
	
	char buf[BUFSIZ];
	memset(buf, 0, BUFSIZ);
	// Get path to executable
	readlink("/proc/self/exe", buf, BUFSIZ-1);
	
	pathRemoveFile(buf, '/');

	path_data = std::string(buf) + "/../share/" + APPNAME;
	//path_data = std::string(INSTALL_PREFIX) + "/share/" + APPNAME;
	
	path_userdata = std::string(getenv("HOME")) + "/." + APPNAME;

	/*
		OS X
	*/
	#elif defined(__APPLE__)
		#include <unistd.h>

	path_userdata = std::string(getenv("HOME")) + "/Library/Application Support/" + APPNAME;
	path_data = std::string("minetest-mac.app/Contents/Resources/data/");

	#endif

#endif // RUN_IN_PLACE

	dstream<<"path_data = "<<path_data<<std::endl;
	dstream<<"path_userdata = "<<path_userdata<<std::endl;
}

} //namespace porting

