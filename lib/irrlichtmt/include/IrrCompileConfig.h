// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_COMPILE_CONFIG_H_INCLUDED__
#define __IRR_COMPILE_CONFIG_H_INCLUDED__

//! Identifies the IrrlichtMt fork customized for the Minetest engine
#define IRRLICHT_VERSION_MT_REVISION 12
#define IRRLICHT_VERSION_MT "mt12"

//! Irrlicht SDK Version
#define IRRLICHT_VERSION_MAJOR 1
#define IRRLICHT_VERSION_MINOR 9
#define IRRLICHT_VERSION_REVISION 0
// This flag will be defined only in SVN, the official release code will have
// it undefined
#define IRRLICHT_VERSION_SVN alpha
#define IRRLICHT_SDK_VERSION "1.9.0" IRRLICHT_VERSION_MT

#include <stdio.h> // TODO: Although included elsewhere this is required at least for mingw

#ifdef _WIN32
#define IRRCALLCONV __stdcall
#else
#define IRRCALLCONV
#endif

#ifndef IRRLICHT_API
#define IRRLICHT_API
#endif

#endif // __IRR_COMPILE_CONFIG_H_INCLUDED__
