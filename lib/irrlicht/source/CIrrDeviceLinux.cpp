// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CIrrDeviceLinux.h"

#ifdef _IRR_COMPILE_WITH_X11_DEVICE_

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <time.h>
#include "IEventReceiver.h"
#include "ISceneManager.h"
#include "IGUIEnvironment.h"
#include "os.h"
#include "CTimer.h"
#include "irrString.h"
#include "Keycodes.h"
#include "COSOperator.h"
#include "CColorConverter.h"
#include "SIrrCreationParameters.h"
#include "IGUISpriteBank.h"
#include <X11/XKBlib.h>
#include <X11/Xatom.h>

#ifdef _IRR_LINUX_XCURSOR_
#include <X11/Xcursor/Xcursor.h>
#endif

#if defined _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
#include <fcntl.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <sys/joystick.h>
#else

// linux/joystick.h includes linux/input.h, which #defines values for various KEY_FOO keys.
// These override the irr::KEY_FOO equivalents, which stops key handling from working.
// As a workaround, defining _INPUT_H stops linux/input.h from being included; it
// doesn't actually seem to be necessary except to pull in sys/ioctl.h.
#define _INPUT_H
#include <sys/ioctl.h> // Would normally be included in linux/input.h
#include <linux/joystick.h>
#undef _INPUT_H
#endif

#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_

namespace irr
{
	namespace video
	{
		IVideoDriver* createOpenGLDriver(const SIrrlichtCreationParameters& params,
				io::IFileSystem* io, CIrrDeviceLinux* device);
	}
} // end namespace irr

namespace
{
	Atom X_ATOM_CLIPBOARD;
	Atom X_ATOM_TARGETS;
	Atom X_ATOM_UTF8_STRING;
	Atom X_ATOM_TEXT;
};

