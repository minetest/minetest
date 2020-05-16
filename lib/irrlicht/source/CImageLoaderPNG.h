// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// this file was created by rt (www.tomkorp.com), based on ttk's png-reader
// i wanted to be able to read in PNG images with irrlicht :)
// why?  lossless compression with 8-bit alpha channel!

#ifndef __C_IMAGE_LOADER_PNG_H_INCLUDED__
#define __C_IMAGE_LOADER_PNG_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_PNG_LOADER_

#include "IImageLoader.h"

namespace irr
{
namespace video
{

//!  Surface Loader for PNG files
class CImageLoaderPng : public IImageLoader
{
public:

   //! returns true if the file maybe is able to be loaded by this class
   //! based on the file extension (e.g. ".png")
   virtual bool isALoadableFileExtension(const io::path& filename) const;

   //! returns true if the file maybe is able to be loaded by this class
   virtual bool isALoadableFileFormat(io::IReadFile* file) const;

   //! creates a surface from the file
   virtual IImage* loadImage(io::IReadFile* file) const;
};


} // end namespace video
} // end namespace irr

#endif
#endif

