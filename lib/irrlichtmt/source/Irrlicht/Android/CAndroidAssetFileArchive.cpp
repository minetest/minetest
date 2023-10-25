// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h


#ifdef   _IRR_COMPILE_ANDROID_ASSET_READER_

#include "CAndroidAssetReader.h"

#include "CReadFile.h"
#include "coreutil.h"
#include "CAndroidAssetFileArchive.h"
#include "CIrrDeviceAndroid.h"
#include "os.h"	// for logging (just keep it in even when not needed right now as it's used all the time)

#include <android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/log.h>

namespace irr
{
namespace io
{

CAndroidAssetFileArchive::CAndroidAssetFileArchive(AAssetManager *assetManager, bool ignoreCase, bool ignorePaths)
  : CFileList("/asset", ignoreCase, ignorePaths), AssetManager(assetManager)
{
}


CAndroidAssetFileArchive::~CAndroidAssetFileArchive()
{
}


//! get the archive type
E_FILE_ARCHIVE_TYPE CAndroidAssetFileArchive::getType() const
{
	return EFAT_ANDROID_ASSET;
}

const IFileList* CAndroidAssetFileArchive::getFileList() const
{
    // The assert_manager can not read directory names, so
    // getFileList returns only files in folders which have been added.
    return this;
}


//! opens a file by file name
IReadFile* CAndroidAssetFileArchive::createAndOpenFile(const io::path& filename)
{
    CAndroidAssetReader *reader = new CAndroidAssetReader(AssetManager, filename);

    if(reader->isOpen())
		return reader;

	reader->drop();
    return NULL;
}

//! opens a file by index
IReadFile* CAndroidAssetFileArchive::createAndOpenFile(u32 index)
{
	const io::path& filename(getFullFileName(index));
	if ( filename.empty() )
		return 0;

    return createAndOpenFile(filename);
}

void CAndroidAssetFileArchive::addDirectoryToFileList(const io::path &dirname_)
{
	io::path dirname(dirname_);
	fschar_t lastChar = dirname.lastChar();
	if ( lastChar == '/' || lastChar == '\\' )
		dirname.erase(dirname.size()-1);

	// os::Printer::log("addDirectoryToFileList:", dirname.c_str(), ELL_DEBUG);
	if  (findFile(dirname, true) >= 0 )
		return;	// was already added

	AAssetDir *dir = AAssetManager_openDir(AssetManager, core::stringc(dirname).c_str());
	if(!dir)
		return;

	// add directory itself
	addItem(dirname, 0, 0, /*isDir*/true, getFileCount());

	// add all files in folder.
	// Note: AAssetDir_getNextFileName does not return directory names (neither does any other NDK function)
	while(const char *filename = AAssetDir_getNextFileName(dir))
	{
		core::stringc full_filename= dirname=="" ? filename
                                             : dirname+"/"+filename;

		// We can't get the size without opening the file - so for performance
		// reasons we set the file size to 0.
		// TODO: Does this really cost so much performance that it's worth losing this information? Dirs are usually just added once at startup...
		addItem(full_filename, /*offet*/0, /*size*/0, /*isDir*/false, getFileCount());
		// os::Printer::log("addItem:", full_filename.c_str(), ELL_DEBUG);
	}
	AAssetDir_close(dir);
}

} // end namespace io
} // end namespace irr

#endif //  _IRR_COMPILE_ANDROID_ASSET_READER_
