// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CMetaTriangleSelector.h"

namespace irr
{
namespace scene
{

//! constructor
CMetaTriangleSelector::CMetaTriangleSelector()
{
	#ifdef _DEBUG
	setDebugName("CMetaTriangleSelector");
	#endif
}


//! destructor
CMetaTriangleSelector::~CMetaTriangleSelector()
{
	removeAllTriangleSelectors();
}


//! Returns amount of all available triangles in this selector
s32 CMetaTriangleSelector::getTriangleCount() const
{
	s32 count = 0;
	for (u32 i=0; i<TriangleSelectors.size(); ++i)
		count += TriangleSelectors[i]->getTriangleCount();

	return count;
}


//! Gets all triangles.
void CMetaTriangleSelector::getTriangles(core::triangle3df* triangles, s32 arraySize,
		s32& outTriangleCount, const core::matrix4* transform) const
{
	s32 outWritten = 0;
	for (u32 i=0; i<TriangleSelectors.size(); ++i)
	{
		s32 t = 0;
		TriangleSelectors[i]->getTriangles(triangles + outWritten,
				arraySize - outWritten, t, transform);
		outWritten += t;
		if (outWritten==arraySize)
			break;
	}

	outTriangleCount = outWritten;
}


//! Gets all triangles which lie within a specific bounding box.
void CMetaTriangleSelector::getTriangles(core::triangle3df* triangles, s32 arraySize,
		s32& outTriangleCount, const core::aabbox3d<f32>& box,
		const core::matrix4* transform) const
{
	s32 outWritten = 0;
	for (u32 i=0; i<TriangleSelectors.size(); ++i)
	{
		s32 t = 0;
		TriangleSelectors[i]->getTriangles(triangles + outWritten,
				arraySize - outWritten, t, box, transform);
		outWritten += t;
		if (outWritten==arraySize)
			break;
	}

	outTriangleCount = outWritten;
}


//! Gets all triangles which have or may have contact with a 3d line.
void CMetaTriangleSelector::getTriangles(core::triangle3df* triangles, s32 arraySize,
		s32& outTriangleCount, const core::line3d<f32>& line,
		const core::matrix4* transform) const
{
	s32 outWritten = 0;
	for (u32 i=0; i<TriangleSelectors.size(); ++i)
	{
		s32 t = 0;
		TriangleSelectors[i]->getTriangles(triangles + outWritten,
				arraySize - outWritten, t, line, transform);
		outWritten += t;
		if (outWritten==arraySize)
			break;
	}

	outTriangleCount = outWritten;
}


//! Adds a triangle selector to the collection of triangle selectors
//! in this metaTriangleSelector.
void CMetaTriangleSelector::addTriangleSelector(ITriangleSelector* toAdd)
{
	if (!toAdd)
		return;

	TriangleSelectors.push_back(toAdd);
	toAdd->grab();
}


//! Removes a specific triangle selector which was added before	from the collection.
bool CMetaTriangleSelector::removeTriangleSelector(ITriangleSelector* toRemove)
{
	for (u32 i=0; i<TriangleSelectors.size(); ++i)
	{
		if (toRemove == TriangleSelectors[i])
		{
			TriangleSelectors[i]->drop();
			TriangleSelectors.erase(i);
			return true;
		}
	}

	return false;
}


//! Removes all triangle selectors from the collection.
void CMetaTriangleSelector::removeAllTriangleSelectors()
{
	for (u32 i=0; i<TriangleSelectors.size(); ++i)
		TriangleSelectors[i]->drop();

	TriangleSelectors.clear();
}


//! Return the scene node associated with a given triangle.
ISceneNode* CMetaTriangleSelector::getSceneNodeForTriangle(u32 triangleIndex) const
{
	u32 totalTriangles = 0;

	for (u32 i=0; i<TriangleSelectors.size(); ++i)
	{
		totalTriangles += TriangleSelectors[i]->getTriangleCount();

		if(totalTriangles > triangleIndex)
			return TriangleSelectors[i]->getSceneNodeForTriangle(0);
	}

	// For lack of anything more sensible, return the first selector.
	return TriangleSelectors[0]->getSceneNodeForTriangle(0);
}


/* Return the number of TriangleSelectors that are inside this one, 
Only useful for MetaTriangleSelector others return 1
*/
u32 CMetaTriangleSelector::getSelectorCount() const
{
	return TriangleSelectors.size();
}


/* Returns the TriangleSelector based on index based on getSelectorCount
Only useful for MetaTriangleSelector others return 'this'
*/
ITriangleSelector* CMetaTriangleSelector::getSelector(u32 index)
{
	if (index >= TriangleSelectors.size())
		return 0;
	return TriangleSelectors[index];
}


/* Returns the TriangleSelector based on index based on getSelectorCount
Only useful for MetaTriangleSelector others return 'this'
*/
const ITriangleSelector* CMetaTriangleSelector::getSelector(u32 index) const
{
	if (index >= TriangleSelectors.size())
		return 0;
	return TriangleSelectors[index];
}


} // end namespace scene
} // end namespace irr

