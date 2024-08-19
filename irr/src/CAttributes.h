// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include <map>
#include <string>
#include "IAttributes.h"
#include "IAttribute.h"

namespace irr
{

namespace io
{

//! Implementation of the IAttributes interface
class CAttributes : public IAttributes
{
public:
	CAttributes();
	~CAttributes();

	//! Returns the type of an attribute
	//! \param attributeName: Name for the attribute
	E_ATTRIBUTE_TYPE getAttributeType(const c8 *attributeName) const override;

	//! Returns if an attribute with a name exists
	bool existsAttribute(const c8 *attributeName) const override;

	//! Removes all attributes
	void clear() override;

	/*

		Integer Attribute

	*/

	//! Adds an attribute as integer
	void addInt(const c8 *attributeName, s32 value) override {
		setAttribute(attributeName, value);
	}

	//! Sets an attribute as integer value
	void setAttribute(const c8 *attributeName, s32 value) override;

	//! Gets an attribute as integer value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	s32 getAttributeAsInt(const c8 *attributeName, irr::s32 defaultNotFound = 0) const override;

	/*

		Float Attribute

	*/

	//! Adds an attribute as float
	void addFloat(const c8 *attributeName, f32 value) override {
		setAttribute(attributeName, value);
	}

	//! Sets a attribute as float value
	void setAttribute(const c8 *attributeName, f32 value) override;

	//! Gets an attribute as float value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	f32 getAttributeAsFloat(const c8 *attributeName, irr::f32 defaultNotFound = 0.f) const override;

	/*
		Bool Attribute
	*/

	//! Adds an attribute as bool
	void addBool(const c8 *attributeName, bool value) override {
		setAttribute(attributeName, value);
	}

	//! Sets an attribute as boolean value
	void setAttribute(const c8 *attributeName, bool value) override;

	//! Gets an attribute as boolean value
	//! \param attributeName: Name of the attribute to get.
	//! \param defaultNotFound Value returned when attributeName was not found
	//! \return Returns value of the attribute previously set by setAttribute()
	bool getAttributeAsBool(const c8 *attributeName, bool defaultNotFound = false) const override;

protected:
	typedef std::basic_string<c8> string;

	// specify a comparator so we can directly look up in the map with const c8*
	// (works since C++14)
	std::map<string, IAttribute*, std::less<>> Attributes;
};

} // end namespace io
} // end namespace irr
