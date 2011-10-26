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

#include "filesys.h"
#include "strfnd.h"
#include <iostream>
#include <string.h>

namespace fs
{

#ifdef _WIN32 // WINDOWS

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h> 
#include <wchar.h> 
#include <stdio.h>

#define BUFSIZE MAX_PATH

std::vector<DirListNode> GetDirListing(std::string pathstring)
{
	std::vector<DirListNode> listing;

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError;
	LPTSTR DirSpec;
	INT retval;

	DirSpec = (LPTSTR) malloc (BUFSIZE);

	if( DirSpec == NULL )
	{
	  printf( "Insufficient memory available\n" );
	  retval = 1;
	  goto Cleanup;
	}

	// Check that the input is not larger than allowed.
	if (pathstring.size() > (BUFSIZE - 2))
	{
	  _tprintf(TEXT("Input directory is too large.\n"));
	  retval = 3;
	  goto Cleanup;
	}

	//_tprintf (TEXT("Target directory is %s.\n"), pathstring.c_str());

	sprintf(DirSpec, "%s", (pathstring + "\\*").c_str());

	// Find the first file in the directory.
	hFind = FindFirstFile(DirSpec, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE) 
	{
	  _tprintf (TEXT("Invalid file handle. Error is %u.\n"), 
				GetLastError());
	  retval = (-1);
	} 
	else 
	{
		// NOTE:
		// Be very sure to not include '..' in the results, it will
		// result in an epic failure when deleting stuff.

		DirListNode node;
		node.name = FindFileData.cFileName;
		node.dir = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		if(node.name != "." && node.name != "..")
			listing.push_back(node);

		// List all the other files in the directory.
		while (FindNextFile(hFind, &FindFileData) != 0) 
		{
			DirListNode node;
			node.name = FindFileData.cFileName;
			node.dir = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			if(node.name != "." && node.name != "..")
				listing.push_back(node);
		}

		dwError = GetLastError();
		FindClose(hFind);
		if (dwError != ERROR_NO_MORE_FILES) 
		{
		 _tprintf (TEXT("FindNextFile error. Error is %u.\n"), 
				   dwError);
		retval = (-1);
		goto Cleanup;
		}
	}
	retval  = 0;

Cleanup:
	free(DirSpec);

	if(retval != 0) listing.clear();

	//for(unsigned int i=0; i<listing.size(); i++){
	//	std::cout<<listing[i].name<<(listing[i].dir?" (dir)":" (file)")<<std::endl;
	//}
	
	return listing;
}

bool CreateDir(std::string path)
{
	bool r = CreateDirectory(path.c_str(), NULL);
	if(r == true)
		return true;
	if(GetLastError() == ERROR_ALREADY_EXISTS)
		return true;
	return false;
}

bool PathExists(std::string path)
{
	return (GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES);
}

bool RecursiveDelete(std::string path)
{
	std::cerr<<"Removing \""<<path<<"\""<<std::endl;

	//return false;
	
	// This silly function needs a double-null terminated string...
	// Well, we'll just make sure it has at least two, then.
	path += "\0\0";

	SHFILEOPSTRUCT sfo;
	sfo.hwnd = NULL;
	sfo.wFunc = FO_DELETE;
	sfo.pFrom = path.c_str();
	sfo.pTo = NULL;
	sfo.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
	
	int r = SHFileOperation(&sfo);

	if(r != 0)
		std::cerr<<"SHFileOperation returned "<<r<<std::endl;

	//return (r == 0);
	return true;
}

#else // POSIX

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

std::vector<DirListNode> GetDirListing(std::string pathstring)
{
	std::vector<DirListNode> listing;

    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(pathstring.c_str())) == NULL) {
		//std::cout<<"Error("<<errno<<") opening "<<pathstring<<std::endl;
        return listing;
    }

    while ((dirp = readdir(dp)) != NULL) {
		// NOTE:
		// Be very sure to not include '..' in the results, it will
		// result in an epic failure when deleting stuff.
		if(dirp->d_name[0]!='.'){
			DirListNode node;
			node.name = dirp->d_name;
			if(dirp->d_type == DT_DIR) node.dir = true;
			else node.dir = false;
			if(node.name != "." && node.name != "..")
				listing.push_back(node);
		}
    }
    closedir(dp);

	return listing;
}

bool CreateDir(std::string path)
{
	int r = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(r == 0)
	{
		return true;
	}
	else
	{
		// If already exists, return true
		if(errno == EEXIST)
			return true;
		return false;
	}
}

bool PathExists(std::string path)
{
	struct stat st;
	return (stat(path.c_str(),&st) == 0);
}

bool RecursiveDelete(std::string path)
{
	/*
		Execute the 'rm' command directly, by fork() and execve()
	*/
	
	std::cerr<<"Removing \""<<path<<"\""<<std::endl;

	//return false;
	
	pid_t child_pid = fork();

	if(child_pid == 0)
	{
		// Child
		char argv_data[3][10000];
		strcpy(argv_data[0], "/bin/rm");
		strcpy(argv_data[1], "-rf");
		strncpy(argv_data[2], path.c_str(), 10000);
		char *argv[4];
		argv[0] = argv_data[0];
		argv[1] = argv_data[1];
		argv[2] = argv_data[2];
		argv[3] = NULL;

		std::cerr<<"Executing '"<<argv[0]<<"' '"<<argv[1]<<"' '"
				<<argv[2]<<"'"<<std::endl;
		
		execv(argv[0], argv);
		
		// Execv shouldn't return. Failed.
		_exit(1);
	}
	else
	{
		// Parent
		int child_status;
		pid_t tpid;
		do{
			tpid = wait(&child_status);
			//if(tpid != child_pid) process_terminated(tpid);
		}while(tpid != child_pid);
		return (child_status == 0);
	}
}

#endif

bool RecursiveDeleteContent(std::string path)
{
	std::cerr<<"Removing content of \""<<path<<"\""<<std::endl;
	std::vector<DirListNode> list = GetDirListing(path);
	for(unsigned int i=0; i<list.size(); i++)
	{
		if(trim(list[i].name) == "." || trim(list[i].name) == "..")
			continue;
		std::string childpath = path + DIR_DELIM + list[i].name;
		bool r = RecursiveDelete(childpath);
		if(r == false)
		{
			std::cerr<<"Removing \""<<childpath<<"\" failed"<<std::endl;
			return false;
		}
	}
	return true;
}

bool CreateAllDirs(std::string path)
{

	size_t pos;
	std::vector<std::string> tocreate;
	std::string basepath = path;
	while(!PathExists(basepath))
	{
		tocreate.push_back(basepath);
		pos = basepath.rfind(DIR_DELIM_C);
		if(pos == std::string::npos)
			return false;
		basepath = basepath.substr(0,pos);
	}
	for(int i=tocreate.size()-1;i>=0;i--)
		CreateDir(tocreate[i]);
	return true;
}

} // namespace fs