namespace irr
{

const char* wmDeleteWindow = "WM_DELETE_WINDOW";

//! constructor
CIrrDeviceLinux::CIrrDeviceLinux(const SIrrlichtCreationParameters& param)
	: CIrrDeviceStub(param),
#ifdef _IRR_COMPILE_WITH_X11_
	display(0), visual(0), screennr(0), window(0), StdHints(0), SoftwareImage(0),
#ifdef _IRR_COMPILE_WITH_OPENGL_
	glxWin(0),
	Context(0),
#endif
#endif
	Width(param.WindowSize.Width), Height(param.WindowSize.Height),
	WindowHasFocus(false), WindowMinimized(false),
	UseXVidMode(false), UseXRandR(false), UseGLXWindow(false),
	ExternalWindow(false), AutorepeatSupport(0)
{
	#ifdef _DEBUG
	setDebugName("CIrrDeviceLinux");
	#endif

	// print version, distribution etc.
	// thx to LynxLuna for pointing me to the uname function
	core::stringc linuxversion;
	struct utsname LinuxInfo;
	uname(&LinuxInfo);

	linuxversion += LinuxInfo.sysname;
	linuxversion += " ";
	linuxversion += LinuxInfo.release;
	linuxversion += " ";
	linuxversion += LinuxInfo.version;
	linuxversion += " ";
	linuxversion += LinuxInfo.machine;

	Operator = new COSOperator(linuxversion, this);
	os::Printer::log(linuxversion.c_str(), ELL_INFORMATION);

	// create keymap
	createKeyMap();

	// create window
	if (CreationParams.DriverType != video::EDT_NULL)
	{
		// create the window, only if we do not use the null device
		if (!createWindow())
			return;
	}

	// create cursor control
	CursorControl = new CCursorControl(this, CreationParams.DriverType == video::EDT_NULL);

	// create driver
	createDriver();

	if (!VideoDriver)
		return;

	createGUIAndScene();
}


//! destructor
CIrrDeviceLinux::~CIrrDeviceLinux()
{
#ifdef _IRR_COMPILE_WITH_X11_
	if (StdHints)
		XFree(StdHints);
	// Disable cursor (it is drop'ed in stub)
	if (CursorControl)
	{
		CursorControl->setVisible(false);
		static_cast<CCursorControl*>(CursorControl)->clearCursors();
	}

	// Must free OpenGL textures etc before destroying context, so can't wait for stub destructor
	if ( GUIEnvironment )
	{
		GUIEnvironment->drop();
		GUIEnvironment = NULL;
	}
	if ( SceneManager )
	{
		SceneManager->drop();
		SceneManager = NULL;
	}
	if ( VideoDriver )
	{
		VideoDriver->drop();
		VideoDriver = NULL;
	}

	if (display)
	{
		#ifdef _IRR_COMPILE_WITH_OPENGL_
		if (Context)
		{
			if (glxWin)
			{
				if (!glXMakeContextCurrent(display, None, None, NULL))
					os::Printer::log("Could not release glx context.", ELL_WARNING);
			}
			else
			{
				if (!glXMakeCurrent(display, None, NULL))
					os::Printer::log("Could not release glx context.", ELL_WARNING);
			}
			glXDestroyContext(display, Context);
			if (glxWin)
				glXDestroyWindow(display, glxWin);
		}
		#endif // #ifdef _IRR_COMPILE_WITH_OPENGL_

		// Reset fullscreen resolution change
		switchToFullscreen(true);

		if (SoftwareImage)
			XDestroyImage(SoftwareImage);

		if (!ExternalWindow)
		{
			XDestroyWindow(display,window);
			XCloseDisplay(display);
		}
	}
	if (visual)
		XFree(visual);

#endif // #ifdef _IRR_COMPILE_WITH_X11_

#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
	for (u32 joystick = 0; joystick < ActiveJoysticks.size(); ++joystick)
	{
		if (ActiveJoysticks[joystick].fd >= 0)
		{
			close(ActiveJoysticks[joystick].fd);
		}
	}
#endif
}


#if defined(_IRR_COMPILE_WITH_X11_) && defined(_DEBUG)
int IrrPrintXError(Display *display, XErrorEvent *event)
{
	char msg[256];
	char msg2[256];

	snprintf(msg, 256, "%d", event->request_code);
	XGetErrorDatabaseText(display, "XRequest", msg, "unknown", msg2, 256);
	XGetErrorText(display, event->error_code, msg, 256);
	os::Printer::log("X Error", msg, ELL_WARNING);
	os::Printer::log("From call ", msg2, ELL_WARNING);
	return 0;
}
#endif


bool CIrrDeviceLinux::switchToFullscreen(bool reset)
{
	if (!CreationParams.Fullscreen)
		return true;
	if (reset)
	{
#ifdef _IRR_LINUX_X11_VIDMODE_
		if (UseXVidMode && CreationParams.Fullscreen)
		{
			XF86VidModeSwitchToMode(display, screennr, &oldVideoMode);
			XF86VidModeSetViewPort(display, screennr, 0, 0);
		}
		#endif
		#ifdef _IRR_LINUX_X11_RANDR_
		if (UseXRandR && CreationParams.Fullscreen)
		{
			XRRScreenConfiguration *config=XRRGetScreenInfo(display,DefaultRootWindow(display));
			XRRSetScreenConfig(display,config,DefaultRootWindow(display),oldRandrMode,oldRandrRotation,CurrentTime);
			XRRFreeScreenConfigInfo(config);
		}
		#endif
		return true;
	}

	getVideoModeList();
	#if defined(_IRR_LINUX_X11_VIDMODE_) || defined(_IRR_LINUX_X11_RANDR_)
	s32 eventbase, errorbase;
	s32 bestMode = -1;
	#endif

	#ifdef _IRR_LINUX_X11_VIDMODE_
	if (XF86VidModeQueryExtension(display, &eventbase, &errorbase))
	{
		// enumerate video modes
		s32 modeCount;
		XF86VidModeModeInfo** modes;

		XF86VidModeGetAllModeLines(display, screennr, &modeCount, &modes);

		// find fitting mode
		for (s32 i = 0; i<modeCount; ++i)
		{
			if (bestMode==-1 && modes[i]->hdisplay >= Width && modes[i]->vdisplay >= Height)
				bestMode = i;
			else if (bestMode!=-1 &&
					modes[i]->hdisplay >= Width &&
					modes[i]->vdisplay >= Height &&
					modes[i]->hdisplay <= modes[bestMode]->hdisplay &&
					modes[i]->vdisplay <= modes[bestMode]->vdisplay)
				bestMode = i;
		}
		if (bestMode != -1)
		{
			os::Printer::log("Starting vidmode fullscreen mode...", ELL_INFORMATION);
			os::Printer::log("hdisplay: ", core::stringc(modes[bestMode]->hdisplay).c_str(), ELL_INFORMATION);
			os::Printer::log("vdisplay: ", core::stringc(modes[bestMode]->vdisplay).c_str(), ELL_INFORMATION);

			XF86VidModeSwitchToMode(display, screennr, modes[bestMode]);
			XF86VidModeSetViewPort(display, screennr, 0, 0);
			UseXVidMode=true;
		}
		else
		{
			os::Printer::log("Could not find specified video mode, running windowed.", ELL_WARNING);
			CreationParams.Fullscreen = false;
		}

		XFree(modes);
	}
	else
	#endif
	#ifdef _IRR_LINUX_X11_RANDR_
	if (XRRQueryExtension(display, &eventbase, &errorbase))
	{
		s32 modeCount;
		XRRScreenConfiguration *config=XRRGetScreenInfo(display,DefaultRootWindow(display));
		XRRScreenSize *modes=XRRConfigSizes(config,&modeCount);
		for (s32 i = 0; i<modeCount; ++i)
		{
			if (bestMode==-1 && (u32)modes[i].width >= Width && (u32)modes[i].height >= Height)
				bestMode = i;
			else if (bestMode!=-1 &&
					(u32)modes[i].width >= Width &&
					(u32)modes[i].height >= Height &&
					modes[i].width <= modes[bestMode].width &&
					modes[i].height <= modes[bestMode].height)
				bestMode = i;
		}
		if (bestMode != -1)
		{
			os::Printer::log("Starting randr fullscreen mode...", ELL_INFORMATION);
			os::Printer::log("width: ", core::stringc(modes[bestMode].width).c_str(), ELL_INFORMATION);
			os::Printer::log("height: ", core::stringc(modes[bestMode].height).c_str(), ELL_INFORMATION);

			XRRSetScreenConfig(display,config,DefaultRootWindow(display),bestMode,oldRandrRotation,CurrentTime);
			UseXRandR=true;
		}
		XRRFreeScreenConfigInfo(config);
	}
	else
	#endif
	{
		os::Printer::log("VidMode or RandR extension must be installed to allow Irrlicht "
		"to switch to fullscreen mode. Running in windowed mode instead.", ELL_WARNING);
		CreationParams.Fullscreen = false;
	}
	return CreationParams.Fullscreen;
}


#if defined(_IRR_COMPILE_WITH_X11_)
void IrrPrintXGrabError(int grabResult, const c8 * grabCommand )
{
	if ( grabResult == GrabSuccess )
	{
//		os::Printer::log(grabCommand, ": GrabSuccess", ELL_INFORMATION);
		return;
	}

	switch ( grabResult )
	{
		case AlreadyGrabbed:
			os::Printer::log(grabCommand, ": AlreadyGrabbed", ELL_WARNING);
			break;
		case GrabNotViewable:
			os::Printer::log(grabCommand, ": GrabNotViewable", ELL_WARNING);
			break;
		case GrabFrozen:
			os::Printer::log(grabCommand, ": GrabFrozen", ELL_WARNING);
			break;
		case GrabInvalidTime:
			os::Printer::log(grabCommand, ": GrabInvalidTime", ELL_WARNING);
			break;
		default:
			os::Printer::log(grabCommand, ": grab failed with unknown problem", ELL_WARNING);
			break;
	}
}
#endif


bool CIrrDeviceLinux::createWindow()
{
#ifdef _IRR_COMPILE_WITH_X11_
#ifdef _DEBUG
	os::Printer::log("Creating X window...", ELL_INFORMATION);
	XSetErrorHandler(IrrPrintXError);
#endif

	display = XOpenDisplay(0);
	if (!display)
	{
		os::Printer::log("Error: Need running XServer to start Irrlicht Engine.", ELL_ERROR);
		if (XDisplayName(0)[0])
			os::Printer::log("Could not open display", XDisplayName(0), ELL_ERROR);
		else
			os::Printer::log("Could not open display, set DISPLAY variable", ELL_ERROR);
		return false;
	}

	screennr = DefaultScreen(display);

	switchToFullscreen();

#ifdef _IRR_COMPILE_WITH_OPENGL_

	GLXFBConfig glxFBConfig;
	int major, minor;
	bool isAvailableGLX=false;
	if (CreationParams.DriverType==video::EDT_OPENGL)
	{
		isAvailableGLX=glXQueryExtension(display,&major,&minor);
		if (isAvailableGLX && glXQueryVersion(display, &major, &minor))
		{
#ifdef GLX_VERSION_1_3
			typedef GLXFBConfig * ( * PFNGLXCHOOSEFBCONFIGPROC) (Display *dpy, int screen, const int *attrib_list, int *nelements);

#ifdef _IRR_OPENGL_USE_EXTPOINTER_
			PFNGLXCHOOSEFBCONFIGPROC glxChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXChooseFBConfig"));
#else
			PFNGLXCHOOSEFBCONFIGPROC glxChooseFBConfig=glXChooseFBConfig;
#endif
			if (major==1 && minor>2 && glxChooseFBConfig)
			{
				// attribute array for the draw buffer
				int visualAttrBuffer[] =
				{
					GLX_RENDER_TYPE, GLX_RGBA_BIT,
					GLX_RED_SIZE, 4,
					GLX_GREEN_SIZE, 4,
					GLX_BLUE_SIZE, 4,
					GLX_ALPHA_SIZE, CreationParams.WithAlphaChannel?1:0,
					GLX_DEPTH_SIZE, CreationParams.ZBufferBits, //10,11
					GLX_DOUBLEBUFFER, CreationParams.Doublebuffer?True:False,
					GLX_STENCIL_SIZE, CreationParams.Stencilbuffer?1:0,
#if defined(GLX_VERSION_1_4) && defined(GLX_SAMPLE_BUFFERS) // we need to check the extension string!
					GLX_SAMPLE_BUFFERS, 1,
					GLX_SAMPLES, CreationParams.AntiAlias, // 18,19
#elif defined(GLX_ARB_multisample)
					GLX_SAMPLE_BUFFERS_ARB, 1,
					GLX_SAMPLES_ARB, CreationParams.AntiAlias, // 18,19
#elif defined(GLX_SGIS_multisample)
					GLX_SAMPLE_BUFFERS_SGIS, 1,
					GLX_SAMPLES_SGIS, CreationParams.AntiAlias, // 18,19
#endif
//#ifdef GL_ARB_framebuffer_sRGB
//					GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, CreationParams.HandleSRGB,
//#elif defined(GL_EXT_framebuffer_sRGB)
//					GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, CreationParams.HandleSRGB,
//#endif
					GLX_STEREO, CreationParams.Stereobuffer?True:False,
					None
				};

				GLXFBConfig *configList=0;
				int nitems=0;
				if (CreationParams.AntiAlias<2)
				{
					visualAttrBuffer[17] = 0;
					visualAttrBuffer[19] = 0;
				}
				// first round with unchanged values
				{
					configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
					if (!configList && CreationParams.AntiAlias)
					{
						while (!configList && (visualAttrBuffer[19]>1))
						{
							visualAttrBuffer[19] -= 1;
							configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
						}
						if (!configList)
						{
							visualAttrBuffer[17] = 0;
							visualAttrBuffer[19] = 0;
							configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
							if (configList)
							{
								os::Printer::log("No FSAA available.", ELL_WARNING);
								CreationParams.AntiAlias=0;
							}
							else
							{
								//reenable multisampling
								visualAttrBuffer[17] = 1;
								visualAttrBuffer[19] = CreationParams.AntiAlias;
							}
						}
					}
				}
				// Next try with flipped stencil buffer value
				// If the first round was with stencil flag it's now without
				// Other way round also makes sense because some configs
				// only have depth buffer combined with stencil buffer
				if (!configList)
				{
					if (CreationParams.Stencilbuffer)
						os::Printer::log("No stencilbuffer available, disabling stencil shadows.", ELL_WARNING);
					CreationParams.Stencilbuffer = !CreationParams.Stencilbuffer;
					visualAttrBuffer[15]=CreationParams.Stencilbuffer?1:0;

					configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
					if (!configList && CreationParams.AntiAlias)
					{
						while (!configList && (visualAttrBuffer[19]>1))
						{
							visualAttrBuffer[19] -= 1;
							configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
						}
						if (!configList)
						{
							visualAttrBuffer[17] = 0;
							visualAttrBuffer[19] = 0;
							configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
							if (configList)
							{
								os::Printer::log("No FSAA available.", ELL_WARNING);
								CreationParams.AntiAlias=0;
							}
							else
							{
								//reenable multisampling
								visualAttrBuffer[17] = 1;
								visualAttrBuffer[19] = CreationParams.AntiAlias;
							}
						}
					}
				}
				// Next try without double buffer
				if (!configList && CreationParams.Doublebuffer)
				{
					os::Printer::log("No doublebuffering available.", ELL_WARNING);
					CreationParams.Doublebuffer=false;
					visualAttrBuffer[13] = GLX_DONT_CARE;
					CreationParams.Stencilbuffer = false;
					visualAttrBuffer[15]=0;
					configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
					if (!configList && CreationParams.AntiAlias)
					{
						while (!configList && (visualAttrBuffer[19]>1))
						{
							visualAttrBuffer[19] -= 1;
							configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
						}
						if (!configList)
						{
							visualAttrBuffer[17] = 0;
							visualAttrBuffer[19] = 0;
							configList=glxChooseFBConfig(display, screennr, visualAttrBuffer,&nitems);
							if (configList)
							{
								os::Printer::log("No FSAA available.", ELL_WARNING);
								CreationParams.AntiAlias=0;
							}
							else
							{
								//reenable multisampling
								visualAttrBuffer[17] = 1;
								visualAttrBuffer[19] = CreationParams.AntiAlias;
							}
						}
					}
				}
				if (configList)
				{
					glxFBConfig=configList[0];
					XFree(configList);
					UseGLXWindow=true;
#ifdef _IRR_OPENGL_USE_EXTPOINTER_
					typedef XVisualInfo * ( * PFNGLXGETVISUALFROMFBCONFIGPROC) (Display *dpy, GLXFBConfig config);
					PFNGLXGETVISUALFROMFBCONFIGPROC glxGetVisualFromFBConfig= (PFNGLXGETVISUALFROMFBCONFIGPROC)glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXGetVisualFromFBConfig"));
					if (glxGetVisualFromFBConfig)
						visual = glxGetVisualFromFBConfig(display,glxFBConfig);
#else
						visual = glXGetVisualFromFBConfig(display,glxFBConfig);
#endif
				}
			}
			else
#endif
			{
				// attribute array for the draw buffer
				int visualAttrBuffer[] =
				{
					GLX_RGBA, GLX_USE_GL,
					GLX_RED_SIZE, 4,
					GLX_GREEN_SIZE, 4,
					GLX_BLUE_SIZE, 4,
					GLX_ALPHA_SIZE, CreationParams.WithAlphaChannel?1:0,
					GLX_DEPTH_SIZE, CreationParams.ZBufferBits,
					GLX_STENCIL_SIZE, CreationParams.Stencilbuffer?1:0, // 12,13
					// The following attributes have no flags, but are
					// either present or not. As a no-op we use
					// GLX_USE_GL, which is silently ignored by glXChooseVisual
					CreationParams.Doublebuffer?GLX_DOUBLEBUFFER:GLX_USE_GL, // 14
					CreationParams.Stereobuffer?GLX_STEREO:GLX_USE_GL, // 15
//#ifdef GL_ARB_framebuffer_sRGB
//					CreationParams.HandleSRGB?GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB:GLX_USE_GL,
//#elif defined(GL_EXT_framebuffer_sRGB)
//					CreationParams.HandleSRGB?GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT:GLX_USE_GL,
//#endif
					None
				};

				visual=glXChooseVisual(display, screennr, visualAttrBuffer);
				if (!visual)
				{
					if (CreationParams.Stencilbuffer)
						os::Printer::log("No stencilbuffer available, disabling.", ELL_WARNING);
					CreationParams.Stencilbuffer = !CreationParams.Stencilbuffer;
					visualAttrBuffer[13]=CreationParams.Stencilbuffer?1:0;

					visual=glXChooseVisual(display, screennr, visualAttrBuffer);
					if (!visual && CreationParams.Doublebuffer)
					{
						os::Printer::log("No doublebuffering available.", ELL_WARNING);
						CreationParams.Doublebuffer=false;
						visualAttrBuffer[14] = GLX_USE_GL;
						visual=glXChooseVisual(display, screennr, visualAttrBuffer);
					}
				}
			}
		}
		else
			os::Printer::log("No GLX support available. OpenGL driver will not work.", ELL_WARNING);
	}
	// don't use the XVisual with OpenGL, because it ignores all requested
	// properties of the CreationParams
	else if (!visual)
#endif // _IRR_COMPILE_WITH_OPENGL_

	// create visual with standard X methods
	{
		os::Printer::log("Using plain X visual");
		XVisualInfo visTempl; //Template to hold requested values
		int visNumber; // Return value of available visuals

		visTempl.screen = screennr;
		// ARGB visuals should be avoided for usual applications
		visTempl.depth = CreationParams.WithAlphaChannel?32:24;
		while ((!visual) && (visTempl.depth>=16))
		{
			visual = XGetVisualInfo(display, VisualScreenMask|VisualDepthMask,
				&visTempl, &visNumber);
			visTempl.depth -= 8;
		}
	}

	if (!visual)
	{
		os::Printer::log("Fatal error, could not get visual.", ELL_ERROR);
		XCloseDisplay(display);
		display=0;
		return false;
	}
#ifdef _DEBUG
	else
		os::Printer::log("Visual chosen: ", core::stringc(static_cast<u32>(visual->visualid)).c_str(), ELL_DEBUG);
#endif

	// create color map
	Colormap colormap;
	colormap = XCreateColormap(display,
			RootWindow(display, visual->screen),
			visual->visual, AllocNone);

	attributes.colormap = colormap;
	attributes.border_pixel = 0;
	attributes.event_mask = StructureNotifyMask | FocusChangeMask | ExposureMask;
	if (!CreationParams.IgnoreInput)
		attributes.event_mask |= PointerMotionMask |
				ButtonPressMask | KeyPressMask |
				ButtonReleaseMask | KeyReleaseMask;

	if (!CreationParams.WindowId)
	{
		// create new Window
		// Remove window manager decoration in fullscreen
		attributes.override_redirect = CreationParams.Fullscreen;
		window = XCreateWindow(display,
				RootWindow(display, visual->screen),
				0, 0, Width, Height, 0, visual->depth,
				InputOutput, visual->visual,
				CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
				&attributes);
		XMapRaised(display, window);
		CreationParams.WindowId = (void*)window;
		Atom wmDelete;
		wmDelete = XInternAtom(display, wmDeleteWindow, True);
		XSetWMProtocols(display, window, &wmDelete, 1);
		if (CreationParams.Fullscreen)
		{
			XSetInputFocus(display, window, RevertToParent, CurrentTime);
			int grabKb = XGrabKeyboard(display, window, True, GrabModeAsync,
				GrabModeAsync, CurrentTime);
			IrrPrintXGrabError(grabKb, "XGrabKeyboard");
			int grabPointer = XGrabPointer(display, window, True, ButtonPressMask,
				GrabModeAsync, GrabModeAsync, window, None, CurrentTime);
			IrrPrintXGrabError(grabPointer, "XGrabPointer");
			XWarpPointer(display, None, window, 0, 0, 0, 0, 0, 0);
		}
	}
	else
	{
		// attach external window
		window = (Window)CreationParams.WindowId;
		if (!CreationParams.IgnoreInput)
		{
			XCreateWindow(display,
					window,
					0, 0, Width, Height, 0, visual->depth,
					InputOutput, visual->visual,
					CWBorderPixel | CWColormap | CWEventMask,
					&attributes);
		}
		XWindowAttributes wa;
		XGetWindowAttributes(display, window, &wa);
		CreationParams.WindowSize.Width = wa.width;
		CreationParams.WindowSize.Height = wa.height;
		CreationParams.Fullscreen = false;
		ExternalWindow = true;
	}

	WindowMinimized=false;
	// Currently broken in X, see Bug ID 2795321
	// XkbSetDetectableAutoRepeat(display, True, &AutorepeatSupport);

#ifdef _IRR_COMPILE_WITH_OPENGL_

	// connect glx context to window
	Context=0;
	if (isAvailableGLX && CreationParams.DriverType==video::EDT_OPENGL)
	{
	if (UseGLXWindow)
	{
		glxWin=glXCreateWindow(display,glxFBConfig,window,NULL);
		if (glxWin)
		{
			// create glx context
			Context = glXCreateNewContext(display, glxFBConfig, GLX_RGBA_TYPE, NULL, True);
			if (Context)
			{
				if (!glXMakeContextCurrent(display, glxWin, glxWin, Context))
				{
					os::Printer::log("Could not make context current.", ELL_WARNING);
					glXDestroyContext(display, Context);
				}
			}
			else
			{
				os::Printer::log("Could not create GLX rendering context.", ELL_WARNING);
			}
		}
		else
		{
			os::Printer::log("Could not create GLX window.", ELL_WARNING);
		}
	}
	else
	{
		Context = glXCreateContext(display, visual, NULL, True);
		if (Context)
		{
			if (!glXMakeCurrent(display, window, Context))
			{
				os::Printer::log("Could not make context current.", ELL_WARNING);
				glXDestroyContext(display, Context);
			}
		}
		else
		{
			os::Printer::log("Could not create GLX rendering context.", ELL_WARNING);
		}
	}
	}
#endif // _IRR_COMPILE_WITH_OPENGL_

	Window tmp;
	u32 borderWidth;
	int x,y;
	unsigned int bits;

	XGetGeometry(display, window, &tmp, &x, &y, &Width, &Height, &borderWidth, &bits);
	CreationParams.Bits = bits;
	CreationParams.WindowSize.Width = Width;
	CreationParams.WindowSize.Height = Height;

	StdHints = XAllocSizeHints();
	long num;
	XGetWMNormalHints(display, window, StdHints, &num);

	// create an XImage for the software renderer
	//(thx to Nadav for some clues on how to do that!)

	if (CreationParams.DriverType == video::EDT_SOFTWARE || CreationParams.DriverType == video::EDT_BURNINGSVIDEO)
	{
		SoftwareImage = XCreateImage(display,
			visual->visual, visual->depth,
			ZPixmap, 0, 0, Width, Height,
			BitmapPad(display), 0);

		// use malloc because X will free it later on
		if (SoftwareImage)
			SoftwareImage->data = (char*) malloc(SoftwareImage->bytes_per_line * SoftwareImage->height * sizeof(char));
	}

	initXAtoms();

#endif // #ifdef _IRR_COMPILE_WITH_X11_
	return true;
}


//! create the driver
void CIrrDeviceLinux::createDriver()
{
	switch(CreationParams.DriverType)
	{
#ifdef _IRR_COMPILE_WITH_X11_

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
		if (Context)
			VideoDriver = video::createOpenGLDriver(CreationParams, FileSystem, this);
		#else
		os::Printer::log("No OpenGL support compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_DIRECT3D8:
	case video::EDT_DIRECT3D9:
		os::Printer::log("This driver is not available in Linux. Try OpenGL or Software renderer.",
			ELL_ERROR);
		break;

	case video::EDT_NULL:
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;

	default:
		os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
		break;
#else
	case video::EDT_NULL:
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;
	default:
		os::Printer::log("No X11 support compiled in. Only Null driver available.", ELL_ERROR);
		break;
#endif
	}
}


//! runs the device. Returns false if device wants to be deleted
bool CIrrDeviceLinux::run()
{
	os::Timer::tick();

#ifdef _IRR_COMPILE_WITH_X11_

	if ( CursorControl )
		static_cast<CCursorControl*>(CursorControl)->update();

	if ((CreationParams.DriverType != video::EDT_NULL) && display)
	{
		SEvent irrevent;
		irrevent.MouseInput.ButtonStates = 0xffffffff;

		while (XPending(display) > 0 && !Close)
		{
			XEvent event;
			XNextEvent(display, &event);

			switch (event.type)
			{
			case ConfigureNotify:
				// check for changed window size
				if ((event.xconfigure.width != (int) Width) ||
					(event.xconfigure.height != (int) Height))
				{
					Width = event.xconfigure.width;
					Height = event.xconfigure.height;

					// resize image data
					if (SoftwareImage)
					{
						XDestroyImage(SoftwareImage);

						SoftwareImage = XCreateImage(display,
							visual->visual, visual->depth,
							ZPixmap, 0, 0, Width, Height,
							BitmapPad(display), 0);

						// use malloc because X will free it later on
						if (SoftwareImage)
							SoftwareImage->data = (char*) malloc(SoftwareImage->bytes_per_line * SoftwareImage->height * sizeof(char));
					}

					if (VideoDriver)
						VideoDriver->OnResize(core::dimension2d<u32>(Width, Height));
				}
				break;

			case MapNotify:
				WindowMinimized=false;
				break;

			case UnmapNotify:
				WindowMinimized=true;
				break;

			case FocusIn:
				WindowHasFocus=true;
				break;

			case FocusOut:
				WindowHasFocus=false;
				break;

			case MotionNotify:
				irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
				irrevent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
				irrevent.MouseInput.X = event.xbutton.x;
				irrevent.MouseInput.Y = event.xbutton.y;
				irrevent.MouseInput.Control = (event.xkey.state & ControlMask) != 0;
				irrevent.MouseInput.Shift = (event.xkey.state & ShiftMask) != 0;

				// mouse button states
				irrevent.MouseInput.ButtonStates = (event.xbutton.state & Button1Mask) ? irr::EMBSM_LEFT : 0;
				irrevent.MouseInput.ButtonStates |= (event.xbutton.state & Button3Mask) ? irr::EMBSM_RIGHT : 0;
				irrevent.MouseInput.ButtonStates |= (event.xbutton.state & Button2Mask) ? irr::EMBSM_MIDDLE : 0;

				postEventFromUser(irrevent);
				break;

			case ButtonPress:
			case ButtonRelease:

				irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
				irrevent.MouseInput.X = event.xbutton.x;
				irrevent.MouseInput.Y = event.xbutton.y;
				irrevent.MouseInput.Control = (event.xkey.state & ControlMask) != 0;
				irrevent.MouseInput.Shift = (event.xkey.state & ShiftMask) != 0;

				// mouse button states
				// This sets the state which the buttons had _prior_ to the event.
				// So unlike on Windows the button which just got changed has still the old state here.
				// We handle that below by flipping the corresponding bit later.
				irrevent.MouseInput.ButtonStates = (event.xbutton.state & Button1Mask) ? irr::EMBSM_LEFT : 0;
				irrevent.MouseInput.ButtonStates |= (event.xbutton.state & Button3Mask) ? irr::EMBSM_RIGHT : 0;
				irrevent.MouseInput.ButtonStates |= (event.xbutton.state & Button2Mask) ? irr::EMBSM_MIDDLE : 0;

				irrevent.MouseInput.Event = irr::EMIE_COUNT;

				switch(event.xbutton.button)
				{
				case  Button1:
					irrevent.MouseInput.Event =
						(event.type == ButtonPress) ? irr::EMIE_LMOUSE_PRESSED_DOWN : irr::EMIE_LMOUSE_LEFT_UP;
					irrevent.MouseInput.ButtonStates ^= irr::EMBSM_LEFT;
					break;

				case  Button3:
					irrevent.MouseInput.Event =
						(event.type == ButtonPress) ? irr::EMIE_RMOUSE_PRESSED_DOWN : irr::EMIE_RMOUSE_LEFT_UP;
					irrevent.MouseInput.ButtonStates ^= irr::EMBSM_RIGHT;
					break;

				case  Button2:
					irrevent.MouseInput.Event =
						(event.type == ButtonPress) ? irr::EMIE_MMOUSE_PRESSED_DOWN : irr::EMIE_MMOUSE_LEFT_UP;
					irrevent.MouseInput.ButtonStates ^= irr::EMBSM_MIDDLE;
					break;

				case  Button4:
					if (event.type == ButtonPress)
					{
						irrevent.MouseInput.Event = EMIE_MOUSE_WHEEL;
						irrevent.MouseInput.Wheel = 1.0f;
					}
					break;

				case  Button5:
					if (event.type == ButtonPress)
					{
						irrevent.MouseInput.Event = EMIE_MOUSE_WHEEL;
						irrevent.MouseInput.Wheel = -1.0f;
					}
					break;
				}

				if (irrevent.MouseInput.Event != irr::EMIE_COUNT)
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

			case MappingNotify:
				XRefreshKeyboardMapping (&event.xmapping) ;
				break;

			case KeyRelease:
				if (0 == AutorepeatSupport && (XPending( display ) > 0) )
				{
					// check for Autorepeat manually
					// We'll do the same as Windows does: Only send KeyPressed
					// So every KeyRelease is a real release
					XEvent next_event;
					XPeekEvent (event.xkey.display, &next_event);
					if ((next_event.type == KeyPress) &&
						(next_event.xkey.keycode == event.xkey.keycode) &&
						(next_event.xkey.time - event.xkey.time) < 2)	// usually same time, but on some systems a difference of 1 is possible
					{
						/* Ignore the key release event */
						break;
					}
				}
				// fall-through in case the release should be handled
			case KeyPress:
				{
					SKeyMap mp;
					char buf[8]={0};
					XLookupString(&event.xkey, buf, sizeof(buf), &mp.X11Key, NULL);

					irrevent.EventType = irr::EET_KEY_INPUT_EVENT;
					irrevent.KeyInput.PressedDown = (event.type == KeyPress);
//					mbtowc(&irrevent.KeyInput.Char, buf, sizeof(buf));
					irrevent.KeyInput.Char = ((wchar_t*)(buf))[0];
					irrevent.KeyInput.Control = (event.xkey.state & ControlMask) != 0;
					irrevent.KeyInput.Shift = (event.xkey.state & ShiftMask) != 0;

					event.xkey.state = 0; // ignore shift-ctrl states for figuring out the key
					XLookupString(&event.xkey, buf, sizeof(buf), &mp.X11Key, NULL);
					const s32 idx = KeyMap.binary_search(mp);
					if (idx != -1)
					{
						irrevent.KeyInput.Key = (EKEY_CODE)KeyMap[idx].Win32Key;
					}
					else
					{
						irrevent.KeyInput.Key = (EKEY_CODE)0;
					}
					if (irrevent.KeyInput.Key == 0)
					{
						// 1:1 mapping to windows-keys would require testing for keyboard type (us, ger, ...)
						// So unless we do that we will have some unknown keys here.
						if (idx == -1)
						{
							os::Printer::log("Could not find EKEY_CODE, using orig. X11 keycode instead", core::stringc(event.xkey.keycode).c_str(), ELL_INFORMATION);
						}
						else
						{
							os::Printer::log("EKEY_CODE is 0, using orig. X11 keycode instead", core::stringc(event.xkey.keycode).c_str(), ELL_INFORMATION);
						}
						// Any value is better than none, that allows at least using the keys.
						// Worst case is that some keys will be identical, still better than _all_
						// unknown keys being identical.
						irrevent.KeyInput.Key = (EKEY_CODE)event.xkey.keycode;
					}

					postEventFromUser(irrevent);
				}
				break;

			case ClientMessage:
				{
					char *atom = XGetAtomName(display, event.xclient.message_type);
					if (*atom == *wmDeleteWindow)
					{
						os::Printer::log("Quit message received.", ELL_INFORMATION);
						Close = true;
					}
					else
					{
						// we assume it's a user message
						irrevent.EventType = irr::EET_USER_EVENT;
						irrevent.UserEvent.UserData1 = (s32)event.xclient.data.l[0];
						irrevent.UserEvent.UserData2 = (s32)event.xclient.data.l[1];
						postEventFromUser(irrevent);
					}
					XFree(atom);
				}
				break;

			case SelectionRequest:
				{
					XEvent respond;
					XSelectionRequestEvent *req = &(event.xselectionrequest);
					if (  req->target == XA_STRING)
					{
						XChangeProperty (display,
								req->requestor,
								req->property, req->target,
								8, // format
								PropModeReplace,
								(unsigned char*) Clipboard.c_str(),
								Clipboard.size());
						respond.xselection.property = req->property;
					}
					else if ( req->target == X_ATOM_TARGETS )
					{
						long data[2];

						data[0] = X_ATOM_TEXT;
						data[1] = XA_STRING;

						XChangeProperty (display, req->requestor,
								req->property, req->target,
								8, PropModeReplace,
								(unsigned char *) &data,
								sizeof (data));
						respond.xselection.property = req->property;
					}
					else
					{
						respond.xselection.property= None;
					}
					respond.xselection.type= SelectionNotify;
					respond.xselection.display= req->display;
					respond.xselection.requestor= req->requestor;
					respond.xselection.selection=req->selection;
					respond.xselection.target= req->target;
					respond.xselection.time = req->time;
					XSendEvent (display, req->requestor,0,0,&respond);
					XFlush (display);
				}
				break;

			default:
				break;
			} // end switch

		} // end while
	}
#endif //_IRR_COMPILE_WITH_X11_

	if (!Close)
		pollJoysticks();

	return !Close;
}


//! Pause the current process for the minimum time allowed only to allow other processes to execute
void CIrrDeviceLinux::yield()
{
	struct timespec ts = {0,1};
	nanosleep(&ts, NULL);
}


//! Pause execution and let other processes to run for a specified amount of time.
void CIrrDeviceLinux::sleep(u32 timeMs, bool pauseTimer=false)
{
	const bool wasStopped = Timer ? Timer->isStopped() : true;

	struct timespec ts;
	ts.tv_sec = (time_t) (timeMs / 1000);
	ts.tv_nsec = (long) (timeMs % 1000) * 1000000;

	if (pauseTimer && !wasStopped)
		Timer->stop();

	nanosleep(&ts, NULL);

	if (pauseTimer && !wasStopped)
		Timer->start();
}


//! sets the caption of the window
void CIrrDeviceLinux::setWindowCaption(const wchar_t* text)
{
#ifdef _IRR_COMPILE_WITH_X11_
	if (CreationParams.DriverType == video::EDT_NULL)
		return;

	XTextProperty txt;
	if (Success==XwcTextListToTextProperty(display, const_cast<wchar_t**>(&text),
				1, XStdICCTextStyle, &txt))
	{
		XSetWMName(display, window, &txt);
		XSetWMIconName(display, window, &txt);
		XFree(txt.value);
	}
#endif
}


//! presents a surface in the client area
bool CIrrDeviceLinux::present(video::IImage* image, void* windowId, core::rect<s32>* srcRect)
{
#ifdef _IRR_COMPILE_WITH_X11_
	// this is only necessary for software drivers.
	if (!SoftwareImage)
		return true;

	// thx to Nadav, who send me some clues of how to display the image
	// to the X Server.

	const u32 destwidth = SoftwareImage->width;
	const u32 minWidth = core::min_(image->getDimension().Width, destwidth);
	const u32 destPitch = SoftwareImage->bytes_per_line;

	video::ECOLOR_FORMAT destColor;
	switch (SoftwareImage->bits_per_pixel)
	{
		case 16:
			if (SoftwareImage->depth==16)
				destColor = video::ECF_R5G6B5;
			else
				destColor = video::ECF_A1R5G5B5;
		break;
		case 24: destColor = video::ECF_R8G8B8; break;
		case 32: destColor = video::ECF_A8R8G8B8; break;
		default:
			os::Printer::log("Unsupported screen depth.");
			return false;
	}

	u8* srcdata = reinterpret_cast<u8*>(image->lock());
	u8* destData = reinterpret_cast<u8*>(SoftwareImage->data);

	const u32 destheight = SoftwareImage->height;
	const u32 srcheight = core::min_(image->getDimension().Height, destheight);
	const u32 srcPitch = image->getPitch();
	for (u32 y=0; y!=srcheight; ++y)
	{
		video::CColorConverter::convert_viaFormat(srcdata,image->getColorFormat(), minWidth, destData, destColor);
		srcdata+=srcPitch;
		destData+=destPitch;
	}
	image->unlock();

	GC gc = DefaultGC(display, DefaultScreen(display));
	Window myWindow=window;
	if (windowId)
		myWindow = reinterpret_cast<Window>(windowId);
	XPutImage(display, myWindow, gc, SoftwareImage, 0, 0, 0, 0, destwidth, destheight);
#endif
	return true;
}


//! notifies the device that it should close itself
void CIrrDeviceLinux::closeDevice()
{
	Close = true;
}


//! returns if window is active. if not, nothing need to be drawn
bool CIrrDeviceLinux::isWindowActive() const
{
	return (WindowHasFocus && !WindowMinimized);
}


//! returns if window has focus.
bool CIrrDeviceLinux::isWindowFocused() const
{
	return WindowHasFocus;
}


//! returns if window is minimized.
bool CIrrDeviceLinux::isWindowMinimized() const
{
	return WindowMinimized;
}


//! returns color format of the window.
video::ECOLOR_FORMAT CIrrDeviceLinux::getColorFormat() const
{
#ifdef _IRR_COMPILE_WITH_X11_
	if (visual && (visual->depth != 16))
		return video::ECF_R8G8B8;
	else
#endif
		return video::ECF_R5G6B5;
}


//! Sets if the window should be resizable in windowed mode.
void CIrrDeviceLinux::setResizable(bool resize)
{
#ifdef _IRR_COMPILE_WITH_X11_
	if (CreationParams.DriverType == video::EDT_NULL || CreationParams.Fullscreen )
		return;

	XUnmapWindow(display, window);
	if ( !resize )
	{
		// Must be heap memory because data size depends on X Server
		XSizeHints *hints = XAllocSizeHints();
		hints->flags=PSize|PMinSize|PMaxSize;
		hints->min_width=hints->max_width=hints->base_width=Width;
		hints->min_height=hints->max_height=hints->base_height=Height;
		XSetWMNormalHints(display, window, hints);
		XFree(hints);
	}
	else
	{
		XSetWMNormalHints(display, window, StdHints);
	}
	XMapWindow(display, window);
	XFlush(display);
#endif // #ifdef _IRR_COMPILE_WITH_X11_
}


//! Return pointer to a list with all video modes supported by the gfx adapter.
video::IVideoModeList* CIrrDeviceLinux::getVideoModeList()
{
#ifdef _IRR_COMPILE_WITH_X11_
	if (!VideoModeList->getVideoModeCount())
	{
		bool temporaryDisplay = false;

		if (!display)
		{
			display = XOpenDisplay(0);
			temporaryDisplay=true;
		}
		if (display)
		{
			#if defined(_IRR_LINUX_X11_VIDMODE_) || defined(_IRR_LINUX_X11_RANDR_)
			s32 eventbase, errorbase;
			s32 defaultDepth=DefaultDepth(display,screennr);
			#endif

			#ifdef _IRR_LINUX_X11_VIDMODE_
			if (XF86VidModeQueryExtension(display, &eventbase, &errorbase))
			{
				// enumerate video modes
				int modeCount;
				XF86VidModeModeInfo** modes;

				XF86VidModeGetAllModeLines(display, screennr, &modeCount, &modes);

				// save current video mode
				oldVideoMode = *modes[0];

				// find fitting mode

				VideoModeList->setDesktop(defaultDepth, core::dimension2d<u32>(
					modes[0]->hdisplay, modes[0]->vdisplay));
				for (int i = 0; i<modeCount; ++i)
				{
					VideoModeList->addMode(core::dimension2d<u32>(
						modes[i]->hdisplay, modes[i]->vdisplay), defaultDepth);
				}
				XFree(modes);
			}
			else
			#endif
			#ifdef _IRR_LINUX_X11_RANDR_
			if (XRRQueryExtension(display, &eventbase, &errorbase))
			{
				int modeCount;
				XRRScreenConfiguration *config=XRRGetScreenInfo(display,DefaultRootWindow(display));
				oldRandrMode=XRRConfigCurrentConfiguration(config,&oldRandrRotation);
				XRRScreenSize *modes=XRRConfigSizes(config,&modeCount);
				VideoModeList->setDesktop(defaultDepth, core::dimension2d<u32>(
					modes[oldRandrMode].width, modes[oldRandrMode].height));
				for (int i = 0; i<modeCount; ++i)
				{
					VideoModeList->addMode(core::dimension2d<u32>(
						modes[i].width, modes[i].height), defaultDepth);
				}
				XRRFreeScreenConfigInfo(config);
			}
			else
			#endif
			{
				os::Printer::log("VidMode or RandR X11 extension requireed for VideoModeList." , ELL_WARNING);
			}
		}
		if (display && temporaryDisplay)
		{
			XCloseDisplay(display);
			display=0;
		}
	}
#endif

	return VideoModeList;
}


//! Minimize window
void CIrrDeviceLinux::minimizeWindow()
{
#ifdef _IRR_COMPILE_WITH_X11_
	XIconifyWindow(display, window, screennr);
#endif
}


//! Maximize window
void CIrrDeviceLinux::maximizeWindow()
{
#ifdef _IRR_COMPILE_WITH_X11_
	XMapWindow(display, window);
#endif
}


//! Restore original window size
void CIrrDeviceLinux::restoreWindow()
{
#ifdef _IRR_COMPILE_WITH_X11_
	XMapWindow(display, window);
#endif
}


void CIrrDeviceLinux::createKeyMap()
{
	// I don't know if this is the best method  to create
	// the lookuptable, but I'll leave it like that until
	// I find a better version.

#ifdef _IRR_COMPILE_WITH_X11_
	KeyMap.reallocate(84);
	KeyMap.push_back(SKeyMap(XK_BackSpace, KEY_BACK));
	KeyMap.push_back(SKeyMap(XK_Tab, KEY_TAB));
	KeyMap.push_back(SKeyMap(XK_ISO_Left_Tab, KEY_TAB));
	KeyMap.push_back(SKeyMap(XK_Linefeed, 0)); // ???
	KeyMap.push_back(SKeyMap(XK_Clear, KEY_CLEAR));
	KeyMap.push_back(SKeyMap(XK_Return, KEY_RETURN));
	KeyMap.push_back(SKeyMap(XK_Pause, KEY_PAUSE));
	KeyMap.push_back(SKeyMap(XK_Scroll_Lock, KEY_SCROLL));
	KeyMap.push_back(SKeyMap(XK_Sys_Req, 0)); // ???
	KeyMap.push_back(SKeyMap(XK_Escape, KEY_ESCAPE));
	KeyMap.push_back(SKeyMap(XK_Insert, KEY_INSERT));
	KeyMap.push_back(SKeyMap(XK_Delete, KEY_DELETE));
	KeyMap.push_back(SKeyMap(XK_Home, KEY_HOME));
	KeyMap.push_back(SKeyMap(XK_Left, KEY_LEFT));
	KeyMap.push_back(SKeyMap(XK_Up, KEY_UP));
	KeyMap.push_back(SKeyMap(XK_Right, KEY_RIGHT));
	KeyMap.push_back(SKeyMap(XK_Down, KEY_DOWN));
	KeyMap.push_back(SKeyMap(XK_Prior, KEY_PRIOR));
	KeyMap.push_back(SKeyMap(XK_Page_Up, KEY_PRIOR));
	KeyMap.push_back(SKeyMap(XK_Next, KEY_NEXT));
	KeyMap.push_back(SKeyMap(XK_Page_Down, KEY_NEXT));
	KeyMap.push_back(SKeyMap(XK_End, KEY_END));
	KeyMap.push_back(SKeyMap(XK_Begin, KEY_HOME));
	KeyMap.push_back(SKeyMap(XK_Num_Lock, KEY_NUMLOCK));
	KeyMap.push_back(SKeyMap(XK_KP_Space, KEY_SPACE));
	KeyMap.push_back(SKeyMap(XK_KP_Tab, KEY_TAB));
	KeyMap.push_back(SKeyMap(XK_KP_Enter, KEY_RETURN));
	KeyMap.push_back(SKeyMap(XK_KP_F1, KEY_F1));
	KeyMap.push_back(SKeyMap(XK_KP_F2, KEY_F2));
	KeyMap.push_back(SKeyMap(XK_KP_F3, KEY_F3));
	KeyMap.push_back(SKeyMap(XK_KP_F4, KEY_F4));
	KeyMap.push_back(SKeyMap(XK_KP_Home, KEY_HOME));
	KeyMap.push_back(SKeyMap(XK_KP_Left, KEY_LEFT));
	KeyMap.push_back(SKeyMap(XK_KP_Up, KEY_UP));
	KeyMap.push_back(SKeyMap(XK_KP_Right, KEY_RIGHT));
	KeyMap.push_back(SKeyMap(XK_KP_Down, KEY_DOWN));
	KeyMap.push_back(SKeyMap(XK_Print, KEY_PRINT));
	KeyMap.push_back(SKeyMap(XK_KP_Prior, KEY_PRIOR));
	KeyMap.push_back(SKeyMap(XK_KP_Page_Up, KEY_PRIOR));
	KeyMap.push_back(SKeyMap(XK_KP_Next, KEY_NEXT));
	KeyMap.push_back(SKeyMap(XK_KP_Page_Down, KEY_NEXT));
	KeyMap.push_back(SKeyMap(XK_KP_End, KEY_END));
	KeyMap.push_back(SKeyMap(XK_KP_Begin, KEY_HOME));
	KeyMap.push_back(SKeyMap(XK_KP_Insert, KEY_INSERT));
	KeyMap.push_back(SKeyMap(XK_KP_Delete, KEY_DELETE));
	KeyMap.push_back(SKeyMap(XK_KP_Equal, 0)); // ???
	KeyMap.push_back(SKeyMap(XK_KP_Multiply, KEY_MULTIPLY));
	KeyMap.push_back(SKeyMap(XK_KP_Add, KEY_ADD));
	KeyMap.push_back(SKeyMap(XK_KP_Separator, KEY_SEPARATOR));
	KeyMap.push_back(SKeyMap(XK_KP_Subtract, KEY_SUBTRACT));
	KeyMap.push_back(SKeyMap(XK_KP_Decimal, KEY_DECIMAL));
	KeyMap.push_back(SKeyMap(XK_KP_Divide, KEY_DIVIDE));
	KeyMap.push_back(SKeyMap(XK_KP_0, KEY_KEY_0));
	KeyMap.push_back(SKeyMap(XK_KP_1, KEY_KEY_1));
	KeyMap.push_back(SKeyMap(XK_KP_2, KEY_KEY_2));
	KeyMap.push_back(SKeyMap(XK_KP_3, KEY_KEY_3));
	KeyMap.push_back(SKeyMap(XK_KP_4, KEY_KEY_4));
	KeyMap.push_back(SKeyMap(XK_KP_5, KEY_KEY_5));
	KeyMap.push_back(SKeyMap(XK_KP_6, KEY_KEY_6));
	KeyMap.push_back(SKeyMap(XK_KP_7, KEY_KEY_7));
	KeyMap.push_back(SKeyMap(XK_KP_8, KEY_KEY_8));
	KeyMap.push_back(SKeyMap(XK_KP_9, KEY_KEY_9));
	KeyMap.push_back(SKeyMap(XK_F1, KEY_F1));
	KeyMap.push_back(SKeyMap(XK_F2, KEY_F2));
	KeyMap.push_back(SKeyMap(XK_F3, KEY_F3));
	KeyMap.push_back(SKeyMap(XK_F4, KEY_F4));
	KeyMap.push_back(SKeyMap(XK_F5, KEY_F5));
	KeyMap.push_back(SKeyMap(XK_F6, KEY_F6));
	KeyMap.push_back(SKeyMap(XK_F7, KEY_F7));
	KeyMap.push_back(SKeyMap(XK_F8, KEY_F8));
	KeyMap.push_back(SKeyMap(XK_F9, KEY_F9));
	KeyMap.push_back(SKeyMap(XK_F10, KEY_F10));
	KeyMap.push_back(SKeyMap(XK_F11, KEY_F11));
	KeyMap.push_back(SKeyMap(XK_F12, KEY_F12));
	KeyMap.push_back(SKeyMap(XK_Shift_L, KEY_LSHIFT));
	KeyMap.push_back(SKeyMap(XK_Shift_R, KEY_RSHIFT));
	KeyMap.push_back(SKeyMap(XK_Control_L, KEY_LCONTROL));
	KeyMap.push_back(SKeyMap(XK_Control_R, KEY_RCONTROL));
	KeyMap.push_back(SKeyMap(XK_Caps_Lock, KEY_CAPITAL));
	KeyMap.push_back(SKeyMap(XK_Shift_Lock, KEY_CAPITAL));
	KeyMap.push_back(SKeyMap(XK_Meta_L, KEY_LWIN));
	KeyMap.push_back(SKeyMap(XK_Meta_R, KEY_RWIN));
	KeyMap.push_back(SKeyMap(XK_Alt_L, KEY_LMENU));
	KeyMap.push_back(SKeyMap(XK_Alt_R, KEY_RMENU));
	KeyMap.push_back(SKeyMap(XK_ISO_Level3_Shift, KEY_RMENU));
	KeyMap.push_back(SKeyMap(XK_Menu, KEY_MENU));
	KeyMap.push_back(SKeyMap(XK_space, KEY_SPACE));
	KeyMap.push_back(SKeyMap(XK_exclam, 0)); //?
	KeyMap.push_back(SKeyMap(XK_quotedbl, 0)); //?
	KeyMap.push_back(SKeyMap(XK_section, 0)); //?
	KeyMap.push_back(SKeyMap(XK_numbersign, KEY_OEM_2));
	KeyMap.push_back(SKeyMap(XK_dollar, 0)); //?
	KeyMap.push_back(SKeyMap(XK_percent, 0)); //?
	KeyMap.push_back(SKeyMap(XK_ampersand, 0)); //?
	KeyMap.push_back(SKeyMap(XK_apostrophe, KEY_OEM_7));
	KeyMap.push_back(SKeyMap(XK_parenleft, 0)); //?
	KeyMap.push_back(SKeyMap(XK_parenright, 0)); //?
	KeyMap.push_back(SKeyMap(XK_asterisk, 0)); //?
	KeyMap.push_back(SKeyMap(XK_plus, KEY_PLUS)); //?
	KeyMap.push_back(SKeyMap(XK_comma, KEY_COMMA)); //?
	KeyMap.push_back(SKeyMap(XK_minus, KEY_MINUS)); //?
	KeyMap.push_back(SKeyMap(XK_period, KEY_PERIOD)); //?
	KeyMap.push_back(SKeyMap(XK_slash, KEY_OEM_2)); //?
	KeyMap.push_back(SKeyMap(XK_0, KEY_KEY_0));
	KeyMap.push_back(SKeyMap(XK_1, KEY_KEY_1));
	KeyMap.push_back(SKeyMap(XK_2, KEY_KEY_2));
	KeyMap.push_back(SKeyMap(XK_3, KEY_KEY_3));
	KeyMap.push_back(SKeyMap(XK_4, KEY_KEY_4));
	KeyMap.push_back(SKeyMap(XK_5, KEY_KEY_5));
	KeyMap.push_back(SKeyMap(XK_6, KEY_KEY_6));
	KeyMap.push_back(SKeyMap(XK_7, KEY_KEY_7));
	KeyMap.push_back(SKeyMap(XK_8, KEY_KEY_8));
	KeyMap.push_back(SKeyMap(XK_9, KEY_KEY_9));
	KeyMap.push_back(SKeyMap(XK_colon, 0)); //?
	KeyMap.push_back(SKeyMap(XK_semicolon, KEY_OEM_1));
	KeyMap.push_back(SKeyMap(XK_less, KEY_OEM_102));
	KeyMap.push_back(SKeyMap(XK_equal, KEY_PLUS));
	KeyMap.push_back(SKeyMap(XK_greater, 0)); //?
	KeyMap.push_back(SKeyMap(XK_question, 0)); //?
	KeyMap.push_back(SKeyMap(XK_at, KEY_KEY_2)); //?
	KeyMap.push_back(SKeyMap(XK_mu, 0)); //?
	KeyMap.push_back(SKeyMap(XK_EuroSign, 0)); //?
	KeyMap.push_back(SKeyMap(XK_A, KEY_KEY_A));
	KeyMap.push_back(SKeyMap(XK_B, KEY_KEY_B));
	KeyMap.push_back(SKeyMap(XK_C, KEY_KEY_C));
	KeyMap.push_back(SKeyMap(XK_D, KEY_KEY_D));
	KeyMap.push_back(SKeyMap(XK_E, KEY_KEY_E));
	KeyMap.push_back(SKeyMap(XK_F, KEY_KEY_F));
	KeyMap.push_back(SKeyMap(XK_G, KEY_KEY_G));
	KeyMap.push_back(SKeyMap(XK_H, KEY_KEY_H));
	KeyMap.push_back(SKeyMap(XK_I, KEY_KEY_I));
	KeyMap.push_back(SKeyMap(XK_J, KEY_KEY_J));
	KeyMap.push_back(SKeyMap(XK_K, KEY_KEY_K));
	KeyMap.push_back(SKeyMap(XK_L, KEY_KEY_L));
	KeyMap.push_back(SKeyMap(XK_M, KEY_KEY_M));
	KeyMap.push_back(SKeyMap(XK_N, KEY_KEY_N));
	KeyMap.push_back(SKeyMap(XK_O, KEY_KEY_O));
	KeyMap.push_back(SKeyMap(XK_P, KEY_KEY_P));
	KeyMap.push_back(SKeyMap(XK_Q, KEY_KEY_Q));
	KeyMap.push_back(SKeyMap(XK_R, KEY_KEY_R));
	KeyMap.push_back(SKeyMap(XK_S, KEY_KEY_S));
	KeyMap.push_back(SKeyMap(XK_T, KEY_KEY_T));
	KeyMap.push_back(SKeyMap(XK_U, KEY_KEY_U));
	KeyMap.push_back(SKeyMap(XK_V, KEY_KEY_V));
	KeyMap.push_back(SKeyMap(XK_W, KEY_KEY_W));
	KeyMap.push_back(SKeyMap(XK_X, KEY_KEY_X));
	KeyMap.push_back(SKeyMap(XK_Y, KEY_KEY_Y));
	KeyMap.push_back(SKeyMap(XK_Z, KEY_KEY_Z));
	KeyMap.push_back(SKeyMap(XK_bracketleft, KEY_OEM_4));
	KeyMap.push_back(SKeyMap(XK_backslash, KEY_OEM_5));
	KeyMap.push_back(SKeyMap(XK_bracketright, KEY_OEM_6));
	KeyMap.push_back(SKeyMap(XK_asciicircum, KEY_OEM_5));
	KeyMap.push_back(SKeyMap(XK_degree, 0)); //?
	KeyMap.push_back(SKeyMap(XK_underscore, KEY_MINUS)); //?
	KeyMap.push_back(SKeyMap(XK_grave, KEY_OEM_3));
	KeyMap.push_back(SKeyMap(XK_acute, KEY_OEM_6));
	KeyMap.push_back(SKeyMap(XK_a, KEY_KEY_A));
	KeyMap.push_back(SKeyMap(XK_b, KEY_KEY_B));
	KeyMap.push_back(SKeyMap(XK_c, KEY_KEY_C));
	KeyMap.push_back(SKeyMap(XK_d, KEY_KEY_D));
	KeyMap.push_back(SKeyMap(XK_e, KEY_KEY_E));
	KeyMap.push_back(SKeyMap(XK_f, KEY_KEY_F));
	KeyMap.push_back(SKeyMap(XK_g, KEY_KEY_G));
	KeyMap.push_back(SKeyMap(XK_h, KEY_KEY_H));
	KeyMap.push_back(SKeyMap(XK_i, KEY_KEY_I));
	KeyMap.push_back(SKeyMap(XK_j, KEY_KEY_J));
	KeyMap.push_back(SKeyMap(XK_k, KEY_KEY_K));
	KeyMap.push_back(SKeyMap(XK_l, KEY_KEY_L));
	KeyMap.push_back(SKeyMap(XK_m, KEY_KEY_M));
	KeyMap.push_back(SKeyMap(XK_n, KEY_KEY_N));
	KeyMap.push_back(SKeyMap(XK_o, KEY_KEY_O));
	KeyMap.push_back(SKeyMap(XK_p, KEY_KEY_P));
	KeyMap.push_back(SKeyMap(XK_q, KEY_KEY_Q));
	KeyMap.push_back(SKeyMap(XK_r, KEY_KEY_R));
	KeyMap.push_back(SKeyMap(XK_s, KEY_KEY_S));
	KeyMap.push_back(SKeyMap(XK_t, KEY_KEY_T));
	KeyMap.push_back(SKeyMap(XK_u, KEY_KEY_U));
	KeyMap.push_back(SKeyMap(XK_v, KEY_KEY_V));
	KeyMap.push_back(SKeyMap(XK_w, KEY_KEY_W));
	KeyMap.push_back(SKeyMap(XK_x, KEY_KEY_X));
	KeyMap.push_back(SKeyMap(XK_y, KEY_KEY_Y));
	KeyMap.push_back(SKeyMap(XK_z, KEY_KEY_Z));
	KeyMap.push_back(SKeyMap(XK_ssharp, KEY_OEM_4));
	KeyMap.push_back(SKeyMap(XK_adiaeresis, KEY_OEM_7));
	KeyMap.push_back(SKeyMap(XK_odiaeresis, KEY_OEM_3));
	KeyMap.push_back(SKeyMap(XK_udiaeresis, KEY_OEM_1));
	KeyMap.push_back(SKeyMap(XK_Super_L, KEY_LWIN));
	KeyMap.push_back(SKeyMap(XK_Super_R, KEY_RWIN));

	KeyMap.sort();
#endif
}

bool CIrrDeviceLinux::activateJoysticks(core::array<SJoystickInfo> & joystickInfo)
{
#if defined (_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)

	joystickInfo.clear();

	u32 joystick;
	for (joystick = 0; joystick < 32; ++joystick)
	{
		// The joystick device could be here...
		core::stringc devName = "/dev/js";
		devName += joystick;

		SJoystickInfo returnInfo;
		JoystickInfo info;

		info.fd = open(devName.c_str(), O_RDONLY);
		if (-1 == info.fd)
		{
			// ...but Ubuntu and possibly other distros
			// create the devices in /dev/input
			devName = "/dev/input/js";
			devName += joystick;
			info.fd = open(devName.c_str(), O_RDONLY);
			if (-1 == info.fd)
			{
				// and BSD here
				devName = "/dev/joy";
				devName += joystick;
				info.fd = open(devName.c_str(), O_RDONLY);
			}
		}

		if (-1 == info.fd)
			continue;

#ifdef __FreeBSD__
		info.axes=2;
		info.buttons=2;
#else
		ioctl( info.fd, JSIOCGAXES, &(info.axes) );
		ioctl( info.fd, JSIOCGBUTTONS, &(info.buttons) );
		fcntl( info.fd, F_SETFL, O_NONBLOCK );
#endif

		(void)memset(&info.persistentData, 0, sizeof(info.persistentData));
		info.persistentData.EventType = irr::EET_JOYSTICK_INPUT_EVENT;
		info.persistentData.JoystickEvent.Joystick = ActiveJoysticks.size();

		// There's no obvious way to determine which (if any) axes represent a POV
		// hat, so we'll just set it to "not used" and forget about it.
		info.persistentData.JoystickEvent.POV = 65535;

		ActiveJoysticks.push_back(info);

		returnInfo.Joystick = joystick;
		returnInfo.PovHat = SJoystickInfo::POV_HAT_UNKNOWN;
		returnInfo.Axes = info.axes;
		returnInfo.Buttons = info.buttons;

#ifndef __FreeBSD__
		char name[80];
		ioctl( info.fd, JSIOCGNAME(80), name);
		returnInfo.Name = name;
#endif

		joystickInfo.push_back(returnInfo);
	}

	for (joystick = 0; joystick < joystickInfo.size(); ++joystick)
	{
		char logString[256];
		(void)sprintf(logString, "Found joystick %u, %u axes, %u buttons '%s'",
			joystick, joystickInfo[joystick].Axes,
			joystickInfo[joystick].Buttons, joystickInfo[joystick].Name.c_str());
		os::Printer::log(logString, ELL_INFORMATION);
	}

	return true;
#else
	return false;
#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
}


void CIrrDeviceLinux::pollJoysticks()
{
#if defined (_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
	if (0 == ActiveJoysticks.size())
		return;

	for (u32 j= 0; j< ActiveJoysticks.size(); ++j)
	{
		JoystickInfo & info =  ActiveJoysticks[j];

#ifdef __FreeBSD__
		struct joystick js;
		if (read(info.fd, &js, sizeof(js)) == sizeof(js))
		{
			info.persistentData.JoystickEvent.ButtonStates = js.b1 | (js.b2 << 1); /* should be a two-bit field */
			info.persistentData.JoystickEvent.Axis[0] = js.x; /* X axis */
			info.persistentData.JoystickEvent.Axis[1] = js.y; /* Y axis */
		}
#else
		struct js_event event;
		while (sizeof(event) == read(info.fd, &event, sizeof(event)))
		{
			switch(event.type & ~JS_EVENT_INIT)
			{
			case JS_EVENT_BUTTON:
				if (event.value)
						info.persistentData.JoystickEvent.ButtonStates |= (1 << event.number);
				else
						info.persistentData.JoystickEvent.ButtonStates &= ~(1 << event.number);
				break;

			case JS_EVENT_AXIS:
				if (event.number < SEvent::SJoystickEvent::NUMBER_OF_AXES)
					info.persistentData.JoystickEvent.Axis[event.number] = event.value;
				break;

			default:
				break;
			}
		}
#endif

		// Send an irrlicht joystick event once per ::run() even if no new data were received.
		(void)postEventFromUser(info.persistentData);
	}
#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
}


//! Set the current Gamma Value for the Display
bool CIrrDeviceLinux::setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast )
{
	#if defined(_IRR_LINUX_X11_VIDMODE_) || defined(_IRR_LINUX_X11_RANDR_)
	s32 eventbase, errorbase;
	#ifdef _IRR_LINUX_X11_VIDMODE_
	if (XF86VidModeQueryExtension(display, &eventbase, &errorbase))
	{
		XF86VidModeGamma gamma;
		gamma.red=red;
		gamma.green=green;
		gamma.blue=blue;
		XF86VidModeSetGamma(display, screennr, &gamma);
		return true;
	}
	#endif
	#if defined(_IRR_LINUX_X11_VIDMODE_) && defined(_IRR_LINUX_X11_RANDR_)
	else
	#endif
	#ifdef _IRR_LINUX_X11_RANDR_
	if (XRRQueryExtension(display, &eventbase, &errorbase))
	{
		XRRQueryVersion(display, &eventbase, &errorbase); // major, minor
		if (eventbase>=1 && errorbase>1)
		{
		#if (RANDR_MAJOR>1 || RANDR_MINOR>1)
			XRRCrtcGamma *gamma = XRRGetCrtcGamma(display, screennr);
			if (gamma)
			{
				*gamma->red=(u16)red;
				*gamma->green=(u16)green;
				*gamma->blue=(u16)blue;
				XRRSetCrtcGamma(display, screennr, gamma);
				XRRFreeGamma(gamma);
				return true;
			}
		#endif
		}
	}
	#endif
	#endif
	return false;
}


//! Get the current Gamma Value for the Display
bool CIrrDeviceLinux::getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast )
{
	brightness = 0.f;
	contrast = 0.f;
	#if defined(_IRR_LINUX_X11_VIDMODE_) || defined(_IRR_LINUX_X11_RANDR_)
	s32 eventbase, errorbase;
	#ifdef _IRR_LINUX_X11_VIDMODE_
	if (XF86VidModeQueryExtension(display, &eventbase, &errorbase))
	{
		XF86VidModeGamma gamma;
		XF86VidModeGetGamma(display, screennr, &gamma);
		red = gamma.red;
		green = gamma.green;
		blue = gamma.blue;
		return true;
	}
	#endif
	#if defined(_IRR_LINUX_X11_VIDMODE_) && defined(_IRR_LINUX_X11_RANDR_)
	else
	#endif
	#ifdef _IRR_LINUX_X11_RANDR_
	if (XRRQueryExtension(display, &eventbase, &errorbase))
	{
		XRRQueryVersion(display, &eventbase, &errorbase); // major, minor
		if (eventbase>=1 && errorbase>1)
		{
		#if (RANDR_MAJOR>1 || RANDR_MINOR>1)
			XRRCrtcGamma *gamma = XRRGetCrtcGamma(display, screennr);
			if (gamma)
			{
				red = *gamma->red;
				green = *gamma->green;
				blue= *gamma->blue;
				XRRFreeGamma(gamma);
				return true;
			}
		#endif
		}
	}
	#endif
	#endif
	return false;
}


