// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_

#include "CIrrDeviceSDL.h"
#include "IEventReceiver.h"
#include "irrList.h"
#include "os.h"
#include "CTimer.h"
#include "irrString.h"
#include "Keycodes.h"
#include "COSOperator.h"
#include <stdio.h>
#include <stdlib.h>
#include "SIrrCreationParameters.h"
#include <SDL/SDL_syswm.h>
#include <SDL/SDL_video.h>

#ifdef _MSC_VER
#pragma comment(lib, "SDL.lib")
#endif // _MSC_VER

namespace irr
{
	namespace video
	{

		#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_
		IVideoDriver* createDirectX8Driver(const irr::SIrrlichtCreationParameters& params,
			io::IFileSystem* io, HWND window);
		#endif

		#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_
		IVideoDriver* createDirectX9Driver(const irr::SIrrlichtCreationParameters& params,
			io::IFileSystem* io, HWND window);
		#endif

		#ifdef _IRR_COMPILE_WITH_OPENGL_
		IVideoDriver* createOpenGLDriver(const SIrrlichtCreationParameters& params,
				io::IFileSystem* io, CIrrDeviceSDL* device);
		#endif
	} // end namespace video

} // end namespace irr


namespace irr
{

//! constructor
CIrrDeviceSDL::CIrrDeviceSDL(const SIrrlichtCreationParameters& param)
	: CIrrDeviceStub(param),
	Screen((SDL_Surface*)param.WindowId), SDL_Flags(SDL_ANYFORMAT),
	MouseX(0), MouseY(0), MouseButtonStates(0),
	Width(param.WindowSize.Width), Height(param.WindowSize.Height),
	Resizable(false), WindowHasFocus(false), WindowMinimized(false)
{
	#ifdef _DEBUG
	setDebugName("CIrrDeviceSDL");
	#endif

	// Initialize SDL... Timer for sleep, video for the obvious, and
	// noparachute prevents SDL from catching fatal errors.
	if (SDL_Init( SDL_INIT_TIMER|SDL_INIT_VIDEO|
#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
				SDL_INIT_JOYSTICK|
#endif
				SDL_INIT_NOPARACHUTE ) < 0)
	{
		os::Printer::log( "Unable to initialize SDL!", SDL_GetError());
		Close = true;
	}

#if defined(_IRR_WINDOWS_)
	SDL_putenv("SDL_VIDEODRIVER=directx");
#elif defined(_IRR_OSX_PLATFORM_)
	SDL_putenv("SDL_VIDEODRIVER=Quartz");
#else
	SDL_putenv("SDL_VIDEODRIVER=x11");
#endif
//	SDL_putenv("SDL_WINDOWID=");

	SDL_VERSION(&Info.version);

	SDL_GetWMInfo(&Info);
	core::stringc sdlversion = "SDL Version ";
	sdlversion += Info.version.major;
	sdlversion += ".";
	sdlversion += Info.version.minor;
	sdlversion += ".";
	sdlversion += Info.version.patch;

	Operator = new COSOperator(sdlversion);
	os::Printer::log(sdlversion.c_str(), ELL_INFORMATION);

	// create keymap
	createKeyMap();
	// enable key to character translation
	SDL_EnableUNICODE(1);

	(void)SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	if ( CreationParams.Fullscreen )
		SDL_Flags |= SDL_FULLSCREEN;
	if (CreationParams.DriverType == video::EDT_OPENGL)
		SDL_Flags |= SDL_OPENGL;
	else if (CreationParams.Doublebuffer)
		SDL_Flags |= SDL_DOUBLEBUF;
	// create window
	if (CreationParams.DriverType != video::EDT_NULL)
	{
		// create the window, only if we do not use the null device
		createWindow();
	}

	// create cursor control
	CursorControl = new CCursorControl(this);

	// create driver
	createDriver();

	if (VideoDriver)
		createGUIAndScene();
}


//! destructor
CIrrDeviceSDL::~CIrrDeviceSDL()
{
#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
	const u32 numJoysticks = Joysticks.size();
	for (u32 i=0; i<numJoysticks; ++i)
		SDL_JoystickClose(Joysticks[i]);
#endif
	SDL_Quit();
}


bool CIrrDeviceSDL::createWindow()
{
	if ( Close )
		return false;

	if (CreationParams.DriverType == video::EDT_OPENGL)
	{
		if (CreationParams.Bits==16)
		{
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 4 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 4 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 4 );
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, CreationParams.WithAlphaChannel?1:0 );
		}
		else
		{
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, CreationParams.WithAlphaChannel?8:0 );
		}
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, CreationParams.ZBufferBits);
		if (CreationParams.Doublebuffer)
			SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		if (CreationParams.Stereobuffer)
			SDL_GL_SetAttribute( SDL_GL_STEREO, 1 );
		if (CreationParams.AntiAlias>1)
		{
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, CreationParams.AntiAlias );
		}
		if ( !Screen )
			Screen = SDL_SetVideoMode( Width, Height, CreationParams.Bits, SDL_Flags );
		if ( !Screen && CreationParams.AntiAlias>1)
		{
			while (--CreationParams.AntiAlias>1)
			{
				SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, CreationParams.AntiAlias );
				Screen = SDL_SetVideoMode( Width, Height, CreationParams.Bits, SDL_Flags );
				if (Screen)
					break;
			}
			if ( !Screen )
			{
				SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
				SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );
				Screen = SDL_SetVideoMode( Width, Height, CreationParams.Bits, SDL_Flags );
				if (Screen)
					os::Printer::log("AntiAliasing disabled due to lack of support!" );
			}
		}
	}
	else if ( !Screen )
		Screen = SDL_SetVideoMode( Width, Height, CreationParams.Bits, SDL_Flags );

	if ( !Screen && CreationParams.Doublebuffer)
	{
		// Try single buffer
		if (CreationParams.DriverType == video::EDT_OPENGL)
			SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_Flags &= ~SDL_DOUBLEBUF;
		Screen = SDL_SetVideoMode( Width, Height, CreationParams.Bits, SDL_Flags );
	}
	if ( !Screen )
	{
		os::Printer::log( "Could not initialize display!" );
		return false;
	}

	return true;
}


