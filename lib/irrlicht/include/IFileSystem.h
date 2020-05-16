// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_FILE_SYSTEM_H_INCLUDED__
#define __I_FILE_SYSTEM_H_INCLUDED__

#include "IReferenceCounted.h"
#include "IXMLReader.h"
#include "IFileArchive.h"

namespace irr
{
namespace video
{
	class IVideoDriver;
} // end namespace video
namespace io
{

class IReadFile;
class IWriteFile;
class IFileList;
class IXMLWriter;
class IAttributes;


//! The FileSystem manages files and archives and provides access to them.
/** It manages where files are, so that modules which use the the IO do not
need to know where every file is located. A file could be in a .zip-Archive or
as file on disk, using the IFileSystem makes no difference to this. */
class IFileSystem : public virtual IReferenceCounted
{
public:

	//! Opens a file for read access.
	/** \param filename: Name of file to open.
	\return Pointer to the created file interface.
	The returned pointer should be dropped when no longer needed.
	See IReferenceCounted::drop() for more information. */
	virtual IReadFile* createAndOpenFile(const path& filename) =0;

	//! Creates an IReadFile interface for accessing memory like a file.
	/** This allows you to use a pointer to memory where an IReadFile is requested.
	\param memory: A pointer to the start of the file in memory
	\param len: The length of the memory in bytes
	\param fileName: The name given to this file
	\param deleteMemoryWhenDropped: True if the memory should be deleted
	along with the IReadFile when it is dropped.
	\return Pointer to the created file interface.
	The returned pointer should be dropped when no longer needed.
	See IReferenceCounted::drop() for more information.
	*/
	virtual IReadFile* createMemoryReadFile(void* memory, s32 len, const path& fileName, bool deleteMemoryWhenDropped=false) =0;

	//! Creates an IReadFile interface for accessing files inside files.
	/** This is useful e.g. for archives.
	\param fileName: The name given to this file
	\param alreadyOpenedFile: Pointer to the enclosing file
	\param pos: Start of the file inside alreadyOpenedFile
	\param areaSize: The length of the file
	\return A pointer to the created file interface.
	The returned pointer should be dropped when no longer needed.
	See IReferenceCounted::drop() for more information.
	*/
	virtual IReadFile* createLimitReadFile(const path& fileName,
			IReadFile* alreadyOpenedFile, long pos, long areaSize) =0;

	//! Creates an IWriteFile interface for accessing memory like a file.
	/** This allows you to use a pointer to memory where an IWriteFile is requested.
		You are responsible for allocating enough memory.
	\param memory: A pointer to the start of the file in memory (allocated by you)
	\param len: The length of the memory in bytes
	\param fileName: The name given to this file
	\param deleteMemoryWhenDropped: True if the memory should be deleted
	along with the IWriteFile when it is dropped.
	\return Pointer to the created file interface.
	The returned pointer should be dropped when no longer needed.
	See IReferenceCounted::drop() for more information.
	*/
	virtual IWriteFile* createMemoryWriteFile(void* memory, s32 len, const path& fileName, bool deleteMemoryWhenDropped=false) =0;


	//! Opens a file for write access.
	/** \param filename: Name of file to open.
	\param append: If the file already exist, all write operations are
	appended to the file.
	\return Pointer to the created file interface. 0 is returned, if the
	file could not created or opened for writing.
	The returned pointer should be dropped when no longer needed.
	See IReferenceCounted::drop() for more information. */
	virtual IWriteFile* createAndWriteFile(const path& filename, bool append=false) =0;

	//! Adds an archive to the file system.
	/** After calling this, the Irrlicht Engine will also search and open
	files directly from this archive. This is useful for hiding data from
	the end user, speeding up file access and making it possible to access
	for example Quake3 .pk3 files, which are just renamed .zip files. By
	default Irrlicht supports ZIP, PAK, TAR, PNK, and directories as
	archives. You can provide your own archive types by implementing
	IArchiveLoader and passing an instance to addArchiveLoader.
	Irrlicht supports AES-encrypted zip files, and the advanced compression
	techniques lzma and bzip2.
	\param filename: Filename of the archive to add to the file system.
	\param ignoreCase: If set to true, files in the archive can be accessed without
	writing all letters in the right case.
	\param ignorePaths: If set to true, files in the added archive can be accessed
	without its complete path.
	\param archiveType: If no specific E_FILE_ARCHIVE_TYPE is selected then
	the type of archive will depend on the extension of the file name. If
	you use a different extension then you can use this parameter to force
	a specific type of archive.
	\param password An optional password, which is used in case of encrypted archives.
	\param retArchive A pointer that will be set to the archive that is added.
	\return True if the archive was added successfully, false if not. */
	virtual bool addFileArchive(const path& filename, bool ignoreCase=true,
			bool ignorePaths=true,
			E_FILE_ARCHIVE_TYPE archiveType=EFAT_UNKNOWN,
			const core::stringc& password="",
			IFileArchive** retArchive=0) =0;

