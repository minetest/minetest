// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

//! Identifies the IrrlichtMt fork customized for the Minetest engine
#define IRRLICHT_VERSION_MT_REVISION 15
#define IRRLICHT_VERSION_MT "mt15"

//! Irrlicht SDK Version
#define IRRLICHT_VERSION_MAJOR 1
#define IRRLICHT_VERSION_MINOR 9
#define IRRLICHT_VERSION_REVISION 0
// This flag will be defined only in SVN, the official release code will have
// it undefined
#define IRRLICHT_VERSION_SVN alpha
#define IRRLICHT_SDK_VERSION "1.9.0" IRRLICHT_VERSION_MT

#ifdef _WIN32
#define IRRCALLCONV __stdcall
#else
#define IRRCALLCONV
#endif

#ifndef IRRLICHT_API
#define IRRLICHT_API
#endif