//! create the driver
void CIrrDeviceSDL::createDriver()
{
	switch(CreationParams.DriverType)
	{
	case video::EDT_DIRECT3D8:
		#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_

		VideoDriver = video::createDirectX8Driver(CreationParams, FileSystem, HWnd);
		if (!VideoDriver)
		{
			os::Printer::log("Could not create DIRECT3D8 Driver.", ELL_ERROR);
		}
		#else
		os::Printer::log("DIRECT3D8 Driver was not compiled into this dll. Try another one.", ELL_ERROR);
		#endif // _IRR_COMPILE_WITH_DIRECT3D_8_

		break;

	case video::EDT_DIRECT3D9:
		#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_

		VideoDriver = video::createDirectX9Driver(CreationParams, FileSystem, HWnd);
		if (!VideoDriver)
		{
			os::Printer::log("Could not create DIRECT3D9 Driver.", ELL_ERROR);
		}
		#else
		os::Printer::log("DIRECT3D9 Driver was not compiled into this dll. Try another one.", ELL_ERROR);
		#endif // _IRR_COMPILE_WITH_DIRECT3D_9_

		break;

	case video::EDT_SOFTWARE:
		#ifdef _IRR_COMPILE_WITH_SOFTWARE_
		VideoDriver = video::createSoftwareDriver(CreationParams.WindowSize, CreationParams.Fullscreen, FileSystem, this);
		#else
		os::Printer::log("No Software driver support compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_BURNINGSVIDEO:
		#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
		VideoDriver = video::createBurningVideoDriver(CreationParams, FileSystem, this);
		#else
		os::Printer::log("Burning's video driver was not compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_OPENGL:
		#ifdef _IRR_COMPILE_WITH_OPENGL_
		VideoDriver = video::createOpenGLDriver(CreationParams, FileSystem, this);
		#else
		os::Printer::log("No OpenGL support compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_NULL:
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;

	default:
		os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
		break;
	}
}


//! runs the device. Returns false if device wants to be deleted
bool CIrrDeviceSDL::run()
{
	os::Timer::tick();

	SEvent irrevent;
	SDL_Event SDL_event;

	while ( !Close && SDL_PollEvent( &SDL_event ) )
	{
		switch ( SDL_event.type )
		{
		case SDL_MOUSEMOTION:
			irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
			irrevent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
			MouseX = irrevent.MouseInput.X = SDL_event.motion.x;
			MouseY = irrevent.MouseInput.Y = SDL_event.motion.y;
			irrevent.MouseInput.ButtonStates = MouseButtonStates;

			postEventFromUser(irrevent);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:

			irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
			irrevent.MouseInput.X = SDL_event.button.x;
			irrevent.MouseInput.Y = SDL_event.button.y;

			irrevent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;

			switch(SDL_event.button.button)
			{
			case SDL_BUTTON_LEFT:
				if (SDL_event.type == SDL_MOUSEBUTTONDOWN)
				{
					irrevent.MouseInput.Event = irr::EMIE_LMOUSE_PRESSED_DOWN;
					MouseButtonStates |= irr::EMBSM_LEFT;
				}
				else
				{
					irrevent.MouseInput.Event = irr::EMIE_LMOUSE_LEFT_UP;
					MouseButtonStates &= !irr::EMBSM_LEFT;
				}
				break;

			case SDL_BUTTON_RIGHT:
				if (SDL_event.type == SDL_MOUSEBUTTONDOWN)
				{
					irrevent.MouseInput.Event = irr::EMIE_RMOUSE_PRESSED_DOWN;
					MouseButtonStates |= irr::EMBSM_RIGHT;
				}
				else
				{
					irrevent.MouseInput.Event = irr::EMIE_RMOUSE_LEFT_UP;
					MouseButtonStates &= !irr::EMBSM_RIGHT;
				}
				break;

			case SDL_BUTTON_MIDDLE:
				if (SDL_event.type == SDL_MOUSEBUTTONDOWN)
				{
					irrevent.MouseInput.Event = irr::EMIE_MMOUSE_PRESSED_DOWN;
					MouseButtonStates |= irr::EMBSM_MIDDLE;
				}
				else
				{
					irrevent.MouseInput.Event = irr::EMIE_MMOUSE_LEFT_UP;
					MouseButtonStates &= !irr::EMBSM_MIDDLE;
				}
				break;

			case SDL_BUTTON_WHEELUP:
				irrevent.MouseInput.Event = irr::EMIE_MOUSE_WHEEL;
				irrevent.MouseInput.Wheel = 1.0f;
				break;

			case SDL_BUTTON_WHEELDOWN:
				irrevent.MouseInput.Event = irr::EMIE_MOUSE_WHEEL;
				irrevent.MouseInput.Wheel = -1.0f;
				break;
			}

			irrevent.MouseInput.ButtonStates = MouseButtonStates;

			if (irrevent.MouseInput.Event != irr::EMIE_MOUSE_MOVED)
			{
				postEventFromUser(irrevent);

				if ( irrevent.MouseInput.Event >= EMIE_LMOUSE_PRESSED_DOWN && irrevent.MouseInput.Event <= EMIE_MMOUSE_PRESSED_DOWN )
				{
					u32 clicks = checkSuccessiveClicks(irrevent.MouseInput.X, irrevent.MouseInput.Y, irrevent.MouseInput.Event);
					if ( clicks == 2 )
					{
						irrevent.MouseInput.Event = (EMOUSE_INPUT_EVENT)(EMIE_LMOUSE_DOUBLE_CLICK + irrevent.MouseInput.Event-EMIE_LMOUSE_PRESSED_DOWN);
						postEventFromUser(irrevent);
					}
					else if ( clicks == 3 )
					{
						irrevent.MouseInput.Event = (EMOUSE_INPUT_EVENT)(EMIE_LMOUSE_TRIPLE_CLICK + irrevent.MouseInput.Event-EMIE_LMOUSE_PRESSED_DOWN);
						postEventFromUser(irrevent);
					}
				}
			}
			break;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			{
				SKeyMap mp;
				mp.SDLKey = SDL_event.key.keysym.sym;
				s32 idx = KeyMap.binary_search(mp);

				EKEY_CODE key;
				if (idx == -1)
					key = (EKEY_CODE)0;
				else
					key = (EKEY_CODE)KeyMap[idx].Win32Key;

#ifdef _IRR_WINDOWS_API_
				// handle alt+f4 in Windows, because SDL seems not to
				if ( (SDL_event.key.keysym.mod & KMOD_LALT) && key == KEY_F4)
				{
					Close = true;
					break;
				}
#endif
				irrevent.EventType = irr::EET_KEY_INPUT_EVENT;
				irrevent.KeyInput.Char = SDL_event.key.keysym.unicode;
				irrevent.KeyInput.Key = key;
				irrevent.KeyInput.PressedDown = (SDL_event.type == SDL_KEYDOWN);
				irrevent.KeyInput.Shift = (SDL_event.key.keysym.mod & KMOD_SHIFT) != 0;
				irrevent.KeyInput.Control = (SDL_event.key.keysym.mod & KMOD_CTRL ) != 0;
				postEventFromUser(irrevent);
			}
			break;

		case SDL_QUIT:
			Close = true;
			break;

		case SDL_ACTIVEEVENT:
			if ((SDL_event.active.state == SDL_APPMOUSEFOCUS) ||
					(SDL_event.active.state == SDL_APPINPUTFOCUS))
				WindowHasFocus = (SDL_event.active.gain==1);
			else
			if (SDL_event.active.state == SDL_APPACTIVE)
				WindowMinimized = (SDL_event.active.gain!=1);
			break;

		case SDL_VIDEORESIZE:
			if ((SDL_event.resize.w != (int)Width) || (SDL_event.resize.h != (int)Height))
			{
				Width = SDL_event.resize.w;
				Height = SDL_event.resize.h;
				Screen = SDL_SetVideoMode( Width, Height, 0, SDL_Flags );
				if (VideoDriver)
					VideoDriver->OnResize(core::dimension2d<u32>(Width, Height));
			}
			break;

		case SDL_USEREVENT:
			irrevent.EventType = irr::EET_USER_EVENT;
			irrevent.UserEvent.UserData1 = *(reinterpret_cast<s32*>(&SDL_event.user.data1));
			irrevent.UserEvent.UserData2 = *(reinterpret_cast<s32*>(&SDL_event.user.data2));

			postEventFromUser(irrevent);
			break;

		default:
			break;
		} // end switch

	} // end while

#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
	// TODO: Check if the multiple open/close calls are too expensive, then
	// open/close in the constructor/destructor instead

	// update joystick states manually
	SDL_JoystickUpdate();
	// we'll always send joystick input events...
	SEvent joyevent;
	joyevent.EventType = EET_JOYSTICK_INPUT_EVENT;
	for (u32 i=0; i<Joysticks.size(); ++i)
	{
		SDL_Joystick* joystick = Joysticks[i];
		if (joystick)
		{
			int j;
			// query all buttons
			const int numButtons = core::min_(SDL_JoystickNumButtons(joystick), 32);
			joyevent.JoystickEvent.ButtonStates=0;
			for (j=0; j<numButtons; ++j)
				joyevent.JoystickEvent.ButtonStates |= (SDL_JoystickGetButton(joystick, j)<<j);

			// query all axes, already in correct range
			const int numAxes = core::min_(SDL_JoystickNumAxes(joystick), (int)SEvent::SJoystickEvent::NUMBER_OF_AXES);
			joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_X]=0;
			joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_Y]=0;
			joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_Z]=0;
			joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_R]=0;
			joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_U]=0;
			joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_V]=0;
			for (j=0; j<numAxes; ++j)
				joyevent.JoystickEvent.Axis[j] = SDL_JoystickGetAxis(joystick, j);

			// we can only query one hat, SDL only supports 8 directions
			if (SDL_JoystickNumHats(joystick)>0)
			{
				switch (SDL_JoystickGetHat(joystick, 0))
				{
					case SDL_HAT_UP:
						joyevent.JoystickEvent.POV=0;
						break;
					case SDL_HAT_RIGHTUP:
						joyevent.JoystickEvent.POV=4500;
						break;
					case SDL_HAT_RIGHT:
						joyevent.JoystickEvent.POV=9000;
						break;
					case SDL_HAT_RIGHTDOWN:
						joyevent.JoystickEvent.POV=13500;
						break;
					case SDL_HAT_DOWN:
						joyevent.JoystickEvent.POV=18000;
						break;
					case SDL_HAT_LEFTDOWN:
						joyevent.JoystickEvent.POV=22500;
						break;
					case SDL_HAT_LEFT:
						joyevent.JoystickEvent.POV=27000;
						break;
					case SDL_HAT_LEFTUP:
						joyevent.JoystickEvent.POV=31500;
						break;
					case SDL_HAT_CENTERED:
					default:
						joyevent.JoystickEvent.POV=65535;
						break;
				}
			}
			else
			{
				joyevent.JoystickEvent.POV=65535;
			}

			// we map the number directly
			joyevent.JoystickEvent.Joystick=static_cast<u8>(i);
			// now post the event
			postEventFromUser(joyevent);
			// and close the joystick
		}
	}
