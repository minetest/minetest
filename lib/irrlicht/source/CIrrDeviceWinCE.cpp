// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_

#include "CIrrDeviceWinCE.h"
#include "IEventReceiver.h"
#include "irrList.h"
#include "os.h"

#include "CTimer.h"
#include "irrString.h"
#include "COSOperator.h"
#include "dimension2d.h"
#include <winuser.h>
#include "irrlicht.h"

#ifdef _MSC_VER
	#pragma comment (lib, "aygshell.lib")
#endif


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
		IVideoDriver* createOpenGLDriver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, this);
		#endif
	}
} // end namespace irr



struct SEnvMapper
{
	HWND hWnd;
	irr::CIrrDeviceWinCE* irrDev;
};

irr::core::list<SEnvMapper> EnvMap;

SEnvMapper* getEnvMapperFromHWnd(HWND hWnd)
{
	irr::core::list<SEnvMapper>::Iterator it = EnvMap.begin();
	for (; it!= EnvMap.end(); ++it)
		if ((*it).hWnd == hWnd)
			return &(*it);

	return 0;
}

irr::CIrrDeviceWinCE* getDeviceFromHWnd(HWND hWnd)
{
	irr::core::list<SEnvMapper>::Iterator it = EnvMap.begin();
	for (; it!= EnvMap.end(); ++it)
		if ((*it).hWnd == hWnd)
			return (*it).irrDev;

	return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	#ifndef WM_MOUSEWHEEL
	#define WM_MOUSEWHEEL 0x020A
	#endif
	#ifndef WHEEL_DELTA
	#define WHEEL_DELTA 120
	#endif

	irr::CIrrDeviceWinCE* dev = 0;
	irr::SEvent event;
	SEnvMapper* envm = 0;

	//BYTE allKeys[256];

	static irr::s32 ClickCount=0;
	if (GetCapture() != hWnd && ClickCount > 0)
		ClickCount = 0;

	switch (message)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		return 0;

	case WM_ERASEBKGND:
		return 0;

	case WM_SETCURSOR:
		envm = getEnvMapperFromHWnd(hWnd);
		if (envm && !envm->irrDev->getWin32CursorControl()->isVisible())
		{
			SetCursor(NULL);
			return 0;
		}
		break;

	case WM_MOUSEWHEEL:
		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Wheel = ((irr::f32)((short)HIWORD(wParam))) / (irr::f32)WHEEL_DELTA;
		event.MouseInput.Event = irr::EMIE_MOUSE_WHEEL;

		POINT p; // fixed by jox
		p.x = 0; p.y = 0;
		ClientToScreen(hWnd, &p);
		event.MouseInput.X = LOWORD(lParam) - p.x;
		event.MouseInput.Y = HIWORD(lParam) - p.y;
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->postEventFromUser(event);
		break;

	case WM_LBUTTONDOWN:
		ClickCount++;
		SetCapture(hWnd);
		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = irr::EMIE_LMOUSE_PRESSED_DOWN;
		event.MouseInput.X = (short)LOWORD(lParam);
		event.MouseInput.Y = (short)HIWORD(lParam);
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->postEventFromUser(event);
		return 0;

	case WM_LBUTTONUP:
		ClickCount--;
		if (ClickCount<1)
		{
			ClickCount=0;
			ReleaseCapture();
		}
		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = irr::EMIE_LMOUSE_LEFT_UP;
		event.MouseInput.X = (short)LOWORD(lParam);
		event.MouseInput.Y = (short)HIWORD(lParam);
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->postEventFromUser(event);
		return 0;

	case WM_RBUTTONDOWN:
		ClickCount++;
		SetCapture(hWnd);
		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = irr::EMIE_RMOUSE_PRESSED_DOWN;
		event.MouseInput.X = (short)LOWORD(lParam);
		event.MouseInput.Y = (short)HIWORD(lParam);
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->postEventFromUser(event);
		return 0;

	case WM_RBUTTONUP:
		ClickCount--;
		if (ClickCount<1)
		{
			ClickCount=0;
			ReleaseCapture();
		}
		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = irr::EMIE_RMOUSE_LEFT_UP;
		event.MouseInput.X = (short)LOWORD(lParam);
		event.MouseInput.Y = (short)HIWORD(lParam);
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->postEventFromUser(event);
		return 0;

	case WM_MBUTTONDOWN:
		ClickCount++;
		SetCapture(hWnd);
		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = irr::EMIE_MMOUSE_PRESSED_DOWN;
		event.MouseInput.X = (short)LOWORD(lParam);
		event.MouseInput.Y = (short)HIWORD(lParam);
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->postEventFromUser(event);
		return 0;

	case WM_MBUTTONUP:
		ClickCount--;
		if (ClickCount<1)
		{
			ClickCount=0;
			ReleaseCapture();
		}
		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = irr::EMIE_MMOUSE_LEFT_UP;
		event.MouseInput.X = (short)LOWORD(lParam);
		event.MouseInput.Y = (short)HIWORD(lParam);
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->postEventFromUser(event);
		return 0;

	case WM_MOUSEMOVE:
		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
		event.MouseInput.X = (short)LOWORD(lParam);
		event.MouseInput.Y = (short)HIWORD(lParam);
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->postEventFromUser(event);

		return 0;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
		{
			event.EventType = irr::EET_KEY_INPUT_EVENT;
			event.KeyInput.Key = (irr::EKEY_CODE)wParam;
			event.KeyInput.PressedDown = (message==WM_KEYDOWN || message == WM_SYSKEYDOWN);
			dev = getDeviceFromHWnd(hWnd);
/*
			WORD KeyAsc=0;
			GetKeyboardState(allKeys);
			ToAscii(wParam,lParam,allKeys,&KeyAsc,0);
*/
//			event.KeyInput.Shift = ((allKeys[VK_SHIFT] & 0x80)!=0);
//			event.KeyInput.Control = ((allKeys[VK_CONTROL] & 0x80)!=0);
//			event.KeyInput.Char = (KeyAsc & 0x00ff); //KeyAsc >= 0 ? KeyAsc : 0;

			if (dev)
				dev->postEventFromUser(event);

			return 0;
		}

	case WM_SIZE:
	{
		// resize
		dev = getDeviceFromHWnd(hWnd);
		if (dev)
			dev->OnResized();
	}
	return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

namespace irr
{

//! constructor
CIrrDeviceWinCE::CIrrDeviceWinCE(const SIrrlichtCreationParameters& params)
: CIrrDeviceStub(params), HWnd(0),
	Win32CursorControl(0), ChangedToFullScreen(false), Resized(false),
	ExternalWindow(false)
{
	#ifdef _DEBUG
	setDebugName("CIrrDeviceWinCE");
	#endif

	core::stringc winversion;
	getWindowsVersion(winversion);
	Operator = new COSOperator(winversion);
	os::Printer::log(winversion.c_str(), ELL_INFORMATION);

	HINSTANCE hInstance = GetModuleHandle(0);

	// create the window only if we do not use the null device
	if (!CreationParams.WindowId && (CreationParams.DriverType != video::EDT_NULL))
	{
		const wchar_t* ClassName = L"CIrrDeviceWinCE";

		// Register Class
		WNDCLASS wc;
		wc.style		= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc		= WndProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= hInstance;
		wc.hIcon		= NULL;
		wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName		= 0;
		wc.lpszClassName	= ClassName;

		// if there is an icon, load it
		wc.hIcon = (HICON)LoadImageW(hInstance, L"irrlicht.ico", IMAGE_ICON, 0,0, 0);

		RegisterClass(&wc);

		// calculate client size

		RECT clientSize;
		clientSize.top = 0;
		clientSize.left = 0;
		clientSize.right = CreationParams.WindowSize.Width;
		clientSize.bottom = CreationParams.WindowSize.Height;

		DWORD style = WS_POPUP;

		if (!CreationParams.Fullscreen)
			style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

		AdjustWindowRectEx(&clientSize, style, FALSE, 0);

		const s32 realWidth = clientSize.right - clientSize.left;
		const s32 realHeight = clientSize.bottom - clientSize.top;

		const s32 windowLeft = core::s32_max ( 0, (GetSystemMetrics(SM_CXSCREEN) - realWidth) >> 1 );
		const s32 windowTop = core::s32_max ( 0, (GetSystemMetrics(SM_CYSCREEN) - realHeight) >> 1 );

		// create window

		HWnd = CreateWindowW( ClassName, L"", style, windowLeft, windowTop,
					realWidth, realHeight,	NULL, NULL, hInstance, NULL);

		ShowWindow(HWnd , SW_SHOW);
		UpdateWindow(HWnd);

		// fix ugly ATI driver bugs. Thanks to ariaci
		MoveWindow(HWnd, windowLeft, windowTop, realWidth, realHeight, TRUE);
	}
	else if (CreationParams.WindowId)
	{
		// attach external window
		HWnd = static_cast<HWND>(CreationParams.WindowId);
		RECT r;
		GetWindowRect(HWnd, &r);
		CreationParams.WindowSize.Width = r.right - r.left;
		CreationParams.WindowSize.Height = r.bottom - r.top;
		CreationParams.Fullscreen = false;
		ExternalWindow = true;
	}

	// create cursor control

	Win32CursorControl = new CCursorControl(CreationParams.WindowSize, HWnd, CreationParams.Fullscreen);
	CursorControl = Win32CursorControl;

	// create driver

	createDriver();

	if (VideoDriver)
		createGUIAndScene();

	// register environment

	SEnvMapper em;
	em.irrDev = this;
	em.hWnd = HWnd;
	EnvMap.push_back(em);

	// set this as active window
	SetActiveWindow(HWnd);
	SetForegroundWindow(HWnd);
}


//! destructor
CIrrDeviceWinCE::~CIrrDeviceWinCE()
{
	// unregister environment

	if (ChangedToFullScreen)
		SHFullScreen(HWnd, SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON | SHFS_SHOWSIPBUTTON);

	irr::core::list<SEnvMapper>::Iterator it = EnvMap.begin();
	for (; it!= EnvMap.end(); ++it)
		if ((*it).hWnd == HWnd)
		{
			EnvMap.erase(it);
			break;
		}
}


//! create the driver
void CIrrDeviceWinCE::createDriver()
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

	case video::EDT_OPENGL:

		#ifdef _IRR_COMPILE_WITH_OPENGL_
		if (CreationParams.Fullscreen)
			switchToFullScreen();
		VideoDriver = video::createOpenGLDriver(CreationParams, FileSystem);
		if (!VideoDriver)
		{
			os::Printer::log("Could not create OpenGL driver.", ELL_ERROR);
		}
		#else
		os::Printer::log("OpenGL driver was not compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_SOFTWARE:

		#ifdef _IRR_COMPILE_WITH_SOFTWARE_
		if (CreationParams.Fullscreen)
			switchToFullScreen();
		VideoDriver = video::createSoftwareDriver(CreationParams.WindowSize, CreationParams.Fullscreen, FileSystem, this);
		#else
		os::Printer::log("Software driver was not compiled in.", ELL_ERROR);
		#endif

		break;

	case video::EDT_BURNINGSVIDEO:
		#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
		if (CreationParams.Fullscreen)
			switchToFullScreen();
		VideoDriver = video::createBurningVideoDriver(CreationParams, FileSystem, this);
		#else
		os::Printer::log("Burning's Video driver was not compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_NULL:
		// create null driver
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;

	default:
		os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
		break;
	}
}


//! runs the device. Returns false if device wants to be deleted
bool CIrrDeviceWinCE::run()
{
	os::Timer::tick();

	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);

		if (ExternalWindow && msg.hwnd == HWnd)
			WndProc(HWnd, msg.message, msg.wParam, msg.lParam);
		else
			DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
			Close = true;
	}

	if (!Close)
		resizeIfNecessary();

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return !Close;
}


