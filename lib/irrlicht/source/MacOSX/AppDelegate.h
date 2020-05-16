// Copyright (C) 2005-2006 Etienne Petitjean
// Copyright (C) 2007-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_OSX_DEVICE_

#import <Cocoa/Cocoa.h>
#import "CIrrDeviceMacOSX.h"

@interface AppDelegate : NSObject
{
	BOOL			_quit;
	irr::CIrrDeviceMacOSX	*_device;
}

- (id)initWithDevice:(irr::CIrrDeviceMacOSX *)device;
- (BOOL)isQuit;

@end

#endif // _IRR_COMPILE_WITH_OSX_DEVICE_
