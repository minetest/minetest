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
	basic types
*/

// Attribute implemented for boolean values
class CBoolAttribute : public IAttribute
{
public:

	CBoolAttribute(const char* name, bool value)
	{
		Name = name;
		setBool(value);
	}

	virtual s32 getInt()
	{
		return BoolValue ? 1 : 0;
	}

	virtual f32 getFloat()
	{
		return BoolValue ? 1.0f : 0.0f;
	}

	virtual bool getBool()
	{
		return BoolValue;
	}

	virtual core::stringw getStringW()
	{
		return core::stringw( BoolValue ? L"true" : L"false" );
	}

	virtual void setInt(s32 intValue)
	{
		BoolValue = (intValue != 0);
	}

	virtual void setFloat(f32 floatValue)
	{
		BoolValue = (floatValue != 0);
	}

	virtual void setBool(bool boolValue)
	{
		BoolValue = boolValue;
	}

	virtual void setString(const char* string)
	{
		BoolValue = strcmp(string, "true") == 0;
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_BOOL;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"bool";
	}

	bool BoolValue;
};

// Attribute implemented for integers
class CIntAttribute : public IAttribute
{
public:

	CIntAttribute(const char* name, s32 value)
	{
		Name = name;
		setInt(value);
	}

	virtual s32 getInt()
	{
		return Value;
	}

	virtual f32 getFloat()
	{
		return (f32)Value;
	}

	virtual bool getBool()
	{
		return (Value != 0);
	}

	virtual core::stringw getStringW()
	{
		return core::stringw(Value);
	}

	virtual void setInt(s32 intValue)
	{
		Value = intValue;
	}

	virtual void setFloat(f32 floatValue)
	{
		Value = (s32)floatValue;
	};

	virtual void setString(const char* text)
	{
		Value = atoi(text);
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_INT;
	}


	virtual const wchar_t* getTypeString() const
	{
		return L"int";
	}

	s32 Value;
};

// Attribute implemented for floats
class CFloatAttribute : public IAttribute
{
public:

	CFloatAttribute(const char* name, f32 value)
	{
		Name = name;
		setFloat(value);
	}

	virtual s32 getInt()
	{
		return (s32)Value;
	}

	virtual f32 getFloat()
	{
		return Value;
	}

	virtual bool getBool()
	{
		return (Value != 0);
	}

	virtual core::stringw getStringW()
	{
		return core::stringw((double)Value);
	}

	virtual void setInt(s32 intValue)
	{
		Value = (f32)intValue;
	}

	virtual void setFloat(f32 floatValue)
	{
		Value = floatValue;
	}

	virtual void setString(const char* text)
	{
		Value = core::fast_atof(text);
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_FLOAT;
	}


	virtual const wchar_t* getTypeString() const
	{
		return L"float";
	}

	f32 Value;
};



/*
	Types which can be represented as a list of numbers
*/

// Base class for all attributes which are a list of numbers-
// vectors, colors, positions, triangles, etc
class CNumbersAttribute : public IAttribute
{
public:

