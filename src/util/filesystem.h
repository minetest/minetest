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

#ifndef FILESYSTEM_HEADER
#define FILESYSTEM_HEADER

#include <string>
#include <iostream>
#include <cstdio>
#include <stdint.h>
#include "exceptions.h"
#include "log.h"

#ifdef _WIN32 // WINDOWS
	#define DIR_DELIM "\\"
	#define DIR_DELIM_CHAR '\\'
	#define FILESYS_CASE_INSENSITIVE 1
	#define PATH_DELIM ";"

	#ifndef _WIN32_LEAN_AND_MEAN
		#define _WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else // POSIX
	#define DIR_DELIM "/"
	#define DIR_DELIM_CHAR '/'
	#define FILESYS_CASE_INSENSITIVE 0
	#define PATH_DELIM ":"

	#include <dirent.h>
#endif

namespace fs {

class FilesystemError : public BaseException {
public:
	FilesystemError(const std::string &s) : BaseException(s) {}
};


enum FileType {
	FT_ERROR,
	FT_REGULAR,
	FT_DIRECTORY,
	FT_SYMLINK,
	FT_BLOCK,
	FT_CHARACTER,
	FT_FIFO,
	FT_SOCKET,
	FT_UNKNOWN,
};


enum DirectoryOptions {
	DO_NONE = 0x0,
	DO_ALLOW_STAT = 0x1,
	DO_FOLLOW_SYMLINKS = 0x2,
	DO_DEFAULT = DO_ALLOW_STAT | DO_FOLLOW_SYMLINKS,
};


struct DirectoryEntry {
	DirectoryEntry() : type(FT_ERROR) {}
	DirectoryEntry(const std::string name, FileType type) :
		name(name), type(type) {}
	std::string name;
	FileType type;
	time_t atime, mtime, ctime;
	long atime_nsec, mtime_nsec, ctime_nsec;
	uintmax_t size;
};


class DirectoryIterator : public std::iterator<std::input_iterator_tag, DirectoryEntry> {
public:
	DirectoryIterator();  // End iterator
	DirectoryIterator(const std::string &path,
		DirectoryOptions opts=DO_DEFAULT);
	~DirectoryIterator();

	// Only equal if both are end iterators
	bool operator == (const DirectoryIterator &other);
	bool operator != (const DirectoryIterator &other)
		{ return !(*this == other); }

	DirectoryIterator& operator ++ ();
	const DirectoryEntry& operator * () { return entry; }
	const DirectoryEntry* operator -> () { return &entry; }

private:
	DirectoryOptions options;
	std::string path;
	DirectoryEntry entry;

#ifdef _WIN32
	void populateEntry(const WIN32_FIND_DATA &data);

	HANDLE find_handle;
#else  // POSIX
	void stat();

	DIR *dirp;
#endif
};


FILE* open(const std::string &path, const char *mode);

// Returns true if already exists
bool create_directory(const std::string &path);

// Create all directories on the given path that don't already exist.
bool create_directories(const std::string &path);

bool exists(const std::string &path);

#ifdef _WIN32
bool is_absolute(const std::string &path);
#else
inline bool is_absolute(const std::string &path)
	{ return path[0] == '/'; }
#endif

#ifdef _WIN32
inline bool is_directory_delimiter(char c)
	{ return c == '/' || c == '\\'; }
#else
inline bool is_directory_delimiter(char c)
	{ return c == '/'; }
#endif

bool remove(const std::string &path);

/// Recursively removes directory and contents. Only pass full paths.
bool remove_all(const std::string &path, bool remove_top=true);

/// Returns path to temp directory, returns "" on error.
std::string get_temp_path();

/// Copy a regular file.
bool copy_file(const std::string &source, const std::string &target);

/// Copy directory and all subdirectories.
bool copy_directory(const std::string &source, const std::string &target);

bool is_directory(const std::string &path);

/// Check if one path is prefix of another.
/// For example, "/tmp" is a prefix of "/tmp" and "/tmp/file" but not "/tmp2".
/// Ignores case differences and '/' vs. '\\' on Windows.
bool path_starts_with(const std::string &path, const std::string &prefix);

/// Remove last path components and dir delimiters before and/or after them.
/// Returns "" if there is only one path component.
/// @p removed If non-NULL, receives the removed component(s).
/// @p count Number of components to remove.
std::string remove_path_components(const std::string &path,
               std::string *removed = NULL, int count = 1);

/// Remove "." and ".." path components, and for every ".." removed remove
/// the last normal path component before it.  Unlike AbsolutePath,
/// this does not resolve symlinks and check for existence of directories.
std::string remove_relative_path_components(const std::string &path);

/// Returns the absolute path for the passed path, with "." and ".." path
/// components and symlinks removed.  Returns "" on error.
std::string canonical(const std::string &path);

// Returns the filename from a path or the entire path if no directory
// delimiter is found.
const char *filename(const char *path);

bool rename(const std::string &from, const std::string &to);


class SafeWriteBuffer : public std::streambuf {
public:
	SafeWriteBuffer(const std::string &path, const char *mode) :
		fd(fs::open(path, mode))
	{}
	void close() { if (fd) { fclose(fd); fd = NULL; } }
	bool good() { return (fd != NULL) && (ferror(fd) == 0); }

	int overflow(int c) { return putc(c, fd); }
	std::streamsize xsputn(const char *s, std::streamsize n)
		{ return fwrite(s, n, 1, fd) ? n : 0; }
	void push_back(char c) { putc(c, fd); }

private:
	int sync() { return fflush(fd); }

	FILE *fd;
};


class SafeWriteStream : public std::ostream {
public:
	SafeWriteStream(const std::string &path, const char *mode="w");
	~SafeWriteStream();

	bool save();
	bool good() { return std::ostream::good() && buf.good(); }
private:
	std::string path;
	std::string temp_path;
	SafeWriteBuffer buf;
};

} // namespace fs

#endif