#endif
	return !Close;
}

//! Activate any joysticks, and generate events for them.
bool CIrrDeviceSDL::activateJoysticks(core::array<SJoystickInfo> & joystickInfo)
{
#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
	joystickInfo.clear();

	// we can name up to 256 different joysticks
	const int numJoysticks = core::min_(SDL_NumJoysticks(), 256);
	Joysticks.reallocate(numJoysticks);
	joystickInfo.reallocate(numJoysticks);

	int joystick = 0;
	for (; joystick<numJoysticks; ++joystick)
	{
		Joysticks.push_back(SDL_JoystickOpen(joystick));
		SJoystickInfo info;

		info.Joystick = joystick;
		info.Axes = SDL_JoystickNumAxes(Joysticks[joystick]);
		info.Buttons = SDL_JoystickNumButtons(Joysticks[joystick]);
		info.Name = SDL_JoystickName(joystick);
		info.PovHat = (SDL_JoystickNumHats(Joysticks[joystick]) > 0)
						? SJoystickInfo::POV_HAT_PRESENT : SJoystickInfo::POV_HAT_ABSENT;

		joystickInfo.push_back(info);
	}

	for(joystick = 0; joystick < (int)joystickInfo.size(); ++joystick)
	{
		char logString[256];
		(void)sprintf(logString, "Found joystick %d, %d axes, %d buttons '%s'",
		joystick, joystickInfo[joystick].Axes,
		joystickInfo[joystick].Buttons, joystickInfo[joystick].Name.c_str());
		os::Printer::log(logString, ELL_INFORMATION);
	}

	return true;

#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_

	return false;
}