	CNumbersAttribute(const char* name, video::SColorf value) :
		ValueI(), ValueF(), Count(4), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.r);
		ValueF.push_back(value.g);
		ValueF.push_back(value.b);
		ValueF.push_back(value.a);
	}

	CNumbersAttribute(const char* name, video::SColor value) :
		ValueI(), ValueF(), Count(4), IsFloat(false)
	{
		Name = name;
		ValueI.push_back(value.getRed());
		ValueI.push_back(value.getGreen());
		ValueI.push_back(value.getBlue());
		ValueI.push_back(value.getAlpha());
	}


	CNumbersAttribute(const char* name, core::vector3df value) :
		ValueI(), ValueF(), Count(3), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.X);
		ValueF.push_back(value.Y);
		ValueF.push_back(value.Z);
	}

	CNumbersAttribute(const char* name, core::rect<s32> value) :
		ValueI(), ValueF(), Count(4), IsFloat(false)
	{
		Name = name;
		ValueI.push_back(value.UpperLeftCorner.X);
		ValueI.push_back(value.UpperLeftCorner.Y);
		ValueI.push_back(value.LowerRightCorner.X);
		ValueI.push_back(value.LowerRightCorner.Y);
	}

	CNumbersAttribute(const char* name, core::rect<f32> value) :
		ValueI(), ValueF(), Count(4), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.UpperLeftCorner.X);
		ValueF.push_back(value.UpperLeftCorner.Y);
		ValueF.push_back(value.LowerRightCorner.X);
		ValueF.push_back(value.LowerRightCorner.Y);
	}

	CNumbersAttribute(const char* name, core::matrix4 value) :
		ValueI(), ValueF(), Count(16), IsFloat(true)
	{
		Name = name;
		for (s32 r=0; r<4; ++r)
			for (s32 c=0; c<4; ++c)
				ValueF.push_back(value(r,c));
	}

	CNumbersAttribute(const char* name, core::quaternion value) :
		ValueI(), ValueF(), Count(4), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.X);
		ValueF.push_back(value.Y);
		ValueF.push_back(value.Z);
		ValueF.push_back(value.W);
	}

	CNumbersAttribute(const char* name, core::aabbox3d<f32> value) :
		ValueI(), ValueF(), Count(6), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.MinEdge.X);
		ValueF.push_back(value.MinEdge.Y);
		ValueF.push_back(value.MinEdge.Z);
		ValueF.push_back(value.MaxEdge.X);
		ValueF.push_back(value.MaxEdge.Y);
		ValueF.push_back(value.MaxEdge.Z);
	}

	CNumbersAttribute(const char* name, core::plane3df value) :
		ValueI(), ValueF(), Count(4), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.Normal.X);
		ValueF.push_back(value.Normal.Y);
		ValueF.push_back(value.Normal.Z);
		ValueF.push_back(value.D);
	}

	CNumbersAttribute(const char* name, core::triangle3df value) :
		ValueI(), ValueF(), Count(9), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.pointA.X);
		ValueF.push_back(value.pointA.Y);
		ValueF.push_back(value.pointA.Z);
		ValueF.push_back(value.pointB.X);
		ValueF.push_back(value.pointB.Y);
		ValueF.push_back(value.pointB.Z);
		ValueF.push_back(value.pointC.X);
		ValueF.push_back(value.pointC.Y);
		ValueF.push_back(value.pointC.Z);
	}

	CNumbersAttribute(const char* name, core::vector2df value) :
		ValueI(), ValueF(), Count(2), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.X);
		ValueF.push_back(value.Y);
	}

	CNumbersAttribute(const char* name, core::vector2di value) :
		ValueI(), ValueF(), Count(2), IsFloat(false)
	{
		Name = name;
		ValueI.push_back(value.X);
		ValueI.push_back(value.Y);
	}

	CNumbersAttribute(const char* name, core::line2di value) :
		ValueI(), ValueF(), Count(4), IsFloat(false)
	{
		Name = name;
		ValueI.push_back(value.start.X);
		ValueI.push_back(value.start.Y);
		ValueI.push_back(value.end.X);
		ValueI.push_back(value.end.Y);
	}

	CNumbersAttribute(const char* name, core::line2df value) :
		ValueI(), ValueF(), Count(4), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.start.X);
		ValueF.push_back(value.start.Y);
		ValueF.push_back(value.end.X);
		ValueF.push_back(value.end.Y);
	}

	CNumbersAttribute(const char* name, core::line3df value) :
		ValueI(), ValueF(), Count(6), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.start.X);
		ValueF.push_back(value.start.Y);
		ValueF.push_back(value.start.Z);
		ValueF.push_back(value.end.X);
		ValueF.push_back(value.end.Y);
		ValueF.push_back(value.end.Z);
	}

	CNumbersAttribute(const char* name, core::dimension2du value) :
		ValueI(), ValueF(), Count(2), IsFloat(false)
	{
		Name = name;
		ValueI.push_back(value.Width);
		ValueI.push_back(value.Height);
	}


	CNumbersAttribute(const char* name, core::dimension2df value) :
		ValueI(), ValueF(), Count(2), IsFloat(true)
	{
		Name = name;
		ValueF.push_back(value.Width);
		ValueF.push_back(value.Height);
	}



	// getting values
	virtual s32 getInt()
	{
		if (Count==0)
			return 0;

		if (IsFloat)
			return (s32)ValueF[0];
		else
			return ValueI[0];
	}

	virtual f32 getFloat()
	{
		if (Count==0)
			return 0.0f;

		if (IsFloat)
			return ValueF[0];
		else
			return (f32)ValueI[0];
	}

	virtual bool getBool()
	{
		// return true if any number is nonzero
		bool ret=false;

		for (u32 i=0; i < Count; ++i)
			if ( IsFloat ? (ValueF[i] != 0) : (ValueI[i] != 0) )
			{
				ret=true;
				break;
			}

		return ret;

	}


	virtual core::stringc getString()
	{
		core::stringc outstr;

		for (u32 i=0; i <Count; ++i)
		{
			if (IsFloat)
				outstr += ValueF[i];
			else
				outstr += ValueI[i];

			if (i < Count-1)
				outstr += ", ";
		}
		return outstr;
	}
	virtual core::stringw getStringW()
	{
		core::stringw outstr;

		for (u32 i=0; i <Count; ++i)
		{
			if (IsFloat)
				outstr += ValueF[i];
			else
				outstr += ValueI[i];

			if (i < Count-1)
				outstr += L", ";
		}
		return outstr;
	}

	virtual core::position2di getPosition()
	{
		core::position2di p;

		if (IsFloat)
		{
			p.X = (s32)(Count > 0 ? ValueF[0] : 0);
			p.Y = (s32)(Count > 1 ? ValueF[1] : 0);
		}
		else
		{
			p.X = Count > 0 ? ValueI[0] : 0;
			p.Y = Count > 1 ? ValueI[1] : 0;
		}

		return p;
	}

	virtual core::vector3df getVector()
	{
		core::vector3df v;

		if (IsFloat)
		{
			v.X = Count > 0 ? ValueF[0] : 0;
			v.Y = Count > 1 ? ValueF[1] : 0;
			v.Z = Count > 2 ? ValueF[2] : 0;
		}
		else
		{
			v.X = (f32)(Count > 0 ? ValueI[0] : 0);
			v.Y = (f32)(Count > 1 ? ValueI[1] : 0);
			v.Z = (f32)(Count > 2 ? ValueI[2] : 0);
		}

		return v;
	}

	virtual core::vector2df getVector2d()
	{
		core::vector2df v;

		if (IsFloat)
		{
			v.X = Count > 0 ? ValueF[0] : 0;
			v.Y = Count > 1 ? ValueF[1] : 0;
		}
		else
		{
			v.X = (f32)(Count > 0 ? ValueI[0] : 0);
			v.Y = (f32)(Count > 1 ? ValueI[1] : 0);
		}

		return v;
	}

	virtual video::SColorf getColorf()
	{
		video::SColorf c;
		if (IsFloat)
		{
			c.setColorComponentValue(0, Count > 0 ? ValueF[0] : 0);
			c.setColorComponentValue(1, Count > 1 ? ValueF[1] : 0);
			c.setColorComponentValue(2, Count > 2 ? ValueF[2] : 0);
			c.setColorComponentValue(3, Count > 3 ? ValueF[3] : 0);
		}
		else
		{
			c.setColorComponentValue(0, Count > 0 ? (f32)(ValueI[0]) / 255.0f : 0);
			c.setColorComponentValue(1, Count > 1 ? (f32)(ValueI[1]) / 255.0f : 0);
			c.setColorComponentValue(2, Count > 2 ? (f32)(ValueI[2]) / 255.0f : 0);
			c.setColorComponentValue(3, Count > 3 ? (f32)(ValueI[3]) / 255.0f : 0);
		}

		return c;
	}

	virtual video::SColor getColor()
	{
		return getColorf().toSColor();
	}


	virtual core::rect<s32> getRect()
	{
		core::rect<s32> r;

		if (IsFloat)
		{
			r.UpperLeftCorner.X  = (s32)(Count > 0 ? ValueF[0] : 0);
			r.UpperLeftCorner.Y  = (s32)(Count > 1 ? ValueF[1] : 0);
			r.LowerRightCorner.X = (s32)(Count > 2 ? ValueF[2] : r.UpperLeftCorner.X);
			r.LowerRightCorner.Y = (s32)(Count > 3 ? ValueF[3] : r.UpperLeftCorner.Y);
		}
		else
		{
			r.UpperLeftCorner.X  = Count > 0 ? ValueI[0] : 0;
			r.UpperLeftCorner.Y  = Count > 1 ? ValueI[1] : 0;
			r.LowerRightCorner.X = Count > 2 ? ValueI[2] : r.UpperLeftCorner.X;
			r.LowerRightCorner.Y = Count > 3 ? ValueI[3] : r.UpperLeftCorner.Y;
		}
		return r;
	}

	virtual core::dimension2du getDimension2d()
	{
		core::dimension2d<u32> dim;

		if (IsFloat)
		{
			dim.Width = (u32)(Count > 0 ? ValueF[0] : 0);
			dim.Height = (u32)(Count > 1 ? ValueF[1] : 0);
		}
		else
		{
			dim.Width = (u32)(Count > 0 ? ValueI[0] : 0);
			dim.Height = (u32)(Count > 1 ? ValueI[1] : 0);
		}
		return dim;
	}

	virtual core::matrix4 getMatrix()
	{
		core::matrix4 ret;
		if (IsFloat)
		{
			for (u32 r=0; r<4; ++r)
				for (u32 c=0; c<4; ++c)
					if (Count > c+r*4)
						ret(r,c) = ValueF[c+r*4];
		}
		else
		{
			for (u32 r=0; r<4; ++r)
				for (u32 c=0; c<4; ++c)
					if (Count > c+r*4)
						ret(r,c) = (f32)ValueI[c+r*4];
		}
		return ret;
	}

	virtual core::quaternion getQuaternion()
	{
		core::quaternion ret;
		if (IsFloat)
		{
			ret.X = Count > 0 ? ValueF[0] : 0.0f;
			ret.Y = Count > 1 ? ValueF[1] : 0.0f;
			ret.Z = Count > 2 ? ValueF[2] : 0.0f;
			ret.W = Count > 3 ? ValueF[3] : 0.0f;
		}
		else
		{
			ret.X = Count > 0 ? (f32)ValueI[0] : 0.0f;
			ret.Y = Count > 1 ? (f32)ValueI[1] : 0.0f;
			ret.Z = Count > 2 ? (f32)ValueI[2] : 0.0f;
			ret.W = Count > 3 ? (f32)ValueI[3] : 0.0f;
		}
		return ret;
	}

	virtual core::triangle3df getTriangle()
	{
		core::triangle3df ret;

		if (IsFloat)
		{
			ret.pointA.X = Count > 0 ? ValueF[0] : 0.0f;
			ret.pointA.Y = Count > 1 ? ValueF[1] : 0.0f;
			ret.pointA.Z = Count > 2 ? ValueF[2] : 0.0f;
			ret.pointB.X = Count > 3 ? ValueF[3] : 0.0f;
			ret.pointB.Y = Count > 4 ? ValueF[4] : 0.0f;
			ret.pointB.Z = Count > 5 ? ValueF[5] : 0.0f;
			ret.pointC.X = Count > 6 ? ValueF[6] : 0.0f;
			ret.pointC.Y = Count > 7 ? ValueF[7] : 0.0f;
			ret.pointC.Z = Count > 8 ? ValueF[8] : 0.0f;
		}
		else
		{
			ret.pointA.X = Count > 0 ? (f32)ValueI[0] : 0.0f;
			ret.pointA.Y = Count > 1 ? (f32)ValueI[1] : 0.0f;
			ret.pointA.Z = Count > 2 ? (f32)ValueI[2] : 0.0f;
			ret.pointB.X = Count > 3 ? (f32)ValueI[3] : 0.0f;
			ret.pointB.Y = Count > 4 ? (f32)ValueI[4] : 0.0f;
			ret.pointB.Z = Count > 5 ? (f32)ValueI[5] : 0.0f;
			ret.pointC.X = Count > 6 ? (f32)ValueI[6] : 0.0f;
			ret.pointC.Y = Count > 7 ? (f32)ValueI[7] : 0.0f;
			ret.pointC.Z = Count > 8 ? (f32)ValueI[8] : 0.0f;
		}

		return ret;
	}

	virtual core::plane3df getPlane()
	{
		core::plane3df ret;

		if (IsFloat)
		{
			ret.Normal.X = Count > 0 ? ValueF[0] : 0.0f;
			ret.Normal.Y = Count > 1 ? ValueF[1] : 0.0f;
			ret.Normal.Z = Count > 2 ? ValueF[2] : 0.0f;
			ret.D		 = Count > 3 ? ValueF[3] : 0.0f;
		}
		else
		{
			ret.Normal.X = Count > 0 ? (f32)ValueI[0] : 0.0f;
			ret.Normal.Y = Count > 1 ? (f32)ValueI[1] : 0.0f;
			ret.Normal.Z = Count > 2 ? (f32)ValueI[2] : 0.0f;
			ret.D		 = Count > 3 ? (f32)ValueI[3] : 0.0f;
		}

		return ret;
	}

	virtual core::aabbox3df getBBox()
	{
		core::aabbox3df ret;
		if (IsFloat)
		{
			ret.MinEdge.X = Count > 0 ? ValueF[0] : 0.0f;
			ret.MinEdge.Y = Count > 1 ? ValueF[1] : 0.0f;
			ret.MinEdge.Z = Count > 2 ? ValueF[2] : 0.0f;
			ret.MaxEdge.X = Count > 3 ? ValueF[3] : 0.0f;
			ret.MaxEdge.Y = Count > 4 ? ValueF[4] : 0.0f;
			ret.MaxEdge.Z = Count > 5 ? ValueF[5] : 0.0f;
		}
		else
		{
			ret.MinEdge.X = Count > 0 ? (f32)ValueI[0] : 0.0f;
			ret.MinEdge.Y = Count > 1 ? (f32)ValueI[1] : 0.0f;
			ret.MinEdge.Z = Count > 2 ? (f32)ValueI[2] : 0.0f;
			ret.MaxEdge.X = Count > 3 ? (f32)ValueI[3] : 0.0f;
			ret.MaxEdge.Y = Count > 4 ? (f32)ValueI[4] : 0.0f;
			ret.MaxEdge.Z = Count > 5 ? (f32)ValueI[5] : 0.0f;
		}
		return ret;

	}

	virtual core::line2df getLine2d()
	{
		core::line2df ret;
		if (IsFloat)
		{
			ret.start.X = Count > 0 ? ValueF[0] : 0.0f;
			ret.start.Y = Count > 1 ? ValueF[1] : 0.0f;
			ret.end.X   = Count > 2 ? ValueF[2] : 0.0f;
			ret.end.Y   = Count > 3 ? ValueF[3] : 0.0f;
		}
		else
		{
			ret.start.X = Count > 0 ? (f32)ValueI[0] : 0.0f;
			ret.start.Y = Count > 1 ? (f32)ValueI[1] : 0.0f;
			ret.end.X   = Count > 2 ? (f32)ValueI[2] : 0.0f;
			ret.end.Y   = Count > 3 ? (f32)ValueI[3] : 0.0f;
		}
		return ret;
	}

	virtual core::line3df getLine3d()
	{
		core::line3df ret;
		if (IsFloat)
		{
			ret.start.X = Count > 0 ? ValueF[0] : 0.0f;
			ret.start.Y = Count > 1 ? ValueF[1] : 0.0f;
			ret.start.Z = Count > 2 ? ValueF[2] : 0.0f;
			ret.end.X   = Count > 3 ? ValueF[3] : 0.0f;
			ret.end.Y   = Count > 4 ? ValueF[4] : 0.0f;
			ret.end.Z   = Count > 5 ? ValueF[5] : 0.0f;
		}
		else
		{
			ret.start.X = Count > 0 ? (f32)ValueI[0] : 0.0f;
			ret.start.Y = Count > 1 ? (f32)ValueI[1] : 0.0f;
			ret.start.Z = Count > 2 ? (f32)ValueI[2] : 0.0f;
			ret.end.X   = Count > 3 ? (f32)ValueI[3] : 0.0f;
			ret.end.Y   = Count > 4 ? (f32)ValueI[4] : 0.0f;
			ret.end.Z   = Count > 5 ? (f32)ValueI[5] : 0.0f;
		}
		return ret;
	}

	//! get float array
	virtual core::array<f32> getFloatArray()
	{
		if (!IsFloat)
		{
			ValueF.clear();
			for (u32 i=0; i<Count; ++i)
				ValueF.push_back( (f32) ValueI[i] );
		}
		return ValueF;
	}

	//! get int array
	virtual core::array<s32> getIntArray()
	{
		if (IsFloat)
		{
			ValueI.clear();
			for (u32 i=0; i<Count; ++i)
				ValueI.push_back( (s32) ValueF[i] );
		}
		return ValueI;
	}


	// setting values
	virtual void setInt(s32 intValue)
	{
		// set all values
		for (u32 i=0; i < Count; ++i)
			if (IsFloat)
				ValueF[i] = (f32)intValue;
			else
				ValueI[i] = intValue;
	}

	virtual void setFloat(f32 floatValue)
	{
		// set all values
		for (u32 i=0; i < Count; ++i)
			if (IsFloat)
				ValueF[i] = floatValue;
			else
				ValueI[i] = (s32)floatValue;
	}

	virtual void setBool(bool boolValue)
	{
		setInt( boolValue ? 1 : 0);
	}

	virtual void setString(const char* text)
	{
		// parse text

		const char* P = (const char*)text;

		reset();

		u32 i=0;

		for ( i=0; i<Count && *P; ++i )
		{
			while(*P && P[0]!='-' && ( P[0]==' ' || (P[0] < '0' || P[0] > '9') ) )
				++P;

			// set value
			if ( *P)
			{
				if (IsFloat)
				{
					f32 c = 0;
					P = core::fast_atof_move(P, c);
					ValueF[i] = c;
				}
				else
				{
					// todo: fix this to read ints properly
					f32 c = 0;
					P = core::fast_atof_move(P, c);
					ValueI[i] = (s32)c;

				}
			}
		}
		// todo: warning message
		//if (i < Count-1)
		//{
		//
		//}
	}

	virtual void setPosition(core::position2di v)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = (f32)v.X;
			if (Count > 1) ValueF[1] = (f32)v.Y;
		}
		else
		{
			if (Count > 0) ValueI[0] = v.X;
			if (Count > 1) ValueI[1] = v.Y;
		}
	}

	virtual void setVector(core::vector3df v)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = v.X;
			if (Count > 1) ValueF[1] = v.Y;
			if (Count > 2) ValueF[2] = v.Z;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)v.X;
			if (Count > 1) ValueI[1] = (s32)v.Y;
			if (Count > 2) ValueI[2] = (s32)v.Z;
		}
	}

	virtual void setColor(video::SColorf color)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = color.r;
			if (Count > 1) ValueF[1] = color.g;
			if (Count > 2) ValueF[2] = color.b;
			if (Count > 3) ValueF[3] = color.a;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)(color.r * 255);
			if (Count > 1) ValueI[1] = (s32)(color.g * 255);
			if (Count > 2) ValueI[2] = (s32)(color.b * 255);
			if (Count > 3) ValueI[3] = (s32)(color.a * 255);
		}

	}

	virtual void setColor(video::SColor color)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = (f32)color.getRed() / 255.0f;
			if (Count > 1) ValueF[1] = (f32)color.getGreen() / 255.0f;
			if (Count > 2) ValueF[2] = (f32)color.getBlue() / 255.0f;
			if (Count > 3) ValueF[3] = (f32)color.getAlpha() / 255.0f;
		}
		else
		{
			if (Count > 0) ValueI[0] = color.getRed();
			if (Count > 1) ValueI[1] = color.getGreen();
			if (Count > 2) ValueI[2] = color.getBlue();
			if (Count > 3) ValueI[3] = color.getAlpha();
		}
	}

	virtual void setRect(core::rect<s32> value)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = (f32)value.UpperLeftCorner.X;
			if (Count > 1) ValueF[1] = (f32)value.UpperLeftCorner.Y;
			if (Count > 2) ValueF[2] = (f32)value.LowerRightCorner.X;
			if (Count > 3) ValueF[3] = (f32)value.LowerRightCorner.Y;
		}
		else
		{
			if (Count > 0) ValueI[0] = value.UpperLeftCorner.X;
			if (Count > 1) ValueI[1] = value.UpperLeftCorner.Y;
			if (Count > 2) ValueI[2] = value.LowerRightCorner.X;
			if (Count > 3) ValueI[3] = value.LowerRightCorner.Y;
		}
	}

	virtual void setMatrix(core::matrix4 value)
	{
		reset();
		if (IsFloat)
		{
			for (u32 r=0; r<4; ++r)
				for (u32 c=0; c<4; ++c)
					if (Count > c+r*4)
						ValueF[c+r*4] = value(r,c);
		}
		else
		{
			for (u32 r=0; r<4; ++r)
				for (u32 c=0; c<4; ++c)
					if (Count > c+r*4)
						ValueI[c+r*4] = (s32)value(r,c);
		}
	}

	virtual void setQuaternion(core::quaternion value)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = value.X;
			if (Count > 1) ValueF[1] = value.Y;
			if (Count > 2) ValueF[2] = value.Z;
			if (Count > 3) ValueF[3] = value.W;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)value.X;
			if (Count > 1) ValueI[1] = (s32)value.Y;
			if (Count > 2) ValueI[2] = (s32)value.Z;
			if (Count > 3) ValueI[3] = (s32)value.W;
		}
	}

	virtual void setBoundingBox(core::aabbox3d<f32> value)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = value.MinEdge.X;
			if (Count > 1) ValueF[1] = value.MinEdge.Y;
			if (Count > 2) ValueF[2] = value.MinEdge.Z;
			if (Count > 3) ValueF[3] = value.MaxEdge.X;
			if (Count > 4) ValueF[4] = value.MaxEdge.Y;
			if (Count > 5) ValueF[5] = value.MaxEdge.Z;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)value.MinEdge.X;
			if (Count > 1) ValueI[1] = (s32)value.MinEdge.Y;
			if (Count > 2) ValueI[2] = (s32)value.MinEdge.Z;
			if (Count > 3) ValueI[3] = (s32)value.MaxEdge.X;
			if (Count > 4) ValueI[4] = (s32)value.MaxEdge.Y;
			if (Count > 5) ValueI[5] = (s32)value.MaxEdge.Z;
		}
	}

	virtual void setPlane(core::plane3df value)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = value.Normal.X;
			if (Count > 1) ValueF[1] = value.Normal.Y;
			if (Count > 2) ValueF[2] = value.Normal.Z;
			if (Count > 3) ValueF[3] = value.D;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)value.Normal.X;
			if (Count > 1) ValueI[1] = (s32)value.Normal.Y;
			if (Count > 2) ValueI[2] = (s32)value.Normal.Z;
			if (Count > 3) ValueI[3] = (s32)value.D;
		}
	}

	virtual void setTriangle3d(core::triangle3df value)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = value.pointA.X;
			if (Count > 1) ValueF[1] = value.pointA.Y;
			if (Count > 2) ValueF[2] = value.pointA.Z;
			if (Count > 3) ValueF[3] = value.pointB.X;
			if (Count > 4) ValueF[4] = value.pointB.Y;
			if (Count > 5) ValueF[5] = value.pointB.Z;
			if (Count > 6) ValueF[6] = value.pointC.X;
			if (Count > 7) ValueF[7] = value.pointC.Y;
			if (Count > 8) ValueF[8] = value.pointC.Z;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)value.pointA.X;
			if (Count > 1) ValueI[1] = (s32)value.pointA.Y;
			if (Count > 2) ValueI[2] = (s32)value.pointA.Z;
			if (Count > 3) ValueI[3] = (s32)value.pointB.X;
			if (Count > 4) ValueI[4] = (s32)value.pointB.Y;
			if (Count > 5) ValueI[5] = (s32)value.pointB.Z;
			if (Count > 6) ValueI[6] = (s32)value.pointC.X;
			if (Count > 7) ValueI[7] = (s32)value.pointC.Y;
			if (Count > 8) ValueI[8] = (s32)value.pointC.Z;
		}
	}

	virtual void setVector2d(core::vector2df v)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = v.X;
			if (Count > 1) ValueF[1] = v.Y;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)v.X;
			if (Count > 1) ValueI[1] = (s32)v.Y;
		}
	}

	virtual void setVector2d(core::vector2di v)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = (f32)v.X;
			if (Count > 1) ValueF[1] = (f32)v.Y;
		}
		else
		{
			if (Count > 0) ValueI[0] = v.X;
			if (Count > 1) ValueI[1] = v.Y;
		}
	}

	virtual void setLine2d(core::line2di v)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = (f32)v.start.X;
			if (Count > 1) ValueF[1] = (f32)v.start.Y;
			if (Count > 2) ValueF[2] = (f32)v.end.X;
			if (Count > 3) ValueF[3] = (f32)v.end.Y;
		}
		else
		{
			if (Count > 0) ValueI[0] = v.start.X;
			if (Count > 1) ValueI[1] = v.start.Y;
			if (Count > 2) ValueI[2] = v.end.X;
			if (Count > 3) ValueI[3] = v.end.Y;
		}
	}

	virtual void setLine2d(core::line2df v)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = v.start.X;
			if (Count > 1) ValueF[1] = v.start.Y;
			if (Count > 2) ValueF[2] = v.end.X;
			if (Count > 3) ValueF[3] = v.end.Y;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)v.start.X;
			if (Count > 1) ValueI[1] = (s32)v.start.Y;
			if (Count > 2) ValueI[2] = (s32)v.end.X;
			if (Count > 3) ValueI[3] = (s32)v.end.Y;
		}
	}

	virtual void setDimension2d(core::dimension2du v)
	{
		reset();
		if (IsFloat)
		{
			if (Count > 0) ValueF[0] = (f32)v.Width;
			if (Count > 1) ValueF[1] = (f32)v.Height;
		}
		else
		{
			if (Count > 0) ValueI[0] = (s32)v.Width;
			if (Count > 1) ValueI[1] = (s32)v.Height;
		}
	}

	//! set float array
	virtual void setFloatArray(core::array<f32> &vals)
	{
		reset();

		for (u32 i=0; i<vals.size() && i<Count; ++i)
		{
			if (IsFloat)
				ValueF[i] = vals[i];
			else
				ValueI[i] = (s32)vals[i];
		}
	}

	//! set int array
	virtual void setIntArray(core::array<s32> &vals)
	{
		reset();

		for (u32 i=0; i<vals.size() && i<Count; ++i)
		{
			if (IsFloat)
				ValueF[i] = (f32)vals[i];
			else
				ValueI[i] = vals[i];
		}
	}


	//! is it a number list?
	virtual bool isNumberList()
	{
		return true;
	}

	//! is it a float list?
	virtual bool isFloat()
	{
		return IsFloat;
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		if (IsFloat)
			return EAT_FLOATARRAY;
		else
			return EAT_INTARRAY;
	}

	virtual const wchar_t* getTypeString() const
	{
		if (IsFloat)
			return L"floatlist";
		else
			return L"intlist";
	}

