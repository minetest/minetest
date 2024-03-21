// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CAndroidAssetReader.h"

#include "CReadFile.h"
#include "coreutil.h"
#include "CAndroidAssetReader.h"
#include "CIrrDeviceAndroid.h"

#include <android_native_app_glue.h>
#include <android/native_activity.h>

namespace irr
{
namespace io
{

CAndroidAssetReader::CAndroidAssetReader(AAssetManager *assetManager, const io::path &filename) :
		AssetManager(assetManager), Filename(filename)
{
	Asset = AAssetManager_open(AssetManager,
			core::stringc(filename).c_str(),
			AASSET_MODE_RANDOM);
}

CAndroidAssetReader::~CAndroidAssetReader()
{
	if (Asset)
		AAsset_close(Asset);
}

size_t CAndroidAssetReader::read(void *buffer, size_t sizeToRead)
{
	int readBytes = AAsset_read(Asset, buffer, sizeToRead);
	if (readBytes >= 0)
		return size_t(readBytes);
	return 0; // direct fd access is not possible (for example, if the asset is compressed).
}

bool CAndroidAssetReader::seek(long finalPos, bool relativeMovement)
{
	off_t status = AAsset_seek(Asset, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET);

	return status + 1;
}

long CAndroidAssetReader::getSize() const
{
	return AAsset_getLength(Asset);
}

long CAndroidAssetReader::getPos() const
{
	return AAsset_getLength(Asset) - AAsset_getRemainingLength(Asset);
}

const io::path &CAndroidAssetReader::getFileName() const
{
	return Filename;
}

} // end namespace io
} // end namespace irr