//! pause execution temporarily
void CIrrDeviceSDL::yield()
{
	SDL_Delay(0);
}


//! pause execution for a specified time
void CIrrDeviceSDL::sleep(u32 timeMs, bool pauseTimer)
{
	const bool wasStopped = Timer ? Timer->isStopped() : true;
	if (pauseTimer && !wasStopped)
		Timer->stop();

	SDL_Delay(timeMs);

	if (pauseTimer && !wasStopped)
		Timer->start();
}


//! sets the caption of the window
void CIrrDeviceSDL::setWindowCaption(const wchar_t* text)
{
	core::stringc textc = text;
	SDL_WM_SetCaption( textc.c_str( ), textc.c_str( ) );
}


//! presents a surface in the client area
bool CIrrDeviceSDL::present(video::IImage* surface, void* windowId, core::rect<s32>* srcClip)
{
	SDL_Surface *sdlSurface = SDL_CreateRGBSurfaceFrom(
			surface->lock(), surface->getDimension().Width, surface->getDimension().Height,
			surface->getBitsPerPixel(), surface->getPitch(),
			surface->getRedMask(), surface->getGreenMask(), surface->getBlueMask(), surface->getAlphaMask());
	if (!sdlSurface)
		return false;
	SDL_SetAlpha(sdlSurface, 0, 0);
	SDL_SetColorKey(sdlSurface, 0, 0);
	sdlSurface->format->BitsPerPixel=surface->getBitsPerPixel();
	sdlSurface->format->BytesPerPixel=surface->getBytesPerPixel();
	if ((surface->getColorFormat()==video::ECF_R8G8B8) ||
			(surface->getColorFormat()==video::ECF_A8R8G8B8))
	{
		sdlSurface->format->Rloss=0;
		sdlSurface->format->Gloss=0;
		sdlSurface->format->Bloss=0;
		sdlSurface->format->Rshift=16;
		sdlSurface->format->Gshift=8;
		sdlSurface->format->Bshift=0;
		if (surface->getColorFormat()==video::ECF_R8G8B8)
		{
			sdlSurface->format->Aloss=8;
			sdlSurface->format->Ashift=32;
		}
		else
		{
			sdlSurface->format->Aloss=0;
			sdlSurface->format->Ashift=24;
		}
	}
	else if (surface->getColorFormat()==video::ECF_R5G6B5)
	{
		sdlSurface->format->Rloss=3;
		sdlSurface->format->Gloss=2;
		sdlSurface->format->Bloss=3;
		sdlSurface->format->Aloss=8;
		sdlSurface->format->Rshift=11;
		sdlSurface->format->Gshift=5;
		sdlSurface->format->Bshift=0;
		sdlSurface->format->Ashift=16;
	}
	else if (surface->getColorFormat()==video::ECF_A1R5G5B5)
	{
		sdlSurface->format->Rloss=3;
		sdlSurface->format->Gloss=3;
		sdlSurface->format->Bloss=3;
		sdlSurface->format->Aloss=7;
		sdlSurface->format->Rshift=10;
		sdlSurface->format->Gshift=5;
		sdlSurface->format->Bshift=0;
		sdlSurface->format->Ashift=15;
	}

	SDL_Surface* scr = (SDL_Surface* )windowId;
	if (!scr)
		scr = Screen;
	if (scr)
	{
		if (srcClip)
		{
			SDL_Rect sdlsrcClip;
			sdlsrcClip.x = srcClip->UpperLeftCorner.X;
			sdlsrcClip.y = srcClip->UpperLeftCorner.Y;
			sdlsrcClip.w = srcClip->getWidth();
			sdlsrcClip.h = srcClip->getHeight();
			SDL_BlitSurface(sdlSurface, &sdlsrcClip, scr, NULL);
		}
		else
			SDL_BlitSurface(sdlSurface, NULL, scr, NULL);
		SDL_Flip(scr);
	}

	SDL_FreeSurface(sdlSurface);
	surface->unlock();
	return (scr != 0);
}


