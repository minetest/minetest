// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_ATTRIBUTES_H_INCLUDED__
#define __C_ATTRIBUTES_H_INCLUDED__

#include "IAttributes.h"
#include "IAttribute.h"

namespace irr
{
namespace video
{
	class ITexture;
	class IVideoDriver;
}
namespace io
{


//! Implementation of the IAttributes interface
class CAttributes : public IAttributes
{
public:

	CAttributes(video::IVideoDriver* driver=0);
	~CAttributes();

	//! Returns amount of attributes in this collection of attributes.
	virtual u32 getAttributeCount() const;

	//! Returns attribute name by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual const c8* getAttributeName(s32 index);

	//! Returns the type of an attribute
	//! \param attributeName: Name for the attribute
	virtual E_ATTRIBUTE_TYPE getAttributeType(const c8* attributeName);

	//! Returns attribute type by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual E_ATTRIBUTE_TYPE getAttributeType(s32 index);

	//! Returns the type string of the attribute
	//! \param attributeName: String for the attribute type
	virtual const wchar_t* getAttributeTypeString(const c8* attributeName);

	//! Returns the type string of the attribute by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual const wchar_t* getAttributeTypeString(s32 index);

	//! Returns if an attribute with a name exists
	virtual bool existsAttribute(const c8* attributeName);

	//! Returns attribute index from name, -1 if not found
	virtual s32 findAttribute(const c8* attributeName) const;

	//! Removes all attributes
	virtual void clear();

	//! Reads attributes from a xml file.
	//! \param readCurrentElementOnly: If set to true, reading only works if current element has the name 'attributes'.
	//! IF set to false, the first appearing list attributes are read.
	virtual bool read(io::IXMLReader* reader, bool readCurrentElementOnly=false,
					  const wchar_t* nonDefaultElementName = 0);

	//! Write these attributes into a xml file
	virtual bool write(io::IXMLWriter* writer, bool writeXMLHeader=false, const wchar_t* nonDefaultElementName=0);


	/*

		Integer Attribute

	*/

	//! Adds an attribute as integer
	virtual void addInt(const c8* attributeName, s32 value);

	//! Sets an attribute as integer value
	virtual void setAttribute(const c8* attributeName, s32 value);

	//! Gets an attribute as integer value
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual s32 getAttributeAsInt(const c8* attributeName) const;

	//! Gets an attribute as integer value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual s32 getAttributeAsInt(s32 index) const;

	//! Sets an attribute as integer value
	virtual void setAttribute(s32 index, s32 value);

	/*

		Float Attribute

	*/

	//! Adds an attribute as float
	virtual void addFloat(const c8* attributeName, f32 value);

	//! Sets a attribute as float value
	virtual void setAttribute(const c8* attributeName, f32 value);

	//! Gets an attribute as float value
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual f32 getAttributeAsFloat(const c8* attributeName);

	//! Gets an attribute as float value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual f32 getAttributeAsFloat(s32 index);

	//! Sets an attribute as float value
	virtual void setAttribute(s32 index, f32 value);

	/*

		String Attribute

	*/

	//! Adds an attribute as string
	virtual void addString(const c8* attributeName, const c8* value);

	//! Sets an attribute value as string.
	//! \param attributeName: Name for the attribute
	//! \param value: Value for the attribute. Set this to 0 to delete the attribute
	virtual void setAttribute(const c8* attributeName, const c8* value);

	//! Gets an attribute as string.
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	//! or 0 if attribute is not set.
	virtual core::stringc getAttributeAsString(const c8* attributeName);

	//! Gets an attribute as string.
	//! \param attributeName: Name of the attribute to get.
	//! \param target: Buffer where the string is copied to.
	virtual void getAttributeAsString(const c8* attributeName, c8* target);

	//! Returns attribute value as string by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::stringc getAttributeAsString(s32 index);

	//! Sets an attribute value as string.
	//! \param attributeName: Name for the attribute
	virtual void setAttribute(s32 index, const c8* value);

	// wide strings

	//! Adds an attribute as string
	virtual void addString(const c8* attributeName, const wchar_t* value);

	//! Sets an attribute value as string.
	//! \param attributeName: Name for the attribute
	//! \param value: Value for the attribute. Set this to 0 to delete the attribute
	virtual void setAttribute(const c8* attributeName, const wchar_t* value);

	//! Gets an attribute as string.
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	//! or 0 if attribute is not set.
	virtual core::stringw getAttributeAsStringW(const c8* attributeName);

	//! Gets an attribute as string.
	//! \param attributeName: Name of the attribute to get.
	//! \param target: Buffer where the string is copied to.
	virtual void getAttributeAsStringW(const c8* attributeName, wchar_t* target);

	//! Returns attribute value as string by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::stringw getAttributeAsStringW(s32 index);

