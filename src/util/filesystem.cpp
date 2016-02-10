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

#ifdef _WIN32
	#ifdef _WIN32_WINNT
		#undef _WIN32_WINNT
	#endif
	#define _WIN32_WINNT 0x0501
	#define UNICODE  // Enable Windows 16-bit pseudo-unicode
	#include <windows.h>
	#include <shlwapi.h>
#else  // POSIX
	#include <sys/types.h>
	#include <dirent.h>
	#include <sys/stat.h>
	#include <sys/wait.h>
	#include <unistd.h>
#endif

#include "util/filesystem.h"
#include "util/string.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include "config.h"


namespace fs {


bool create_directories(const std::string &path)
{
	std::vector<std::string> to_create;
	std::string base_path = path;
	while (!exists(base_path)) {
		to_create.push_back(base_path);
		base_path = remove_path_components(base_path);
		if (base_path.empty())
			break;
	}
	for (int i = to_create.size() - 1; i >= 0; i--)
		if (!create_directory(to_create[i]))
			return false;
	return true;
}


bool copy_file(const std::string &from, const std::string &to)
{
	FILE *from_file = fs::open(from.c_str(), "rb");
	if (from_file == NULL) {
		errorstream << from << ": can't open for reading: "
			<< strerror(errno) << std::endl;
		return false;
	}

	FILE *to_file = fs::open(to.c_str(), "wb");
	if (to_file == NULL) {
		errorstream << to << ": can't open for writing: "
			<< strerror(errno) << std::endl;
		fclose(from_file);
		return false;
	}

	size_t total = 0;
	bool retval = true;
	char buf[BUFSIZ];
	while (!feof(from_file)) {
		size_t bytes = fread(buf, 1, sizeof(buf), from_file);
		total += bytes;
		if (ferror(from_file)) {
			errorstream << from << ": IO error: "
				<< strerror(errno) << std::endl;
			retval = false;
			goto cleanup;
		}
		if (bytes > 0)
			fwrite(buf, 1, bytes, to_file);
		if (ferror(to_file)) {
			errorstream << to << ": IO error: "
					<< strerror(errno) << std::endl;
			retval = false;
			goto cleanup;
		}
	}

cleanup:
	infostream << "copied " << total << " bytes from "
		<< from << " to " << to << std::endl;
	fclose(from_file);
	fclose(to_file);
	return retval;
}


bool copy_directory(const std::string &from, const std::string &to)
{
	if (!exists(from)) {
		return false;
	}
	if (!exists(to)) {
		fs::create_directories(to);
	}

	bool retval = true;

	for (DirectoryIterator it(from); it != DirectoryIterator(); ++it) {
		std::string from_child = from + DIR_DELIM + it->name;
		std::string to_child = to + DIR_DELIM + it->name;
		if (it->type == FT_DIRECTORY) {
			if (!copy_directory(from_child, to_child))
				retval = false;
		} else {
			if (!copy_file(from_child, to_child))
				retval = false;
		}
	}
	return retval;
}


bool path_starts_with(const std::string &path, const std::string &prefix)
{
	size_t pathsize = path.size();
	size_t pathpos = 0;
	size_t prefixsize = prefix.size();
	size_t prefixpos = 0;
	while (true) {
		bool delim1 = pathpos == pathsize ||
			is_directory_delimiter(path[pathpos]);
		bool delim2 = prefixpos == prefixsize ||
			is_directory_delimiter(prefix[prefixpos]);

		if (delim1 != delim2)
			return false;

		if (delim1) {
			while (pathpos < pathsize &&
					is_directory_delimiter(path[pathpos]))
				++pathpos;
			while (prefixpos < prefixsize &&
					is_directory_delimiter(prefix[prefixpos]))
				++prefixpos;
			if (prefixpos == prefixsize)
				return true;
			if (pathpos == pathsize)
				return false;
		} else {
			size_t len = 0;
			do {
				char pathchar = path[pathpos+len];
				char prefixchar = prefix[prefixpos+len];
				if (FILESYS_CASE_INSENSITIVE) {
					pathchar = tolower(pathchar);
					prefixchar = tolower(prefixchar);
				}
				if (pathchar != prefixchar)
					return false;
				++len;
			} while (pathpos+len < pathsize
					&& !is_directory_delimiter(path[pathpos+len])
					&& prefixpos+len < prefixsize
					&& !is_directory_delimiter(
						prefix[prefixpos+len]));
			pathpos += len;
			prefixpos += len;
		}
	}
}


std::string remove_path_components(const std::string &path,
		std::string *removed, int count)
{
	if (removed)
		*removed = "";

	size_t remaining = path.size();

	for (int i = 0; i < count; ++i) {
		// Strip a dir delimiter
		while (remaining != 0 && is_directory_delimiter(path[remaining-1]))
			remaining--;
		// Strip a path component
		size_t component_end = remaining;
		while (remaining != 0 && !is_directory_delimiter(path[remaining-1]))
			remaining--;
		size_t component_start = remaining;
		// Strip a dir delimiter
		while (remaining != 0 && is_directory_delimiter(path[remaining-1]))
			remaining--;
		if (removed) {
			std::string component = path.substr(component_start,
					component_end - component_start);
			if (i)
				*removed = component + DIR_DELIM + *removed;
			else
				*removed = component;
		}
	}
	return path.substr(0, remaining);
}


std::string remove_relative_path_components(const std::string &p)
{
	std::string path(p);
	size_t pos = path.size();
	size_t dotdot_count = 0;
	while (pos != 0) {
		size_t component_with_delim_end = pos;
		// Skip a dir delimiter
		while (pos != 0 && is_directory_delimiter(path[pos-1]))
			pos--;
		// Strip a path component
		size_t component_end = pos;
		while (pos != 0 && !is_directory_delimiter(path[pos-1]))
			pos--;
		size_t component_start = pos;

		std::string component = path.substr(component_start,
				component_end - component_start);
		bool remove_this_component = false;
		if (component == ".") {
			remove_this_component = true;
		} else if (component == "..") {
			remove_this_component = true;
			dotdot_count += 1;
		} else if (dotdot_count != 0) {
			remove_this_component = true;
			dotdot_count -= 1;
		}

		if (remove_this_component) {
			while (pos != 0 && is_directory_delimiter(path[pos-1]))
				pos--;
			path = path.substr(0, pos) + DIR_DELIM +
				path.substr(component_with_delim_end,
						std::string::npos);
			pos++;
		}
	}

	if (dotdot_count > 0)
		return "";

	// Remove trailing dir delimiters
	pos = path.size();
	while (pos != 0 && is_directory_delimiter(path[pos-1]))
		pos--;
	return path.substr(0, pos);
}


std::string canonical(const std::string &path)
{
#ifdef _WIN32
	char *cpath = _fullpath(NULL, path.c_str(), MAX_PATH);
#else
	char *cpath = realpath(path.c_str(), NULL);
#endif
	if (!cpath)
		return "";
	std::string cpath_str(cpath);
	free(cpath);
	return cpath_str;
}


const char *filename(const char *path)
{
	const char *filename = strrchr(path, DIR_DELIM_CHAR);
	return filename ? filename + 1 : path;
}


SafeWriteStream::SafeWriteStream(const std::string &path, const char *mode) :
	std::ostream(&buf),
	path(path),
	temp_path(path + ".mt~"),
	buf(temp_path, mode)
{}


SafeWriteStream::~SafeWriteStream()
{
	buf.close();
	fs::remove(temp_path);
}


bool SafeWriteStream::save()
{
	flush();
	bool ok = good();
	buf.close();
	if (!ok) {
		// Remove the temporary file because writing it failed and it's useless.
		fs::remove(temp_path);
		return false;
	}

	// Move the finished temporary file over the real file
	if (!fs::rename(temp_path, path)) {
		// Remove the temporary file because moving
		// it over the target file failed.
		fs::remove(temp_path);
		return false;
	} else {
		return true;
	}
}


#ifdef _WIN32  // WINDOWS


static inline std::wstring winapi_path(const std::string &path)
{
	return L"\\\\?\\" + narrow_to_wide(path);
}


static std::string gle_to_string(DWORD err)
{
	wchar_t *buf = NULL;
	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&buf, 0, NULL);
	std::wstring message(buf, size);
	LocalFree(buf);
	return wide_to_narrow(message);
}


