// Copyright (C) 2012 Joerg Henrichs
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#ifdef  _IRR_COMPILE_ANDROID_ASSET_READER_


#include "IReadFile.h"
#include "IFileArchive.h"
#include "CFileList.h"

#include <android/native_activity.h>

namespace irr
{
namespace io
{

/*!
	Android asset file system written August 2012 by J.Henrichs (later reworked by others).
*/
	class CAndroidAssetFileArchive : public virtual IFileArchive,
                                          virtual CFileList
	{
    public:

		//! constructor
		CAndroidAssetFileArchive(AAssetManager *assetManager, bool ignoreCase, bool ignorePaths);

		//! destructor
		virtual ~CAndroidAssetFileArchive();

		//! opens a file by file name
		virtual IReadFile* createAndOpenFile(const io::path& filename);

		//! opens a file by index
		virtual IReadFile* createAndOpenFile(u32 index);

		//! returns the list of files
		virtual const IFileList* getFileList() const;

		//! get the archive type
		virtual E_FILE_ARCHIVE_TYPE getType() const;

		//! Add a directory to read files from. Since the Android
		//! API does not return names of directories, they need to
		//! be added manually.
		virtual void addDirectoryToFileList(const io::path &filename);

		//! return the name (id) of the file Archive
		const io::path& getArchiveName() const override {return Path;}

	protected:
		//! Android's asset manager
		AAssetManager *AssetManager;

    };   // CAndroidAssetFileArchive

} // end namespace io
} // end namespace irr

#endif //   _IRR_COMPILE_ANDROID_ASSET_READER_