//! notifies the device that it should close itself
void CIrrDeviceSDL::closeDevice()
{
	Close = true;
}


//! \return Pointer to a list with all video modes supported
video::IVideoModeList* CIrrDeviceSDL::getVideoModeList()
{
	if (!VideoModeList->getVideoModeCount())
	{
		// enumerate video modes.
		const SDL_VideoInfo *vi = SDL_GetVideoInfo();
		SDL_Rect **modes = SDL_ListModes(vi->vfmt, SDL_Flags);
		if (modes != 0)
		{
			if (modes == (SDL_Rect **)-1)
				os::Printer::log("All modes available.\n");
			else
			{
				for (u32 i=0; modes[i]; ++i)
					VideoModeList->addMode(core::dimension2d<u32>(modes[i]->w, modes[i]->h), vi->vfmt->BitsPerPixel);
			}
		}
	}

	return VideoModeList;
}


//! Sets if the window should be resizable in windowed mode.
void CIrrDeviceSDL::setResizable(bool resize)
{
	if (resize != Resizable)
	{
		if (resize)
			SDL_Flags |= SDL_RESIZABLE;
		else
			SDL_Flags &= ~SDL_RESIZABLE;
		Screen = SDL_SetVideoMode( 0, 0, 0, SDL_Flags );
		Resizable = resize;
	}
}