//! Pause the current process for the minimum time allowed only to allow other processes to execute
void CIrrDeviceWinCE::yield()
{
	Sleep(1);
}


//! Pause execution and let other processes to run for a specified amount of time.
void CIrrDeviceWinCE::sleep(u32 timeMs, bool pauseTimer)
{
	const bool wasStopped = Timer ? Timer->isStopped() : true;
	if (pauseTimer && !wasStopped)
		Timer->stop();

	Sleep(timeMs);

	if (pauseTimer && !wasStopped)
		Timer->start();
}


void CIrrDeviceWinCE::resizeIfNecessary()
{
	if (!Resized)
		return;

	RECT r;
	GetClientRect(HWnd, &r);

	char tmp[255];

	if (r.right < 2 || r.bottom < 2)
	{
		sprintf(tmp, "Ignoring resize operation to (%ld %ld)", r.right, r.bottom);
		os::Printer::log(tmp);
	}
	else
	{
		sprintf(tmp, "Resizing window (%ld %ld)", r.right, r.bottom);
		os::Printer::log(tmp);

		getVideoDriver()->OnResize(irr::core::dimension2d<irr::u32>(r.right, r.bottom));
		getWin32CursorControl()->OnResize(getVideoDriver()->getScreenSize());
	}

	Resized = false;
}


