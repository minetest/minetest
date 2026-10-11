// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IImageLoader.h"

#include <cstdio>    // IWYU pragma: keep (required for jpeglib.h)
#include <jpeglib.h> // use system lib

namespace video
{

//! Surface Loader for JPG images
class CImageLoaderJPG : public IImageLoader
{
public:
	//! constructor
	CImageLoaderJPG();

	//! destructor
	virtual ~CImageLoaderJPG();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".tga")
	bool isALoadableFileExtension(const io::path &filename) const override;

	//! returns true if the file maybe is able to be loaded by this class
	bool isALoadableFileFormat(io::IReadFile *file) const override;

	//! creates a surface from the file
	IImage *loadImage(io::IReadFile *file) const override;

private:
	// several methods used via function pointers by jpeglib

	/* Receives control for a fatal error. Information sufficient to
	generate the error message has been stored in cinfo->err; call
	output_message to display it. Control must NOT return to the caller;
	generally this routine will exit() or longjmp() somewhere.
	Typically you would override this routine to get rid of the exit()
	default behavior. Note that if you continue processing, you should
	clean up the JPEG object with jpeg_abort() or jpeg_destroy().
	*/
	static void error_exit(j_common_ptr cinfo);

	/* output error messages via Irrlicht logger. */
	static void output_message(j_common_ptr cinfo);
};

} // end namespace video