//! gets text from the clipboard
//! \return Returns 0 if no string is in there.
const c8* CIrrDeviceLinux::getTextFromClipboard() const
{
#if defined(_IRR_COMPILE_WITH_X11_)
	Window ownerWindow = XGetSelectionOwner (display, X_ATOM_CLIPBOARD);
	if ( ownerWindow ==  window )
	{
		return Clipboard.c_str();
	}
	Clipboard = "";
	if (ownerWindow != None )
	{
		XConvertSelection (display, X_ATOM_CLIPBOARD, XA_STRING, XA_PRIMARY, ownerWindow, CurrentTime);
		XFlush (display);

		// check for data
		Atom type;
		int format;
		unsigned long numItems, bytesLeft, dummy;
		unsigned char *data;
		XGetWindowProperty (display, ownerWindow,
				XA_PRIMARY, // property name
				0, // offset
				0, // length (we only check for data, so 0)
				0, // Delete 0==false
				AnyPropertyType, // AnyPropertyType or property identifier
				&type, // return type
				&format, // return format
				&numItems, // number items
				&bytesLeft, // remaining bytes for partial reads
				&data); // data
		if ( bytesLeft > 0 )
		{
			// there is some data to get
			int result = XGetWindowProperty (display, ownerWindow, XA_PRIMARY, 0,
										bytesLeft, 0, AnyPropertyType, &type, &format,
										&numItems, &dummy, &data);
			if (result == Success)
				Clipboard = (irr::c8*)data;
			XFree (data);
		}
	}

	return Clipboard.c_str();

#else
	return 0;
#endif
}

