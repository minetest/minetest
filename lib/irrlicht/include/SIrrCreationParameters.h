// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_IRRLICHT_CREATION_PARAMETERS_H_INCLUDED__
#define __I_IRRLICHT_CREATION_PARAMETERS_H_INCLUDED__

#include "EDriverTypes.h"
#include "EDeviceTypes.h"
#include "dimension2d.h"
#include "ILogger.h"

namespace irr
{
	class IEventReceiver;

	//! Structure for holding Irrlicht Device creation parameters.
	/** This structure is used in the createDeviceEx() function. */
	struct SIrrlichtCreationParameters
	{
		//! Constructs a SIrrlichtCreationParameters structure with default values.
		SIrrlichtCreationParameters() :
			DeviceType(EIDT_BEST),
			DriverType(video::EDT_BURNINGSVIDEO),
			WindowSize(core::dimension2d<u32>(800, 600)),
			Bits(16),
			ZBufferBits(16),
			Fullscreen(false),
			Stencilbuffer(false),
			Vsync(false),
			AntiAlias(0),
			HandleSRGB(false),
			WithAlphaChannel(false),
			Doublebuffer(true),
			IgnoreInput(false),
			Stereobuffer(false),
			HighPrecisionFPU(false),
			EventReceiver(0),
			WindowId(0),
#ifdef _DEBUG
			LoggingLevel(ELL_DEBUG),
#else
			LoggingLevel(ELL_INFORMATION),
#endif
			DisplayAdapter(0),
			DriverMultithreaded(false),
			UsePerformanceTimer(true),
			SDK_version_do_not_use(IRRLICHT_SDK_VERSION)
		{
		}

		SIrrlichtCreationParameters(const SIrrlichtCreationParameters& other) :
			SDK_version_do_not_use(IRRLICHT_SDK_VERSION)
		{*this = other;}

		SIrrlichtCreationParameters& operator=(const SIrrlichtCreationParameters& other)
		{
			DeviceType = other.DeviceType;
			DriverType = other.DriverType;
			WindowSize = other.WindowSize;
			Bits = other.Bits;
			ZBufferBits = other.ZBufferBits;
			Fullscreen = other.Fullscreen;
			Stencilbuffer = other.Stencilbuffer;
			Vsync = other.Vsync;
			AntiAlias = other.AntiAlias;
			HandleSRGB = other.HandleSRGB;
			WithAlphaChannel = other.WithAlphaChannel;
			Doublebuffer = other.Doublebuffer;
			IgnoreInput = other.IgnoreInput;
			Stereobuffer = other.Stereobuffer;
			HighPrecisionFPU = other.HighPrecisionFPU;
			EventReceiver = other.EventReceiver;
			WindowId = other.WindowId;
			LoggingLevel = other.LoggingLevel;
			DriverMultithreaded = other.DriverMultithreaded;
			DisplayAdapter = other.DisplayAdapter;
			UsePerformanceTimer = other.UsePerformanceTimer;
			return *this;
		}

		//! Type of the device.
		/** This setting decides the windowing system used by the device, most device types are native
		to a specific operating system and so may not be available.
		EIDT_WIN32 is only available on Windows desktops,
		EIDT_WINCE is only available on Windows mobile devices,
		EIDT_COCOA is only available on Mac OSX,
		EIDT_X11 is available on Linux, Solaris, BSD and other operating systems which use X11,
		EIDT_SDL is available on most systems if compiled in,
		EIDT_CONSOLE is usually available but can only render to text,
		EIDT_BEST will select the best available device for your operating system.
		Default: EIDT_BEST. */
		E_DEVICE_TYPE DeviceType;

		//! Type of video driver used to render graphics.
		/** This can currently be video::EDT_NULL, video::EDT_SOFTWARE,
		video::EDT_BURNINGSVIDEO, video::EDT_DIRECT3D8,
		video::EDT_DIRECT3D9, and video::EDT_OPENGL.
		Default: Software. */
		video::E_DRIVER_TYPE DriverType;

		//! Size of the window or the video mode in fullscreen mode. Default: 800x600
		core::dimension2d<u32> WindowSize;

		//! Minimum Bits per pixel of the color buffer in fullscreen mode. Ignored if windowed mode. Default: 16.
		u8 Bits;

		//! Minimum Bits per pixel of the depth buffer. Default: 16.
		u8 ZBufferBits;

		//! Should be set to true if the device should run in fullscreen.
		/** Otherwise the device runs in windowed mode. Default: false. */
		bool Fullscreen;

		//! Specifies if the stencil buffer should be enabled.
		/** Set this to true, if you want the engine be able to draw
		stencil buffer shadows. Note that not all drivers are able to
		use the stencil buffer, hence it can be ignored during device
		creation. Without the stencil buffer no shadows will be drawn.
		Default: false. */
		bool Stencilbuffer;

		//! Specifies vertical syncronisation.
		/** If set to true, the driver will wait for the vertical
		retrace period, otherwise not. May be silently ignored.
		Default: false */
		bool Vsync;

		//! Specifies if the device should use fullscreen anti aliasing
		/** Makes sharp/pixelated edges softer, but requires more
		performance. Also, 2D elements might look blurred with this
		switched on. The resulting rendering quality also depends on
		the hardware and driver you are using, your program might look
		different on different hardware with this. So if you are
		writing a game/application with AntiAlias switched on, it would
		be a good idea to make it possible to switch this option off
		again by the user.
		The value is the maximal antialiasing factor requested for
		the device. The cretion method will automatically try smaller
		values if no window can be created with the given value.
		Value one is usually the same as 0 (disabled), but might be a
		special value on some platforms. On D3D devices it maps to
		NONMASKABLE.
		Default value: 0 - disabled */
		u8 AntiAlias;

