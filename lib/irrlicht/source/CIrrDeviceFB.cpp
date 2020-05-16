// Copyright (C) 2002-2007 Nikolaus Gebhardt
// Copyright (C) 2007-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CIrrDeviceFB.h"

#ifdef _IRR_COMPILE_WITH_FB_DEVICE_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <time.h>
#include <errno.h>

#include "IEventReceiver.h"
#include "os.h"
#include "CTimer.h"
#include "irrString.h"
#include "Keycodes.h"
#include "COSOperator.h"
#include "CColorConverter.h"
#include "SIrrCreationParameters.h"

#include <linux/input.h>

namespace irr
{

//! constructor
CIrrDeviceFB::CIrrDeviceFB(const SIrrlichtCreationParameters& params)
 : CIrrDeviceStub(params), Framebuffer(-1), EventDevice(-1), SoftwareImage(0),
	Pitch(0), FBColorFormat(video::ECF_A8R8G8B8), Close(false)
{
	#ifdef _DEBUG
	setDebugName("CIrrDeviceFB");
	#endif

	// print version, distribution etc.
	// thx to LynxLuna for pointing me to the uname function
	core::stringc linuxversion;
	struct utsname FBInfo; 
	uname(&FBInfo);

	linuxversion += FBInfo.sysname;
	linuxversion += " ";
	linuxversion += FBInfo.release;
	linuxversion += " ";
	linuxversion += FBInfo.version;
	linuxversion += " ";
	linuxversion += FBInfo.machine;

	Operator = new COSOperator(linuxversion);
	os::Printer::log(linuxversion.c_str(), ELL_INFORMATION);

	// create window
	if (params.DriverType != video::EDT_NULL)
	{
		// create the window, only if we do not use the null device
		if (!createWindow(params.WindowSize, params.Bits))
			return;
	}

	// create cursor control
	CursorControl = new CCursorControl(this, params.DriverType == video::EDT_NULL);

	// create driver
	createDriver();

	if (!VideoDriver)
		return;

	createGUIAndScene();
}



//! destructor
CIrrDeviceFB::~CIrrDeviceFB()
{
	if (SoftwareImage)
		munmap(SoftwareImage, CreationParams.WindowSize.Height*Pitch);
	// go back to previous format
	if (ioctl(Framebuffer, FBIOPUT_VSCREENINFO, &oldscreeninfo) <0)
		perror("Restoring old fb mode");

	if (KeyboardDevice != -1)
		if (ioctl(KeyboardDevice, KDSETMODE, &KeyboardMode) <0)
			perror("Restoring keyboard mode");
	if (EventDevice != -1)
		close(EventDevice);
	if (KeyboardDevice != -1)
		close(KeyboardDevice);
	if (Framebuffer != -1)
		close(Framebuffer);
}


bool CIrrDeviceFB::createWindow(const core::dimension2d<u32>& windowSize, u32 bits)
{
	char buf[256];
	CreationParams.WindowSize.Width = windowSize.Width;
	CreationParams.WindowSize.Height = windowSize.Height;

	KeyboardDevice = open("/dev/tty", O_RDWR);
	if (KeyboardDevice == -1)
		perror("Open keyboard");
	if (ioctl(KeyboardDevice, KDGETMODE, &KeyboardMode) <0)
		perror("Read keyboard mode");
	if (ioctl(KeyboardDevice, KDSETMODE, KD_GRAPHICS) <0)
		perror("Set keyboard mode");

	Framebuffer=open("/dev/fb/0", O_RDWR); 
	if (Framebuffer == -1)
	{
		Framebuffer=open("/dev/fb0", O_RDWR); 
		if (Framebuffer == -1)
		{
			perror("Open framebuffer");
			return false;
		}
	}
	EventDevice = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
	if (EventDevice == -1)
		perror("Open event device");

	// make format settings
	ioctl(Framebuffer, FBIOGET_FSCREENINFO, &fbfixscreeninfo);
	ioctl(Framebuffer, FBIOGET_VSCREENINFO, &oldscreeninfo);
snprintf(buf, 256, "Original resolution: %d x %d\nARGB%d%d%d%d\n",oldscreeninfo.xres,oldscreeninfo.yres,
		oldscreeninfo.transp.length,oldscreeninfo.red.length,oldscreeninfo.green.length,oldscreeninfo.blue.length);
		os::Printer::log(buf);
	memcpy(&fbscreeninfo, &oldscreeninfo, sizeof(struct fb_var_screeninfo));
	if (CreationParams.DriverType != video::EDT_NULL)
	{
		fbscreeninfo.xres = fbscreeninfo.xres_virtual = CreationParams.WindowSize.Width;
		fbscreeninfo.yres = fbscreeninfo.yres_virtual = CreationParams.WindowSize.Height;
		fbscreeninfo.bits_per_pixel = 16;
		fbscreeninfo.red.offset = 10;
		fbscreeninfo.red.length = 5;
		fbscreeninfo.green.offset = 5;
		fbscreeninfo.green.length = 5;
		fbscreeninfo.blue.offset = 0;
		fbscreeninfo.blue.length = 5;
		fbscreeninfo.transp.offset = 15;
		fbscreeninfo.transp.length = 1;
		ioctl(Framebuffer, FBIOPUT_VSCREENINFO, &fbscreeninfo);
		ioctl(Framebuffer, FBIOGET_VSCREENINFO, &fbscreeninfo);

snprintf(buf, 256, "New resolution: %d x %d (%d x %d)\nARGB%d%d%d%d\n",fbscreeninfo.xres,fbscreeninfo.yres,fbscreeninfo.xres_virtual,fbscreeninfo.yres_virtual,
		fbscreeninfo.transp.length,fbscreeninfo.red.length,fbscreeninfo.green.length,fbscreeninfo.blue.length);
		os::Printer::log(buf);

		CreationParams.WindowSize.Width = fbscreeninfo.xres;
		CreationParams.WindowSize.Height = fbscreeninfo.yres;
		CreationParams.Bits = fbscreeninfo.bits_per_pixel;
		Pitch = fbfixscreeninfo.line_length;
		if (fbscreeninfo.bits_per_pixel == 16)
		{
			if (fbscreeninfo.transp.length == 0)
				FBColorFormat = video::ECF_R5G6B5;
			else
				FBColorFormat = video::ECF_A1R5G5B5;
		}
		else
		{
			if (fbscreeninfo.transp.length == 0)
				FBColorFormat = video::ECF_R8G8B8;
			else
				FBColorFormat = video::ECF_A8R8G8B8;
		}
		if (MAP_FAILED==(SoftwareImage=(u8*)mmap(0, CreationParams.WindowSize.Height*Pitch, PROT_READ|PROT_WRITE, MAP_SHARED, Framebuffer, 0)))
		{
			perror("mmap render target");
			return false;
		}
	}
	return true;
}


//! create the driver
void CIrrDeviceFB::createDriver()
{
	switch(CreationParams.DriverType)
	{
	case video::EDT_SOFTWARE:
		#ifdef _IRR_COMPILE_WITH_SOFTWARE_
		VideoDriver = video::createSoftwareDriver(CreationParams.WindowSize, CreationParams.Fullscreen, FileSystem, this);
		#else
		os::Printer::log("No Software driver support compiled in.", ELL_WARNING);
		#endif
		break;
		
	case video::EDT_BURNINGSVIDEO:
		#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
		VideoDriver = video::createBurningVideoDriver(CreationParams, FileSystem, this);
		#else
		os::Printer::log("Burning's video driver was not compiled in.", ELL_WARNING);
		#endif
		break;

	case video::EDT_OPENGL:
	case video::EDT_DIRECT3D8:
	case video::EDT_DIRECT3D9:
		os::Printer::log("This driver is not available in FB. Try Software renderer.",
			ELL_WARNING);
		break;

	default:
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;
	}
}


//! runs the device. Returns false if device wants to be deleted
bool CIrrDeviceFB::run()
{
	os::Timer::tick();

	struct input_event ev;
	if (EventDevice>=0)
	{
		if ((read(EventDevice, &ev, sizeof(input_event)) < 0) &&
				errno != EAGAIN)
			perror("Read input event");
		if (ev.type == EV_KEY)
		{
			irr::SEvent irrevent;
			irrevent.EventType = irr::EET_KEY_INPUT_EVENT;
			irrevent.KeyInput.PressedDown = true;
			
			switch (ev.code)
			{
				case KEY_RIGHTCTRL:
				case KEY_LEFTCTRL:
					irrevent.KeyInput.Control = true;
				break;
				case KEY_RIGHTSHIFT:
				case KEY_LEFTSHIFT:
					irrevent.KeyInput.Shift = true;
				break;
				case KEY_ESC:
					irrevent.KeyInput.Key = (EKEY_CODE)0x1B;
				break;
				case KEY_SPACE:
					irrevent.KeyInput.Key = (EKEY_CODE)0x20;
				break;
				case KEY_UP:
					irrevent.KeyInput.Key = (EKEY_CODE)0x26;
				break;
				case KEY_LEFT:
					irrevent.KeyInput.Key = (EKEY_CODE)0x25;
				break;
				case KEY_RIGHT:
					irrevent.KeyInput.Key = (EKEY_CODE)0x27;
				break;
				case KEY_DOWN:
					irrevent.KeyInput.Key = (EKEY_CODE)0x28;
				break;
				default:
					irrevent.KeyInput.Key = (EKEY_CODE)0;
				break;
			}
			postEventFromUser(irrevent);
		}
	}

	return !Close;
}


//! Pause the current process for the minimum time allowed only to allow other processes to execute
void CIrrDeviceFB::yield()
{
	struct timespec ts = {0,0};
	nanosleep(&ts, NULL);
}


//! Pause execution and let other processes to run for a specified amount of time.
void CIrrDeviceFB::sleep(u32 timeMs, bool pauseTimer=false)
{
	bool wasStopped = Timer ? Timer->isStopped() : true;
	
	struct timespec ts;
	ts.tv_sec = (time_t) (timeMs / 1000);
	ts.tv_nsec = (long) (timeMs % 1000) * 1000000;

	if (pauseTimer && !wasStopped)
		Timer->stop();

	nanosleep(&ts, NULL);

	if (pauseTimer && !wasStopped)
		Timer->start();
}


//! presents a surface in the client area
bool CIrrDeviceFB::present(video::IImage* image, void* windowId, core::rect<s32>* src )
{
	// this is only necessary for software drivers.
	if (CreationParams.DriverType != video::EDT_SOFTWARE && CreationParams.DriverType != video::EDT_BURNINGSVIDEO)
		return false;

	if (!SoftwareImage)
		return false;

	u8* destData = SoftwareImage;
	u32 srcwidth = (u32)image->getDimension().Width;
	u32 srcheight = (u32)image->getDimension().Height;
	// clip images
	srcheight = core::min_(srcheight, CreationParams.WindowSize.Height);
	srcwidth = core::min_(srcwidth, CreationParams.WindowSize.Width);

	u8* srcdata = (u8*)image->lock();
	for (u32 y=0; y<srcheight; ++y)
	{
		video::CColorConverter::convert_viaFormat(srcdata, image->getColorFormat(), srcwidth, destData, FBColorFormat);
		srcdata+=image->getPitch();
		destData+=Pitch;
	}
	image->unlock();
	msync(SoftwareImage,CreationParams.WindowSize.Width*CreationParams.WindowSize.Height,MS_ASYNC);
	return true;
}


//! notifies the device that it should close itself
void CIrrDeviceFB::closeDevice()
{
	Close = true;
}


//! returns if window is active. if not, nothing need to be drawn
bool CIrrDeviceFB::isWindowActive() const
{
	return true;
}


//! returns if window has focus
bool CIrrDeviceFB::isWindowFocused() const
{
	return true;
}


//! returns if window is minimized
bool CIrrDeviceFB::isWindowMinimized() const
{
	return false;
}


//! sets the caption of the window
void CIrrDeviceFB::setWindowCaption(const wchar_t* text)
{
}


//! Sets if the window should be resizeable in windowed mode.
void CIrrDeviceFB::setResizable(bool resize)
{
}


//! Minimizes window
void CIrrDeviceFB::minimizeWindow()
{
}


//! Maximizes window
void CIrrDeviceFB::maximizeWindow()
{
}


//! Restores original window size
void CIrrDeviceFB::restoreWindow()
{
}


//! Returns the type of this device
E_DEVICE_TYPE CIrrDeviceFB::getType() const
{
	return EIDT_FRAMEBUFFER;
}


} // end namespace irr

#endif // _IRR_USE_FB_DEVICE_