//! copies text to the clipboard
void CIrrDeviceLinux::copyToClipboard(const c8* text) const
{
#if defined(_IRR_COMPILE_WITH_X11_)
	// Actually there is no clipboard on X but applications just say they own the clipboard and return text when asked.
	// Which btw. also means that on X you lose clipboard content when closing applications.
	Clipboard = text;
	XSetSelectionOwner (display, X_ATOM_CLIPBOARD, window, CurrentTime);
	XFlush (display);
#endif
}

#ifdef _IRR_COMPILE_WITH_X11_
// return true if the passed event has the type passed in parameter arg
Bool PredicateIsEventType(Display *display, XEvent *event, XPointer arg)
{
	if ( event && event->type == *(int*)arg )
	{
//		os::Printer::log("remove event:", core::stringc((int)arg).c_str(), ELL_INFORMATION);
		return True;
	}
	return False;
}
#endif //_IRR_COMPILE_WITH_X11_

//! Remove all messages pending in the system message loop
void CIrrDeviceLinux::clearSystemMessages()
{
#ifdef _IRR_COMPILE_WITH_X11_
	if (CreationParams.DriverType != video::EDT_NULL)
	{
		XEvent event;
		int usrArg = ButtonPress;
		while ( XCheckIfEvent(display, &event, PredicateIsEventType, XPointer(&usrArg)) == True ) {}
		usrArg = ButtonRelease;
		while ( XCheckIfEvent(display, &event, PredicateIsEventType, XPointer(&usrArg)) == True ) {}
		usrArg = MotionNotify;
		while ( XCheckIfEvent(display, &event, PredicateIsEventType, XPointer(&usrArg)) == True ) {}
		usrArg = KeyRelease;
		while ( XCheckIfEvent(display, &event, PredicateIsEventType, XPointer(&usrArg)) == True ) {}
		usrArg = KeyPress;
		while ( XCheckIfEvent(display, &event, PredicateIsEventType, XPointer(&usrArg)) == True ) {}
	}
#endif //_IRR_COMPILE_WITH_X11_
}

