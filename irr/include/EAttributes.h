// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

namespace irr
{
namespace io
{

//! Types of attributes available for IAttributes
enum E_ATTRIBUTE_TYPE
{
	// integer attribute
	EAT_INT = 0,

	// float attribute
	EAT_FLOAT,

	// boolean attribute
	EAT_BOOL,

	// known attribute type count
	EAT_COUNT,

	// unknown attribute
	EAT_UNKNOWN
};

} // end namespace io
} // end namespace irr
