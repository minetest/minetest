// Copyright (C) 2008-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "irrTypes.h"

namespace irr
{
namespace video
{
enum E_INDEX_TYPE
{
	EIT_16BIT = 0,
	EIT_32BIT
};

inline u32 getIndexPitchFromType(video::E_INDEX_TYPE indexType)
{
	switch(indexType) {
	case video::EIT_16BIT:
		return sizeof(u16);
	case video::EIT_32BIT:
		return sizeof(u32);
	default:
		return 0;
	}
}

} // end namespace video
} // end namespace irr