protected:

	//! clear all values
	void reset()
	{
		if (IsFloat)
			for (u32 i=0; i < Count ; ++i)
				ValueF[i] = 0.0f;
		else
			for (u32 i=0; i < Count ; ++i)
				ValueI[i] = 0;
	}

	core::array<s32> ValueI;
	core::array<f32> ValueF;
	u32 Count;
	bool IsFloat;
};


// Attribute implemented for floating point colors
class CColorfAttribute : public CNumbersAttribute
{
public:

	CColorfAttribute(const char* name, video::SColorf value) : CNumbersAttribute(name, value) {}

	virtual s32 getInt()
	{
		return getColor().color;
	}

	virtual f32 getFloat()
	{
		return (f32)getColor().color;
	}

	virtual void setInt(s32 intValue)
	{
		video::SColorf c = video::SColor(intValue);
		ValueF[0] = c.r;
		ValueF[1] = c.g;
		ValueF[2] = c.b;
		ValueF[3] = c.a;
	}

	virtual void setFloat(f32 floatValue)
	{
		setInt((s32)floatValue);
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_COLORF;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"colorf";
	}
};



// Attribute implemented for colors
class CColorAttribute : public CNumbersAttribute
{
public:

	CColorAttribute(const char* name, const video::SColorf& value) : CNumbersAttribute(name, value) {}

