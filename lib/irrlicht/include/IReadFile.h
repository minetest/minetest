// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_READ_FILE_H_INCLUDED__
#define __I_READ_FILE_H_INCLUDED__

#include "IReferenceCounted.h"
#include "coreutil.h"

namespace irr
{
namespace io
{

	//! Interface providing read acess to a file.
	class IReadFile : public virtual IReferenceCounted
	{
	public:
		//! Reads an amount of bytes from the file.
		/** \param buffer Pointer to buffer where read bytes are written to.
		\param sizeToRead Amount of bytes to read from the file.
		\return How many bytes were read. */
		virtual s32 read(void* buffer, u32 sizeToRead) = 0;

		//! Changes position in file
		/** \param finalPos Destination position in the file.
		\param relativeMovement If set to true, the position in the file is
		changed relative to current position. Otherwise the position is changed
		from beginning of file.
		\return True if successful, otherwise false. */
		virtual bool seek(long finalPos, bool relativeMovement = false) = 0;

		//! Get size of file.
		/** \return Size of the file in bytes. */
		virtual long getSize() const = 0;

		//! Get the current position in the file.
		/** \return Current position in the file in bytes. */
		virtual long getPos() const = 0;

		//! Get name of file.
		/** \return File name as zero terminated character string. */
		virtual const io::path& getFileName() const = 0;
	};

	//! Internal function, please do not use.
	IReadFile* createReadFile(const io::path& fileName);
	//! Internal function, please do not use.
	IReadFile* createLimitReadFile(const io::path& fileName, IReadFile* alreadyOpenedFile, long pos, long areaSize);
	//! Internal function, please do not use.
	IReadFile* createMemoryReadFile(void* memory, long size, const io::path& fileName, bool deleteMemoryWhenDropped);

} // end namespace io
} // end namespace irr

#endif