	//! Adds an archive to the file system.
	/** After calling this, the Irrlicht Engine will also search and open
	files directly from this archive. This is useful for hiding data from
	the end user, speeding up file access and making it possible to access
	for example Quake3 .pk3 files, which are just renamed .zip files. By
	default Irrlicht supports ZIP, PAK, TAR, PNK, and directories as
	archives. You can provide your own archive types by implementing
	IArchiveLoader and passing an instance to addArchiveLoader.
	Irrlicht supports AES-encrypted zip files, and the advanced compression
	techniques lzma and bzip2.
	If you want to add a directory as an archive, prefix its name with a
	slash in order to let Irrlicht recognize it as a folder mount (mypath/).
	Using this technique one can build up a search order, because archives
	are read first, and can be used more easily with relative filenames.
	\param file: Archive to add to the file system.
	\param ignoreCase: If set to true, files in the archive can be accessed without
	writing all letters in the right case.
	\param ignorePaths: If set to true, files in the added archive can be accessed
	without its complete path.
	\param archiveType: If no specific E_FILE_ARCHIVE_TYPE is selected then
	the type of archive will depend on the extension of the file name. If
	you use a different extension then you can use this parameter to force
	a specific type of archive.
	\param password An optional password, which is used in case of encrypted archives.
	\param retArchive A pointer that will be set to the archive that is added.
	\return True if the archive was added successfully, false if not. */
	virtual bool addFileArchive(IReadFile* file, bool ignoreCase=true,
			bool ignorePaths=true,
			E_FILE_ARCHIVE_TYPE archiveType=EFAT_UNKNOWN,
			const core::stringc& password="",
			IFileArchive** retArchive=0) =0;

	//! Adds an archive to the file system.
	/** \param archive: The archive to add to the file system.
	\return True if the archive was added successfully, false if not. */
	virtual bool addFileArchive(IFileArchive* archive) =0;

	//! Get the number of archives currently attached to the file system
	virtual u32 getFileArchiveCount() const =0;

	//! Removes an archive from the file system.
	/** This will close the archive and free any file handles, but will not
	close resources which have already been loaded and are now cached, for
	example textures and meshes.
	\param index: The index of the archive to remove
	\return True on success, false on failure */
	virtual bool removeFileArchive(u32 index) =0;

	//! Removes an archive from the file system.
	/** This will close the archive and free any file handles, but will not
	close resources which have already been loaded and are now cached, for
	example textures and meshes. Note that a relative filename might be
	interpreted differently on each call, depending on the current working
	directory. In case you want to remove an archive that was added using
	a relative path name, you have to change to the same working directory
	again. This means, that the filename given on creation is not an
	identifier for the archive, but just a usual filename that is used for
	locating the archive to work with.
	\param filename The archive pointed to by the name will be removed
	\return True on success, false on failure */
	virtual bool removeFileArchive(const path& filename) =0;

	//! Removes an archive from the file system.
	/** This will close the archive and free any file handles, but will not
	close resources which have already been loaded and are now cached, for
	example textures and meshes.
	\param archive The archive to remove.
	\return True on success, false on failure */
	virtual bool removeFileArchive(const IFileArchive* archive) =0;

	//! Changes the search order of attached archives.
	/**
	\param sourceIndex: The index of the archive to change
	\param relative: The relative change in position, archives with a lower index are searched first */
	virtual bool moveFileArchive(u32 sourceIndex, s32 relative) =0;

	//! Get the archive at a given index.
	virtual IFileArchive* getFileArchive(u32 index) =0;

	//! Adds an external archive loader to the engine.
	/** Use this function to add support for new archive types to the
	engine, for example proprietary or encrypted file storage. */
	virtual void addArchiveLoader(IArchiveLoader* loader) =0;

	//! Gets the number of archive loaders currently added
	virtual u32 getArchiveLoaderCount() const = 0;

