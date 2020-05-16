// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_XML_READER_H_INCLUDED__
#define __C_XML_READER_H_INCLUDED__

#include "IXMLReader.h"

namespace irr
{
namespace io
{
	class IReadFile;

	//! creates an IXMLReader
	IXMLReader* createIXMLReader(IReadFile* file);

	//! creates an IXMLReader
	IXMLReaderUTF8* createIXMLReaderUTF8(IReadFile* file);

} // end namespace irr
} // end namespace io

#endif

