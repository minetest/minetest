// Copyright (C) 2009-2012 Gaz Davidson
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CTarReader.h"

#ifdef __IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_

#include "CFileList.h"
#include "CLimitReadFile.h"
#include "os.h"
#include "coreutil.h"
#if !defined(_IRR_WINDOWS_CE_PLATFORM_)
#include "errno.h"
#endif

namespace irr
{
namespace io
{

//! Constructor
CArchiveLoaderTAR::CArchiveLoaderTAR(io::IFileSystem* fs)
: FileSystem(fs)
{
	#ifdef _DEBUG
	setDebugName("CArchiveLoaderTAR");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
bool CArchiveLoaderTAR::isALoadableFileFormat(const io::path& filename) const
{
	return core::hasFileExtension(filename, "tar");
}

//! Check to see if the loader can create archives of this type.
bool CArchiveLoaderTAR::isALoadableFileFormat(E_FILE_ARCHIVE_TYPE fileType) const
{
	return fileType == EFAT_TAR;
}

//! Creates an archive from the filename
/** \param file File handle to check.
\return Pointer to newly created archive, or 0 upon error. */
IFileArchive* CArchiveLoaderTAR::createArchive(const io::path& filename, bool ignoreCase, bool ignorePaths) const
{
	IFileArchive *archive = 0;
	io::IReadFile* file = FileSystem->createAndOpenFile(filename);

	if (file)
	{
		archive = createArchive(file, ignoreCase, ignorePaths);
		file->drop();
	}

	return archive;
}


//! creates/loads an archive from the file.
//! \return Pointer to the created archive. Returns 0 if loading failed.
IFileArchive* CArchiveLoaderTAR::createArchive(io::IReadFile* file, bool ignoreCase, bool ignorePaths) const
{
	IFileArchive *archive = 0;
	if (file)
	{
		file->seek(0);
		archive = new CTarReader(file, ignoreCase, ignorePaths);
	}
	return archive;
}

//! Check if the file might be loaded by this class
/** Check might look into the file.
\param file File handle to check.
\return True if file seems to be loadable. */
bool CArchiveLoaderTAR::isALoadableFileFormat(io::IReadFile* file) const
{
	// TAR files consist of blocks of 512 bytes
	// if it isn't a multiple of 512 then it's not a TAR file.
	if (file->getSize() % 512)
		return false;

	file->seek(0);

	// read header of first file
	STarHeader fHead;
	file->read(&fHead, sizeof(STarHeader));

	u32 checksum = 0;
	sscanf(fHead.Checksum, "%o", &checksum);

	// verify checksum

	// some old TAR writers assume that chars are signed, others assume unsigned
	// USTAR archives have a longer header, old TAR archives end after linkname

	u32 checksum1=0;
	s32 checksum2=0;

	// remember to blank the checksum field!
	memset(fHead.Checksum, ' ', 8);

	// old header
	for (u8* p = (u8*)(&fHead); p < (u8*)(&fHead.Magic[0]); ++p)
	{
		checksum1 += *p;
		checksum2 += c8(*p);
	}

	if (!strncmp(fHead.Magic, "ustar", 5))
	{
		for (u8* p = (u8*)(&fHead.Magic[0]); p < (u8*)(&fHead) + sizeof(fHead); ++p)
		{
			checksum1 += *p;
			checksum2 += c8(*p);
		}
	}
	return checksum1 == checksum || checksum2 == (s32)checksum;
}

/*
	TAR Archive
*/
CTarReader::CTarReader(IReadFile* file, bool ignoreCase, bool ignorePaths)
 : CFileList((file ? file->getFileName() : io::path("")), ignoreCase, ignorePaths), File(file)
{
	#ifdef _DEBUG
	setDebugName("CTarReader");
	#endif

	if (File)
	{
		File->grab();

		// fill the file list
		populateFileList();

		sort();
	}
}


CTarReader::~CTarReader()
{
	if (File)
		File->drop();
}

const IFileList* CTarReader::getFileList() const
{
	return this;
}


u32 CTarReader::populateFileList()
{
	STarHeader fHead;
	Files.clear();

	u32 pos = 0;
	while ( s32(pos + sizeof(STarHeader)) < File->getSize())
	{
		// seek to next file header
		File->seek(pos);

		// read the header
		File->read(&fHead, sizeof(fHead));

		// only add standard files for now
		if (fHead.Link == ETLI_REGULAR_FILE || ETLI_REGULAR_FILE_OLD)
		{
			io::path fullPath = "";
			fullPath.reserve(255);

			// USTAR archives have a filename prefix
			// may not be null terminated, copy carefully!
			if (!strncmp(fHead.Magic, "ustar", 5))
			{
				c8* np = fHead.FileNamePrefix;
				while(*np && (np - fHead.FileNamePrefix) < 155)
					fullPath.append(*np);
				np++;
			}

			// append the file name
			c8* np = fHead.FileName;
			while(*np && (np - fHead.FileName) < 100)
			{
				fullPath.append(*np);
				np++;
			}

			// get size
			core::stringc sSize = "";
			sSize.reserve(12);
			np = fHead.Size;
			while(*np && (np - fHead.Size) < 12)
			{
				sSize.append(*np);
				np++;
			}

			u32 size = strtoul(sSize.c_str(), NULL, 8);
#if !defined(_IRR_WINDOWS_CE_PLATFORM_)
			if (errno == ERANGE)
				os::Printer::log("File too large", fullPath, ELL_WARNING);
#endif

			// save start position
			u32 offset = pos + 512;

			// move to next file header block
			pos = offset + (size / 512) * 512 + ((size % 512) ? 512 : 0);

			// add file to list
			addItem(fullPath, offset, size, false );
		}
		else
		{
			// todo: ETLI_DIRECTORY, ETLI_LINK_TO_ARCHIVED_FILE

			// move to next block
			pos += 512;
		}

	}

	return Files.size();
}

//! opens a file by file name
IReadFile* CTarReader::createAndOpenFile(const io::path& filename)
{
	const s32 index = findFile(filename, false);

	if (index != -1)
		return createAndOpenFile(index);

	return 0;
}

//! opens a file by index
IReadFile* CTarReader::createAndOpenFile(u32 index)
{
	if (index >= Files.size() )
		return 0;

	const SFileListEntry &entry = Files[index];
	return createLimitReadFile( entry.FullName, File, entry.Offset, entry.Size );
}

} // end namespace io
} // end namespace irr

#endif // __IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_
