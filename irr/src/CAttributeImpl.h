// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CAttributes.h"
#include "fast_atof.h"
#include "ITexture.h"
#include "IVideoDriver.h"

namespace irr
{
namespace io
{

/*
	Basic types, check documentation in IAttribute.h to see how they generally work.
*/

// Attribute implemented for boolean values
class CBoolAttribute : public IAttribute
{
public:
	CBoolAttribute(const char *name, bool value)
	{
		Name = name;
		setBool(value);
	}

	s32 getInt() const override
	{
		return BoolValue ? 1 : 0;
	}

	f32 getFloat() const override
	{
		return BoolValue ? 1.0f : 0.0f;
	}

	bool getBool() const override
	{
		return BoolValue;
	}

	void setInt(s32 intValue) override
	{
		BoolValue = (intValue != 0);
	}

	void setFloat(f32 floatValue) override
	{
		BoolValue = (floatValue != 0);
	}

	void setBool(bool boolValue) override
	{
		BoolValue = boolValue;
	}

	E_ATTRIBUTE_TYPE getType() const override
	{
		return EAT_BOOL;
	}

	bool BoolValue;
};

// Attribute implemented for integers
class CIntAttribute : public IAttribute
{
public:
	CIntAttribute(const char *name, s32 value)
	{
		Name = name;
		setInt(value);
	}

	s32 getInt() const override
	{
		return Value;
	}

	f32 getFloat() const override
	{
		return (f32)Value;
	}

	void setInt(s32 intValue) override
	{
		Value = intValue;
	}

	void setFloat(f32 floatValue) override
	{
		Value = (s32)floatValue;
	};

	E_ATTRIBUTE_TYPE getType() const override
	{
		return EAT_INT;
	}

	s32 Value;
};

// Attribute implemented for floats
class CFloatAttribute : public IAttribute
{
public:
	CFloatAttribute(const char *name, f32 value)
	{
		Name = name;
		setFloat(value);
	}

	s32 getInt() const override
	{
		return (s32)Value;
	}

	f32 getFloat() const override
	{
		return Value;
	}

	void setInt(s32 intValue) override
	{
		Value = (f32)intValue;
	}

	void setFloat(f32 floatValue) override
	{
		Value = floatValue;
	}

	E_ATTRIBUTE_TYPE getType() const override
	{
		return EAT_FLOAT;
	}

	f32 Value;
};

} // end namespace io
} // end namespace irr
