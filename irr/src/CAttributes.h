// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

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
	CAttributes(video::IVideoDriver *driver = 0);
	~CAttributes();

	//! Returns amount of attributes in this collection of attributes.
	u32 getAttributeCount() const override;

	//! Returns attribute name by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	const c8 *getAttributeName(s32 index) const override;

	//! Returns the type of an attribute
	//! \param attributeName: Name for the attribute
	E_ATTRIBUTE_TYPE getAttributeType(const c8 *attributeName) const override;

	//! Returns attribute type by index.
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	E_ATTRIBUTE_TYPE getAttributeType(s32 index) const override;

	//! Returns if an attribute with a name exists
	bool existsAttribute(const c8 *attributeName) const override;

	//! Returns attribute index from name, -1 if not found
	s32 findAttribute(const c8 *attributeName) const override;

	//! Removes all attributes
	void clear() override;

	/*

		Integer Attribute

	*/

	//! Adds an attribute as integer
	void addInt(const c8 *attributeName, s32 value) override;

	//! Sets an attribute as integer value
	void setAttribute(const c8 *attributeName, s32 value) override;

	//! Gets an attribute as integer value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	s32 getAttributeAsInt(const c8 *attributeName, irr::s32 defaultNotFound = 0) const override;

	//! Gets an attribute as integer value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	s32 getAttributeAsInt(s32 index) const override;

	//! Sets an attribute as integer value
	void setAttribute(s32 index, s32 value) override;

	/*

		Float Attribute

	*/

	//! Adds an attribute as float
	void addFloat(const c8 *attributeName, f32 value) override;

	//! Sets a attribute as float value
	void setAttribute(const c8 *attributeName, f32 value) override;

	//! Gets an attribute as float value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	f32 getAttributeAsFloat(const c8 *attributeName, irr::f32 defaultNotFound = 0.f) const override;

	//! Gets an attribute as float value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	f32 getAttributeAsFloat(s32 index) const override;

	//! Sets an attribute as float value
	void setAttribute(s32 index, f32 value) override;

	/*
		Bool Attribute
	*/

	//! Adds an attribute as bool
	void addBool(const c8 *attributeName, bool value) override;

	//! Sets an attribute as boolean value
	void setAttribute(const c8 *attributeName, bool value) override;

	//! Gets an attribute as boolean value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	bool getAttributeAsBool(const c8 *attributeName, bool defaultNotFound = false) const override;

	//! Gets an attribute as boolean value
	//! \param index: Index value, must be between 0 and getAttributeCount()-1.
	bool getAttributeAsBool(s32 index) const override;

	//! Sets an attribute as boolean value
	void setAttribute(s32 index, bool value) override;

protected:
	core::array<IAttribute *> Attributes;

	IAttribute *getAttributeP(const c8 *attributeName) const;

	video::IVideoDriver *Driver;
};

} // end namespace io
} // end namespace irr