	//! Sets an attribute value as string.
	//! \param attributeName: Name for the attribute
	virtual void setAttribute(s32 index, const wchar_t* value);

	/*

		Binary Data Attribute

	*/

	//! Adds an attribute as binary data
	virtual void addBinary(const c8* attributeName, void* data, s32 dataSizeInBytes);

	//! Sets an attribute as binary data
	virtual void setAttribute(const c8* attributeName, void* data, s32 dataSizeInBytes);

	//! Gets an attribute as binary data
	//! \param attributeName: Name of the attribute to get.
	virtual void getAttributeAsBinaryData(const c8* attributeName, void* outData, s32 maxSizeInBytes);

	//! Gets an attribute as binary data
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual void getAttributeAsBinaryData(s32 index, void* outData, s32 maxSizeInBytes);

	//! Sets an attribute as binary data
	virtual void setAttribute(s32 index, void* data, s32 dataSizeInBytes);


	/*

		Array Attribute

	*/

	//! Adds an attribute as wide string array
	virtual void addArray(const c8* attributeName, const core::array<core::stringw>& value);

	//! Sets an attribute value as a wide string array.
	//! \param attributeName: Name for the attribute
	//! \param value: Value for the attribute. Set this to 0 to delete the attribute
	virtual void setAttribute(const c8* attributeName, const core::array<core::stringw>& value);

	//! Gets an attribute as an array of wide strings.
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	//! or 0 if attribute is not set.
	virtual core::array<core::stringw> getAttributeAsArray(const c8* attributeName);

	//! Returns attribute value as an array of wide strings by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::array<core::stringw> getAttributeAsArray(s32 index);

	//! Sets an attribute as an array of wide strings
	virtual void setAttribute(s32 index, const core::array<core::stringw>& value);

	/*

		Bool Attribute

	*/

	//! Adds an attribute as bool
	virtual void addBool(const c8* attributeName, bool value);

	//! Sets an attribute as boolean value
	virtual void setAttribute(const c8* attributeName, bool value);

	//! Gets an attribute as boolean value
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual bool getAttributeAsBool(const c8* attributeName);

	//! Gets an attribute as boolean value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual bool getAttributeAsBool(s32 index);

	//! Sets an attribute as boolean value
	virtual void setAttribute(s32 index, bool value);

	/*

		Enumeration Attribute

	*/

	//! Adds an attribute as enum
	virtual void addEnum(const c8* attributeName, const c8* enumValue, const c8* const* enumerationLiterals);

	//! Adds an attribute as enum
	virtual void addEnum(const c8* attributeName, s32 enumValue, const c8* const* enumerationLiterals);

	//! Sets an attribute as enumeration
	virtual void setAttribute(const c8* attributeName, const c8* enumValue, const c8* const* enumerationLiterals);

	//! Gets an attribute as enumeration
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual const c8* getAttributeAsEnumeration(const c8* attributeName);

	//! Gets an attribute as enumeration
	//! \param attributeName: Name of the attribute to get.
	//! \param enumerationLiteralsToUse: Use these enumeration literals to get the index value instead of the set ones.
	//! This is useful when the attribute list maybe was read from an xml file, and only contains the enumeration string, but
	//! no information about its index.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual s32 getAttributeAsEnumeration(const c8* attributeName, const c8* const* enumerationLiteralsToUse);

	//! Gets an attribute as enumeration
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual s32 getAttributeAsEnumeration(s32 index, const c8* const* enumerationLiteralsToUse);

	//! Gets an attribute as enumeration
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual const c8* getAttributeAsEnumeration(s32 index);

	//! Gets the list of enumeration literals of an enumeration attribute
	//! \param attributeName: Name of the attribute to get.
	virtual void getAttributeEnumerationLiteralsOfEnumeration(const c8* attributeName, core::array<core::stringc>& outLiterals);

	//! Gets the list of enumeration literals of an enumeration attribute
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual void getAttributeEnumerationLiteralsOfEnumeration(s32 index, core::array<core::stringc>& outLiterals);

	//! Sets an attribute as enumeration
	virtual void setAttribute(s32 index, const c8* enumValue, const c8* const* enumerationLiterals);


	/*

		SColor Attribute

	*/

	//! Adds an attribute as color
	virtual void addColor(const c8* attributeName, video::SColor value);

	//! Sets a attribute as color
	virtual void setAttribute(const c8* attributeName, video::SColor color);

	//! Gets an attribute as color
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual video::SColor getAttributeAsColor(const c8* attributeName);

	//! Gets an attribute as color
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual video::SColor getAttributeAsColor(s32 index);

	//! Sets an attribute as color
	virtual void setAttribute(s32 index, video::SColor color);

	/*

		SColorf Attribute

	*/