void CIrrDeviceLinux::initXAtoms()
{
#ifdef _IRR_COMPILE_WITH_X11_
	X_ATOM_CLIPBOARD = XInternAtom(display, "CLIPBOARD", False);
	X_ATOM_TARGETS = XInternAtom(display, "TARGETS", False);
	X_ATOM_UTF8_STRING = XInternAtom (display, "UTF8_STRING", False);
	X_ATOM_TEXT = XInternAtom (display, "TEXT", False);
#endif
}


#ifdef _IRR_COMPILE_WITH_X11_

Cursor CIrrDeviceLinux::TextureToMonochromeCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot)
{
	XImage * sourceImage = XCreateImage(display, visual->visual,
										1, // depth,
										ZPixmap,	// XYBitmap (depth=1), ZPixmap(depth=x)
										0, 0, sourceRect.getWidth(), sourceRect.getHeight(),
										32, // bitmap_pad,
										0// bytes_per_line (0 means continuos in memory)
										);
	sourceImage->data = new char[sourceImage->height * sourceImage->bytes_per_line];
	XImage * maskImage = XCreateImage(display, visual->visual,
										1, // depth,
										ZPixmap,
										0, 0, sourceRect.getWidth(), sourceRect.getHeight(),
										32, // bitmap_pad,
										0 // bytes_per_line
										);
	maskImage->data = new char[maskImage->height * maskImage->bytes_per_line];

	// write texture into XImage
	video::ECOLOR_FORMAT format = tex->getColorFormat();
	u32 bytesPerPixel = video::IImage::getBitsPerPixelFromFormat(format) / 8;
	u32 bytesLeftGap = sourceRect.UpperLeftCorner.X * bytesPerPixel;
	u32 bytesRightGap = tex->getPitch() - sourceRect.LowerRightCorner.X * bytesPerPixel;
	const u8* data = (const u8*)tex->lock(video::ETLM_READ_ONLY, 0);
	data += sourceRect.UpperLeftCorner.Y*tex->getPitch();
	for ( s32 y = 0; y < sourceRect.getHeight(); ++y )
	{
		data += bytesLeftGap;
		for ( s32 x = 0; x < sourceRect.getWidth(); ++x )
		{
			video::SColor pixelCol;
			pixelCol.setData((const void*)data, format);
			data += bytesPerPixel;

			if ( pixelCol.getAlpha() == 0 )	// transparent
			{
				XPutPixel(maskImage, x, y, 0);
				XPutPixel(sourceImage, x, y, 0);
			}
			else	// color
			{
				if ( pixelCol.getAverage() >= 127 )
					XPutPixel(sourceImage, x, y, 1);
				else
					XPutPixel(sourceImage, x, y, 0);
				XPutPixel(maskImage, x, y, 1);
			}
		}
		data += bytesRightGap;
	}
	tex->unlock();

	Pixmap sourcePixmap = XCreatePixmap(display, window, sourceImage->width, sourceImage->height, sourceImage->depth);
	Pixmap maskPixmap = XCreatePixmap(display, window, maskImage->width, maskImage->height, maskImage->depth);

	XGCValues values;
	values.foreground = 1;
	values.background = 1;
	GC gc = XCreateGC( display, sourcePixmap, GCForeground | GCBackground, &values );

	XPutImage(display, sourcePixmap, gc, sourceImage, 0, 0, 0, 0, sourceImage->width, sourceImage->height);
	XPutImage(display, maskPixmap, gc, maskImage, 0, 0, 0, 0, maskImage->width, maskImage->height);

	XFreeGC(display, gc);
	XDestroyImage(sourceImage);
	XDestroyImage(maskImage);

	Cursor cursorResult = 0;
	XColor foreground, background;
	foreground.red = 65535;
	foreground.green = 65535;
	foreground.blue = 65535;
	foreground.flags = DoRed | DoGreen | DoBlue;
	background.red = 0;
	background.green = 0;
	background.blue = 0;
	background.flags = DoRed | DoGreen | DoBlue;

	cursorResult = XCreatePixmapCursor(display, sourcePixmap, maskPixmap, &foreground, &background, hotspot.X, hotspot.Y);

	XFreePixmap(display, sourcePixmap);
	XFreePixmap(display, maskPixmap);

	return cursorResult;
}

