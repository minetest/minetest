// Copyright (C) 2009-2012 Gaz Davidson
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_PLY_LOADER_

#include "CPLYMeshFileLoader.h"
#include "IMeshManipulator.h"
#include "SMesh.h"
#include "CDynamicMeshBuffer.h"
#include "SAnimatedMesh.h"
#include "IReadFile.h"
#include "fast_atof.h"
#include "os.h"

namespace irr
{
namespace scene
{

// input buffer must be at least twice as long as the longest line in the file
#define PLY_INPUT_BUFFER_SIZE 51200 // file is loaded in 50k chunks

// constructor
CPLYMeshFileLoader::CPLYMeshFileLoader(scene::ISceneManager* smgr)
: SceneManager(smgr), File(0), Buffer(0)
{
}


CPLYMeshFileLoader::~CPLYMeshFileLoader()
{
	// delete the buffer in case we didn't earlier
	// (we do, but this could be disabled to increase the speed of loading hundreds of meshes)
	if (Buffer)
	{
		delete [] Buffer;
		Buffer = 0;
	}

	// Destroy the element list if it exists
	for (u32 i=0; i<ElementList.size(); ++i)
		delete ElementList[i];
	ElementList.clear();
}


//! returns true if the file maybe is able to be loaded by this class
bool CPLYMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension(filename, "ply");
}


//! creates/loads an animated mesh from the file.
IAnimatedMesh* CPLYMeshFileLoader::createMesh(io::IReadFile* file)
{
	if (!file)
		return 0;

	File = file;
	File->grab();

	// attempt to allocate the buffer and fill with data
	if (!allocateBuffer())
	{
		File->drop();
		File = 0;
		return 0;
	}

	// start with empty mesh
	SAnimatedMesh* animMesh = 0;
	u32 vertCount=0;

	// Currently only supports ASCII meshes
	if (strcmp(getNextLine(), "ply"))
	{
		os::Printer::log("Not a valid PLY file", file->getFileName().c_str(), ELL_ERROR);
	}
	else
	{
		// cut the next line out
		getNextLine();
		// grab the word from this line
		c8 *word = getNextWord();

		// ignore comments
		while (strcmp(word, "comment") == 0)
		{
			getNextLine();
			word = getNextWord();
		}

		bool readingHeader = true;
		bool continueReading = true;
		IsBinaryFile = false;
		IsWrongEndian= false;

		do
		{
			if (strcmp(word, "format") == 0)
			{
				word = getNextWord();

				if (strcmp(word, "binary_little_endian") == 0)
 {
					IsBinaryFile = true;
#ifdef __BIG_ENDIAN__
					IsWrongEndian = true;
#endif

				}
				else if (strcmp(word, "binary_big_endian") == 0)
				{
					IsBinaryFile = true;
#ifndef __BIG_ENDIAN__
					IsWrongEndian = true;
#endif
				}
				else if (strcmp(word, "ascii"))
				{
					// abort if this isn't an ascii or a binary mesh
					os::Printer::log("Unsupported PLY mesh format", word, ELL_ERROR);
					continueReading = false;
				}

				if (continueReading)
				{
					word = getNextWord();
					if (strcmp(word, "1.0"))
					{
						os::Printer::log("Unsupported PLY mesh version", word, ELL_WARNING);
					}
				}
			}
			else if (strcmp(word, "property") == 0)
			{
				word = getNextWord();

				if (!ElementList.size())
				{
					os::Printer::log("PLY property found before element", word, ELL_WARNING);
				}
				else
				{
					// get element
					SPLYElement* el = ElementList[ElementList.size()-1];

					// fill property struct
					SPLYProperty prop;
					prop.Type = getPropertyType(word);
					el->KnownSize += prop.size();

					if (prop.Type == EPLYPT_LIST)
					{
						el->IsFixedWidth = false;

						word = getNextWord();

						prop.Data.List.CountType = getPropertyType(word);
						if (IsBinaryFile && prop.Data.List.CountType == EPLYPT_UNKNOWN)
						{
							os::Printer::log("Cannot read binary PLY file containing data types of unknown length", word, ELL_ERROR);
							continueReading = false;
						}
						else
						{
							word = getNextWord();
							prop.Data.List.ItemType = getPropertyType(word);
							if (IsBinaryFile && prop.Data.List.ItemType == EPLYPT_UNKNOWN)
							{
								os::Printer::log("Cannot read binary PLY file containing data types of unknown length", word, ELL_ERROR);
								continueReading = false;
							}
						}
					}
					else if (IsBinaryFile && prop.Type == EPLYPT_UNKNOWN)
					{
						os::Printer::log("Cannot read binary PLY file containing data types of unknown length", word, ELL_ERROR);
						continueReading = false;
					}

					prop.Name = getNextWord();

					// add property to element
					el->Properties.push_back(prop);
				}
			}
			else if (strcmp(word, "element") == 0)
			{
				SPLYElement* el = new SPLYElement;
				el->Name = getNextWord();
				el->Count = atoi(getNextWord());
				el->IsFixedWidth = true;
				el->KnownSize = 0;
				ElementList.push_back(el);

				if (el->Name == "vertex")
					vertCount = el->Count;

			}
			else if (strcmp(word, "end_header") == 0)
			{
				readingHeader = false;
				if (IsBinaryFile)
				{
					StartPointer = LineEndPointer + 1;
				}
			}
			else if (strcmp(word, "comment") == 0)
			{
				// ignore line
			}
			else
			{
				os::Printer::log("Unknown item in PLY file", word, ELL_WARNING);
			}

			if (readingHeader && continueReading)
			{
				getNextLine();
				word = getNextWord();
			}
		}
		while (readingHeader && continueReading);

		// now to read the actual data from the file
		if (continueReading)
		{
			// create a mesh buffer
			CDynamicMeshBuffer *mb = new CDynamicMeshBuffer(video::EVT_STANDARD, vertCount > 65565 ? video::EIT_32BIT : video::EIT_16BIT);
			mb->getVertexBuffer().reallocate(vertCount);
			mb->getIndexBuffer().reallocate(vertCount);
			mb->setHardwareMappingHint(EHM_STATIC);

			bool hasNormals=true;
			// loop through each of the elements
			for (u32 i=0; i<ElementList.size(); ++i)
			{
				// do we want this element type?
				if (ElementList[i]->Name == "vertex")
				{
					// loop through vertex properties
					for (u32 j=0; j < ElementList[i]->Count; ++j)
						hasNormals &= readVertex(*ElementList[i], mb);
				}
				else if (ElementList[i]->Name == "face")
				{
					// read faces
					for (u32 j=0; j < ElementList[i]->Count; ++j)
						readFace(*ElementList[i], mb);
				}
				else
				{
					// skip these elements
					for (u32 j=0; j < ElementList[i]->Count; ++j)
						skipElement(*ElementList[i]);
				}
			}
			mb->recalculateBoundingBox();
			if (!hasNormals)
				SceneManager->getMeshManipulator()->recalculateNormals(mb);
			SMesh* m = new SMesh();
			m->addMeshBuffer(mb);
			m->recalculateBoundingBox();
			mb->drop();
			animMesh = new SAnimatedMesh();
			animMesh->addMesh(m);
			animMesh->recalculateBoundingBox();
			m->drop();
		}
	}


	// free the buffer
	delete [] Buffer;
	Buffer = 0;
	File->drop();
	File = 0;

	// if we managed to create a mesh, return it
	return animMesh;
}


bool CPLYMeshFileLoader::readVertex(const SPLYElement &Element, scene::CDynamicMeshBuffer* mb)
{
	if (!IsBinaryFile)
		getNextLine();

	video::S3DVertex vert;
	vert.Color.set(255,255,255,255);
	vert.TCoords.X = 0.0f;
	vert.TCoords.Y = 0.0f;
	vert.Normal.X = 0.0f;
	vert.Normal.Y = 1.0f;
	vert.Normal.Z = 0.0f;

	bool result=false;
	for (u32 i=0; i < Element.Properties.size(); ++i)
	{
		E_PLY_PROPERTY_TYPE t = Element.Properties[i].Type;

		if (Element.Properties[i].Name == "x")
			vert.Pos.X = getFloat(t);
		else if (Element.Properties[i].Name == "y")
			vert.Pos.Z = getFloat(t);
		else if (Element.Properties[i].Name == "z")
			vert.Pos.Y = getFloat(t);
		else if (Element.Properties[i].Name == "nx")
		{
			vert.Normal.X = getFloat(t);
			result=true;
		}
		else if (Element.Properties[i].Name == "ny")
		{
			vert.Normal.Z = getFloat(t);
			result=true;
		}
		else if (Element.Properties[i].Name == "nz")
		{
			vert.Normal.Y = getFloat(t);
			result=true;
		}
		else if (Element.Properties[i].Name == "u")
			vert.TCoords.X = getFloat(t);
		else if (Element.Properties[i].Name == "v")
			vert.TCoords.Y = getFloat(t);
		else if (Element.Properties[i].Name == "red")
		{
			u32 value = Element.Properties[i].isFloat() ? (u32)(getFloat(t)*255.0f) : getInt(t);
			vert.Color.setRed(value);
		}
		else if (Element.Properties[i].Name == "green")
		{
			u32 value = Element.Properties[i].isFloat() ? (u32)(getFloat(t)*255.0f) : getInt(t);
			vert.Color.setGreen(value);
		}
		else if (Element.Properties[i].Name == "blue")
		{
			u32 value = Element.Properties[i].isFloat() ? (u32)(getFloat(t)*255.0f) : getInt(t);
			vert.Color.setBlue(value);
		}
		else if (Element.Properties[i].Name == "alpha")
		{
			u32 value = Element.Properties[i].isFloat() ? (u32)(getFloat(t)*255.0f) : getInt(t);
			vert.Color.setAlpha(value);
		}
		else
			skipProperty(Element.Properties[i]);
	}

	mb->getVertexBuffer().push_back(vert);

	return result;
}


bool CPLYMeshFileLoader::readFace(const SPLYElement &Element, scene::CDynamicMeshBuffer* mb)
{
	if (!IsBinaryFile)
		getNextLine();

	for (u32 i=0; i < Element.Properties.size(); ++i)
	{
		if ( (Element.Properties[i].Name == "vertex_indices" ||
			Element.Properties[i].Name == "vertex_index") && Element.Properties[i].Type == EPLYPT_LIST)
		{
			// get count
			s32 count = getInt(Element.Properties[i].Data.List.CountType);
			u32 a = getInt(Element.Properties[i].Data.List.ItemType),
				b = getInt(Element.Properties[i].Data.List.ItemType),
				c = getInt(Element.Properties[i].Data.List.ItemType);
			s32 j = 3;

			mb->getIndexBuffer().push_back(a);
			mb->getIndexBuffer().push_back(c);
			mb->getIndexBuffer().push_back(b);

			for (; j < count; ++j)
			{
				b = c;
				c = getInt(Element.Properties[i].Data.List.ItemType);
				mb->getIndexBuffer().push_back(a);
				mb->getIndexBuffer().push_back(c);
				mb->getIndexBuffer().push_back(b);
			}
		}
		else if (Element.Properties[i].Name == "intensity")
		{
			// todo: face intensity
			skipProperty(Element.Properties[i]);
		}
		else
			skipProperty(Element.Properties[i]);
	}
	return true;
}


// skips an element and all properties. return false on EOF
void CPLYMeshFileLoader::skipElement(const SPLYElement &Element)
{
	if (IsBinaryFile)
		if (Element.IsFixedWidth)
			moveForward(Element.KnownSize);
		else
			for (u32 i=0; i < Element.Properties.size(); ++i)
				skipProperty(Element.Properties[i]);
	else
		getNextLine();
}


void CPLYMeshFileLoader::skipProperty(const SPLYProperty &Property)
{
	if (Property.Type == EPLYPT_LIST)
	{
		s32 count = getInt(Property.Data.List.CountType);

		for (s32 i=0; i < count; ++i)
			getInt(Property.Data.List.CountType);
	}
	else
	{
		if (IsBinaryFile)
			moveForward(Property.size());
		else
			getNextWord();
	}
}


bool CPLYMeshFileLoader::allocateBuffer()
{
	// Destroy the element list if it exists
	for (u32 i=0; i<ElementList.size(); ++i)
		delete ElementList[i];
	ElementList.clear();

	if (!Buffer)
		Buffer = new c8[PLY_INPUT_BUFFER_SIZE];

	// not enough memory?
	if (!Buffer)
		return false;

	// blank memory
	memset(Buffer, 0, PLY_INPUT_BUFFER_SIZE);

	StartPointer = Buffer;
	EndPointer = Buffer;
	LineEndPointer = Buffer-1;
	WordLength = -1;
	EndOfFile = false;

	// get data from the file
	fillBuffer();

	return true;
}


// gets more data from the file. returns false on EOF
void CPLYMeshFileLoader::fillBuffer()
{
	if (EndOfFile)
		return;

	u32 length = (u32)(EndPointer - StartPointer);
	if (length && StartPointer != Buffer)
	{
		// copy the remaining data to the start of the buffer
		memcpy(Buffer, StartPointer, length);
	}
	// reset start position
	StartPointer = Buffer;
	EndPointer = StartPointer + length;

	if (File->getPos() == File->getSize())
	{
		EndOfFile = true;
	}
	else
	{
		// read data from the file
		u32 count = File->read(EndPointer, PLY_INPUT_BUFFER_SIZE - length);

		// increment the end pointer by the number of bytes read
		EndPointer = EndPointer + count;

		// if we didn't completely fill the buffer
		if (count != PLY_INPUT_BUFFER_SIZE - length)
		{
			// blank the rest of the memory
			memset(EndPointer, 0, Buffer + PLY_INPUT_BUFFER_SIZE - EndPointer);

			// end of file
			EndOfFile = true;
		}
	}
}


// skips x bytes in the file, getting more data if required
void CPLYMeshFileLoader::moveForward(u32 bytes)
{
	if (StartPointer + bytes >= EndPointer)
		fillBuffer();
	if (StartPointer + bytes < EndPointer)
		StartPointer += bytes;
	else
		StartPointer = EndPointer;
}


E_PLY_PROPERTY_TYPE CPLYMeshFileLoader::getPropertyType(const c8* typeString) const
{
	if (strcmp(typeString, "char") == 0 ||
		strcmp(typeString, "uchar") == 0 ||
		strcmp(typeString, "int8") == 0 ||
		strcmp(typeString, "uint8") == 0)
	{
		return EPLYPT_INT8;
	}
	else if (strcmp(typeString, "uint") == 0 ||
		strcmp(typeString, "int16") == 0 ||
		strcmp(typeString, "uint16") == 0 ||
		strcmp(typeString, "short") == 0 ||
		strcmp(typeString, "ushort") == 0)
	{
		return EPLYPT_INT16;
	}
	else if (strcmp(typeString, "int") == 0 ||
		strcmp(typeString, "long") == 0 ||
		strcmp(typeString, "ulong") == 0 ||
		strcmp(typeString, "int32") == 0 ||
		strcmp(typeString, "uint32") == 0)
	{
		return EPLYPT_INT32;
	}
	else if (strcmp(typeString, "float") == 0 ||
		strcmp(typeString, "float32") == 0)
	{
		return EPLYPT_FLOAT32;
	}
	else if (strcmp(typeString, "float64") == 0 ||
		strcmp(typeString, "double") == 0)
	{
		return EPLYPT_FLOAT64;
	}
	else if ( strcmp(typeString, "list") == 0 )
	{
		return EPLYPT_LIST;
	}
	else
	{
		// unsupported type.
		// cannot be loaded in binary mode
		return EPLYPT_UNKNOWN;
	}
}


// Split the string data into a line in place by terminating it instead of copying.
c8* CPLYMeshFileLoader::getNextLine()
{
	// move the start pointer along
	StartPointer = LineEndPointer + 1;

	// crlf split across buffer move
	if (*StartPointer == '\n')
	{
		*StartPointer = '\0';
		++StartPointer;
	}

	// begin at the start of the next line
	c8* pos = StartPointer;
	while (pos < EndPointer && *pos && *pos != '\r' && *pos != '\n')
		++pos;

	if ( pos < EndPointer && ( *(pos+1) == '\r' || *(pos+1) == '\n') )
	{
		*pos = '\0';
		++pos;
	}

	// we have reached the end of the buffer
	if (pos >= EndPointer)
	{
		// get data from the file
		if (!EndOfFile)
		{
			fillBuffer();
			// reset line end pointer
			LineEndPointer = StartPointer - 1;

			if (StartPointer != EndPointer)
				return getNextLine();
			else
				return Buffer;
		}
		else
		{
			// EOF
			StartPointer = EndPointer-1;
			*StartPointer = '\0';
			return StartPointer;
		}
	}
	else
	{
		// null terminate the string in place
		*pos = '\0';
		LineEndPointer = pos;
		WordLength = -1;
		// return pointer to the start of the line
		return StartPointer;
	}
}


// null terminate the next word on the previous line and move the next word pointer along
// since we already have a full line in the buffer, we never need to retrieve more data
c8* CPLYMeshFileLoader::getNextWord()
{
	// move the start pointer along
	StartPointer += WordLength + 1;

	if (StartPointer == LineEndPointer)
	{
		WordLength = -1; //
		return LineEndPointer;
	}
	// begin at the start of the next word
	c8* pos = StartPointer;
	while (*pos && pos < LineEndPointer && pos < EndPointer && *pos != ' ' && *pos != '\t')
		++pos;

	while(*pos && pos < LineEndPointer && pos < EndPointer && (*pos == ' ' || *pos == '\t') )
	{
		// null terminate the string in place
		*pos = '\0';
		++pos;
	}
	--pos;
	WordLength = (s32)(pos-StartPointer);
	// return pointer to the start of the word
	return StartPointer;
}


// read the next float from the file and move the start pointer along
f32 CPLYMeshFileLoader::getFloat(E_PLY_PROPERTY_TYPE t)
{
	f32 retVal = 0.0f;

	if (IsBinaryFile)
	{
		if (EndPointer - StartPointer < 8)
			fillBuffer();

		if (EndPointer - StartPointer > 0)
		{
			switch (t)
			{
			case EPLYPT_INT8:
				retVal = *StartPointer;
				StartPointer++;
				break;
			case EPLYPT_INT16:
				if (IsWrongEndian)
					retVal = os::Byteswap::byteswap(*(reinterpret_cast<s16*>(StartPointer)));
				else
					retVal = *(reinterpret_cast<s16*>(StartPointer));
				StartPointer += 2;
				break;
			case EPLYPT_INT32:
				if (IsWrongEndian)
					retVal = f32(os::Byteswap::byteswap(*(reinterpret_cast<s32*>(StartPointer))));
				else
					retVal = f32(*(reinterpret_cast<s32*>(StartPointer)));
				StartPointer += 4;
				break;
			case EPLYPT_FLOAT32:
				if (IsWrongEndian)
					retVal = os::Byteswap::byteswap(*(reinterpret_cast<f32*>(StartPointer)));
				else
					retVal = *(reinterpret_cast<f32*>(StartPointer));
				StartPointer += 4;
				break;
			case EPLYPT_FLOAT64:
				// todo: byteswap 64-bit
				retVal = f32(*(reinterpret_cast<f64*>(StartPointer)));
				StartPointer += 8;
				break;
			case EPLYPT_LIST:
			case EPLYPT_UNKNOWN:
			default:
				retVal = 0.0f;
				StartPointer++; // ouch!
			}
		}
		else
			retVal = 0.0f;
	}
	else
	{
		c8* word = getNextWord();
		switch (t)
		{
		case EPLYPT_INT8:
		case EPLYPT_INT16:
		case EPLYPT_INT32:
			retVal = f32(atoi(word));
			break;
		case EPLYPT_FLOAT32:
		case EPLYPT_FLOAT64:
			retVal = f32(atof(word));
			break;
		case EPLYPT_LIST:
		case EPLYPT_UNKNOWN:
		default:
			retVal = 0.0f;
		}
	}
	return retVal;
}


// read the next int from the file and move the start pointer along
u32 CPLYMeshFileLoader::getInt(E_PLY_PROPERTY_TYPE t)
{
	u32 retVal = 0;

	if (IsBinaryFile)
	{
		if (!EndOfFile && EndPointer - StartPointer < 8)
			fillBuffer();

		if (EndPointer - StartPointer)
		{
			switch (t)
			{
			case EPLYPT_INT8:
				retVal = *StartPointer;
				StartPointer++;
				break;
			case EPLYPT_INT16:
				if (IsWrongEndian)
					retVal = os::Byteswap::byteswap(*(reinterpret_cast<u16*>(StartPointer)));
				else
					retVal = *(reinterpret_cast<u16*>(StartPointer));
				StartPointer += 2;
				break;
			case EPLYPT_INT32:
				if (IsWrongEndian)
					retVal = os::Byteswap::byteswap(*(reinterpret_cast<s32*>(StartPointer)));
				else
					retVal = *(reinterpret_cast<s32*>(StartPointer));
				StartPointer += 4;
				break;
			case EPLYPT_FLOAT32:
				if (IsWrongEndian)
					retVal = (u32)os::Byteswap::byteswap(*(reinterpret_cast<f32*>(StartPointer)));
				else
					retVal = (u32)(*(reinterpret_cast<f32*>(StartPointer)));
				StartPointer += 4;
				break;
			case EPLYPT_FLOAT64:
				// todo: byteswap 64-bit
				retVal = (u32)(*(reinterpret_cast<f64*>(StartPointer)));
				StartPointer += 8;
				break;
			case EPLYPT_LIST:
			case EPLYPT_UNKNOWN:
			default:
				retVal = 0;
				StartPointer++; // ouch!
			}
		}
		else
			retVal = 0;
	}
	else
	{
		c8* word = getNextWord();
		switch (t)
		{
		case EPLYPT_INT8:
		case EPLYPT_INT16:
		case EPLYPT_INT32:
			retVal = atoi(word);
			break;
		case EPLYPT_FLOAT32:
		case EPLYPT_FLOAT64:
			retVal = u32(atof(word));
			break;
		case EPLYPT_LIST:
		case EPLYPT_UNKNOWN:
		default:
			retVal = 0;
		}
	}
	return retVal;
}



} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_PLY_LOADER_