	//! Retrieve the given archive loader
	/** \param index The index of the loader to retrieve. This parameter is an 0-based
	array index.
	\return A pointer to the specified loader, 0 if the index is incorrect. */
	virtual IArchiveLoader* getArchiveLoader(u32 index) const = 0;

	//! Adds a zip archive to the file system.
	/** \deprecated This function is provided for compatibility
	with older versions of Irrlicht and may be removed in Irrlicht 1.9,
	you should use addFileArchive instead.
	After calling this, the Irrlicht Engine will search and open files directly from this archive too.
	This is useful for hiding data from the end user, speeding up file access and making it possible to
	access for example Quake3 .pk3 files, which are no different than .zip files.
	\param filename: Filename of the zip archive to add to the file system.
	\param ignoreCase: If set to true, files in the archive can be accessed without
	writing all letters in the right case.
	\param ignorePaths: If set to true, files in the added archive can be accessed
	without its complete path.
	\return True if the archive was added successfully, false if not. */
	_IRR_DEPRECATED_ virtual bool addZipFileArchive(const c8* filename, bool ignoreCase=true, bool ignorePaths=true)
	{
		return addFileArchive(filename, ignoreCase, ignorePaths, EFAT_ZIP);
	}

	//! Adds an unzipped archive (or basedirectory with subdirectories..) to the file system.
	/** \deprecated This function is provided for compatibility
	with older versions of Irrlicht and may be removed in Irrlicht 1.9,
	you should use addFileArchive instead.
	Useful for handling data which will be in a zip file
	\param filename: Filename of the unzipped zip archive base directory to add to the file system.
	\param ignoreCase: If set to true, files in the archive can be accessed without
	writing all letters in the right case.
	\param ignorePaths: If set to true, files in the added archive can be accessed
	without its complete path.
	\return True if the archive was added successful, false if not. */
	_IRR_DEPRECATED_ virtual bool addFolderFileArchive(const c8* filename, bool ignoreCase=true, bool ignorePaths=true)
	{
		return addFileArchive(filename, ignoreCase, ignorePaths, EFAT_FOLDER);
	}

	//! Adds a pak archive to the file system.
	/** \deprecated This function is provided for compatibility
	with older versions of Irrlicht and may be removed in Irrlicht 1.9,
	you should use addFileArchive instead.
	After calling this, the Irrlicht Engine will search and open files directly from this archive too.
	This is useful for hiding data from the end user, speeding up file access and making it possible to
	access for example Quake2/KingPin/Hexen2 .pak files
	\param filename: Filename of the pak archive to add to the file system.
	\param ignoreCase: If set to true, files in the archive can be accessed without
	writing all letters in the right case.
	\param ignorePaths: If set to true, files in the added archive can be accessed
	without its complete path.(should not use with Quake2 paks
	\return True if the archive was added successful, false if not. */
	_IRR_DEPRECATED_ virtual bool addPakFileArchive(const c8* filename, bool ignoreCase=true, bool ignorePaths=true)
	{
		return addFileArchive(filename, ignoreCase, ignorePaths, EFAT_PAK);
	}

	//! Get the current working directory.
	/** \return Current working directory as a string. */
	virtual const path& getWorkingDirectory() =0;

	//! Changes the current working directory.
	/** \param newDirectory: A string specifying the new working directory.
	The string is operating system dependent. Under Windows it has
	the form "<drive>:\<directory>\<sudirectory>\<..>". An example would be: "C:\Windows\"
	\return True if successful, otherwise false. */
	virtual bool changeWorkingDirectoryTo(const path& newDirectory) =0;

	//! Converts a relative path to an absolute (unique) path, resolving symbolic links if required
	/** \param filename Possibly relative file or directory name to query.
	\result Absolute filename which points to the same file. */
	virtual path getAbsolutePath(const path& filename) const =0;

	//! Get the directory a file is located in.
	/** \param filename: The file to get the directory from.
	\return String containing the directory of the file. */
	virtual path getFileDir(const path& filename) const =0;

	//! Get the base part of a filename, i.e. the name without the directory part.
	/** If no directory is prefixed, the full name is returned.
	\param filename: The file to get the basename from
	\param keepExtension True if filename with extension is returned otherwise everything
	after the final '.' is removed as well. */
	virtual path getFileBasename(const path& filename, bool keepExtension=true) const =0;

	//! flatten a path and file name for example: "/you/me/../." becomes "/you"
	virtual path& flattenFilename(path& directory, const path& root="/") const =0;

