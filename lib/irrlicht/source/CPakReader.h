// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_PAK_READER_H_INCLUDED__
#define __C_PAK_READER_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef __IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_

#include "IReferenceCounted.h"
#include "IReadFile.h"
#include "irrArray.h"
#include "irrString.h"
#include "IFileSystem.h"
#include "CFileList.h"

namespace irr
{
namespace io
{
	//! File header containing location and size of the table of contents
	struct SPAKFileHeader
	{
		// Don't change the order of these fields!  They must match the order stored on disk.
		c8 tag[4];
		u32 offset;
		u32 length;
	};

	//! An entry in the PAK file's table of contents.
	struct SPAKFileEntry
	{
		// Don't change the order of these fields!  They must match the order stored on disk.
		c8 name[56];
		u32 offset;
		u32 length;
	};

	//! Archiveloader capable of loading PAK Archives
	class CArchiveLoaderPAK : public IArchiveLoader
	{
	public:

		//! Constructor
		CArchiveLoaderPAK(io::IFileSystem* fs);

		//! returns true if the file maybe is able to be loaded by this class
		//! based on the file extension (e.g. ".zip")
		virtual bool isALoadableFileFormat(const io::path& filename) const;

		//! Check if the file might be loaded by this class
		/** Check might look into the file.
		\param file File handle to check.
		\return True if file seems to be loadable. */
		virtual bool isALoadableFileFormat(io::IReadFile* file) const;

		//! Check to see if the loader can create archives of this type.
		/** Check based on the archive type.
		\param fileType The archive type to check.
		\return True if the archile loader supports this type, false if not */
		virtual bool isALoadableFileFormat(E_FILE_ARCHIVE_TYPE fileType) const;

		//! Creates an archive from the filename
		/** \param file File handle to check.
		\return Pointer to newly created archive, or 0 upon error. */
		virtual IFileArchive* createArchive(const io::path& filename, bool ignoreCase, bool ignorePaths) const;

		//! creates/loads an archive from the file.
		//! \return Pointer to the created archive. Returns 0 if loading failed.
		virtual io::IFileArchive* createArchive(io::IReadFile* file, bool ignoreCase, bool ignorePaths) const;

		//! Returns the type of archive created by this loader
		virtual E_FILE_ARCHIVE_TYPE getType() const { return EFAT_PAK; }

	private:
		io::IFileSystem* FileSystem;
	};


	//! reads from pak
	class CPakReader : public virtual IFileArchive, virtual CFileList
	{
	public:

		CPakReader(IReadFile* file, bool ignoreCase, bool ignorePaths);
		virtual ~CPakReader();

		// file archive methods

		//! return the id of the file Archive
		virtual const io::path& getArchiveName() const
		{
			return File->getFileName();
		}

		//! opens a file by file name
		virtual IReadFile* createAndOpenFile(const io::path& filename);

		//! opens a file by index
		virtual IReadFile* createAndOpenFile(u32 index);

		//! returns the list of files
		virtual const IFileList* getFileList() const;

		//! get the class Type
		virtual E_FILE_ARCHIVE_TYPE getType() const { return EFAT_PAK; }

	private:

		//! scans for a local header, returns false if the header is invalid
		bool scanLocalHeader();

		IReadFile* File;

	};

} // end namespace io
} // end namespace irr

#endif // __IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_

#endif // __C_PAK_READER_H_INCLUDED__

