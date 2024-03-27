// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IReferenceCounted.h"
#include "SColor.h"
#include "vector3d.h"
#include "vector2d.h"
#include "position2d.h"
#include "rect.h"
#include "dimension2d.h"
#include "irrString.h"
#include "irrArray.h"
#include "EAttributes.h"

namespace irr
{
namespace io
{

// All derived attribute types implement at least getter/setter for their own type (like CBoolAttribute will have setBool/getBool).
// Simple types will also implement getStringW and setString, but don't expect it to work for all types.
// String serialization makes no sense for some attribute-types (like stringw arrays or pointers), but is still useful for many types.
// (Note: I do _not_ know yet why the default string serialization is asymmetric with char* in set and wchar_t* in get).
// Additionally many attribute types will implement conversion functions like CBoolAttribute has p.E. getInt/setInt().
// The reason for conversion functions is likely to make reading old formats easier which have changed in the meantime. For example
// an old xml can contain a bool attribute which is an int in a newer format. You can still call getInt() even thought the attribute has the wrong type.
// And please do _not_ confuse these attributes here with the ones used in the xml-reader (aka SAttribute which is just a key-value pair).

class IAttribute : public virtual IReferenceCounted
{
public:
	virtual ~IAttribute(){};

	virtual s32 getInt() const { return 0; }
	virtual f32 getFloat() const { return 0; }
	virtual bool getBool() const { return false; }

	virtual void setInt(s32 intValue){};
	virtual void setFloat(f32 floatValue){};
	virtual void setBool(bool boolValue){};

	core::stringc Name;

	virtual E_ATTRIBUTE_TYPE getType() const = 0;
};

} // end namespace io
} // end namespace irr
