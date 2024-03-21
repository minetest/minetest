// Copyright (C) 2012 Joerg Henrichs
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IReadFile.h"

struct AAssetManager;
struct AAsset;
struct ANativeActivity;

namespace irr
{
namespace io
{

class CAndroidAssetReader : public virtual IReadFile
{
public:
	CAndroidAssetReader(AAssetManager *assetManager, const io::path &filename);

	virtual ~CAndroidAssetReader();

	//! Reads an amount of bytes from the file.
	/** \param buffer Pointer to buffer where read bytes are written to.
	\param sizeToRead Amount of bytes to read from the file.
	\return How many bytes were read. */
	virtual size_t read(void *buffer, size_t sizeToRead);

	//! Changes position in file
	/** \param finalPos Destination position in the file.
	\param relativeMovement If set to true, the position in the file is
	changed relative to current position. Otherwise the position is changed
	from beginning of file.
	\return True if successful, otherwise false. */
	virtual bool seek(long finalPos, bool relativeMovement = false);

	//! Get size of file.
	/** \return Size of the file in bytes. */
	virtual long getSize() const;

	//! Get the current position in the file.
	/** \return Current position in the file in bytes. */
	virtual long getPos() const;

	//! Get name of file.
	/** \return File name as zero terminated character string. */
	virtual const io::path &getFileName() const;

	/** Return true if the file could be opened. */
	bool isOpen() const { return Asset != NULL; }

private:
	//! Android's asset manager
	AAssetManager *AssetManager;

	// An asset, i.e. file
	AAsset *Asset;
	path Filename;
};

} // end namespace io
} // end namespace irr
