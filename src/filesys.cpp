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

#include "filesys.h"
#include "util/string.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fstream>
#include "log.h"
#include "config.h"

namespace fs
{

#ifdef _WIN32 // WINDOWS

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <malloc.h>
#include <tchar.h>
#include <wchar.h>

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

bool IsDirDelimiter(char c)
{
	return c == '/' || c == '\\';
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

std::string TempPath()
{
	DWORD bufsize = GetTempPath(0, "");
	if(bufsize == 0){
		errorstream<<"GetTempPath failed, error = "<<GetLastError()<<std::endl;
		return "";
	}
	std::vector<char> buf(bufsize);
	DWORD len = GetTempPath(bufsize, &buf[0]);
	if(len == 0 || len > bufsize){
		errorstream<<"GetTempPath failed, error = "<<GetLastError()<<std::endl;
		return "";
	}
	return std::string(buf.begin(), buf.begin() + len);
}

#else // POSIX

#include <sys/types.h>
#include <dirent.h>
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

bool IsDirDelimiter(char c)
{
	return c == '/';
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

std::string TempPath()
{
	/*
		Should the environment variables TMPDIR, TMP and TEMP
		and the macro P_tmpdir (if defined by stdio.h) be checked
		before falling back on /tmp?

		Probably not, because this function is intended to be
		compatible with lua's os.tmpname which under the default
		configuration hardcodes mkstemp("/tmp/lua_XXXXXX").
	*/
#ifdef __ANDROID__
	return DIR_DELIM "sdcard" DIR_DELIM PROJECT_NAME DIR_DELIM "tmp";
#else
	return DIR_DELIM "tmp";
#endif
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

	std::vector<std::string> tocreate;
	std::string basepath = path;
	while(!PathExists(basepath))
	{
		tocreate.push_back(basepath);
		basepath = RemoveLastPathComponent(basepath);
		if(basepath.empty())
			break;
	}
	for(int i=tocreate.size()-1;i>=0;i--)
		if(!CreateDir(tocreate[i]))
			return false;
	return true;
}

bool CopyFileContents(std::string source, std::string target)
{
	FILE *sourcefile = fopen(source.c_str(), "rb");
	if(sourcefile == NULL){
		errorstream<<source<<": can't open for reading: "
			<<strerror(errno)<<std::endl;
		return false;
	}

	FILE *targetfile = fopen(target.c_str(), "wb");
	if(targetfile == NULL){
		errorstream<<target<<": can't open for writing: "
			<<strerror(errno)<<std::endl;
		fclose(sourcefile);
		return false;
	}

	size_t total = 0;
	bool retval = true;
	bool done = false;
	char readbuffer[BUFSIZ];
	while(!done){
		size_t readbytes = fread(readbuffer, 1,
				sizeof(readbuffer), sourcefile);
		total += readbytes;
		if(ferror(sourcefile)){
			errorstream<<source<<": IO error: "
				<<strerror(errno)<<std::endl;
			retval = false;
			done = true;
		}
		if(readbytes > 0){
			fwrite(readbuffer, 1, readbytes, targetfile);
		}
		if(feof(sourcefile) || ferror(sourcefile)){
			// flush destination file to catch write errors
			// (e.g. disk full)
			fflush(targetfile);
			done = true;
		}
		if(ferror(targetfile)){
			errorstream<<target<<": IO error: "
					<<strerror(errno)<<std::endl;
			retval = false;
			done = true;
		}
	}
	infostream<<"copied "<<total<<" bytes from "
		<<source<<" to "<<target<<std::endl;
	fclose(sourcefile);
	fclose(targetfile);
	return retval;
}

bool CopyDir(std::string source, std::string target)
{
	if(PathExists(source)){
		if(!PathExists(target)){
			fs::CreateAllDirs(target);
		}
		bool retval = true;
		std::vector<DirListNode> content = fs::GetDirListing(source);

		for(unsigned int i=0; i < content.size(); i++){
			std::string sourcechild = source + DIR_DELIM + content[i].name;
			std::string targetchild = target + DIR_DELIM + content[i].name;
			if(content[i].dir){
				if(!fs::CopyDir(sourcechild, targetchild)){
					retval = false;
				}
			}
			else {
				if(!fs::CopyFileContents(sourcechild, targetchild)){
					retval = false;
				}
			}
		}
		return retval;
	}
	else {
		return false;
	}
}

bool PathStartsWith(std::string path, std::string prefix)
{
	size_t pathsize = path.size();
	size_t pathpos = 0;
	size_t prefixsize = prefix.size();
	size_t prefixpos = 0;
	for(;;){
		bool delim1 = pathpos == pathsize
			|| IsDirDelimiter(path[pathpos]);
		bool delim2 = prefixpos == prefixsize
			|| IsDirDelimiter(prefix[prefixpos]);

		if(delim1 != delim2)
			return false;

		if(delim1){
			while(pathpos < pathsize &&
					IsDirDelimiter(path[pathpos]))
				++pathpos;
			while(prefixpos < prefixsize &&
					IsDirDelimiter(prefix[prefixpos]))
				++prefixpos;
			if(prefixpos == prefixsize)
				return true;
			if(pathpos == pathsize)
				return false;
		}
		else{
			size_t len = 0;
			do{
				char pathchar = path[pathpos+len];
				char prefixchar = prefix[prefixpos+len];
				if(FILESYS_CASE_INSENSITIVE){
					pathchar = tolower(pathchar);
					prefixchar = tolower(prefixchar);
				}
				if(pathchar != prefixchar)
					return false;
				++len;
			} while(pathpos+len < pathsize
					&& !IsDirDelimiter(path[pathpos+len])
					&& prefixpos+len < prefixsize
					&& !IsDirDelimiter(
						prefix[prefixpos+len]));
			pathpos += len;
			prefixpos += len;
		}
	}
}

std::string RemoveLastPathComponent(std::string path,
		std::string *removed, int count)
{
	if(removed)
		*removed = "";

	size_t remaining = path.size();

	for(int i = 0; i < count; ++i){
		// strip a dir delimiter
		while(remaining != 0 && IsDirDelimiter(path[remaining-1]))
			remaining--;
		// strip a path component
		size_t component_end = remaining;
		while(remaining != 0 && !IsDirDelimiter(path[remaining-1]))
			remaining--;
		size_t component_start = remaining;
		// strip a dir delimiter
		while(remaining != 0 && IsDirDelimiter(path[remaining-1]))
			remaining--;
		if(removed){
			std::string component = path.substr(component_start,
					component_end - component_start);
			if(i)
				*removed = component + DIR_DELIM + *removed;
			else
				*removed = component;
		}
	}
	return path.substr(0, remaining);
}

std::string RemoveRelativePathComponents(std::string path)
{
	size_t pos = path.size();
	size_t dotdot_count = 0;
	while(pos != 0){
		size_t component_with_delim_end = pos;
		// skip a dir delimiter
		while(pos != 0 && IsDirDelimiter(path[pos-1]))
			pos--;
		// strip a path component
		size_t component_end = pos;
		while(pos != 0 && !IsDirDelimiter(path[pos-1]))
			pos--;
		size_t component_start = pos;

		std::string component = path.substr(component_start,
				component_end - component_start);
		bool remove_this_component = false;
		if(component == "."){
			remove_this_component = true;
		}
		else if(component == ".."){
			remove_this_component = true;
			dotdot_count += 1;
		}
		else if(dotdot_count != 0){
			remove_this_component = true;
			dotdot_count -= 1;
		}

		if(remove_this_component){
			while(pos != 0 && IsDirDelimiter(path[pos-1]))
				pos--;
			path = path.substr(0, pos) + DIR_DELIM +
				path.substr(component_with_delim_end,
						std::string::npos);
			pos++;
		}
	}

	if(dotdot_count > 0)
		return "";

	// remove trailing dir delimiters
	pos = path.size();
	while(pos != 0 && IsDirDelimiter(path[pos-1]))
		pos--;
	return path.substr(0, pos);
}

bool safeWriteToFile(const std::string &path, const std::string &content)
{
	std::string tmp_file = path + ".~mt";

	// Write to a tmp file
	std::ofstream os(tmp_file.c_str(), std::ios::binary);
	if (!os.good())
		return false;
	os << content;
	os.flush();
	os.close();
	if (os.fail()) {
		remove(tmp_file.c_str());
		return false;
	}

	// Copy file
	remove(path.c_str());
	if(rename(tmp_file.c_str(), path.c_str())) {
		remove(tmp_file.c_str());
		return false;
	} else {
		return true;
	}
}

} // namespace fs

