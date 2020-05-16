// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_MESSAGE_BOX_FLAGS_H_INCLUDED__
#define __E_MESSAGE_BOX_FLAGS_H_INCLUDED__

namespace irr
{
namespace gui
{

//! enumeration for message box layout flags
enum EMESSAGE_BOX_FLAG
{
	//! Flag for the ok button
	EMBF_OK = 0x1,

	//! Flag for the cancel button
	EMBF_CANCEL = 0x2,

	//! Flag for the yes button
	EMBF_YES = 0x4,

	//! Flag for the no button
	EMBF_NO = 0x8,

	//! This value is not used. It only forces this enumeration to compile in 32 bit.
	EMBF_FORCE_32BIT = 0x7fffffff
};

} // namespace gui
} // namespace irr

#endif

