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

/*
	Random portability stuff

	See comments in porting.h
*/

#include "porting.h"
#include "config.h"
#include "debug.h"
#include "filesys.h"
#include "log.h"
#include "util/string.h"
#include <list>

#ifdef __APPLE__
	#include "CoreFoundation/CoreFoundation.h"
#endif

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
		
		// Comment out for less clutter when testing scripts
		/*dstream<<DTIME<<"INFO: sigint_handler(): "
				<<"Printing debug stacks"<<std::endl;
		debug_stacks_print();*/

		g_killed = true;
	}
	else
	{
		(void)signal(SIGINT, SIG_DFL);
	}
}

void signal_handler_init(void)
{
	(void)signal(SIGINT, sigint_handler);
}

#else // _WIN32
	#include <signal.h>
	#include <windows.h>
	
	BOOL WINAPI event_handler(DWORD sig)
	{
		switch(sig)
		{
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:

			if(g_killed == false)
			{
				dstream<<DTIME<<"INFO: event_handler(): "
						<<"Ctrl+C, Close Event, Logoff Event or Shutdown Event, shutting down."<<std::endl;
				// Comment out for less clutter when testing scripts
				/*dstream<<DTIME<<"INFO: event_handler(): "
						<<"Printing debug stacks"<<std::endl;
				debug_stacks_print();*/

				g_killed = true;
			}
			else
			{
				(void)signal(SIGINT, SIG_DFL);
			}

			break;
		case CTRL_BREAK_EVENT:
			break;
		}
		
		return TRUE;
	}
	
void signal_handler_init(void)
{
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE)event_handler,TRUE);
}

#endif

/*
	Path mangler
*/

// Default to RUN_IN_PLACE style relative paths
std::string path_share = "..";
std::string path_user = "..";

std::string getDataPath(const char *subpath)
{
	return path_share + DIR_DELIM + subpath;
}

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

bool detectMSVCBuildDir(char *c_path)
{
	std::string path(c_path);
	const char *ends[] = {"bin\\Release", "bin\\Build", NULL};
	return (removeStringEnd(path, ends) != "");
}

void initializePaths()
{
#if RUN_IN_PLACE
	/*
		Use relative paths if RUN_IN_PLACE
	*/

	infostream<<"Using relative paths (RUN_IN_PLACE)"<<std::endl;

	/*
		Windows
	*/
	#if defined(_WIN32)
		#include <windows.h>

	const DWORD buflen = 1000;
	char buf[buflen];
	DWORD len;
	
	// Find path of executable and set path_share relative to it
	len = GetModuleFileName(GetModuleHandle(NULL), buf, buflen);
	assert(len < buflen);
	pathRemoveFile(buf, '\\');
	
	if(detectMSVCBuildDir(buf)){
		infostream<<"MSVC build directory detected"<<std::endl;
		path_share = std::string(buf) + "\\..\\..";
		path_user = std::string(buf) + "\\..\\..";
	}
	else{
		path_share = std::string(buf) + "\\..";
		path_user = std::string(buf) + "\\..";
	}

	/*
		Linux
	*/
	#elif defined(linux)
		#include <unistd.h>
	
	char buf[BUFSIZ];
	memset(buf, 0, BUFSIZ);
	// Get path to executable
	assert(readlink("/proc/self/exe", buf, BUFSIZ-1) != -1);
	
	pathRemoveFile(buf, '/');

	path_share = std::string(buf) + "/..";
	path_user = std::string(buf) + "/..";
	
	/*
		OS X
	*/
	#elif defined(__APPLE__) || defined(__FreeBSD__)
	
	//TODO: Get path of executable. This assumes working directory is bin/
	dstream<<"WARNING: Relative path not properly supported on OS X and FreeBSD"
			<<std::endl;
	path_share = std::string("..");
	path_user = std::string("..");

	#endif

#else // RUN_IN_PLACE

	/*
		Use platform-specific paths otherwise
	*/

	infostream<<"Using system-wide paths (NOT RUN_IN_PLACE)"<<std::endl;

	/*
		Windows
	*/
	#if defined(_WIN32)
		#include <windows.h>

	const DWORD buflen = 1000;
	char buf[buflen];
	DWORD len;
	
	// Find path of executable and set path_share relative to it
	len = GetModuleFileName(GetModuleHandle(NULL), buf, buflen);
	assert(len < buflen);
	pathRemoveFile(buf, '\\');
	
	// Use ".\bin\.."
	path_share = std::string(buf) + "\\..";
		
	// Use "C:\Documents and Settings\user\Application Data\<PROJECT_NAME>"
	len = GetEnvironmentVariable("APPDATA", buf, buflen);
	assert(len < buflen);
	path_user = std::string(buf) + DIR_DELIM + PROJECT_NAME;

	/*
		Linux
	*/
	#elif defined(linux)
		#include <unistd.h>
	
	// Get path to executable
	std::string bindir = "";
	{
		char buf[BUFSIZ];
		memset(buf, 0, BUFSIZ);
		assert(readlink("/proc/self/exe", buf, BUFSIZ-1) != -1);
		pathRemoveFile(buf, '/');
		bindir = buf;
	}

	// Find share directory from these.
	// It is identified by containing the subdirectory "builtin".
	std::list<std::string> trylist;
	std::string static_sharedir = STATIC_SHAREDIR;
	if(static_sharedir != "" && static_sharedir != ".")
		trylist.push_back(static_sharedir);
	trylist.push_back(bindir + "/../share/" + PROJECT_NAME);
	trylist.push_back(bindir + "/..");
	
	for(std::list<std::string>::const_iterator i = trylist.begin();
			i != trylist.end(); i++)
	{
		const std::string &trypath = *i;
		if(!fs::PathExists(trypath) || !fs::PathExists(trypath + "/builtin")){
			dstream<<"WARNING: system-wide share not found at \""
					<<trypath<<"\""<<std::endl;
			continue;
		}
		// Warn if was not the first alternative
		if(i != trylist.begin()){
			dstream<<"WARNING: system-wide share found at \""
					<<trypath<<"\""<<std::endl;
		}
		path_share = trypath;
		break;
	}
	
	path_user = std::string(getenv("HOME")) + "/." + PROJECT_NAME;

	/*
		OS X
	*/
	#elif defined(__APPLE__)
		#include <unistd.h>

    // Code based on
    // http://stackoverflow.com/questions/516200/relative-paths-not-working-in-xcode-c
    CFBundleRef main_bundle = CFBundleGetMainBundle();
    CFURLRef resources_url = CFBundleCopyResourcesDirectoryURL(main_bundle);
    char path[PATH_MAX];
    if(CFURLGetFileSystemRepresentation(resources_url, TRUE, (UInt8 *)path, PATH_MAX))
	{
		dstream<<"Bundle resource path: "<<path<<std::endl;
		//chdir(path);
		path_share = std::string(path) + "/share";
	}
	else
    {
        // error!
		dstream<<"WARNING: Could not determine bundle resource path"<<std::endl;
    }
    CFRelease(resources_url);
	
	path_user = std::string(getenv("HOME")) + "/Library/Application Support/" + PROJECT_NAME;

	#elif defined(__FreeBSD__)

	path_share = STATIC_SHAREDIR;
	path_user = std::string(getenv("HOME")) + "/." + PROJECT_NAME;
    
	#endif

#endif // RUN_IN_PLACE
}

} //namespace porting