static inline FileType w32_find_ftype(const WIN32_FIND_DATA &data)
{
	DWORD attr = data.dwFileAttributes;
	if (attr & FILE_ATTRIBUTE_NORMAL)
		return FT_REGULAR;
	else if ((attr & FILE_ATTRIBUTE_REPARSE_POINT) &&
			(data.dwReserved0 & IO_REPARSE_TAG_SYMLINK))
		return FT_SYMLINK;
	else if (attr & FILE_ATTRIBUTE_DIRECTORY)
		return FT_DIRECTORY;
	else
		return FT_UNKNOWN;
}


static inline void convert_filetime(const FILETIME &ft, time_t *time_s, long *time_n)
{
	int64_t time_win_100ns = (uint64_t)ft.dwLowDateTime |
			((uint64_t)ft.dwHighDateTime << (8*sizeof(DWORD)));
	// Subtract difference between January 1st, 1970 (C)
	// and January 1st, 1601 (Windows) in 100ns units.
	int64_t time_unix_100ns = time_win_100ns - 116444736000000000ULL;
	*time_s = time_unix_100ns / 10000000;
	*time_n = (time_unix_100ns % 10000000) * 100;
}


DirectoryIterator::DirectoryIterator() : find_handle(INVALID_HANDLE_VALUE) {}