	CColorAttribute(const char* name, const video::SColor& value) : CNumbersAttribute(name, value) {}

	virtual s32 getInt()
	{
		return getColor().color;
	}

	virtual f32 getFloat()
	{
		return (f32)getColor().color;
	}

	virtual void setInt(s32 intValue)
	{
		video::SColorf c = video::SColor(intValue);
		ValueF[0] = c.r;
		ValueF[1] = c.g;
		ValueF[2] = c.b;
		ValueF[3] = c.a;
	}

	virtual void setFloat(f32 floatValue)
	{
		setInt((s32)floatValue);
	}

	virtual core::stringw getStringW()
	{
		char tmp[10];
		const video::SColor c = getColor();
		sprintf(tmp, "%02x%02x%02x%02x", c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue());
		return core::stringw(tmp);
	}

	virtual void setString(const char* text)
	{
		u32 c;
		if (sscanf(text, "%08x", &c)!=1)
		{
			CNumbersAttribute::setString(text);
		}
		else
			setColor(c);
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_COLOR;
	}


	virtual const wchar_t* getTypeString() const
	{
		return L"color";
	}

};


// Attribute implemented for 3d vectors
class CVector3DAttribute : public CNumbersAttribute
{
public:

	CVector3DAttribute(const char* name, core::vector3df value) : CNumbersAttribute(name, value) {}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_VECTOR3D;
	}

	virtual core::matrix4 getMatrix()
	{
		core::matrix4 ret;
		ret.makeIdentity();
		ret.setTranslation( core::vector3df(ValueF[0],ValueF[1],ValueF[2]) );
		return ret;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"vector3d";
	}
};

