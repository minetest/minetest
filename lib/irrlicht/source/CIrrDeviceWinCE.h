// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_IRR_DEVICE_WINCE_H_INCLUDED__
#define __C_IRR_DEVICE_WINCE_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_

#include "CIrrDeviceStub.h"
#include "IrrlichtDevice.h"
#include "IImagePresenter.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <aygshell.h>
#include <sipapi.h>
#include <gx.h>

namespace irr
{
	class CIrrDeviceWinCE : public CIrrDeviceStub, video::IImagePresenter
	{
	public:

		//! constructor
		CIrrDeviceWinCE( const SIrrlichtCreationParameters& params);

		//! destructor
		virtual ~CIrrDeviceWinCE();

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

		//! returns if window has focus
		virtual bool isWindowFocused() const;

		//! returns if window is minimized
		virtual bool isWindowMinimized() const;

		//! presents a surface in the client area
		virtual bool present(video::IImage* surface, void* windowId = 0, core::rect<s32>* src=0 );

		//! notifies the device that it should close itself
		virtual void closeDevice();

		//! \return Returns a pointer to a list with all video modes
		//! supported by the gfx adapter.
		video::IVideoModeList* getVideoModeList();

		//! Notifies the device, that it has been resized
		void OnResized();

		//! Sets if the window should be resizable in windowed mode.
		virtual void setResizable(bool resize=false);

		//! Minimizes the window.
		virtual void minimizeWindow();

		//! Maximizes the window.
		virtual void maximizeWindow();

		//! Restores the window size.
		virtual void restoreWindow();

		//! Get the device type
		virtual E_DEVICE_TYPE getType() const
		{
				return EIDT_WINCE;
		}

		//! Implementation of the win32 cursor control
		class CCursorControl : public gui::ICursorControl
		{
		public:

			CCursorControl(const core::dimension2d<u32>& wsize, HWND hwnd, bool fullscreen)
				: WindowSize(wsize), InvWindowSize(0.0f, 0.0f),
					HWnd(hwnd), BorderX(0), BorderY(0),
					UseReferenceRect(false), IsVisible(true)
			{
				if (WindowSize.Width!=0)
					InvWindowSize.Width = 1.0f / WindowSize.Width;

				if (WindowSize.Height!=0)
					InvWindowSize.Height = 1.0f / WindowSize.Height;

				if (!fullscreen)
				{
					BorderX = GetSystemMetrics(SM_CXDLGFRAME);
					BorderY = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYDLGFRAME);
				}
			}

			//! Changes the visible state of the mouse cursor.
			virtual void setVisible(bool visible)
			{
				IsVisible = visible;
			}

			//! Returns if the cursor is currently visible.
			virtual bool isVisible() const
			{
				_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
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
				if (!UseReferenceRect)
					setPosition(core::round32(x*WindowSize.Width), core::round32(y*WindowSize.Height));
				else
					setPosition(core::round32(x*ReferenceRect.getWidth()), core::round32(y*ReferenceRect.getHeight()));
			}

			//! Sets the new position of the cursor.
			virtual void setPosition(const core::position2d<s32> &pos)
			{
				setPosition(pos.X, pos.Y);
			}

			//! Sets the new position of the cursor.
			virtual void setPosition(s32 x, s32 y)
			{
				RECT rect;

				if (UseReferenceRect)
				{
					SetCursorPos(ReferenceRect.UpperLeftCorner.X + x,
								 ReferenceRect.UpperLeftCorner.Y + y);
				}
				else
				{
					if (GetWindowRect(HWnd, &rect))
						SetCursorPos(x + rect.left + BorderX, y + rect.top + BorderY);
				}

				CursorPos.X = x;
				CursorPos.Y = y;
			}

			//! Returns the current position of the mouse cursor.
			virtual const core::position2d<s32>& getPosition()
			{
				updateInternalCursorPosition();
				return CursorPos;
			}

			//! Returns the current position of the mouse cursor.
			virtual core::position2d<f32> getRelativePosition()
			{
				updateInternalCursorPosition();

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

			/** Used to notify the cursor that the window was resized. */
			virtual void OnResize(const core::dimension2d<u32>& size)
			{
				WindowSize = size;
				if (size.Width!=0)
					InvWindowSize.Width = 1.0f / size.Width;
				else 
					InvWindowSize.Width = 0.f;
 
				if (size.Height!=0)
					InvWindowSize.Height = 1.0f / size.Height;
				else
					InvWindowSize.Height = 0.f;
			}

		private:

			//! Updates the internal cursor position
			void updateInternalCursorPosition()
			{
				POINT p;
				if (!GetCursorPos(&p))
				{
					DWORD xy = GetMessagePos();
					p.x = GET_X_LPARAM(xy);
					p.y = GET_Y_LPARAM(xy);
				} 

				if (UseReferenceRect)
				{
					CursorPos.X = p.x - ReferenceRect.UpperLeftCorner.X;
					CursorPos.Y = p.y - ReferenceRect.UpperLeftCorner.Y;
				}
				else
				{
					RECT rect;
					if (GetWindowRect(HWnd, &rect))
					{
						CursorPos.X = p.x-rect.left-BorderX;
						CursorPos.Y = p.y-rect.top-BorderY;
					}
					else
					{
						// window seems not to be existent, so set cursor to
						// a negative value
						CursorPos.X = -1;
						CursorPos.Y = -1;
					}
				}
			}

			core::position2d<s32> CursorPos;
			core::dimension2d<s32> WindowSize;
			core::dimension2d<f32> InvWindowSize;
			HWND HWnd;

			s32 BorderX, BorderY;
			core::rect<s32> ReferenceRect;
			bool UseReferenceRect;
			bool IsVisible;
		};


		//! returns the win32 cursor control
		CCursorControl* getWin32CursorControl();

	private:

		//! create the driver
		void createDriver();

		//! switchs to fullscreen
		bool switchToFullScreen();

		void getWindowsVersion(core::stringc& version);

		void resizeIfNecessary();

		HWND HWnd;
		CCursorControl* Win32CursorControl;

		bool ChangedToFullScreen;
		bool Resized;
		bool ExternalWindow;
	};


} // end namespace irr

#endif // _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_
#endif // __C_IRR_DEVICE_WINCE_H_INCLUDED__