#ifdef _IRR_LINUX_XCURSOR_
Cursor CIrrDeviceLinux::TextureToARGBCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot)
{
	XcursorImage * image = XcursorImageCreate (sourceRect.getWidth(), sourceRect.getHeight());
	image->xhot = hotspot.X;
	image->yhot = hotspot.Y;

	// write texture into XcursorImage
	video::ECOLOR_FORMAT format = tex->getColorFormat();
	u32 bytesPerPixel = video::IImage::getBitsPerPixelFromFormat(format) / 8;
	u32 bytesLeftGap = sourceRect.UpperLeftCorner.X * bytesPerPixel;
	u32 bytesRightGap = tex->getPitch() - sourceRect.LowerRightCorner.X * bytesPerPixel;
	XcursorPixel* target = image->pixels;
	const u8* data = (const u8*)tex->lock(video::ETLM_READ_ONLY, 0);
	data += sourceRect.UpperLeftCorner.Y*tex->getPitch();
	for ( s32 y = 0; y < sourceRect.getHeight(); ++y )
	{
		data += bytesLeftGap;
		for ( s32 x = 0; x < sourceRect.getWidth(); ++x )
		{
			video::SColor pixelCol;
			pixelCol.setData((const void*)data, format);
			data += bytesPerPixel;

			*target = (XcursorPixel)pixelCol.color;
			++target;
		}
		data += bytesRightGap;
	}
	tex->unlock();

	Cursor cursorResult=XcursorImageLoadCursor(display, image);

	XcursorImageDestroy(image);


	return cursorResult;
}
#endif // #ifdef _IRR_LINUX_XCURSOR_