// Attribute implemented for 2d vectors
class CVector2DAttribute : public CNumbersAttribute
{
public:

	CVector2DAttribute(const char* name, core::vector2df value) : CNumbersAttribute(name, value) {}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_VECTOR2D;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"vector2d";
	}
};

// Attribute implemented for 2d vectors
class CPosition2DAttribute : public CNumbersAttribute
{
public:

	CPosition2DAttribute(const char* name, core::position2di value) : CNumbersAttribute(name, value) {}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_POSITION2D;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"position";
	}
};



// Attribute implemented for rectangles
class CRectAttribute : public CNumbersAttribute
{
public:

	CRectAttribute(const char* name, core::rect<s32> value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_RECT;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"rect";
	}
};


// Attribute implemented for dimension
class CDimension2dAttribute : public CNumbersAttribute
{
public:

	CDimension2dAttribute (const char* name, core::dimension2d<u32> value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_DIMENSION2D;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"dimension2d";
	}
};

// Attribute implemented for matrices
class CMatrixAttribute : public CNumbersAttribute
{
public:

	CMatrixAttribute(const char* name, core::matrix4 value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_MATRIX;
	}

	virtual core::quaternion getQuaternion()
	{
		return core::quaternion(getMatrix());
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"matrix";
	}
};

// Attribute implemented for quaternions
class CQuaternionAttribute : public CNumbersAttribute
{
public:

