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

#include "filesys.h"
#include "strfnd.h"
#include <iostream>
#include <string.h>
#include "log.h"

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
	  errorstream<<"GetDirListing: Insufficient memory available"<<std::endl;
	  retval = 1;
	  goto Cleanup;
	}

	// Check that the input is not larger than allowed.
	if (pathstring.size() > (BUFSIZE - 2))
	{
	  errorstream<<"GetDirListing: Input directory is too large."<<std::endl;
	  retval = 3;
	  goto Cleanup;
	}

	//_tprintf (TEXT("Target directory is %s.\n"), pathstring.c_str());

	sprintf(DirSpec, "%s", (pathstring + "\\*").c_str());

	// Find the first file in the directory.
	hFind = FindFirstFile(DirSpec, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE) 
	{
		retval = (-1);
		goto Cleanup;
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
			errorstream<<"GetDirListing: FindNextFile error. Error is "
					<<dwError<<std::endl;
			retval = (-1);
			goto Cleanup;
		}
	}
	retval  = 0;

Cleanup:
	free(DirSpec);

	if(retval != 0) listing.clear();

	//for(unsigned int i=0; i<listing.size(); i++){
	//	infostream<<listing[i].name<<(listing[i].dir?" (dir)":" (file)")<<std::endl;
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

bool IsDir(std::string path)
{
	DWORD attr = GetFileAttributes(path.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES &&
			(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool RecursiveDelete(std::string path)
{
	infostream<<"Recursively deleting \""<<path<<"\""<<std::endl;

	DWORD attr = GetFileAttributes(path.c_str());
	bool is_directory = (attr != INVALID_FILE_ATTRIBUTES &&
			(attr & FILE_ATTRIBUTE_DIRECTORY));
	if(!is_directory)
	{
		infostream<<"RecursiveDelete: Deleting file "<<path<<std::endl;
		//bool did = DeleteFile(path.c_str());
		bool did = true;
		if(!did){
			errorstream<<"RecursiveDelete: Failed to delete file "
					<<path<<std::endl;
			return false;
		}
	}
	else
	{
		infostream<<"RecursiveDelete: Deleting content of directory "
				<<path<<std::endl;
		std::vector<DirListNode> content = GetDirListing(path);
		for(int i=0; i<content.size(); i++){
			const DirListNode &n = content[i];
			std::string fullpath = path + DIR_DELIM + n.name;
			bool did = RecursiveDelete(fullpath);
			if(!did){
				errorstream<<"RecursiveDelete: Failed to recurse to "
						<<fullpath<<std::endl;
				return false;
			}
		}
		infostream<<"RecursiveDelete: Deleting directory "<<path<<std::endl;
		//bool did = RemoveDirectory(path.c_str();
		bool did = true;
		if(!did){
			errorstream<<"Failed to recursively delete directory "
					<<path<<std::endl;
			return false;
		}
	}
	return true;
}

bool DeleteSingleFileOrEmptyDirectory(std::string path)
{
	DWORD attr = GetFileAttributes(path.c_str());
	bool is_directory = (attr != INVALID_FILE_ATTRIBUTES &&
			(attr & FILE_ATTRIBUTE_DIRECTORY));
	if(!is_directory)
	{
		bool did = DeleteFile(path.c_str());
		return did;
	}
	else
	{
		bool did = RemoveDirectory(path.c_str());
		return did;
	}
}

#else // POSIX

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

std::vector<DirListNode> GetDirListing(std::string pathstring)
{
	std::vector<DirListNode> listing;

    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(pathstring.c_str())) == NULL) {
		//infostream<<"Error("<<errno<<") opening "<<pathstring<<std::endl;
        return listing;
    }

    while ((dirp = readdir(dp)) != NULL) {
		// NOTE:
		// Be very sure to not include '..' in the results, it will
		// result in an epic failure when deleting stuff.
		if(dirp->d_name[0]!='.'){
			DirListNode node;
			node.name = dirp->d_name;
			if(node.name == "." || node.name == "..")
				continue;

			int isdir = -1; // -1 means unknown

			/*
				POSIX doesn't define d_type member of struct dirent and
				certain filesystems on glibc/Linux will only return
				DT_UNKNOWN for the d_type member.

				Also we don't know whether symlinks are directories or not.
			*/
#ifdef _DIRENT_HAVE_D_TYPE
			if(dirp->d_type != DT_UNKNOWN && dirp->d_type != DT_LNK)
				isdir = (dirp->d_type == DT_DIR);
#endif /* _DIRENT_HAVE_D_TYPE */

			/*
				Was d_type DT_UNKNOWN, DT_LNK or nonexistent?
				If so, try stat().
			*/
			if(isdir == -1)
			{
				struct stat statbuf;
				if (stat((pathstring + "/" + node.name).c_str(), &statbuf))
					continue;
				isdir = ((statbuf.st_mode & S_IFDIR) == S_IFDIR);
			}
			node.dir = isdir;
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

bool IsDir(std::string path)
{
	struct stat statbuf;
	if(stat(path.c_str(), &statbuf))
		return false; // Actually error; but certainly not a directory
	return ((statbuf.st_mode & S_IFDIR) == S_IFDIR);
}

bool RecursiveDelete(std::string path)
{
	/*
		Execute the 'rm' command directly, by fork() and execve()
	*/
	
	infostream<<"Removing \""<<path<<"\""<<std::endl;

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

		verbosestream<<"Executing '"<<argv[0]<<"' '"<<argv[1]<<"' '"
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

bool DeleteSingleFileOrEmptyDirectory(std::string path)
{
	if(IsDir(path)){
		bool did = (rmdir(path.c_str()) == 0);
		if(!did)
			errorstream<<"rmdir errno: "<<errno<<": "<<strerror(errno)
					<<std::endl;
		return did;
	} else {
		bool did = (unlink(path.c_str()) == 0);
		if(!did)
			errorstream<<"unlink errno: "<<errno<<": "<<strerror(errno)
					<<std::endl;
		return did;
	}
}

#endif

void GetRecursiveSubPaths(std::string path, std::vector<std::string> &dst)
{
	std::vector<DirListNode> content = GetDirListing(path);
	for(unsigned int  i=0; i<content.size(); i++){
		const DirListNode &n = content[i];
		std::string fullpath = path + DIR_DELIM + n.name;
		dst.push_back(fullpath);
		GetRecursiveSubPaths(fullpath, dst);
	}
}

bool DeletePaths(const std::vector<std::string> &paths)
{
	bool success = true;
	// Go backwards to succesfully delete the output of GetRecursiveSubPaths
	for(int i=paths.size()-1; i>=0; i--){
		const std::string &path = paths[i];
		bool did = DeleteSingleFileOrEmptyDirectory(path);
		if(!did){
			errorstream<<"Failed to delete "<<path<<std::endl;
			success = false;
		}
	}
	return success;
}

bool RecursiveDeleteContent(std::string path)
{
	infostream<<"Removing content of \""<<path<<"\""<<std::endl;
	std::vector<DirListNode> list = GetDirListing(path);
	for(unsigned int i=0; i<list.size(); i++)
	{
		if(trim(list[i].name) == "." || trim(list[i].name) == "..")
			continue;
		std::string childpath = path + DIR_DELIM + list[i].name;
		bool r = RecursiveDelete(childpath);
		if(r == false)
		{
			errorstream<<"Removing \""<<childpath<<"\" failed"<<std::endl;
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
			break;
		basepath = basepath.substr(0,pos);
	}
	for(int i=tocreate.size()-1;i>=0;i--)
		if(!CreateDir(tocreate[i]))
			return false;
	return true;
}

} // namespace fs

