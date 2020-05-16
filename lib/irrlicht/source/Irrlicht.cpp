// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

static const char* const copyright = "Irrlicht Engine (c) 2002-2012 Nikolaus Gebhardt";

#ifdef _IRR_WINDOWS_
	#include <windows.h>
	#if defined(_DEBUG) && !defined(__GNUWIN32__) && !defined(_WIN32_WCE)
		#include <crtdbg.h>
	#endif // _DEBUG
#endif

#include "irrlicht.h"
#ifdef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
#include "CIrrDeviceWin32.h"
#endif

#ifdef _IRR_COMPILE_WITH_OSX_DEVICE_
#include "MacOSX/CIrrDeviceMacOSX.h"
#endif

#ifdef _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_
#include "CIrrDeviceWinCE.h"
#endif

#ifdef _IRR_COMPILE_WITH_X11_DEVICE_
#include "CIrrDeviceLinux.h"
#endif

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#include "CIrrDeviceSDL.h"
#endif

#ifdef _IRR_COMPILE_WITH_FB_DEVICE_
#include "CIrrDeviceFB.h"
#endif

#ifdef _IRR_COMPILE_WITH_CONSOLE_DEVICE_
#include "CIrrDeviceConsole.h"
#endif

namespace irr
{
	//! stub for calling createDeviceEx
	IRRLICHT_API IrrlichtDevice* IRRCALLCONV createDevice(video::E_DRIVER_TYPE driverType,
			const core::dimension2d<u32>& windowSize,
			u32 bits, bool fullscreen,
			bool stencilbuffer, bool vsync, IEventReceiver* res)
	{
		SIrrlichtCreationParameters p;
		p.DriverType = driverType;
		p.WindowSize = windowSize;
		p.Bits = (u8)bits;
		p.Fullscreen = fullscreen;
		p.Stencilbuffer = stencilbuffer;
		p.Vsync = vsync;
		p.EventReceiver = res;

		return createDeviceEx(p);
	}

	extern "C" IRRLICHT_API IrrlichtDevice* IRRCALLCONV createDeviceEx(const SIrrlichtCreationParameters& params)
	{

		IrrlichtDevice* dev = 0;

#ifdef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
		if (params.DeviceType == EIDT_WIN32 || (!dev && params.DeviceType == EIDT_BEST))
			dev = new CIrrDeviceWin32(params);
#endif

#ifdef _IRR_COMPILE_WITH_OSX_DEVICE_
		if (params.DeviceType == EIDT_OSX || (!dev && params.DeviceType == EIDT_BEST))
			dev = new CIrrDeviceMacOSX(params);
#endif

#ifdef _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_
		if (params.DeviceType == EIDT_WINCE || (!dev && params.DeviceType == EIDT_BEST))
			dev = new CIrrDeviceWinCE(params);
#endif

#ifdef _IRR_COMPILE_WITH_X11_DEVICE_
		if (params.DeviceType == EIDT_X11 || (!dev && params.DeviceType == EIDT_BEST))
			dev = new CIrrDeviceLinux(params);
#endif

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
		if (params.DeviceType == EIDT_SDL || (!dev && params.DeviceType == EIDT_BEST))
			dev = new CIrrDeviceSDL(params);
#endif

#ifdef _IRR_COMPILE_WITH_FB_DEVICE_
		if (params.DeviceType == EIDT_FRAMEBUFFER || (!dev && params.DeviceType == EIDT_BEST))
			dev = new CIrrDeviceFB(params);
#endif

#ifdef _IRR_COMPILE_WITH_CONSOLE_DEVICE_
		if (params.DeviceType == EIDT_CONSOLE || (!dev && params.DeviceType == EIDT_BEST))
			dev = new CIrrDeviceConsole(params);
#endif

		if (dev && !dev->getVideoDriver() && params.DriverType != video::EDT_NULL)
		{
			dev->closeDevice(); // destroy window
			dev->run(); // consume quit message
			dev->drop();
			dev = 0;
		}

		return dev;
	}

namespace core
{
	const matrix4 IdentityMatrix(matrix4::EM4CONST_IDENTITY);
	irr::core::stringc LOCALE_DECIMAL_POINTS(".");
}

namespace video
{
	SMaterial IdentityMaterial;
}

} // end namespace irr


#if defined(_IRR_WINDOWS_API_)

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved )
{
	// _crtBreakAlloc = 139;

    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			#if defined(_DEBUG) && !defined(__GNUWIN32__) && !defined(__BORLANDC__) && !defined (_WIN32_WCE) && !defined (_IRR_XBOX_PLATFORM_)
				_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
			#endif
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

#endif // defined(_IRR_WINDOWS_)