	CQuaternionAttribute(const char* name, core::quaternion value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_QUATERNION;
	}

	virtual core::matrix4 getMatrix()
	{
		return getQuaternion().getMatrix();
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"quaternion";
	}
};


// Attribute implemented for bounding boxes
class CBBoxAttribute : public CNumbersAttribute
{
public:

	CBBoxAttribute(const char* name, core::aabbox3df value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_BBOX;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"box3d";
	}
};

// Attribute implemented for planes
class CPlaneAttribute : public CNumbersAttribute
{
public:

	CPlaneAttribute(const char* name, core::plane3df value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_PLANE;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"plane";
	}
};

// Attribute implemented for triangles
class CTriangleAttribute : public CNumbersAttribute
{
public:

	CTriangleAttribute(const char* name, core::triangle3df value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_TRIANGLE3D;
	}

	virtual core::plane3df getPlane()
	{
		return getTriangle().getPlane();
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"triangle";
	}
};


// Attribute implemented for 2d lines
class CLine2dAttribute : public CNumbersAttribute
{
public:

	CLine2dAttribute(const char* name, core::line2df value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_LINE2D;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"line2d";
	}
};

// Attribute implemented for 3d lines
class CLine3dAttribute : public CNumbersAttribute
{
public:

	CLine3dAttribute(const char* name, core::line3df value) : CNumbersAttribute(name, value) { }

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_LINE3D;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"line3d";
	}
};