//! sets the caption of the window
void CIrrDeviceWinCE::setWindowCaption(const wchar_t* text)
{
	SetWindowTextW(HWnd, text);
}


#if !defined(BITMAPV4HEADER)
typedef struct {
  DWORD        bV4Size;
  LONG         bV4Width;
  LONG         bV4Height;
  WORD         bV4Planes;
  WORD         bV4BitCount;
  DWORD        bV4V4Compression;
  DWORD        bV4SizeImage;
  LONG         bV4XPelsPerMeter;
  LONG         bV4YPelsPerMeter;
  DWORD        bV4ClrUsed;
  DWORD        bV4ClrImportant;
  DWORD        bV4RedMask;
  DWORD        bV4GreenMask;
  DWORD        bV4BlueMask;
  DWORD        bV4AlphaMask;
  DWORD        bV4CSType;
  DWORD			un[9];
} BITMAPV4HEADER, *PBITMAPV4HEADER;
#endif


//! presents a surface in the client area
bool CIrrDeviceWinCE::present(video::IImage* image, void* windowId, core::rect<s32>* src)
{
	HWND hwnd = HWnd;
	if ( windowId )
		hwnd = (HWND)windowId;

	HDC dc = GetDC(hwnd);

	if ( dc )
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		const void* memory = (const void *)image->lock();

		BITMAPV4HEADER bi;
		memset (&bi, 0, sizeof(bi));
		bi.bV4Size          = sizeof(BITMAPINFOHEADER);
		bi.bV4BitCount      = image->getBitsPerPixel();
		bi.bV4Planes        = 1;
		bi.bV4Width         = image->getDimension().Width;
		bi.bV4Height        = 0 - image->getDimension().Height;
		bi.bV4V4Compression = BI_BITFIELDS;
		bi.bV4AlphaMask     = image->getAlphaMask ();
		bi.bV4RedMask       = image->getRedMask ();
		bi.bV4GreenMask     = image->getGreenMask();
		bi.bV4BlueMask      = image->getBlueMask();

		int r = 0;
		if ( src )
		{
			r = StretchDIBits(dc, 0,0, rect.right, rect.bottom,
					src->UpperLeftCorner.X, src->UpperLeftCorner.Y,
					src->getWidth(), src->getHeight(),
					memory, (const BITMAPINFO*)(&bi), DIB_RGB_COLORS, SRCCOPY);
		}
		else
		{
			r = StretchDIBits(dc, 0,0, rect.right, rect.bottom,
					0, 0, image->getDimension().Width, image->getDimension().Height,
					memory, (const BITMAPINFO*)(&bi), DIB_RGB_COLORS, SRCCOPY);
		}

		image->unlock();

		ReleaseDC(hwnd, dc);
	}
	return true;
}


