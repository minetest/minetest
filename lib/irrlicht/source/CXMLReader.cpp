// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CXMLReaderImpl.h"
#include "CXMLReader.h"
#include "IReadFile.h"

namespace irr
{
namespace io
{
	//! Irrlicht implementation of the file read callback for the xml parser
	class CIrrXMLFileReadCallBack : public IFileReadCallBack
	{
	public:

		//! construct from FILE pointer
		CIrrXMLFileReadCallBack(IReadFile* file)
			: ReadFile(file)
		{
			ReadFile->grab();
		}

		//! destructor
		virtual ~CIrrXMLFileReadCallBack()
		{
			ReadFile->drop();
		}

		//! Reads an amount of bytes from the file.
		virtual int read(void* buffer, int sizeToRead)
		{
			return ReadFile->read(buffer, sizeToRead);
		}

		//! Returns size of file in bytes
		virtual long getSize() const
		{
			return ReadFile->getSize();
		}

	private:

		IReadFile* ReadFile;
	}; // end class CMyXMLFileReadCallBack


	// now create an implementation for IXMLReader using irrXML.

	//! Creates an instance of a wide character xml parser. 
	IXMLReader* createIXMLReader(IReadFile* file)
	{
		if (!file)
			return 0;

		return new CXMLReaderImpl<wchar_t, IReferenceCounted>(new CIrrXMLFileReadCallBack(file)); 
	}

	//! Creates an instance of an UFT-8 or ASCII character xml parser. 
	IXMLReaderUTF8* createIXMLReaderUTF8(IReadFile* file)
	{
		if (!file)
			return 0;

		return new CXMLReaderImpl<char, IReferenceCounted>(new CIrrXMLFileReadCallBack(file)); 
	}

} // end namespace
} // end namespace