//! Minimizes window if possible
void CIrrDeviceSDL::minimizeWindow()
{
	SDL_WM_IconifyWindow();
}


//! Maximize window
void CIrrDeviceSDL::maximizeWindow()
{
	// do nothing
}


//! Restore original window size
void CIrrDeviceSDL::restoreWindow()
{
	// do nothing
}


//! returns if window is active. if not, nothing need to be drawn
bool CIrrDeviceSDL::isWindowActive() const
{
	return (WindowHasFocus && !WindowMinimized);
}


//! returns if window has focus.
bool CIrrDeviceSDL::isWindowFocused() const
{
	return WindowHasFocus;
}


//! returns if window is minimized.
bool CIrrDeviceSDL::isWindowMinimized() const
{
	return WindowMinimized;
}


//! Set the current Gamma Value for the Display
bool CIrrDeviceSDL::setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast )
{
	/*
	// todo: Gamma in SDL takes ints, what does Irrlicht use?
	return (SDL_SetGamma(red, green, blue) != -1);
	*/
	return false;
}

//! Get the current Gamma Value for the Display
bool CIrrDeviceSDL::getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast )
{
/*	brightness = 0.f;
	contrast = 0.f;
	return (SDL_GetGamma(&red, &green, &blue) != -1);*/
	return false;
}

//! returns color format of the window.
video::ECOLOR_FORMAT CIrrDeviceSDL::getColorFormat() const
{
	if (Screen)
	{
		if (Screen->format->BitsPerPixel==16)
		{
			if (Screen->format->Amask != 0)
				return video::ECF_A1R5G5B5;
			else
				return video::ECF_R5G6B5;
		}
		else
		{
			if (Screen->format->Amask != 0)
				return video::ECF_A8R8G8B8;
			else
				return video::ECF_R8G8B8;
		}
	}
	else
		return CIrrDeviceStub::getColorFormat();
}


