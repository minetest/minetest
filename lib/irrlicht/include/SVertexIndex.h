// Copyright (C) 2008-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __S_VERTEX_INDEX_H_INCLUDED__
#define __S_VERTEX_INDEX_H_INCLUDED__

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


/*
//! vertex index used by the Irrlicht engine.
template <class T>
struct SSpecificVertexIndex
{
	T Index;

	//! default constructor
	SSpecificVertexIndex() {}

	//! constructor
	SSpecificVertexIndex(u32 _index) :Index(_index) {}

	bool operator==(const SSpecificVertexIndex& other) const
	{
		return (Index == other.Index);
	}

	bool operator!=(const SSpecificVertexIndex& other) const
	{
		return (Index != other.Index);
	}

	bool operator<(const SSpecificVertexIndex& other) const
	{
		return (Index < other.Index);
	}

	SSpecificVertexIndex operator+(const u32& other) const
	{
		return SSpecificVertexIndex(Index + other);
	}

	operator const u32() const
	{
		return (const u32)Index;
	}

	E_INDEX_TYPE getType() const
	{
		if (sizeof(T)==sizeof(u16))
			return video::EIT_16BIT;
		return video::EIT_32BIT;
	}

};

//typedef SSpecificVertexIndex<u16> SVertexIndex;

typedef u32 SVertexIndex;
*/


} // end namespace video
} // end namespace irr

#endif

