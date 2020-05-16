// Copyright (C) 2005-2006 Etienne Petitjean
// Copyright (C) 2007-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#ifndef __C_IRR_DEVICE_MACOSX_H_INCLUDED__
#define __C_IRR_DEVICE_MACOSX_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_OSX_DEVICE_

#import <AppKit/NSWindow.h>
#import <AppKit/NSOpenGL.h>
#import <AppKit/NSBitmapImageRep.h>

#include "CIrrDeviceStub.h"
#include "IrrlichtDevice.h"
#include "IImagePresenter.h"
#include "IGUIEnvironment.h"
#include "ICursorControl.h"

#include <OpenGL/OpenGL.h>
#include <map>

namespace irr
{
	class CIrrDeviceMacOSX : public CIrrDeviceStub, video::IImagePresenter
	{
	public:

		//! constructor
		CIrrDeviceMacOSX(const SIrrlichtCreationParameters& params);

		//! destructor
		virtual ~CIrrDeviceMacOSX();

		//! runs the device. Returns false if device wants to be deleted
		virtual bool run();

		//! Cause the device to temporarily pause execution and let other processes to run
		// This should bring down processor usage without major performance loss for Irrlicht
		virtual void yield();

		//! Pause execution and let other processes to run for a specified amount of time.
		virtual void sleep(u32 timeMs, bool pauseTimer);

		//! sets the caption of the window
		virtual void setWindowCaption(const wchar_t* text);

		//! returns if window is active. if not, nothing need to be drawn
		virtual bool isWindowActive() const;

		//! Checks if the Irrlicht window has focus
		virtual bool isWindowFocused() const;

		//! Checks if the Irrlicht window is minimized
		virtual bool isWindowMinimized() const;

		//! presents a surface in the client area
		virtual bool present(video::IImage* surface, void* windowId=0, core::rect<s32>* src=0 );

		//! notifies the device that it should close itself
		virtual void closeDevice();

		//! Sets if the window should be resizable in windowed mode.
		virtual void setResizable(bool resize);

		//! Returns true if the window is resizable, false if not
		virtual bool isResizable() const;

		//! Minimizes the window if possible
		virtual void minimizeWindow();

		//! Maximizes the window if possible.
		virtual void maximizeWindow();

		//! Restore the window to normal size if possible.
		virtual void restoreWindow();

		//! Activate any joysticks, and generate events for them.
		virtual bool activateJoysticks(core::array<SJoystickInfo> & joystickInfo);

		//! \return Returns a pointer to a list with all video modes
		//! supported by the gfx adapter.
		virtual video::IVideoModeList* getVideoModeList();

		//! Get the device type
		virtual E_DEVICE_TYPE getType() const
		{
				return EIDT_OSX;
		}

		void flush();
		void setMouseLocation(int x, int y);
		void setResize(int width, int height);
		void setCursorVisible(bool visible);

	private:

		//! create the driver
		void createDriver();

		//! Implementation of the macos x cursor control
		class CCursorControl : public gui::ICursorControl
		{
		public:

			CCursorControl(const core::dimension2d<u32>& wsize, CIrrDeviceMacOSX *device)
				: WindowSize(wsize), IsVisible(true), InvWindowSize(0.0f, 0.0f), Device(device), UseReferenceRect(false)
			{
				CursorPos.X = CursorPos.Y = 0;
				if (WindowSize.Width!=0)
					InvWindowSize.Width = 1.0f / WindowSize.Width;
				if (WindowSize.Height!=0)
					InvWindowSize.Height = 1.0f / WindowSize.Height;
			}

			//! Changes the visible state of the mouse cursor.
			virtual void setVisible(bool visible)
			{
				IsVisible = visible;
				Device->setCursorVisible(visible);
			}

			//! Returns if the cursor is currently visible.
			virtual bool isVisible() const
			{
				return IsVisible;
			}

			//! Sets the new position of the cursor.
			virtual void setPosition(const core::position2d<f32> &pos)
			{
				setPosition(pos.X, pos.Y);
			}

			//! Sets the new position of the cursor.
			virtual void setPosition(f32 x, f32 y)
			{
				setPosition((s32)(x*WindowSize.Width), (s32)(y*WindowSize.Height));
			}

			//! Sets the new position of the cursor.
			virtual void setPosition(const core::position2d<s32> &pos)
			{
				if (CursorPos.X != pos.X || CursorPos.Y != pos.Y)
					setPosition(pos.X, pos.Y);
			}

			//! Sets the new position of the cursor.
			virtual void setPosition(s32 x, s32 y)
			{
				if (UseReferenceRect)
				{
					Device->setMouseLocation(ReferenceRect.UpperLeftCorner.X + x, ReferenceRect.UpperLeftCorner.Y + y);
				}
				else
				{
					Device->setMouseLocation(x,y);
				}
			}

			//! Returns the current position of the mouse cursor.
			virtual const core::position2d<s32>& getPosition()
			{
				return CursorPos;
			}

			//! Returns the current position of the mouse cursor.
			virtual core::position2d<f32> getRelativePosition()
			{
				if (!UseReferenceRect)
				{
					return core::position2d<f32>(CursorPos.X * InvWindowSize.Width,
						CursorPos.Y * InvWindowSize.Height);
				}

				return core::position2d<f32>(CursorPos.X / (f32)ReferenceRect.getWidth(),
						CursorPos.Y / (f32)ReferenceRect.getHeight());
			}

			//! Sets an absolute reference rect for calculating the cursor position.
			virtual void setReferenceRect(core::rect<s32>* rect=0)
			{
				if (rect)
				{
					ReferenceRect = *rect;
					UseReferenceRect = true;

					// prevent division through zero and uneven sizes

					if (!ReferenceRect.getHeight() || ReferenceRect.getHeight()%2)
						ReferenceRect.LowerRightCorner.Y += 1;

					if (!ReferenceRect.getWidth() || ReferenceRect.getWidth()%2)
						ReferenceRect.LowerRightCorner.X += 1;
				}
				else
					UseReferenceRect = false;
			}

			//! Updates the internal cursor position
			void updateInternalCursorPosition(int x,int y)
			{
				CursorPos.X = x;
				CursorPos.Y = y;
			}

		private:

			core::position2d<s32> CursorPos;
			core::dimension2d<s32> WindowSize;
			core::dimension2d<float> InvWindowSize;
			core::rect<s32> ReferenceRect;
			CIrrDeviceMacOSX *Device;
			bool IsVisible;
			bool UseReferenceRect;
		};

		bool createWindow();
		void initKeycodes();
		void storeMouseLocation();
		void postMouseEvent(void *event, irr::SEvent &ievent);
		void postKeyEvent(void *event, irr::SEvent &ievent, bool pressed);
		void pollJoysticks();

		NSWindow *Window;
		CGLContextObj CGLContext;
		NSOpenGLContext *OGLContext;
		NSBitmapImageRep *SoftwareDriverTarget;
		std::map<int,int> KeyCodes;
		int DeviceWidth;
		int DeviceHeight;
		int ScreenWidth;
		int ScreenHeight;
		u32 MouseButtonStates;
        u32 SoftwareRendererType;
        bool IsFullscreen;
		bool IsActive;
		bool IsShiftDown;
		bool IsControlDown;
		bool IsResizable;
	};


} // end namespace irr

#endif // _IRR_COMPILE_WITH_OSX_DEVICE_
#endif // __C_IRR_DEVICE_MACOSX_H_INCLUDED__