void CIrrDeviceSDL::createKeyMap()
{
	// I don't know if this is the best method  to create
	// the lookuptable, but I'll leave it like that until
	// I find a better version.

	KeyMap.reallocate(105);

	// buttons missing

	KeyMap.push_back(SKeyMap(SDLK_BACKSPACE, KEY_BACK));
	KeyMap.push_back(SKeyMap(SDLK_TAB, KEY_TAB));
	KeyMap.push_back(SKeyMap(SDLK_CLEAR, KEY_CLEAR));
	KeyMap.push_back(SKeyMap(SDLK_RETURN, KEY_RETURN));

	// combined modifiers missing

	KeyMap.push_back(SKeyMap(SDLK_PAUSE, KEY_PAUSE));
	KeyMap.push_back(SKeyMap(SDLK_CAPSLOCK, KEY_CAPITAL));

	// asian letter keys missing

	KeyMap.push_back(SKeyMap(SDLK_ESCAPE, KEY_ESCAPE));

	// asian letter keys missing

	KeyMap.push_back(SKeyMap(SDLK_SPACE, KEY_SPACE));
	KeyMap.push_back(SKeyMap(SDLK_PAGEUP, KEY_PRIOR));
	KeyMap.push_back(SKeyMap(SDLK_PAGEDOWN, KEY_NEXT));
	KeyMap.push_back(SKeyMap(SDLK_END, KEY_END));
	KeyMap.push_back(SKeyMap(SDLK_HOME, KEY_HOME));
	KeyMap.push_back(SKeyMap(SDLK_LEFT, KEY_LEFT));
	KeyMap.push_back(SKeyMap(SDLK_UP, KEY_UP));
	KeyMap.push_back(SKeyMap(SDLK_RIGHT, KEY_RIGHT));
	KeyMap.push_back(SKeyMap(SDLK_DOWN, KEY_DOWN));

	// select missing
	KeyMap.push_back(SKeyMap(SDLK_PRINT, KEY_PRINT));
	// execute missing
	KeyMap.push_back(SKeyMap(SDLK_PRINT, KEY_SNAPSHOT));

	KeyMap.push_back(SKeyMap(SDLK_INSERT, KEY_INSERT));
	KeyMap.push_back(SKeyMap(SDLK_DELETE, KEY_DELETE));
	KeyMap.push_back(SKeyMap(SDLK_HELP, KEY_HELP));

	KeyMap.push_back(SKeyMap(SDLK_0, KEY_KEY_0));
	KeyMap.push_back(SKeyMap(SDLK_1, KEY_KEY_1));
	KeyMap.push_back(SKeyMap(SDLK_2, KEY_KEY_2));
	KeyMap.push_back(SKeyMap(SDLK_3, KEY_KEY_3));
	KeyMap.push_back(SKeyMap(SDLK_4, KEY_KEY_4));
	KeyMap.push_back(SKeyMap(SDLK_5, KEY_KEY_5));
	KeyMap.push_back(SKeyMap(SDLK_6, KEY_KEY_6));
	KeyMap.push_back(SKeyMap(SDLK_7, KEY_KEY_7));
	KeyMap.push_back(SKeyMap(SDLK_8, KEY_KEY_8));
	KeyMap.push_back(SKeyMap(SDLK_9, KEY_KEY_9));

	KeyMap.push_back(SKeyMap(SDLK_a, KEY_KEY_A));
	KeyMap.push_back(SKeyMap(SDLK_b, KEY_KEY_B));
	KeyMap.push_back(SKeyMap(SDLK_c, KEY_KEY_C));
	KeyMap.push_back(SKeyMap(SDLK_d, KEY_KEY_D));
	KeyMap.push_back(SKeyMap(SDLK_e, KEY_KEY_E));
	KeyMap.push_back(SKeyMap(SDLK_f, KEY_KEY_F));
	KeyMap.push_back(SKeyMap(SDLK_g, KEY_KEY_G));
	KeyMap.push_back(SKeyMap(SDLK_h, KEY_KEY_H));
	KeyMap.push_back(SKeyMap(SDLK_i, KEY_KEY_I));
	KeyMap.push_back(SKeyMap(SDLK_j, KEY_KEY_J));
	KeyMap.push_back(SKeyMap(SDLK_k, KEY_KEY_K));
	KeyMap.push_back(SKeyMap(SDLK_l, KEY_KEY_L));
	KeyMap.push_back(SKeyMap(SDLK_m, KEY_KEY_M));
	KeyMap.push_back(SKeyMap(SDLK_n, KEY_KEY_N));
	KeyMap.push_back(SKeyMap(SDLK_o, KEY_KEY_O));
	KeyMap.push_back(SKeyMap(SDLK_p, KEY_KEY_P));
	KeyMap.push_back(SKeyMap(SDLK_q, KEY_KEY_Q));
	KeyMap.push_back(SKeyMap(SDLK_r, KEY_KEY_R));
	KeyMap.push_back(SKeyMap(SDLK_s, KEY_KEY_S));
	KeyMap.push_back(SKeyMap(SDLK_t, KEY_KEY_T));
	KeyMap.push_back(SKeyMap(SDLK_u, KEY_KEY_U));
	KeyMap.push_back(SKeyMap(SDLK_v, KEY_KEY_V));
	KeyMap.push_back(SKeyMap(SDLK_w, KEY_KEY_W));
	KeyMap.push_back(SKeyMap(SDLK_x, KEY_KEY_X));
	KeyMap.push_back(SKeyMap(SDLK_y, KEY_KEY_Y));
	KeyMap.push_back(SKeyMap(SDLK_z, KEY_KEY_Z));

	KeyMap.push_back(SKeyMap(SDLK_LSUPER, KEY_LWIN));
	KeyMap.push_back(SKeyMap(SDLK_RSUPER, KEY_RWIN));
	// apps missing
	KeyMap.push_back(SKeyMap(SDLK_POWER, KEY_SLEEP)); //??

	KeyMap.push_back(SKeyMap(SDLK_KP0, KEY_NUMPAD0));
	KeyMap.push_back(SKeyMap(SDLK_KP1, KEY_NUMPAD1));
	KeyMap.push_back(SKeyMap(SDLK_KP2, KEY_NUMPAD2));
	KeyMap.push_back(SKeyMap(SDLK_KP3, KEY_NUMPAD3));
	KeyMap.push_back(SKeyMap(SDLK_KP4, KEY_NUMPAD4));
	KeyMap.push_back(SKeyMap(SDLK_KP5, KEY_NUMPAD5));
	KeyMap.push_back(SKeyMap(SDLK_KP6, KEY_NUMPAD6));
	KeyMap.push_back(SKeyMap(SDLK_KP7, KEY_NUMPAD7));
	KeyMap.push_back(SKeyMap(SDLK_KP8, KEY_NUMPAD8));
	KeyMap.push_back(SKeyMap(SDLK_KP9, KEY_NUMPAD9));
	KeyMap.push_back(SKeyMap(SDLK_KP_MULTIPLY, KEY_MULTIPLY));
	KeyMap.push_back(SKeyMap(SDLK_KP_PLUS, KEY_ADD));
//	KeyMap.push_back(SKeyMap(SDLK_KP_, KEY_SEPARATOR));
	KeyMap.push_back(SKeyMap(SDLK_KP_MINUS, KEY_SUBTRACT));
	KeyMap.push_back(SKeyMap(SDLK_KP_PERIOD, KEY_DECIMAL));
	KeyMap.push_back(SKeyMap(SDLK_KP_DIVIDE, KEY_DIVIDE));

	KeyMap.push_back(SKeyMap(SDLK_F1,  KEY_F1));
	KeyMap.push_back(SKeyMap(SDLK_F2,  KEY_F2));
	KeyMap.push_back(SKeyMap(SDLK_F3,  KEY_F3));
	KeyMap.push_back(SKeyMap(SDLK_F4,  KEY_F4));
	KeyMap.push_back(SKeyMap(SDLK_F5,  KEY_F5));
	KeyMap.push_back(SKeyMap(SDLK_F6,  KEY_F6));
	KeyMap.push_back(SKeyMap(SDLK_F7,  KEY_F7));
	KeyMap.push_back(SKeyMap(SDLK_F8,  KEY_F8));
	KeyMap.push_back(SKeyMap(SDLK_F9,  KEY_F9));
	KeyMap.push_back(SKeyMap(SDLK_F10, KEY_F10));
	KeyMap.push_back(SKeyMap(SDLK_F11, KEY_F11));
	KeyMap.push_back(SKeyMap(SDLK_F12, KEY_F12));
	KeyMap.push_back(SKeyMap(SDLK_F13, KEY_F13));
	KeyMap.push_back(SKeyMap(SDLK_F14, KEY_F14));
	KeyMap.push_back(SKeyMap(SDLK_F15, KEY_F15));
	// no higher F-keys

	KeyMap.push_back(SKeyMap(SDLK_NUMLOCK, KEY_NUMLOCK));
	KeyMap.push_back(SKeyMap(SDLK_SCROLLOCK, KEY_SCROLL));
	KeyMap.push_back(SKeyMap(SDLK_LSHIFT, KEY_LSHIFT));
	KeyMap.push_back(SKeyMap(SDLK_RSHIFT, KEY_RSHIFT));
	KeyMap.push_back(SKeyMap(SDLK_LCTRL,  KEY_LCONTROL));
	KeyMap.push_back(SKeyMap(SDLK_RCTRL,  KEY_RCONTROL));
	KeyMap.push_back(SKeyMap(SDLK_LALT,  KEY_LMENU));
	KeyMap.push_back(SKeyMap(SDLK_RALT,  KEY_RMENU));

	KeyMap.push_back(SKeyMap(SDLK_PLUS,   KEY_PLUS));
	KeyMap.push_back(SKeyMap(SDLK_COMMA,  KEY_COMMA));
	KeyMap.push_back(SKeyMap(SDLK_MINUS,  KEY_MINUS));
	KeyMap.push_back(SKeyMap(SDLK_PERIOD, KEY_PERIOD));

	// some special keys missing

	KeyMap.sort();
}

} // end namespace irr

#endif // _IRR_COMPILE_WITH_SDL_DEVICE_