DirectoryIterator::DirectoryIterator(const std::string &path, DirectoryOptions opts) :
	options(opts),
	path(path)
{
	std::wstring dir_spec = winapi_path(path) + L"\\*";
	WIN32_FIND_DATA data;

	// Find the first file in the directory.
	find_handle = FindFirstFile(dir_spec.c_str(), &data);

	if (find_handle == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
			throw FilesystemError("GetDirListing: FindFirstFile error: " +
					gle_to_string(err));
		}
		return;
	}
	// NOTE: Make sure not to include '..' in the results,
	// it will result in an epic failure when deleting stuff.

	populateEntry(data);
	if (entry.name == "." || entry.name == "..")
		++(*this);
}


DirectoryIterator::~DirectoryIterator()
{
	if (find_handle != INVALID_HANDLE_VALUE)
		FindClose(find_handle);
}


bool DirectoryIterator::operator == (const DirectoryIterator &other)
{
	return find_handle == INVALID_HANDLE_VALUE &&
		other.find_handle == INVALID_HANDLE_VALUE;
}


DirectoryIterator& DirectoryIterator::operator ++ ()
{
	WIN32_FIND_DATA data;
again:
	if (!FindNextFile(find_handle, &data)) {
		DWORD err = GetLastError();
		FindClose(find_handle);
		find_handle = INVALID_HANDLE_VALUE;
		if (err == ERROR_NO_MORE_FILES) {
			return *this;
		} else {
			throw FilesystemError("GetDirListing: FindNextFile error: " +
					gle_to_string(err));
		}
	}

	populateEntry(data);
	if (entry.name == "." || entry.name == "..")
		goto again;
	return *this;
}


void DirectoryIterator::populateEntry(const WIN32_FIND_DATA &data)
{
	entry.name = wide_to_narrow(data.cFileName);
	entry.type = w32_find_ftype(data);
	entry.size = (uintmax_t)data.nFileSizeLow |
			((uintmax_t)data.nFileSizeHigh << (8 * sizeof(DWORD)));
	convert_filetime(data.ftCreationTime, &entry.ctime, &entry.ctime_nsec);
	convert_filetime(data.ftLastAccessTime, &entry.atime, &entry.atime_nsec);
	convert_filetime(data.ftLastWriteTime, &entry.mtime, &entry.mtime_nsec);
}


FILE* open(const std::string &path, const char *mode)
{
	std::wstring wpath = winapi_path(path);
	std::wstring wmode = narrow_to_wide(mode);
	return _wfopen(wpath.c_str(), wmode.c_str());
}


bool create_directory(const std::string &path)
{
	std::wstring wpath = winapi_path(path);
	if (CreateDirectoryW(wpath.c_str(), NULL))
		return true;
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return true;
	return false;
}


