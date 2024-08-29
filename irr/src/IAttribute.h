// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "irrTypes.h"
#include "irrString.h"
#include "EAttributes.h"

namespace irr
{
namespace io
{

class IAttribute
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