	//! Get the relative filename, relative to the given directory
	virtual path getRelativeFilename(const path& filename, const path& directory) const =0;

	//! Creates a list of files and directories in the current working directory and returns it.
	/** \return a Pointer to the created IFileList is returned. After the list has been used
	it has to be deleted using its IFileList::drop() method.
	See IReferenceCounted::drop() for more information. */
	virtual IFileList* createFileList() =0;

	//! Creates an empty filelist
	/** \return a Pointer to the created IFileList is returned. After the list has been used
	it has to be deleted using its IFileList::drop() method.
	See IReferenceCounted::drop() for more information. */
	virtual IFileList* createEmptyFileList(const io::path& path, bool ignoreCase, bool ignorePaths) =0;

	//! Set the active type of file system.
	virtual EFileSystemType setFileListSystem(EFileSystemType listType) =0;

	//! Determines if a file exists and could be opened.
	/** \param filename is the string identifying the file which should be tested for existence.
	\return True if file exists, and false if it does not exist or an error occured. */
	virtual bool existFile(const path& filename) const =0;

	//! Creates a XML Reader from a file which returns all parsed strings as wide characters (wchar_t*).
	/** Use createXMLReaderUTF8() if you prefer char* instead of wchar_t*. See IIrrXMLReader for
	more information on how to use the parser.
	\return 0, if file could not be opened, otherwise a pointer to the created
	IXMLReader is returned. After use, the reader
	has to be deleted using its IXMLReader::drop() method.
	See IReferenceCounted::drop() for more information. */
	virtual IXMLReader* createXMLReader(const path& filename) =0;

	//! Creates a XML Reader from a file which returns all parsed strings as wide characters (wchar_t*).
	/** Use createXMLReaderUTF8() if you prefer char* instead of wchar_t*. See IIrrXMLReader for
	more information on how to use the parser.
	\return 0, if file could not be opened, otherwise a pointer to the created
	IXMLReader is returned. After use, the reader
	has to be deleted using its IXMLReader::drop() method.
	See IReferenceCounted::drop() for more information. */
	virtual IXMLReader* createXMLReader(IReadFile* file) =0;

	//! Creates a XML Reader from a file which returns all parsed strings as ASCII/UTF-8 characters (char*).
	/** Use createXMLReader() if you prefer wchar_t* instead of char*. See IIrrXMLReader for
	more information on how to use the parser.
	\return 0, if file could not be opened, otherwise a pointer to the created
	IXMLReader is returned. After use, the reader
	has to be deleted using its IXMLReaderUTF8::drop() method.
	See IReferenceCounted::drop() for more information. */
	virtual IXMLReaderUTF8* createXMLReaderUTF8(const path& filename) =0;

	//! Creates a XML Reader from a file which returns all parsed strings as ASCII/UTF-8 characters (char*).
	/** Use createXMLReader() if you prefer wchar_t* instead of char*. See IIrrXMLReader for
	more information on how to use the parser.
	\return 0, if file could not be opened, otherwise a pointer to the created
	IXMLReader is returned. After use, the reader
	has to be deleted using its IXMLReaderUTF8::drop() method.
	See IReferenceCounted::drop() for more information. */
	virtual IXMLReaderUTF8* createXMLReaderUTF8(IReadFile* file) =0;

	//! Creates a XML Writer from a file.
	/** \return 0, if file could not be opened, otherwise a pointer to the created
	IXMLWriter is returned. After use, the reader
	has to be deleted using its IXMLWriter::drop() method.
	See IReferenceCounted::drop() for more information. */
	virtual IXMLWriter* createXMLWriter(const path& filename) =0;

	//! Creates a XML Writer from a file.
	/** \return 0, if file could not be opened, otherwise a pointer to the created
	IXMLWriter is returned. After use, the reader
	has to be deleted using its IXMLWriter::drop() method.
	See IReferenceCounted::drop() for more information. */
	virtual IXMLWriter* createXMLWriter(IWriteFile* file) =0;

	//! Creates a new empty collection of attributes, usable for serialization and more.
	/** \param driver: Video driver to be used to load textures when specified as attribute values.
	Can be null to prevent automatic texture loading by attributes.
	\return Pointer to the created object.
	If you no longer need the object, you should call IAttributes::drop().
	See IReferenceCounted::drop() for more information. */
	virtual IAttributes* createEmptyAttributes(video::IVideoDriver* driver=0) =0;
};


} // end namespace io
} // end namespace irr

#endif