bool exists(const std::string &path)
{
	std::wstring wpath = winapi_path(path);
	return GetFileAttributesW(wpath.c_str()) != INVALID_FILE_ATTRIBUTES;
}


bool is_absolute(const std::string &path)
{
	std::wstring wpath = winapi_path(path);
	return !PathIsRelativeW(wpath.c_str());
}

bool is_directory(const std::string &path)
{
	std::wstring wpath = winapi_path(path);
	DWORD attr = GetFileAttributesW(wpath.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES) &&
			(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool remove_all(const std::string &path, bool remove_top)
{
	std::wstring wpath = winapi_path(path);
	DWORD attr = GetFileAttributesW(wpath.c_str());
	bool is_directory = (attr != INVALID_FILE_ATTRIBUTES) &&
			(attr & FILE_ATTRIBUTE_DIRECTORY);
	if (is_directory) {
		for (DirectoryIterator it(path); it != DirectoryIterator(); ++it) {
			std::string full_path = path + DIR_DELIM + it->name;
			if (!remove_all(full_path)) {
				errorstream << "fs::remove_all: Failed to recurse to "
						<< full_path << std::endl;
				return false;
			}
		}
		if (remove_top && !RemoveDirectoryW(wpath.c_str())) {
			errorstream << "Failed to recursively delete directory "
					<< path << std::endl;
			return false;
		}
	} else {
		if (remove_top && !DeleteFileW(wpath.c_str())) {
			errorstream << "fs::remove_all: Failed to delete file "
					<< path << std::endl;
			return false;
		}
	}
	return true;
}

bool remove(const std::string &path)
{
	std::wstring wpath = winapi_path(path);
	DWORD attr = GetFileAttributesW(wpath.c_str());
	bool is_directory = (attr != INVALID_FILE_ATTRIBUTES) &&
			(attr & FILE_ATTRIBUTE_DIRECTORY);
	if (is_directory) {
		return RemoveDirectoryW(wpath.c_str());
	} else {
		return DeleteFileW(wpath.c_str());
	}
}

std::string get_temp_path()
{
	DWORD buf_size = GetTempPath(0, NULL);
	if (buf_size == 0) {
		errorstream << "fs::get_temp_path failed: "
			<< gle_to_string(GetLastError()) << std::endl;
		return "";
	}
	std::wstring buf;
	buf.resize(buf_size);
	DWORD len = GetTempPathW(buf_size, &buf[0]);
	if (len == 0 || len > buf_size) {
		errorstream << "fs::get_temp_path failed: "
			<< gle_to_string(GetLastError()) << std::endl;
		return "";
	}
	return wide_to_narrow(buf);
}

bool rename(const std::string &from, const std::string &to)
{
	std::wstring wfrom = winapi_path(from);
	std::wstring wto = winapi_path(to);
	// XXX: Works if file to replace doesn't exist?
	return ReplaceFileW(wfrom.c_str(), wto.c_str(), NULL, 0, NULL, NULL);
	//return MoveFileW(wfrom.c_str(), wto.c_str());
}


#else  // POSIX


DirectoryIterator::DirectoryIterator() : dirp(NULL) {}

DirectoryIterator::DirectoryIterator(const std::string &path, DirectoryOptions opts) :
	options(opts),
	path(path)
{
	if ((dirp = opendir(path.c_str())) != NULL)
		++(*this);
}

DirectoryIterator::~DirectoryIterator()
{
	if (dirp != NULL)
		closedir(dirp);
}

bool DirectoryIterator::operator == (const DirectoryIterator &other)
	{ return dirp == NULL && other.dirp == NULL; }

DirectoryIterator& DirectoryIterator::operator ++ ()
{
	dirent data;
	dirent *result;
again:
	if (readdir_r(dirp, &data, &result) != 0 || result == NULL) {
		closedir(dirp);
		dirp = NULL;
		return *this;
	}

	// NOTE: Make sure not to include '..' in the results,
	// it will result in an epic failure when deleting stuff.
	if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0)
		goto again;

	entry.name = result->d_name;
	entry.type = FT_UNKNOWN;

	/* POSIX doesn't define d_type member of struct dirent and
	 * certain filesystems on glibc/Linux will only return
	 * DT_UNKNOWN for the d_type member.
	 */
#ifdef _DIRENT_HAVE_D_TYPE
	switch (result->d_type) {
	case DT_REG:  entry.type = FT_REGULAR; break;
	case DT_BLK:  entry.type = FT_BLOCK; break;
	case DT_CHR:  entry.type = FT_CHARACTER; break;
	case DT_DIR:  entry.type = FT_DIRECTORY; break;
	case DT_FIFO: entry.type = FT_FIFO; break;
	case DT_LNK:  entry.type = FT_SYMLINK; break;
	case DT_SOCK: entry.type = FT_SOCKET; break;
	}
	// If it's a symlink that we should follow or we
	// don't know what it is, we'll have to use stat.
	if (options & DO_ALLOW_STAT && (entry.type == FT_UNKNOWN ||
			(entry.type == FT_SYMLINK && (options & DO_FOLLOW_SYMLINKS))
			))
		stat();
#else
	if (options & DO_ALLOW_STAT)
		stat();
#endif
	return *this;
}

void DirectoryIterator::stat()
{
	struct stat stat_buf;
	int (*stat_func)(const char*, struct stat*) = ::lstat;
	if (options & DO_FOLLOW_SYMLINKS)
		stat_func = ::stat;
	if (stat_func((path + "/" + entry.name).c_str(), &stat_buf))
		return;

	FileType t = FT_UNKNOWN;
	switch (stat_buf.st_mode & S_IFMT) {
	case S_IFSOCK: t = FT_SOCKET; break;
	case S_IFLNK:  t = FT_SYMLINK; break;
	case S_IFREG:  t = FT_REGULAR; break;
	case S_IFBLK:  t = FT_BLOCK; break;
	case S_IFDIR:  t = FT_DIRECTORY; break;
	case S_IFCHR:  t = FT_CHARACTER; break;
	case S_IFIFO:  t = FT_FIFO; break;
	}
	entry.type = t;

	entry.size = stat_buf.st_size;
	entry.atime = stat_buf.st_atim.tv_sec;
	entry.mtime = stat_buf.st_mtim.tv_sec;
	entry.ctime = stat_buf.st_ctim.tv_sec;
	entry.atime_nsec = stat_buf.st_atim.tv_nsec;
	entry.mtime_nsec = stat_buf.st_mtim.tv_nsec;
	entry.ctime_nsec = stat_buf.st_ctim.tv_nsec;
}


FILE* open(const std::string &path, const char *mode)
{
	return fopen(path.c_str(), mode);
}


bool create_directory(const std::string &path)
{
	if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		return true;
	// If already exists, return true
	if (errno == EEXIST)
		return true;
	return false;
}

bool exists(const std::string &path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

bool is_directory(const std::string &path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf))
		// Error; can't be a directory
		return false;
	return (stat_buf.st_mode & S_IFDIR) == S_IFDIR;
}

