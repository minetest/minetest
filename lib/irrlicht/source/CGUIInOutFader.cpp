// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIInOutFader.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "os.h"

namespace irr
{
namespace gui
{


//! constructor
CGUIInOutFader::CGUIInOutFader(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
: IGUIInOutFader(environment, parent, id, rectangle)
{
	#ifdef _DEBUG
	setDebugName("CGUIInOutFader");
	#endif

	Action = EFA_NOTHING;
	StartTime = 0;
	EndTime = 0;

	setColor(video::SColor(0,0,0,0));
}


//! draws the element and its children
void CGUIInOutFader::draw()
{
	if (!IsVisible || !Action)
		return;

	u32 now = os::Timer::getTime();
	if (now > EndTime && Action == EFA_FADE_IN)
	{
		Action = EFA_NOTHING;
		return;
	}

	video::IVideoDriver* driver = Environment->getVideoDriver();

	if (driver)
	{
		f32 d;

		if (now > EndTime)
			d = 0.0f;
		else
			d = (EndTime - now) / (f32)(EndTime - StartTime);

		video::SColor newCol = FullColor.getInterpolated(TransColor, d);
		driver->draw2DRectangle(newCol, AbsoluteRect, &AbsoluteClippingRect);
	}

	IGUIElement::draw();
}


//! Gets the color to fade out to or to fade in from.
video::SColor CGUIInOutFader::getColor() const
{
	return Color[1];
}


//! Sets the color to fade out to or to fade in from.
void CGUIInOutFader::setColor(video::SColor color)
{
	video::SColor s = color;
	video::SColor d = color;

	s.setAlpha ( 255 );
	d.setAlpha ( 0 );
	setColor ( s,d );

/*
	Color[0] = color;

	FullColor = Color[0];
	TransColor = Color[0];

	if (Action == EFA_FADE_OUT)
	{
		FullColor.setAlpha(0);
		TransColor.setAlpha(255);
	}
	else
	if (Action == EFA_FADE_IN)
	{
		FullColor.setAlpha(255);
		TransColor.setAlpha(0);
	}
*/
}


void CGUIInOutFader::setColor(video::SColor source, video::SColor dest)
{
	Color[0] = source;
	Color[1] = dest;

	if (Action == EFA_FADE_OUT)
	{
		FullColor = Color[1];
		TransColor = Color[0];
	}
	else
	if (Action == EFA_FADE_IN)
	{
		FullColor = Color[0];
		TransColor = Color[1];
	}

}


//! Returns if the fade in or out process is done.
bool CGUIInOutFader::isReady() const
{
	u32 now = os::Timer::getTime();
	bool ret = (now > EndTime);
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! Starts the fade in process.
void CGUIInOutFader::fadeIn(u32 time)
{
	StartTime = os::Timer::getTime();
	EndTime = StartTime + time;
	Action = EFA_FADE_IN;
	setColor(Color[0],Color[1]);
}


//! Starts the fade out process.
void CGUIInOutFader::fadeOut(u32 time)
{
	StartTime = os::Timer::getTime();
	EndTime = StartTime + time;
	Action = EFA_FADE_OUT;
	setColor(Color[0],Color[1]);
}


//! Writes attributes of the element.
void CGUIInOutFader::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUIInOutFader::serializeAttributes(out,options);

	out->addColor	("FullColor",		FullColor);
	out->addColor	("TransColor",		TransColor);

}


//! Reads attributes of the element
void CGUIInOutFader::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUIInOutFader::deserializeAttributes(in,options);

	FullColor  = in->getAttributeAsColor("FullColor");
	TransColor = in->getAttributeAsColor("TransColor");
}


} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_

