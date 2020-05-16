// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_READ_FILE_H_INCLUDED__
#define __C_READ_FILE_H_INCLUDED__

#include <stdio.h>
#include "IReadFile.h"
#include "irrString.h"

namespace irr
{

namespace io
{

	/*!
		Class for reading a real file from disk.
	*/
	class CReadFile : public IReadFile
	{
	public:

		CReadFile(const io::path& fileName);

		virtual ~CReadFile();

		//! returns how much was read
		virtual s32 read(void* buffer, u32 sizeToRead);

		//! changes position in file, returns true if successful
		virtual bool seek(long finalPos, bool relativeMovement = false);

		//! returns size of file
		virtual long getSize() const;

		//! returns if file is open
		virtual bool isOpen() const
		{
			return File != 0;
		}

		//! returns where in the file we are.
		virtual long getPos() const;

		//! returns name of file
		virtual const io::path& getFileName() const;

	private:

		//! opens the file
		void openFile();

		FILE* File;
		long FileSize;
		io::path Filename;
	};

} // end namespace io
} // end namespace irr

#endif