// vector2df
// dimension2du

/*
	Special attributes
*/

// Attribute implemented for enumeration literals
class CEnumAttribute : public IAttribute
{
public:

	CEnumAttribute(const char* name, const char* value, const char* const* literals)
	{
		Name = name;
		setEnum(value, literals);
	}

	virtual void setEnum(const char* enumValue, const char* const* enumerationLiterals)
	{
		int literalCount = 0;

		if (enumerationLiterals)
		{
			s32 i;
			for (i=0; enumerationLiterals[i]; ++i)
				++literalCount;

			EnumLiterals.reallocate(literalCount);
			for (i=0; enumerationLiterals[i]; ++i)
				EnumLiterals.push_back(enumerationLiterals[i]);
		}

		setString(enumValue);
	}

	virtual s32 getInt()
	{
		for (u32 i=0; i < EnumLiterals.size(); ++i)
			if (Value.equals_ignore_case(EnumLiterals[i]))
			{
				return (s32)i;
			}

		return -1;
	}

	virtual f32 getFloat()
	{
		return (f32)getInt();
	}

	virtual bool getBool()
	{
		return (getInt() != 0); // does not make a lot of sense, I know
	}

	virtual core::stringc getString()
	{
		return Value;
	}

	virtual core::stringw getStringW()
	{
		return core::stringw(Value.c_str());
	}

	virtual void setInt(s32 intValue)
	{
		if (intValue>=0 && intValue<(s32)EnumLiterals.size())
			Value = EnumLiterals[intValue];
		else
			Value = "";
	}

	virtual void setFloat(f32 floatValue)
	{
		setInt((s32)floatValue);
	};

	virtual void setString(const char* text)
	{
		Value = text;
	}

	virtual const char* getEnum()
	{
		return Value.c_str();
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_ENUM;
	}


	virtual const wchar_t* getTypeString() const
	{
		return L"enum";
	}

	core::stringc Value;
	core::array<core::stringc> EnumLiterals;
};





// Attribute implemented for strings
class CStringAttribute : public IAttribute
{
public:

	CStringAttribute(const char* name, const char* value)
	{
		IsStringW=false;
		Name = name;
		setString(value);
	}

	CStringAttribute(const char* name, const wchar_t* value)
	{
		IsStringW = true;
		Name = name;
		setString(value);
	}

	CStringAttribute(const char* name, void* binaryData, s32 lenghtInBytes)
	{
		IsStringW=false;
		Name = name;
		setBinary(binaryData, lenghtInBytes);
	}

	virtual s32 getInt()
	{
		if (IsStringW)
			return atoi(core::stringc(ValueW.c_str()).c_str());
		else
			return atoi(Value.c_str());
	}

	virtual f32 getFloat()
	{
		if (IsStringW)
			return core::fast_atof(core::stringc(ValueW.c_str()).c_str());
		else
			return core::fast_atof(Value.c_str());
	}

	virtual bool getBool()
	{
		if (IsStringW)
			return ValueW.equals_ignore_case(L"true");
		else
			return Value.equals_ignore_case("true");
	}

	virtual core::stringc getString()
	{
		if (IsStringW)
			return core::stringc(ValueW.c_str());
		else
			return Value;
	}
	virtual core::stringw getStringW()
	{
		if (IsStringW)
			return ValueW;
		else
			return core::stringw(Value.c_str());
	}

	virtual void setInt(s32 intValue)
	{
		if (IsStringW)
			ValueW = core::stringw(intValue);
		else
			Value = core::stringc(intValue);
	}

	virtual void setFloat(f32 floatValue)
	{
		if (IsStringW)
		{
			ValueW = core::stringw((double)floatValue);
		}
		else
		{
			Value = core::stringc((double)floatValue);
		}
	};