bool remove_all(const std::string &path, bool remove_top)
{
	if (is_directory(path)) {
		for (DirectoryIterator it(path); it != DirectoryIterator(); ++it) {
			std::string full_path = path + DIR_DELIM + it->name;
			if (!remove_all(full_path)) {
				errorstream << "fs::remove_all: Failed to recurse to "
						<< full_path << std::endl;
				return false;
			}
		}
	}
	return remove_top || fs::remove(path);
}

bool remove(const std::string &path)
{
	if (is_directory(path))
		return rmdir(path.c_str()) == 0;
	else
		return unlink(path.c_str()) == 0;
}

std::string get_temp_path()
{
	/*
	 * Should the environment variables TMPDIR, TMP and TEMP
	 * and the macro P_tmpdir (if defined by stdio.h) be checked
	 * before falling back on /tmp?

	 * Probably not, because this function is intended to be
	 * compatible with lua's os.tmpname which under the default
	 * configuration hardcodes mkstemp("/tmp/lua_XXXXXX").
	 */
#ifdef __ANDROID__
	return porting::path_cache DIR_DELIM "tmp";
#else
	return DIR_DELIM "tmp";
#endif
}

bool rename(const std::string &from, const std::string &to)
{
	return ::rename(from.c_str(), to.c_str()) == 0;
}

#endif  // ifdef _WIN32 else POSIX

} // namespace fs