Cursor CIrrDeviceLinux::TextureToCursor(irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot)
{
#ifdef _IRR_LINUX_XCURSOR_
	return TextureToARGBCursor( tex, sourceRect, hotspot );
#else
	return TextureToMonochromeCursor( tex, sourceRect, hotspot );
#endif
}
#endif	// _IRR_COMPILE_WITH_X11_


CIrrDeviceLinux::CCursorControl::CCursorControl(CIrrDeviceLinux* dev, bool null)
	: Device(dev)
#ifdef _IRR_COMPILE_WITH_X11_
	, PlatformBehavior(gui::ECPB_NONE), lastQuery(0)
#endif
	, IsVisible(true), Null(null), UseReferenceRect(false)
	, ActiveIcon(gui::ECI_NORMAL), ActiveIconStartTime(0)
{
#ifdef _IRR_COMPILE_WITH_X11_
	if (!Null)
	{
		XGCValues values;
		unsigned long valuemask = 0;

		XColor fg, bg;

		// this code, for making the cursor invisible was sent in by
		// Sirshane, thank your very much!


		Pixmap invisBitmap = XCreatePixmap(Device->display, Device->window, 32, 32, 1);
		Pixmap maskBitmap = XCreatePixmap(Device->display, Device->window, 32, 32, 1);
		Colormap screen_colormap = DefaultColormap( Device->display, DefaultScreen( Device->display ) );
		XAllocNamedColor( Device->display, screen_colormap, "black", &fg, &fg );
		XAllocNamedColor( Device->display, screen_colormap, "white", &bg, &bg );

		GC gc = XCreateGC( Device->display, invisBitmap, valuemask, &values );

		XSetForeground( Device->display, gc, BlackPixel( Device->display, DefaultScreen( Device->display ) ) );
		XFillRectangle( Device->display, invisBitmap, gc, 0, 0, 32, 32 );
		XFillRectangle( Device->display, maskBitmap, gc, 0, 0, 32, 32 );

		invisCursor = XCreatePixmapCursor( Device->display, invisBitmap, maskBitmap, &fg, &bg, 1, 1 );
		XFreeGC(Device->display, gc);
		XFreePixmap(Device->display, invisBitmap);
		XFreePixmap(Device->display, maskBitmap);

		initCursors();
	}
#endif
}