	virtual void setString(const char* text)
	{
		if (IsStringW)
			ValueW = core::stringw(text);
		else
			Value = text;
	}

	virtual void setString(const wchar_t* text)
	{
		if (IsStringW)
			ValueW = text;
		else
			Value = core::stringc(text);
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_STRING;
	}


	virtual const wchar_t* getTypeString() const
	{
		return L"string";
	}

	virtual void getBinary(void* outdata, s32 maxLength)
	{
		s32 dataSize = maxLength;
		c8* datac8 = (c8*)(outdata);
		s32 p = 0;
		const c8* dataString = Value.c_str();

		for (s32 i=0; i<dataSize; ++i)
			datac8[i] = 0;

		while(dataString[p] && p<dataSize)
		{
			s32 v = getByteFromHex((c8)dataString[p*2]) * 16;

			if (dataString[(p*2)+1])
				v += getByteFromHex((c8)dataString[(p*2)+1]);

			datac8[p] = v;
			++p;
		}
	};

	virtual void setBinary(void* data, s32 maxLength)
	{
		s32 dataSize = maxLength;
		c8* datac8 = (c8*)(data);
		char tmp[3];
		tmp[2] = 0;
		Value = "";

		for (s32 b=0; b<dataSize; ++b)
		{
			getHexStrFromByte(datac8[b], tmp);
			Value.append(tmp);
		}
	};

	bool IsStringW;
	core::stringc Value;
	core::stringw ValueW;

protected:

	static inline s32 getByteFromHex(c8 h)
	{
		if (h >= '0' && h <='9')
			return h-'0';

		if (h >= 'a' && h <='f')
			return h-'a' + 10;

		return 0;
	}

	static inline void getHexStrFromByte(c8 byte, c8* out)
	{
		s32 b = (byte & 0xf0) >> 4;

		for (s32 i=0; i<2; ++i)
		{
			if (b >=0 && b <= 9)
				out[i] = b+'0';
			if (b >=10 && b <= 15)
				out[i] = (b-10)+'a';

			b = byte & 0x0f;
		}
	}
};

// Attribute implemented for binary data
class CBinaryAttribute : public CStringAttribute
{
public:

	CBinaryAttribute(const char* name, void* binaryData, s32 lenghtInBytes)
		: CStringAttribute(name, binaryData, lenghtInBytes)
	{

	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_BINARY;
	}


	virtual const wchar_t* getTypeString() const
	{
		return L"binary";
	}
};



// Attribute implemented for texture references
class CTextureAttribute : public IAttribute
{
public:

	CTextureAttribute(const char* name, video::ITexture* value, video::IVideoDriver* driver, const io::path& filename)
		: Value(0), Driver(driver), OverrideName(filename)
	{
		if (Driver)
			Driver->grab();

		Name = name;
		setTexture(value);
	}

	~CTextureAttribute()
	{
		if (Driver)
			Driver->drop();

		if (Value)
			Value->drop();
	}

	virtual video::ITexture* getTexture()
	{
		return Value;
	}

	virtual bool getBool()
	{
		return (Value != 0);
	}

	virtual core::stringw getStringW()
	{
		// (note: don't try to put all this in some ?: operators, or c++ builder will choke)
		if ( OverrideName.size() )
			return core::stringw(OverrideName);

		if ( Value )
			return core::stringw(Value->getName().getPath().c_str());

		return core::stringw(0);
	}

	virtual core::stringc getString()
	{
		// since texture names can be stringw we are careful with the types
		if ( OverrideName.size() )
			return core::stringc(OverrideName);

		if ( Value )
			return core::stringc(Value->getName().getPath().c_str());
		
		return core::stringc(0);
	}

	virtual void setString(const char* text)
	{
		if (Driver)
		{
			if (text && *text)
			{
				setTexture(Driver->getTexture(text));
				OverrideName=text;
			}
			else
				setTexture(0);
		}
	}

	virtual void setTexture(video::ITexture* value)
	{
		if ( value == Value )
			return;

		if (Value)
			Value->drop();

		Value = value;

		if (Value)
			Value->grab();
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_TEXTURE;
	}


	virtual const wchar_t* getTypeString() const
	{
		return L"texture";
	}

	video::ITexture* Value;
	video::IVideoDriver* Driver;
	io::path OverrideName;
};



// Attribute implemented for array of stringw
class CStringWArrayAttribute : public IAttribute
{
public:

	CStringWArrayAttribute(const char* name, const core::array<core::stringw>& value)
	{
		Name = name;
		setArray(value);
	}

	virtual core::array<core::stringw> getArray()
	{
		return Value;
	}

	virtual void setArray(const core::array<core::stringw>& value)
	{
		Value = value;
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_STRINGWARRAY;
	}

	virtual const wchar_t* getTypeString() const
	{
		return L"stringwarray";
	}

	core::array<core::stringw> Value;
};


// Attribute implemented for user pointers
class CUserPointerAttribute : public IAttribute
{
public:

	CUserPointerAttribute(const char* name, void* value)
	{
		Name = name;
		Value = value;
	}

	virtual s32 getInt()
	{
		return *static_cast<s32*>(Value);
	}

	virtual bool getBool()
	{
		return (Value != 0);
	}

	virtual core::stringw getStringW()
	{
		wchar_t buf[32];
		swprintf(buf, 32, L"%p", Value);

		return core::stringw(buf);
	}

	virtual void setString(const char* text)
	{
		u32 tmp;
		sscanf(text, "0x%x", &tmp);
		Value = (void *) tmp;
	}

	virtual E_ATTRIBUTE_TYPE getType() const
	{
		return EAT_USER_POINTER;
	}

	virtual void setUserPointer(void* v)
	{
		Value = v;
	}

	virtual void* getUserPointer()
	{
		return Value;
	}


	virtual const wchar_t* getTypeString() const
	{
		return L"userPointer";
	}

	void* Value;
};



// todo: CGUIFontAttribute

} // end namespace io
} // end namespace irr
