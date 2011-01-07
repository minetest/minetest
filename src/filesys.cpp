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
#include <iostream>

namespace fs
{

#ifdef _WIN32 // WINDOWS

#define _WIN32_WINNT 0x0501
#include <Windows.h>
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
		DirListNode node;
		node.name = FindFileData.cFileName;
		node.dir = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		listing.push_back(node);

		// List all the other files in the directory.
		while (FindNextFile(hFind, &FindFileData) != 0) 
		{
			DirListNode node;
			node.name = FindFileData.cFileName;
			node.dir = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
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

#else // POSIX

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

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
		if(dirp->d_name[0]!='.'){
			DirListNode node;
			node.name = dirp->d_name;
			if(dirp->d_type == DT_DIR) node.dir = true;
			else node.dir = false;
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

#endif

} // namespace fs