		//! Flag to enable proper sRGB and linear color handling
		/** In most situations, it is desireable to have the color handling in
		non-linear sRGB color space, and only do the intermediate color
		calculations in linear RGB space. If this flag is enabled, the device and
		driver try to assure that all color input and output are color corrected
		and only the internal color representation is linear. This means, that
		the color output is properly gamma-adjusted to provide the brighter
		colors for monitor display. And that blending and lighting give a more
		natural look, due to proper conversion from non-linear colors into linear
		color space for blend operations. If this flag is enabled, all texture colors
		(which are usually in sRGB space) are correctly displayed. However vertex colors
		and other explicitly set values have to be manually encoded in linear color space.
		Default value: false. */
		bool HandleSRGB;

		//! Whether the main framebuffer uses an alpha channel.
		/** In some situations it might be desireable to get a color
		buffer with an alpha channel, e.g. when rendering into a
		transparent window or overlay. If this flag is set the device
		tries to create a framebuffer with alpha channel.
		If this flag is set, only color buffers with alpha channel
		are considered. Otherwise, it depends on the actual hardware
		if the colorbuffer has an alpha channel or not.
		Default value: false */
		bool WithAlphaChannel;

		//! Whether the main framebuffer uses doublebuffering.
		/** This should be usually enabled, in order to avoid render
		artifacts on the visible framebuffer. However, it might be
		useful to use only one buffer on very small devices. If no
		doublebuffering is available, the drivers will fall back to
		single buffers. Default value: true */
		bool Doublebuffer;

		//! Specifies if the device should ignore input events
		/** This is only relevant when using external I/O handlers.
		External windows need to take care of this themselves.
		Currently only supported by X11.
		Default value: false */
		bool IgnoreInput;

		//! Specifies if the device should use stereo buffers
		/** Some high-end gfx cards support two framebuffers for direct
		support of stereoscopic output devices. If this flag is set the
		device tries to create a stereo context.
		Currently only supported by OpenGL.
		Default value: false */
		bool Stereobuffer;

		//! Specifies if the device should use high precision FPU setting
		/** This is only relevant for DirectX Devices, which switch to
		low FPU precision by default for performance reasons. However,
		this may lead to problems with the other computations of the
		application. In this case setting this flag to true should help
		- on the expense of performance loss, though.
		Default value: false */
		bool HighPrecisionFPU;

		//! A user created event receiver.
		IEventReceiver* EventReceiver;

		//! Window Id.
		/** If this is set to a value other than 0, the Irrlicht Engine
		will be created in an already existing window. For windows, set
		this to the HWND of the window you want. The windowSize and
		FullScreen options will be ignored when using the WindowId
		parameter. Default this is set to 0.
		To make Irrlicht run inside the custom window, you still will
		have to draw Irrlicht on your own. You can use this loop, as
		usual:
		\code
		while (device->run())
		{
			driver->beginScene(true, true, 0);
			smgr->drawAll();
			driver->endScene();
		}
		\endcode
		Instead of this, you can also simply use your own message loop
		using GetMessage, DispatchMessage and whatever. Calling
		IrrlichtDevice::run() will cause Irrlicht to dispatch messages
		internally too.  You need not call Device->run() if you want to
		do your own message dispatching loop, but Irrlicht will not be
		able to fetch user input then and you have to do it on your own
		using the window messages, DirectInput, or whatever. Also,
		you'll have to increment the Irrlicht timer.
		An alternative, own message dispatching loop without
		device->run() would look like this:
		\code
		MSG msg;
		while (true)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);

				if (msg.message == WM_QUIT)
					break;
			}

			// increase virtual timer time
			device->getTimer()->tick();

			// draw engine picture
			driver->beginScene(true, true, 0);
			smgr->drawAll();
			driver->endScene();
		}
		\endcode
		However, there is no need to draw the picture this often. Just
		do it how you like. */
		void* WindowId;

		//! Specifies the logging level used in the logging interface.
		/** The default value is ELL_INFORMATION. You can access the ILogger interface
		later on from the IrrlichtDevice with getLogger() and set another level.
		But if you need more or less logging information already from device creation,
		then you have to change it here.
		*/
		ELOG_LEVEL LoggingLevel;

		//! Allows to select which graphic card is used for rendering when more than one card is in the system.
		/** So far only supported on D3D */
		u32 DisplayAdapter;

		//! Create the driver multithreaded.
		/** Default is false. Enabling this can slow down your application.
			Note that this does _not_ make Irrlicht threadsafe, but only the underlying driver-API for the graphiccard.
			So far only supported on D3D. */
		bool DriverMultithreaded;

		//! Enables use of high performance timers on Windows platform.
		/** When performance timers are not used, standard GetTickCount()
		is used instead which usually has worse resolution, but also less
		problems with speed stepping and other techniques.
		*/
		bool UsePerformanceTimer;

		//! Don't use or change this parameter.
		/** Always set it to IRRLICHT_SDK_VERSION, which is done by default.
		This is needed for sdk version checks. */
		const c8* const SDK_version_do_not_use;
	};


} // end namespace irr

#endif

