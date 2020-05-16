// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_ATTRIBUTE_H_INCLUDED__
#define __I_ATTRIBUTE_H_INCLUDED__

#include "IReferenceCounted.h"
#include "SColor.h"
#include "vector3d.h"
#include "vector2d.h"
#include "line2d.h"
#include "line3d.h"
#include "triangle3d.h"
#include "position2d.h"
#include "rect.h"
#include "dimension2d.h"
#include "matrix4.h"
#include "quaternion.h"
#include "plane3d.h"
#include "triangle3d.h"
#include "line2d.h"
#include "line3d.h"
#include "irrString.h"
#include "irrArray.h"
#include "EAttributes.h"


namespace irr
{
namespace io
{

class IAttribute : public virtual IReferenceCounted
{
public:

	virtual ~IAttribute() {};

	virtual s32 getInt()				{ return 0; }
	virtual f32 getFloat()				{ return 0; }
	virtual video::SColorf getColorf()		{ return video::SColorf(1.0f,1.0f,1.0f,1.0f); }
	virtual video::SColor getColor()		{ return video::SColor(255,255,255,255); }
	virtual core::stringc getString()		{ return core::stringc(getStringW().c_str()); }
	virtual core::stringw getStringW()		{ return core::stringw(); }
	virtual core::array<core::stringw> getArray()	{ return core::array<core::stringw>(); };
	virtual bool getBool()				{ return false; }
	virtual void getBinary(void* outdata, s32 maxLength) {};
	virtual core::vector3df getVector()		{ return core::vector3df(); }
	virtual core::position2di getPosition()	{ return core::position2di(); }
	virtual core::rect<s32> getRect()		{ return core::rect<s32>(); }
	virtual core::quaternion getQuaternion(){ return core::quaternion(); }
	virtual core::matrix4 getMatrix()		{ return core::matrix4(); }
	virtual core::triangle3df getTriangle()		{ return core::triangle3df(); }
	virtual core::vector2df getVector2d()		{ return core::vector2df(); }
	virtual core::vector2di getVector2di()		{ return core::vector2di(); }
	virtual core::line2df getLine2d()		{ return core::line2df(); }
	virtual core::line2di getLine2di()		{ return core::line2di(); }
	virtual core::line3df getLine3d()		{ return core::line3df(); }
	virtual core::line3di getLine3di()		{ return core::line3di(); }
	virtual core::dimension2du getDimension2d()	{ return core::dimension2du(); }
	virtual core::aabbox3d<f32> getBBox()		{ return core::aabbox3d<f32>(); }
	virtual core::plane3df getPlane()		{ return core::plane3df(); }

	virtual video::ITexture* getTexture()		{ return 0; }
	virtual const char* getEnum()			{ return 0; }
	virtual void* getUserPointer()			{ return 0; }

	virtual void setInt(s32 intValue)		{};
	virtual void setFloat(f32 floatValue)		{};
	virtual void setString(const char* text)	{};
	virtual void setString(const wchar_t* text){ setString(core::stringc(text).c_str()); };
	virtual void setArray(const core::array<core::stringw>& arr )	{};
	virtual void setColor(video::SColorf color)	{};
	virtual void setColor(video::SColor color)	{};
	virtual void setBool(bool boolValue)		{};
	virtual void setBinary(void* data, s32 maxLenght) {};
	virtual void setVector(core::vector3df v)	{};
	virtual void setPosition(core::position2di v)	{};
	virtual void setRect(core::rect<s32> v)		{};
	virtual void setQuaternion(core::quaternion v) {};
	virtual void setMatrix(core::matrix4 v) {};
	virtual void setTriangle(core::triangle3df v) {};
	virtual void setVector2d(core::vector2df v) {};
	virtual void setVector2d(core::vector2di v) {};
	virtual void setLine2d(core::line2df v) {};
	virtual void setLine2d(core::line2di v) {};
	virtual void setLine3d(core::line3df v) {};
	virtual void setLine3d(core::line3di v) {};
	virtual void setDimension2d(core::dimension2du v) {};
	virtual void setBBox(core::aabbox3d<f32> v) {};
	virtual void setPlane(core::plane3df v) {};
	virtual void setUserPointer(void* v)	{};

	virtual void setEnum(const char* enumValue, const char* const* enumerationLiterals) {};
	virtual void setTexture(video::ITexture*, const path& filename)	{};

	core::stringc Name;

	virtual E_ATTRIBUTE_TYPE getType() const = 0;
	virtual const wchar_t* getTypeString() const = 0;
};

} // end namespace io
} // end namespace irr

#endif