CIrrDeviceLinux::CCursorControl::~CCursorControl()
{
	// Do not clearCursors here as the display is already closed
	// TODO (cutealien): droping cursorcontrol earlier might work, not sure about reason why that's done in stub currently.
}

#ifdef _IRR_COMPILE_WITH_X11_
void CIrrDeviceLinux::CCursorControl::clearCursors()
{
	if (!Null)
		XFreeCursor(Device->display, invisCursor);
	for ( u32 i=0; i < Cursors.size(); ++i )
	{
		for ( u32 f=0; f < Cursors[i].Frames.size(); ++f )
		{
			XFreeCursor(Device->display, Cursors[i].Frames[f].IconHW);
		}
	}
}

void CIrrDeviceLinux::CCursorControl::initCursors()
{
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_top_left_arrow)) ); //  (or XC_arrow?)
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_crosshair)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_hand2)) ); // (or XC_hand1? )
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_question_arrow)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_xterm)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_X_cursor)) );	//  (or XC_pirate?)
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_watch)) );	// (or XC_clock?)
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_fleur)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_top_right_corner)) );	// NESW not available in X11
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_top_left_corner)) );	// NWSE not available in X11
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_sb_v_double_arrow)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_sb_h_double_arrow)) );
	Cursors.push_back( CursorX11(XCreateFontCursor(Device->display, XC_sb_up_arrow)) );	// (or XC_center_ptr?)
}

void CIrrDeviceLinux::CCursorControl::update()
{
	if ( (u32)ActiveIcon < Cursors.size() && !Cursors[ActiveIcon].Frames.empty() && Cursors[ActiveIcon].FrameTime )
	{
		// update animated cursors. This could also be done by X11 in case someone wants to figure that out (this way was just easier to implement)
		u32 now = Device->getTimer()->getRealTime();
		u32 frame = ((now - ActiveIconStartTime) / Cursors[ActiveIcon].FrameTime) % Cursors[ActiveIcon].Frames.size();
		XDefineCursor(Device->display, Device->window, Cursors[ActiveIcon].Frames[frame].IconHW);
	}
}
#endif

//! Sets the active cursor icon
void CIrrDeviceLinux::CCursorControl::setActiveIcon(gui::ECURSOR_ICON iconId)
{
#ifdef _IRR_COMPILE_WITH_X11_
	if ( iconId >= (s32)Cursors.size() )
		return;

	if ( Cursors[iconId].Frames.size() )
		XDefineCursor(Device->display, Device->window, Cursors[iconId].Frames[0].IconHW);

	ActiveIconStartTime = Device->getTimer()->getRealTime();
	ActiveIcon = iconId;
#endif
}


//! Add a custom sprite as cursor icon.
gui::ECURSOR_ICON CIrrDeviceLinux::CCursorControl::addIcon(const gui::SCursorSprite& icon)
{
#ifdef _IRR_COMPILE_WITH_X11_
	if ( icon.SpriteId >= 0 )
	{
		CursorX11 cX11;
		cX11.FrameTime = icon.SpriteBank->getSprites()[icon.SpriteId].frameTime;
		for ( u32 i=0; i < icon.SpriteBank->getSprites()[icon.SpriteId].Frames.size(); ++i )
		{
			irr::u32 texId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].textureNumber;
			irr::u32 rectId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].rectNumber;
			irr::core::rect<s32> rectIcon = icon.SpriteBank->getPositions()[rectId];
			Cursor cursor = Device->TextureToCursor(icon.SpriteBank->getTexture(texId), rectIcon, icon.HotSpot);
			cX11.Frames.push_back( CursorFrameX11(cursor) );
		}

		Cursors.push_back( cX11 );

		return (gui::ECURSOR_ICON)(Cursors.size() - 1);
	}
#endif
	return gui::ECI_NORMAL;
}

//! replace the given cursor icon.
void CIrrDeviceLinux::CCursorControl::changeIcon(gui::ECURSOR_ICON iconId, const gui::SCursorSprite& icon)
{
#ifdef _IRR_COMPILE_WITH_X11_
	if ( iconId >= (s32)Cursors.size() )
		return;

	for ( u32 i=0; i < Cursors[iconId].Frames.size(); ++i )
		XFreeCursor(Device->display, Cursors[iconId].Frames[i].IconHW);

	if ( icon.SpriteId >= 0 )
	{
		CursorX11 cX11;
		cX11.FrameTime = icon.SpriteBank->getSprites()[icon.SpriteId].frameTime;
		for ( u32 i=0; i < icon.SpriteBank->getSprites()[icon.SpriteId].Frames.size(); ++i )
		{
			irr::u32 texId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].textureNumber;
			irr::u32 rectId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].rectNumber;
			irr::core::rect<s32> rectIcon = icon.SpriteBank->getPositions()[rectId];
			Cursor cursor = Device->TextureToCursor(icon.SpriteBank->getTexture(texId), rectIcon, icon.HotSpot);
			cX11.Frames.push_back( CursorFrameX11(cursor) );
		}

		Cursors[iconId] = cX11;
	}
#endif
}

irr::core::dimension2di CIrrDeviceLinux::CCursorControl::getSupportedIconSize() const
{
	// this returns the closest match that is smaller or same size, so we just pass a value which should be large enough for cursors
	unsigned int width=0, height=0;
#ifdef _IRR_COMPILE_WITH_X11_
	XQueryBestCursor(Device->display, Device->window, 64, 64, &width, &height);
#endif
	return core::dimension2di(width, height);
}

} // end namespace

#endif // _IRR_COMPILE_WITH_X11_DEVICE_

