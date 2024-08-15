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

CAttributes::CAttributes()
{
#ifdef _DEBUG
	setDebugName("CAttributes");
#endif
}

CAttributes::~CAttributes()
{
	clear();
}

//! Removes all attributes
void CAttributes::clear()
{
	for (auto it : Attributes)
		delete it.second;
	Attributes.clear();
}

IAttribute *CAttributes::getAttributeP(const c8 *attributeName) const
{
	auto it = Attributes.find(attributeName);
	if (it == Attributes.end())
		return nullptr;
	return const_cast<IAttribute*>(it->second); // who cares
}

//! Sets a attribute as boolean value
void CAttributes::setAttribute(const c8 *attributeName, bool value)
{
	IAttribute *att = getAttributeP(attributeName);
	if (att)
		att->setBool(value);
	else {
		Attributes[attributeName] = new CBoolAttribute(attributeName, value);
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
		Attributes[attributeName] = new CIntAttribute(attributeName, value);
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
		Attributes[attributeName] = new CFloatAttribute(attributeName, value);
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

//! Returns the type of an attribute
E_ATTRIBUTE_TYPE CAttributes::getAttributeType(const c8 *attributeName) const
{
	E_ATTRIBUTE_TYPE ret = EAT_UNKNOWN;

	const IAttribute *att = getAttributeP(attributeName);
	if (att)
		ret = att->getType();

	return ret;
}

//! Returns if an attribute with a name exists
bool CAttributes::existsAttribute(const c8 *attributeName) const
{
	return !!getAttributeP(attributeName);
}

} // end namespace io
} // end namespace irr
