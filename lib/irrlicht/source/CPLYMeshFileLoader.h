// Copyright (C) 2009-2012 Gaz Davidson
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_PLY_MESH_FILE_LOADER_H_INCLUDED__
#define __C_PLY_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "ISceneManager.h"
#include "CDynamicMeshBuffer.h"

namespace irr
{
namespace scene
{

enum E_PLY_PROPERTY_TYPE
{
	EPLYPT_INT8  = 0,
	EPLYPT_INT16,
	EPLYPT_INT32,
	EPLYPT_FLOAT32,
	EPLYPT_FLOAT64,
	EPLYPT_LIST,
	EPLYPT_UNKNOWN
};

//! Meshloader capable of loading obj meshes.
class CPLYMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CPLYMeshFileLoader(scene::ISceneManager* smgr);

	//! Destructor
	virtual ~CPLYMeshFileLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".ply")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

private:

	struct SPLYProperty
	{
		core::stringc Name;
		E_PLY_PROPERTY_TYPE Type;
		union
		{
			u8  Int8;
			u16 Int16;
			u32 Int32;
			f32 Float32;
			f64 Double;
			struct SPLYListProperty
			{
				E_PLY_PROPERTY_TYPE CountType;
				E_PLY_PROPERTY_TYPE ItemType;
			} List;

		} Data;

		inline u32 size() const
		{
			switch(Type)
			{
			case EPLYPT_INT8:
				return 1;
			case EPLYPT_INT16:
				return 2;
			case EPLYPT_INT32:
			case EPLYPT_FLOAT32:
				return 4;
			case EPLYPT_FLOAT64:
				return 8;
			case EPLYPT_LIST:
			case EPLYPT_UNKNOWN:
			default:
				return 0;
			}
		}

		inline bool isFloat() const
		{
			switch(Type)
			{
			case EPLYPT_FLOAT32:
			case EPLYPT_FLOAT64:
				return true;
			case EPLYPT_INT8:
			case EPLYPT_INT16:
			case EPLYPT_INT32:
			case EPLYPT_LIST:
			case EPLYPT_UNKNOWN:
			default:
				return false;
			}
		}
	};

	struct SPLYElement
	{
		// name of the element. We only want "vertex" and "face" elements
		// but we have to parse the others anyway.
		core::stringc Name;
		// The number of elements in the file
		u32 Count;
		// Properties of this element
		core::array<SPLYProperty> Properties;
		// in binary files, true if this is a fixed size
		bool IsFixedWidth;
		// known size in bytes, 0 if unknown
		u32 KnownSize;
	};

	bool allocateBuffer();
	c8* getNextLine();
	c8* getNextWord();
	void fillBuffer();
	E_PLY_PROPERTY_TYPE getPropertyType(const c8* typeString) const;

	bool readVertex(const SPLYElement &Element, scene::CDynamicMeshBuffer* mb);
	bool readFace(const SPLYElement &Element, scene::CDynamicMeshBuffer* mb);
	void skipElement(const SPLYElement &Element);
	void skipProperty(const SPLYProperty &Property);
	f32 getFloat(E_PLY_PROPERTY_TYPE t);
	u32 getInt(E_PLY_PROPERTY_TYPE t);
	void moveForward(u32 bytes);

	core::array<SPLYElement*> ElementList;

	scene::ISceneManager* SceneManager;
	io::IReadFile *File;
	c8 *Buffer;
	bool IsBinaryFile, IsWrongEndian, EndOfFile;
	s32 LineLength, WordLength;
	c8 *StartPointer, *EndPointer, *LineEndPointer;
};

} // end namespace scene
} // end namespace irr

#endif

