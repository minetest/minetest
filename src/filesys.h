#ifndef FILESYS_HEADER
#define FILESYS_HEADER

#include <string>
#include <vector>
#include "exceptions.h"

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

}//fs

#endif