//! notifies the device that it should close itself
void CIrrDeviceWinCE::closeDevice()
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
	PostQuitMessage(0);
	PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
	if (!ExternalWindow)
	{
		DestroyWindow(HWnd);
		const fschar_t* ClassName = __TEXT("CIrrDeviceWin32");
		HINSTANCE hInstance = GetModuleHandle(0);
		UnregisterClass(ClassName, hInstance);
	}
	Close=true;
}


//! returns if window is active. if not, nothing need to be drawn
bool CIrrDeviceWinCE::isWindowActive() const
{
	bool ret = (GetActiveWindow() == HWnd);
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! returns if window has focus
bool CIrrDeviceWinCE::isWindowFocused() const
{
	bool ret = (GetFocus() == HWnd);
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! returns if window is minimized
bool CIrrDeviceWinCE::isWindowMinimized() const
{
#if 0
	WINDOWPLACEMENT plc;
	plc.length=sizeof(WINDOWPLACEMENT);
	bool ret=false;
	if (GetWindowPlacement(HWnd,&plc))
		ret=(plc.showCmd & SW_SHOWMINIMIZED);
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
#endif
	return false;
}


//! switches to fullscreen
bool CIrrDeviceWinCE::switchToFullScreen()
{
	ChangedToFullScreen = SHFullScreen(HWnd, SHFS_HIDESIPBUTTON | SHFS_HIDETASKBAR) != 0;
	return ChangedToFullScreen;
}


//! returns the win32 cursor control
CIrrDeviceWinCE::CCursorControl* CIrrDeviceWinCE::getWin32CursorControl()
{
	return Win32CursorControl;
}


//! Return pointer to a list with all video modes supported by the gfx adapter.
/** \return Pointer to video modes list */
video::IVideoModeList* CIrrDeviceWinCE::getVideoModeList()
{
	if (!VideoModeList->getVideoModeCount())
	{
		// enumerate video modes.
		DWORD i=0;
		DEVMODE mode;
		memset(&mode, 0, sizeof(mode));
		mode.dmSize = sizeof(mode);

		while (EnumDisplaySettings(NULL, i, &mode))
		{
			VideoModeList->addMode(core::dimension2d<u32>(mode.dmPelsWidth, mode.dmPelsHeight),
				mode.dmBitsPerPel);

			++i;
		}

		if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode))
			VideoModeList->setDesktop(mode.dmBitsPerPel, core::dimension2d<u32>(mode.dmPelsWidth, mode.dmPelsHeight));
	}

	return VideoModeList;
}