	//! Adds an attribute as floating point color
	virtual void addColorf(const c8* attributeName, video::SColorf value);

	//! Sets a attribute as floating point color
	virtual void setAttribute(const c8* attributeName, video::SColorf color);

	//! Gets an attribute as floating point color
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual video::SColorf getAttributeAsColorf(const c8* attributeName);

	//! Gets an attribute as floating point color
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual video::SColorf getAttributeAsColorf(s32 index);

	//! Sets an attribute as floating point color
	virtual void setAttribute(s32 index, video::SColorf color);


	/*

		Vector3d Attribute

	*/

	//! Adds an attribute as 3d vector
	virtual void addVector3d(const c8* attributeName, core::vector3df value);

	//! Sets a attribute as 3d vector
	virtual void setAttribute(const c8* attributeName, core::vector3df v);

	//! Gets an attribute as 3d vector
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::vector3df getAttributeAsVector3d(const c8* attributeName);

	//! Gets an attribute as 3d vector
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::vector3df getAttributeAsVector3d(s32 index);

	//! Sets an attribute as vector
	virtual void setAttribute(s32 index, core::vector3df v);


	/*

		Vector2d Attribute

	*/

	//! Adds an attribute as 2d vector
	virtual void addVector2d(const c8* attributeName, core::vector2df value);

	//! Sets a attribute as 2d vector
	virtual void setAttribute(const c8* attributeName, core::vector2df v);

	//! Gets an attribute as 2d vector
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::vector2df getAttributeAsVector2d(const c8* attributeName);

	//! Gets an attribute as 3d vector
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::vector2df getAttributeAsVector2d(s32 index);

	//! Sets an attribute as vector
	virtual void setAttribute(s32 index, core::vector2df v);


	/*

		Position2d Attribute

	*/

	//! Adds an attribute as 2d position
	virtual void addPosition2d(const c8* attributeName, core::position2di value);

	//! Sets a attribute as 2d position
	virtual void setAttribute(const c8* attributeName, core::position2di v);

	//! Gets an attribute as position
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::position2di getAttributeAsPosition2d(const c8* attributeName);

	//! Gets an attribute as position
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::position2di getAttributeAsPosition2d(s32 index);

	//! Sets an attribute as 2d position
	virtual void setAttribute(s32 index, core::position2di v);

	/*

		Rectangle Attribute

	*/

	//! Adds an attribute as rectangle
	virtual void addRect(const c8* attributeName, core::rect<s32> value);

	//! Sets an attribute as rectangle
	virtual void setAttribute(const c8* attributeName, core::rect<s32> v);

	//! Gets an attribute as rectangle
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::rect<s32> getAttributeAsRect(const c8* attributeName);

	//! Gets an attribute as rectangle
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::rect<s32> getAttributeAsRect(s32 index);

	//! Sets an attribute as rectangle
	virtual void setAttribute(s32 index, core::rect<s32> v);


	/*

		Dimension2d Attribute

	*/

	//! Adds an attribute as dimension2d
	virtual void addDimension2d(const c8* attributeName, core::dimension2d<u32> value);

	//! Sets an attribute as dimension2d
	virtual void setAttribute(const c8* attributeName, core::dimension2d<u32> v);

	//! Gets an attribute as dimension2d
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::dimension2d<u32> getAttributeAsDimension2d(const c8* attributeName);

	//! Gets an attribute as dimension2d
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::dimension2d<u32> getAttributeAsDimension2d(s32 index);

	//! Sets an attribute as dimension2d
	virtual void setAttribute(s32 index, core::dimension2d<u32> v);


	/*

		matrix attribute

	*/

	//! Adds an attribute as matrix
	virtual void addMatrix(const c8* attributeName, const core::matrix4& v);

	//! Sets an attribute as matrix
	virtual void setAttribute(const c8* attributeName, const core::matrix4& v);

	//! Gets an attribute as a matrix4
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::matrix4 getAttributeAsMatrix(const c8* attributeName);

	//! Gets an attribute as matrix
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::matrix4 getAttributeAsMatrix(s32 index);

	//! Sets an attribute as matrix
	virtual void setAttribute(s32 index, const core::matrix4& v);

	/*
		quaternion attribute

	*/

	//! Adds an attribute as quaternion
	virtual void addQuaternion(const c8* attributeName, core::quaternion v);

	//! Sets an attribute as quaternion
	virtual void setAttribute(const c8* attributeName, core::quaternion v);

	//! Gets an attribute as a quaternion
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::quaternion getAttributeAsQuaternion(const c8* attributeName);

	//! Gets an attribute as quaternion
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::quaternion getAttributeAsQuaternion(s32 index);

	//! Sets an attribute as quaternion
	virtual void setAttribute(s32 index, core::quaternion v);

