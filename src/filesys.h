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

#ifndef FILESYS_HEADER
#define FILESYS_HEADER

#include <string>
#include <vector>
#include "exceptions.h"

#ifdef _WIN32 // WINDOWS
#define DIR_DELIM "\\"
#define FILESYS_CASE_INSENSITIVE 1
#else // POSIX
#define DIR_DELIM "/"
#define FILESYS_CASE_INSENSITIVE 0
#endif

namespace fs
{

struct DirListNode
{
	std::string name;
	bool dir;
};
std::vector<DirListNode> GetDirListing(std::string path);

// Returns true if already exists
bool CreateDir(std::string path);

bool PathExists(std::string path);

bool IsDir(std::string path);

bool IsDirDelimiter(char c);

// Only pass full paths to this one. True on success.
// NOTE: The WIN32 version returns always true.
bool RecursiveDelete(std::string path);

bool DeleteSingleFileOrEmptyDirectory(std::string path);

// Returns path to temp directory, can return "" on error
std::string TempPath();

/* Multiplatform */

// The path itself not included
void GetRecursiveSubPaths(std::string path, std::vector<std::string> &dst);

// Tries to delete all, returns false if any failed
bool DeletePaths(const std::vector<std::string> &paths);

// Only pass full paths to this one. True on success.
bool RecursiveDeleteContent(std::string path);

// Create all directories on the given path that don't already exist.
bool CreateAllDirs(std::string path);

// Copy a regular file
bool CopyFileContents(std::string source, std::string target);

// Copy directory and all subdirectories
// Omits files and subdirectories that start with a period
bool CopyDir(std::string source, std::string target);

// Check if one path is prefix of another
// For example, "/tmp" is a prefix of "/tmp" and "/tmp/file" but not "/tmp2"
// Ignores case differences and '/' vs. '\\' on Windows
bool PathStartsWith(std::string path, std::string prefix);

// Remove last path component and the dir delimiter before and/or after it,
// returns "" if there is only one path component.
// removed: If non-NULL, receives the removed component(s).
// count: Number of components to remove
std::string RemoveLastPathComponent(std::string path,
               std::string *removed = NULL, int count = 1);

// Remove "." and ".." path components and for every ".." removed, remove
// the last normal path component before it. Unlike AbsolutePath,
// this does not resolve symlinks and check for existence of directories.
std::string RemoveRelativePathComponents(std::string path);

bool safeWriteToFile(const std::string &path, const std::string &content);

}//fs

#endif