void CIrrDeviceWinCE::getWindowsVersion(core::stringc& out)
{
	out = "WinCE";
}


//! Notifies the device, that it has been resized
void CIrrDeviceWinCE::OnResized()
{
	Resized = true;
}


//! Sets if the window should be resizable in windowed mode.
void CIrrDeviceWinCE::setResizable(bool resize)
{
	if (ExternalWindow || !getVideoDriver() || CreationParams.Fullscreen)
		return;

	LONG style = WS_POPUP;

	if (!resize)
		style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	else
		style = WS_THICKFRAME | WS_SYSMENU | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MAXIMIZEBOX;

	if (!SetWindowLong(HWnd, GWL_STYLE, style))
		os::Printer::log("Could not change window style.");

	RECT clientSize;
	clientSize.top = 0;
	clientSize.left = 0;
	clientSize.right = getVideoDriver()->getScreenSize().Width;
	clientSize.bottom = getVideoDriver()->getScreenSize().Height;

	AdjustWindowRectEx(&clientSize, style, FALSE, 0);

	const s32 realWidth = clientSize.right - clientSize.left;
	const s32 realHeight = clientSize.bottom - clientSize.top;

	const s32 windowLeft = (GetSystemMetrics(SM_CXSCREEN) - realWidth) / 2;
	const s32 windowTop = (GetSystemMetrics(SM_CYSCREEN) - realHeight) / 2;

	SetWindowPos(HWnd, HWND_TOP, windowLeft, windowTop, realWidth, realHeight,
		SWP_FRAMECHANGED | SWP_NOMOVE | SWP_SHOWWINDOW);
}


//! Minimizes the window.
void CIrrDeviceWinCE::minimizeWindow()
{
	// do nothing
}

//! Maximize window
void CIrrDeviceWinCE::maximizeWindow()
{
	// do nothing
}


//! Restore original window size
void CIrrDeviceWinCE::restoreWindow()
{
	// do nothing
}


} // end namespace

#endif // _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_