	/*

		3d bounding box

	*/

	//! Adds an attribute as axis aligned bounding box
	virtual void addBox3d(const c8* attributeName, core::aabbox3df v);

	//! Sets an attribute as axis aligned bounding box
	virtual void setAttribute(const c8* attributeName, core::aabbox3df v);

	//! Gets an attribute as a axis aligned bounding box
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::aabbox3df getAttributeAsBox3d(const c8* attributeName);

	//! Gets an attribute as axis aligned bounding box
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::aabbox3df getAttributeAsBox3d(s32 index);

	//! Sets an attribute as axis aligned bounding box
	virtual void setAttribute(s32 index, core::aabbox3df v);

	/*

		plane

	*/

	//! Adds an attribute as 3d plane
	virtual void addPlane3d(const c8* attributeName, core::plane3df v);

	//! Sets an attribute as 3d plane
	virtual void setAttribute(const c8* attributeName, core::plane3df v);

	//! Gets an attribute as a 3d plane
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::plane3df getAttributeAsPlane3d(const c8* attributeName);

	//! Gets an attribute as 3d plane
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::plane3df getAttributeAsPlane3d(s32 index);

	//! Sets an attribute as 3d plane
	virtual void setAttribute(s32 index, core::plane3df v);


	/*

		3d triangle

	*/

	//! Adds an attribute as 3d triangle
	virtual void addTriangle3d(const c8* attributeName, core::triangle3df v);

	//! Sets an attribute as 3d trianle
	virtual void setAttribute(const c8* attributeName, core::triangle3df v);

	//! Gets an attribute as a 3d triangle
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::triangle3df getAttributeAsTriangle3d(const c8* attributeName);

	//! Gets an attribute as 3d triangle
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::triangle3df getAttributeAsTriangle3d(s32 index);

	//! Sets an attribute as 3d triangle
	virtual void setAttribute(s32 index, core::triangle3df v);


	/*

		line 2d

	*/

	//! Adds an attribute as a 2d line
	virtual void addLine2d(const c8* attributeName, core::line2df v);

	//! Sets an attribute as a 2d line
	virtual void setAttribute(const c8* attributeName, core::line2df v);

	//! Gets an attribute as a 2d line
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::line2df getAttributeAsLine2d(const c8* attributeName);

	//! Gets an attribute as a 2d line
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::line2df getAttributeAsLine2d(s32 index);

	//! Sets an attribute as a 2d line
	virtual void setAttribute(s32 index, core::line2df v);


	/*

		line 3d

	*/

	//! Adds an attribute as a 3d line
	virtual void addLine3d(const c8* attributeName, core::line3df v);

	//! Sets an attribute as a 3d line
	virtual void setAttribute(const c8* attributeName, core::line3df v);

	//! Gets an attribute as a 3d line
	//! \param attributeName: Name of the attribute to get.
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual core::line3df getAttributeAsLine3d(const c8* attributeName);

	//! Gets an attribute as a 3d line
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual core::line3df getAttributeAsLine3d(s32 index);

	//! Sets an attribute as a 3d line
	virtual void setAttribute(s32 index, core::line3df v);


	/*

		Texture Attribute

	*/

	//! Adds an attribute as texture reference
	virtual void addTexture(const c8* attributeName, video::ITexture* texture, const io::path& filename = "");

	//! Sets an attribute as texture reference
	virtual void setAttribute(const c8* attributeName, video::ITexture* texture, const io::path& filename = "");

	//! Gets an attribute as texture reference
	//! \param attributeName: Name of the attribute to get.
	virtual video::ITexture* getAttributeAsTexture(const c8* attributeName);

	//! Gets an attribute as texture reference
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual video::ITexture* getAttributeAsTexture(s32 index);

	//! Sets an attribute as texture reference
	virtual void setAttribute(s32 index, video::ITexture* texture, const io::path& filename = "");



	/*

		User Pointer Attribute

	*/

	//! Adds an attribute as user pointner
	virtual void addUserPointer(const c8* attributeName, void* userPointer);

	//! Sets an attribute as user pointer
	virtual void setAttribute(const c8* attributeName, void* userPointer);

	//! Gets an attribute as user pointer
	//! \param attributeName: Name of the attribute to get.
	virtual void* getAttributeAsUserPointer(const c8* attributeName);

	//! Gets an attribute as user pointer
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual void* getAttributeAsUserPointer(s32 index);

	//! Sets an attribute as user pointer
	virtual void setAttribute(s32 index, void* userPointer);

protected:

	void readAttributeFromXML(io::IXMLReader* reader);

	core::array<IAttribute*> Attributes;

	IAttribute* getAttributeP(const c8* attributeName) const;

	video::IVideoDriver* Driver;
};

} // end namespace io
} // end namespace irr

#endif

