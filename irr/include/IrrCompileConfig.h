// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#ifdef _WIN32
#define IRRCALLCONV __stdcall
#else
#define IRRCALLCONV
#endif

#ifndef IRRLICHT_API
#define IRRLICHT_API
#endif
