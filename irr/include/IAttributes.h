// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IReferenceCounted.h"
#include "EAttributes.h"

// not needed here but I can't be bothered to clean the transitive includes up.
#include "quaternion.h"

namespace irr
{
namespace video
{
class ITexture;
} // end namespace video
namespace io
{

//! Provides a generic interface for attributes and their values and the possibility to serialize them
class IAttributes : public virtual IReferenceCounted
{
public:
	//! Returns amount of attributes in this collection of attributes.
	virtual u32 getAttributeCount() const = 0;

	//! Returns attribute name by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual const c8 *getAttributeName(s32 index) const = 0;

	//! Returns the type of an attribute
	//! \param attributeName: Name for the attribute
	virtual E_ATTRIBUTE_TYPE getAttributeType(const c8 *attributeName) const = 0;

	//! Returns attribute type by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual E_ATTRIBUTE_TYPE getAttributeType(s32 index) const = 0;

	//! Returns if an attribute with a name exists
	virtual bool existsAttribute(const c8 *attributeName) const = 0;

	//! Returns attribute index from name, -1 if not found
	virtual s32 findAttribute(const c8 *attributeName) const = 0;

	//! Removes all attributes
	virtual void clear() = 0;

	/*

		Integer Attribute

	*/

	//! Adds an attribute as integer
	virtual void addInt(const c8 *attributeName, s32 value) = 0;

	//! Sets an attribute as integer value
	virtual void setAttribute(const c8 *attributeName, s32 value) = 0;

	//! Gets an attribute as integer value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual s32 getAttributeAsInt(const c8 *attributeName, irr::s32 defaultNotFound = 0) const = 0;

	//! Gets an attribute as integer value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual s32 getAttributeAsInt(s32 index) const = 0;

	//! Sets an attribute as integer value
	virtual void setAttribute(s32 index, s32 value) = 0;

	/*

		Float Attribute

	*/

	//! Adds an attribute as float
	virtual void addFloat(const c8 *attributeName, f32 value) = 0;

	//! Sets a attribute as float value
	virtual void setAttribute(const c8 *attributeName, f32 value) = 0;

	//! Gets an attribute as float value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual f32 getAttributeAsFloat(const c8 *attributeName, irr::f32 defaultNotFound = 0.f) const = 0;

	//! Gets an attribute as float value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual f32 getAttributeAsFloat(s32 index) const = 0;

	//! Sets an attribute as float value
	virtual void setAttribute(s32 index, f32 value) = 0;

	/*
		Bool Attribute
	*/

	//! Adds an attribute as bool
	virtual void addBool(const c8 *attributeName, bool value) = 0;

	//! Sets an attribute as boolean value
	virtual void setAttribute(const c8 *attributeName, bool value) = 0;

	//! Gets an attribute as boolean value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	virtual bool getAttributeAsBool(const c8 *attributeName, bool defaultNotFound = false) const = 0;

	//! Gets an attribute as boolean value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	virtual bool getAttributeAsBool(s32 index) const = 0;

	//! Sets an attribute as boolean value
	virtual void setAttribute(s32 index, bool value) = 0;
};

} // end namespace io
} // end namespace irr
