// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CAttributes.h"
#include "CAttributeImpl.h"
#include "ITexture.h"
#include "IVideoDriver.h"

namespace irr
{
namespace io
{

CAttributes::CAttributes(video::IVideoDriver *driver) :
		Driver(driver)
{
#ifdef _DEBUG
	setDebugName("CAttributes");
#endif

	if (Driver)
		Driver->grab();
}

CAttributes::~CAttributes()
{
	clear();

	if (Driver)
		Driver->drop();
}

//! Removes all attributes
void CAttributes::clear()
{
	for (u32 i = 0; i < Attributes.size(); ++i)
		Attributes[i]->drop();

	Attributes.clear();
}

//! Returns attribute index from name, -1 if not found
s32 CAttributes::findAttribute(const c8 *attributeName) const
{
	for (u32 i = 0; i < Attributes.size(); ++i)
		if (Attributes[i]->Name == attributeName)
			return i;

	return -1;
}

IAttribute *CAttributes::getAttributeP(const c8 *attributeName) const
{
	for (u32 i = 0; i < Attributes.size(); ++i)
		if (Attributes[i]->Name == attributeName)
			return Attributes[i];

	return 0;
}

//! Sets a attribute as boolean value
void CAttributes::setAttribute(const c8 *attributeName, bool value)
{
	IAttribute *att = getAttributeP(attributeName);
	if (att)
		att->setBool(value);
	else {
		Attributes.push_back(new CBoolAttribute(attributeName, value));
	}
}

//! Gets a attribute as boolean value
//! \param attributeName: Name of the attribute to get.
//! \return Returns value of the attribute previously set by setAttribute() as bool
//! or 0 if attribute is not set.
bool CAttributes::getAttributeAsBool(const c8 *attributeName, bool defaultNotFound) const
{
	const IAttribute *att = getAttributeP(attributeName);
	if (att)
		return att->getBool();
	else
		return defaultNotFound;
}

//! Sets a attribute as integer value
void CAttributes::setAttribute(const c8 *attributeName, s32 value)
{
	IAttribute *att = getAttributeP(attributeName);
	if (att)
		att->setInt(value);
	else {
		Attributes.push_back(new CIntAttribute(attributeName, value));
	}
}

//! Gets a attribute as integer value
//! \param attributeName: Name of the attribute to get.
//! \return Returns value of the attribute previously set by setAttribute() as integer
//! or 0 if attribute is not set.
s32 CAttributes::getAttributeAsInt(const c8 *attributeName, irr::s32 defaultNotFound) const
{
	const IAttribute *att = getAttributeP(attributeName);
	if (att)
		return att->getInt();
	else
		return defaultNotFound;
}

//! Sets a attribute as float value
void CAttributes::setAttribute(const c8 *attributeName, f32 value)
{
	IAttribute *att = getAttributeP(attributeName);
	if (att)
		att->setFloat(value);
	else
		Attributes.push_back(new CFloatAttribute(attributeName, value));
}

//! Gets a attribute as integer value
//! \param attributeName: Name of the attribute to get.
//! \return Returns value of the attribute previously set by setAttribute() as float value
//! or 0 if attribute is not set.
f32 CAttributes::getAttributeAsFloat(const c8 *attributeName, irr::f32 defaultNotFound) const
{
	const IAttribute *att = getAttributeP(attributeName);
	if (att)
		return att->getFloat();

	return defaultNotFound;
}

//! Returns amount of string attributes set in this scene manager.
u32 CAttributes::getAttributeCount() const
{
	return Attributes.size();
}

//! Returns string attribute name by index.
//! \param index: Index value, must be between 0 and getStringAttributeCount()-1.
const c8 *CAttributes::getAttributeName(s32 index) const
{
	if ((u32)index >= Attributes.size())
		return 0;

	return Attributes[index]->Name.c_str();
}

//! Returns the type of an attribute
E_ATTRIBUTE_TYPE CAttributes::getAttributeType(const c8 *attributeName) const
{
	E_ATTRIBUTE_TYPE ret = EAT_UNKNOWN;

	const IAttribute *att = getAttributeP(attributeName);
	if (att)
		ret = att->getType();

	return ret;
}

//! Returns attribute type by index.
//! \param index: Index value, must be between 0 and getAttributeCount()-1.
E_ATTRIBUTE_TYPE CAttributes::getAttributeType(s32 index) const
{
	if ((u32)index >= Attributes.size())
		return EAT_UNKNOWN;

	return Attributes[index]->getType();
}

//! Gets an attribute as integer value
//! \param index: Index value, must be between 0 and getAttributeCount()-1.
s32 CAttributes::getAttributeAsInt(s32 index) const
{
	if ((u32)index < Attributes.size())
		return Attributes[index]->getInt();
	else
		return 0;
}

//! Gets an attribute as float value
//! \param index: Index value, must be between 0 and getAttributeCount()-1.
f32 CAttributes::getAttributeAsFloat(s32 index) const
{
	if ((u32)index < Attributes.size())
		return Attributes[index]->getFloat();
	else
		return 0.f;
}

//! Gets an attribute as boolean value
//! \param index: Index value, must be between 0 and getAttributeCount()-1.
bool CAttributes::getAttributeAsBool(s32 index) const
{
	bool ret = false;

	if ((u32)index < Attributes.size())
		ret = Attributes[index]->getBool();

	return ret;
}

//! Adds an attribute as integer
void CAttributes::addInt(const c8 *attributeName, s32 value)
{
	Attributes.push_back(new CIntAttribute(attributeName, value));
}

//! Adds an attribute as float
void CAttributes::addFloat(const c8 *attributeName, f32 value)
{
	Attributes.push_back(new CFloatAttribute(attributeName, value));
}

//! Adds an attribute as bool
void CAttributes::addBool(const c8 *attributeName, bool value)
{
	Attributes.push_back(new CBoolAttribute(attributeName, value));
}

//! Returns if an attribute with a name exists
bool CAttributes::existsAttribute(const c8 *attributeName) const
{
	return getAttributeP(attributeName) != 0;
}

//! Sets an attribute as boolean value
void CAttributes::setAttribute(s32 index, bool value)
{
	if ((u32)index < Attributes.size())
		Attributes[index]->setBool(value);
}

//! Sets an attribute as integer value
void CAttributes::setAttribute(s32 index, s32 value)
{
	if ((u32)index < Attributes.size())
		Attributes[index]->setInt(value);
}

//! Sets a attribute as float value
void CAttributes::setAttribute(s32 index, f32 value)
{
	if ((u32)index < Attributes.size())
		Attributes[index]->setFloat(value);
}

} // end namespace io
} // end namespace irr
