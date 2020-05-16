// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_WINDOWS_DEVICE_

#if defined (__STRICT_ANSI__)
    #error Compiling with __STRICT_ANSI__ not supported. g++ does set this when compiling with -std=c++11 or -std=c++0x. Use instead -std=gnu++11 or -std=gnu++0x. Or use -U__STRICT_ANSI__ to disable strict ansi.
#endif

#include "CIrrDeviceWin32.h"
#include "IEventReceiver.h"
#include "irrList.h"
#include "os.h"

#include "CTimer.h"
#include "irrString.h"
#include "COSOperator.h"
#include "dimension2d.h"
#include "IGUISpriteBank.h"
#include <winuser.h>
#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
#ifdef _IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#ifdef _MSC_VER
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#endif
#else
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#endif
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
		IVideoDriver* createOpenGLDriver(const irr::SIrrlichtCreationParameters& params,
			io::IFileSystem* io, CIrrDeviceWin32* device);
		#endif
	}
} // end namespace irr

namespace irr
{
struct SJoystickWin32Control
{
	CIrrDeviceWin32* Device;

#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_) && defined(_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_)
	IDirectInput8* DirectInputDevice;
#endif
#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
	struct JoystickInfo
	{
		u32 Index;
#ifdef _IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_
		core::stringc Name;
		GUID guid;
		LPDIRECTINPUTDEVICE8 lpdijoy;
		DIDEVCAPS devcaps;
		u8 axisValid[8];
#else
		JOYCAPS Caps;
#endif
	};
	core::array<JoystickInfo> ActiveJoysticks;
#endif

	SJoystickWin32Control(CIrrDeviceWin32* dev) : Device(dev)
	{
#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_) && defined(_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_)
		DirectInputDevice=0;
		if (DI_OK != (DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&DirectInputDevice, NULL)))
		{
			os::Printer::log("Could not create DirectInput8 Object", ELL_WARNING);
			return;
		}
#endif
	}
	~SJoystickWin32Control()
	{
#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_) && defined(_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_)
		for(u32 joystick = 0; joystick < ActiveJoysticks.size(); ++joystick)
		{
			LPDIRECTINPUTDEVICE8 dev = ActiveJoysticks[joystick].lpdijoy;
			if (dev)
			{
				dev->Unacquire();
			}
			dev->Release();
		}

		if (DirectInputDevice)
			DirectInputDevice->Release();
#endif
	}

#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_) && defined(_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_)
	static BOOL CALLBACK EnumJoysticks(LPCDIDEVICEINSTANCE lpddi, LPVOID cp)
	{
		SJoystickWin32Control* p=(SJoystickWin32Control*)cp;
		p->directInputAddJoystick(lpddi);
		return DIENUM_CONTINUE;
	}
	void directInputAddJoystick(LPCDIDEVICEINSTANCE lpddi)
	{
		//Get the GUID of the joystuck
		const GUID guid = lpddi->guidInstance;

		JoystickInfo activeJoystick;
		activeJoystick.Index=ActiveJoysticks.size();
		activeJoystick.guid=guid;
		activeJoystick.Name=lpddi->tszProductName;
		if (FAILED(DirectInputDevice->CreateDevice(guid, &activeJoystick.lpdijoy, NULL)))
		{
			os::Printer::log("Could not create DirectInput device", ELL_WARNING);
			return;
		}

		activeJoystick.devcaps.dwSize=sizeof(activeJoystick.devcaps);
		if (FAILED(activeJoystick.lpdijoy->GetCapabilities(&activeJoystick.devcaps)))
		{
			os::Printer::log("Could not create DirectInput device", ELL_WARNING);
			return;
		}

		if (FAILED(activeJoystick.lpdijoy->SetCooperativeLevel(Device->HWnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE)))
		{
			os::Printer::log("Could not set DirectInput device cooperative level", ELL_WARNING);
			return;
		}

		if (FAILED(activeJoystick.lpdijoy->SetDataFormat(&c_dfDIJoystick2)))
		{
			os::Printer::log("Could not set DirectInput device data format", ELL_WARNING);
			return;
		}

		if (FAILED(activeJoystick.lpdijoy->Acquire()))
		{
			os::Printer::log("Could not set DirectInput cooperative level", ELL_WARNING);
			return;
		}

		DIJOYSTATE2 info;
		if (FAILED(activeJoystick.lpdijoy->GetDeviceState(sizeof(info),&info)))
		{
			os::Printer::log("Could not read DirectInput device state", ELL_WARNING);
			return;
		}

		ZeroMemory(activeJoystick.axisValid,sizeof(activeJoystick.axisValid));
		activeJoystick.axisValid[0]= (info.lX!=0) ? 1 : 0;
		activeJoystick.axisValid[1]= (info.lY!=0) ? 1 : 0;
		activeJoystick.axisValid[2]= (info.lZ!=0) ? 1 : 0;
		activeJoystick.axisValid[3]= (info.lRx!=0) ? 1 : 0;
		activeJoystick.axisValid[4]= (info.lRy!=0) ? 1 : 0;
		activeJoystick.axisValid[5]= (info.lRz!=0) ? 1 : 0;

		int caxis=0;
		for (u8 i=0; i<6; i++)
		{
			if (activeJoystick.axisValid[i])
				caxis++;
		}

		for (u8 i=0; i<(activeJoystick.devcaps.dwAxes)-caxis; i++)
		{
			if (i+caxis < 8)
				activeJoystick.axisValid[i+caxis]=1;
		}

		ActiveJoysticks.push_back(activeJoystick);
	}
#endif

void pollJoysticks()
{
#if defined _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
#ifdef _IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_
	if(0 == ActiveJoysticks.size())
		return;

	u32 joystick;
	DIJOYSTATE2 info;

	for(joystick = 0; joystick < ActiveJoysticks.size(); ++joystick)
	{
		// needs to be reset for each joystick
		// request ALL values and POV as continuous if possible

		const DIDEVCAPS & caps = ActiveJoysticks[joystick].devcaps;
		// if no POV is available don't ask for POV values

		if (!FAILED(ActiveJoysticks[joystick].lpdijoy->GetDeviceState(sizeof(info),&info)))
		{
			SEvent event;

			event.EventType = irr::EET_JOYSTICK_INPUT_EVENT;
			event.JoystickEvent.Joystick = (u8)joystick;

			event.JoystickEvent.POV = (u16)info.rgdwPOV[0];
			// set to undefined if no POV value was returned or the value
			// is out of range
			if ((caps.dwPOVs==0) || (event.JoystickEvent.POV > 35900))
				event.JoystickEvent.POV = 65535;

			for(int axis = 0; axis < SEvent::SJoystickEvent::NUMBER_OF_AXES; ++axis)
				event.JoystickEvent.Axis[axis] = 0;

			u16 dxAxis=0;
			u16 irrAxis=0;

			while (dxAxis < 6 && irrAxis <caps.dwAxes)
			{
				bool axisFound=0;
				s32 axisValue=0;

				switch (dxAxis)
				{
				case 0:
					axisValue=info.lX;
					break;
				case 1:
					axisValue=info.lY;
					break;
				case 2:
					axisValue=info.lZ;
					break;
				case 3:
					axisValue=info.lRx;
					break;
				case 4:
					axisValue=info.lRy;
					break;
				case 5:
					axisValue=info.lRz;
					break;
				case 6:
					axisValue=info.rglSlider[0];
					break;
				case 7:
					axisValue=info.rglSlider[1];
					break;
				default:
					break;
				}

				if (ActiveJoysticks[joystick].axisValid[dxAxis]>0)
					axisFound=1;

				if (axisFound)
				{
					s32 val=axisValue - 32768;

					if (val <-32767) val=-32767;
					if (val > 32767) val=32767;
					event.JoystickEvent.Axis[irrAxis]=(s16)(val);
					irrAxis++;
				}

				dxAxis++;
			}

			u32 buttons=0;
			BYTE* bytebuttons=info.rgbButtons;
			for (u16 i=0; i<32; i++)
			{
				if (bytebuttons[i] >0)
				{
					buttons |= (1 << i);
				}
			}
			event.JoystickEvent.ButtonStates = buttons;

			(void)Device->postEventFromUser(event);
		}
	}
#else
	if (0 == ActiveJoysticks.size())
		return;

	u32 joystick;
	JOYINFOEX info;

	for(joystick = 0; joystick < ActiveJoysticks.size(); ++joystick)
	{
		// needs to be reset for each joystick
		// request ALL values and POV as continuous if possible
		info.dwSize = sizeof(info);
		info.dwFlags = JOY_RETURNALL|JOY_RETURNPOVCTS;
		const JOYCAPS & caps = ActiveJoysticks[joystick].Caps;
		// if no POV is available don't ask for POV values
		if (!(caps.wCaps & JOYCAPS_HASPOV))
			info.dwFlags &= ~(JOY_RETURNPOV|JOY_RETURNPOVCTS);
		if(JOYERR_NOERROR == joyGetPosEx(ActiveJoysticks[joystick].Index, &info))
		{
			SEvent event;

			event.EventType = irr::EET_JOYSTICK_INPUT_EVENT;
			event.JoystickEvent.Joystick = (u8)joystick;

			event.JoystickEvent.POV = (u16)info.dwPOV;
			// set to undefined if no POV value was returned or the value
			// is out of range
			if (!(info.dwFlags & JOY_RETURNPOV) || (event.JoystickEvent.POV > 35900))
				event.JoystickEvent.POV = 65535;

			for(int axis = 0; axis < SEvent::SJoystickEvent::NUMBER_OF_AXES; ++axis)
				event.JoystickEvent.Axis[axis] = 0;

			event.JoystickEvent.ButtonStates = info.dwButtons;

			switch(caps.wNumAxes)
			{
			default:
			case 6:
				event.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_V] =
					(s16)((65535 * (info.dwVpos - caps.wVmin)) / (caps.wVmax - caps.wVmin) - 32768);

			case 5:
				event.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_U] =
					(s16)((65535 * (info.dwUpos - caps.wUmin)) / (caps.wUmax - caps.wUmin) - 32768);

			case 4:
				event.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_R] =
					(s16)((65535 * (info.dwRpos - caps.wRmin)) / (caps.wRmax - caps.wRmin) - 32768);

			case 3:
				event.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_Z] =
					(s16)((65535 * (info.dwZpos - caps.wZmin)) / (caps.wZmax - caps.wZmin) - 32768);

			case 2:
				event.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_Y] =
					(s16)((65535 * (info.dwYpos - caps.wYmin)) / (caps.wYmax - caps.wYmin) - 32768);

			case 1:
				event.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_X] =
					(s16)((65535 * (info.dwXpos - caps.wXmin)) / (caps.wXmax - caps.wXmin) - 32768);
			}

			(void)Device->postEventFromUser(event);
		}
	}
#endif
#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
}

bool activateJoysticks(core::array<SJoystickInfo> & joystickInfo)
{
#if defined _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
#ifdef _IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_
	if (!DirectInputDevice || (DirectInputDevice->EnumDevices(DI8DEVCLASS_GAMECTRL, SJoystickWin32Control::EnumJoysticks, this, DIEDFL_ATTACHEDONLY )))
	{
		os::Printer::log("Could not enum DirectInput8 controllers", ELL_WARNING);
		return false;
	}

	for(u32 joystick = 0; joystick < ActiveJoysticks.size(); ++joystick)
	{
		JoystickInfo& activeJoystick = ActiveJoysticks[joystick];
		SJoystickInfo info;
		info.Axes=activeJoystick.devcaps.dwAxes;
		info.Buttons=activeJoystick.devcaps.dwButtons;
		info.Name=activeJoystick.Name;
		info.PovHat = (activeJoystick.devcaps.dwPOVs  != 0)
				? SJoystickInfo::POV_HAT_PRESENT : SJoystickInfo::POV_HAT_ABSENT;
		joystickInfo.push_back(info);
	}
	return true;
#else
	joystickInfo.clear();
	ActiveJoysticks.clear();

	const u32 numberOfJoysticks = ::joyGetNumDevs();
	JOYINFOEX info;
	info.dwSize = sizeof(info);
	info.dwFlags = JOY_RETURNALL;

	JoystickInfo activeJoystick;
	SJoystickInfo returnInfo;

	joystickInfo.reallocate(numberOfJoysticks);
	ActiveJoysticks.reallocate(numberOfJoysticks);

	u32 joystick = 0;
	for(; joystick < numberOfJoysticks; ++joystick)
	{
		if(JOYERR_NOERROR == joyGetPosEx(joystick, &info)
			&&
			JOYERR_NOERROR == joyGetDevCaps(joystick,
											&activeJoystick.Caps,
											sizeof(activeJoystick.Caps)))
		{
			activeJoystick.Index = joystick;
			ActiveJoysticks.push_back(activeJoystick);

			returnInfo.Joystick = (u8)joystick;
			returnInfo.Axes = activeJoystick.Caps.wNumAxes;
			returnInfo.Buttons = activeJoystick.Caps.wNumButtons;
			returnInfo.Name = activeJoystick.Caps.szPname;
			returnInfo.PovHat = ((activeJoystick.Caps.wCaps & JOYCAPS_HASPOV) == JOYCAPS_HASPOV)
								? SJoystickInfo::POV_HAT_PRESENT : SJoystickInfo::POV_HAT_ABSENT;

			joystickInfo.push_back(returnInfo);
		}
	}

	for(joystick = 0; joystick < joystickInfo.size(); ++joystick)
	{
		char logString[256];
		(void)sprintf(logString, "Found joystick %d, %d axes, %d buttons '%s'",
			joystick, joystickInfo[joystick].Axes,
			joystickInfo[joystick].Buttons, joystickInfo[joystick].Name.c_str());
		os::Printer::log(logString, ELL_INFORMATION);
	}

	return true;
#endif
#else
	return false;
#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
}
};
} // end namespace irr

// Get the codepage from the locale language id
// Based on the table from http://www.science.co.il/Language/Locale-Codes.asp?s=decimal
static unsigned int LocaleIdToCodepage(unsigned int lcid)
{
	switch ( lcid )
	{
		case 1098:  // Telugu
		case 1095:  // Gujarati
		case 1094:  // Punjabi
		case 1103:  // Sanskrit
		case 1111:  // Konkani
		case 1114:  // Syriac
		case 1099:  // Kannada
		case 1102:  // Marathi
		case 1125:  // Divehi
		case 1067:  // Armenian
		case 1081:  // Hindi
		case 1079:  // Georgian
		case 1097:  // Tamil
			return 0;
		case 1054:  // Thai
			return 874;
		case 1041:  // Japanese
			return 932;
		case 2052:  // Chinese (PRC)
		case 4100:  // Chinese (Singapore)
			return 936;
		case 1042:  // Korean
			return 949;
		case 5124:  // Chinese (Macau S.A.R.)
		case 3076:  // Chinese (Hong Kong S.A.R.)
		case 1028:  // Chinese (Taiwan)
			return 950;
		case 1048:  // Romanian
		case 1060:  // Slovenian
		case 1038:  // Hungarian
		case 1051:  // Slovak
		case 1045:  // Polish
		case 1052:  // Albanian
		case 2074:  // Serbian (Latin)
		case 1050:  // Croatian
		case 1029:  // Czech
			return 1250;
		case 1104:  // Mongolian (Cyrillic)
		case 1071:  // FYRO Macedonian
		case 2115:  // Uzbek (Cyrillic)
		case 1058:  // Ukrainian
		case 2092:  // Azeri (Cyrillic)
		case 1092:  // Tatar
		case 1087:  // Kazakh
		case 1059:  // Belarusian
		case 1088:  // Kyrgyz (Cyrillic)
		case 1026:  // Bulgarian
		case 3098:  // Serbian (Cyrillic)
		case 1049:  // Russian
			return 1251;
		case 8201:  // English (Jamaica)
		case 3084:  // French (Canada)
		case 1036:  // French (France)
		case 5132:  // French (Luxembourg)
		case 5129:  // English (New Zealand)
		case 6153:  // English (Ireland)
		case 1043:  // Dutch (Netherlands)
		case 9225:  // English (Caribbean)
		case 4108:  // French (Switzerland)
		case 4105:  // English (Canada)
		case 1110:  // Galician
		case 10249:  // English (Belize)
		case 3079:  // German (Austria)
		case 6156:  // French (Monaco)
		case 12297:  // English (Zimbabwe)
		case 1069:  // Basque
		case 2067:  // Dutch (Belgium)
		case 2060:  // French (Belgium)
		case 1035:  // Finnish
		case 1080:  // Faroese
		case 1031:  // German (Germany)
		case 3081:  // English (Australia)
		case 1033:  // English (United States)
		case 2057:  // English (United Kingdom)
		case 1027:  // Catalan
		case 11273:  // English (Trinidad)
		case 7177:  // English (South Africa)
		case 1030:  // Danish
		case 13321:  // English (Philippines)
		case 15370:  // Spanish (Paraguay)
		case 9226:  // Spanish (Colombia)
		case 5130:  // Spanish (Costa Rica)
		case 7178:  // Spanish (Dominican Republic)
		case 12298:  // Spanish (Ecuador)
		case 17418:  // Spanish (El Salvador)
		case 4106:  // Spanish (Guatemala)
		case 18442:  // Spanish (Honduras)
		case 3082:  // Spanish (International Sort)
		case 13322:  // Spanish (Chile)
		case 19466:  // Spanish (Nicaragua)
		case 2058:  // Spanish (Mexico)
		case 10250:  // Spanish (Peru)
		case 20490:  // Spanish (Puerto Rico)
		case 1034:  // Spanish (Traditional Sort)
		case 14346:  // Spanish (Uruguay)
		case 8202:  // Spanish (Venezuela)
		case 1089:  // Swahili
		case 1053:  // Swedish
		case 2077:  // Swedish (Finland)
		case 5127:  // German (Liechtenstein)
		case 1078:  // Afrikaans
		case 6154:  // Spanish (Panama)
		case 4103:  // German (Luxembourg)
		case 16394:  // Spanish (Bolivia)
		case 2055:  // German (Switzerland)
		case 1039:  // Icelandic
		case 1057:  // Indonesian
		case 1040:  // Italian (Italy)
		case 2064:  // Italian (Switzerland)
		case 2068:  // Norwegian (Nynorsk)
		case 11274:  // Spanish (Argentina)
		case 1046:  // Portuguese (Brazil)
		case 1044:  // Norwegian (Bokmal)
		case 1086:  // Malay (Malaysia)
		case 2110:  // Malay (Brunei Darussalam)
		case 2070:  // Portuguese (Portugal)
			return 1252;
		case 1032:  // Greek
			return 1253;
		case 1091:  // Uzbek (Latin)
		case 1068:  // Azeri (Latin)
		case 1055:  // Turkish
			return 1254;
		case 1037:  // Hebrew
			return 1255;
		case 5121:  // Arabic (Algeria)
		case 15361:  // Arabic (Bahrain)
		case 9217:  // Arabic (Yemen)
		case 3073:  // Arabic (Egypt)
		case 2049:  // Arabic (Iraq)
		case 11265:  // Arabic (Jordan)
		case 13313:  // Arabic (Kuwait)
		case 12289:  // Arabic (Lebanon)
		case 4097:  // Arabic (Libya)
		case 6145:  // Arabic (Morocco)
		case 8193:  // Arabic (Oman)
		case 16385:  // Arabic (Qatar)
		case 1025:  // Arabic (Saudi Arabia)
		case 10241:  // Arabic (Syria)
		case 14337:  // Arabic (U.A.E.)
		case 1065:  // Farsi
		case 1056:  // Urdu
		case 7169:  // Arabic (Tunisia)
			return 1256;
		case 1061:  // Estonian
		case 1062:  // Latvian
		case 1063:  // Lithuanian
			return 1257;
		case 1066:  // Vietnamese
			return 1258;
	}
	return 65001;   // utf-8
}

namespace
{
	struct SEnvMapper
	{
		HWND hWnd;
		irr::CIrrDeviceWin32* irrDev;
	};
	irr::core::list<SEnvMapper> EnvMap;

	HKL KEYBOARD_INPUT_HKL=0;
	unsigned int KEYBOARD_INPUT_CODEPAGE = 1252;
}

SEnvMapper* getEnvMapperFromHWnd(HWND hWnd)
{
	irr::core::list<SEnvMapper>::Iterator it = EnvMap.begin();
	for (; it!= EnvMap.end(); ++it)
		if ((*it).hWnd == hWnd)
			return &(*it);

	return 0;
}


irr::CIrrDeviceWin32* getDeviceFromHWnd(HWND hWnd)
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

	irr::CIrrDeviceWin32* dev = 0;
	irr::SEvent event;

	static irr::s32 ClickCount=0;
	if (GetCapture() != hWnd && ClickCount > 0)
		ClickCount = 0;


	struct messageMap
	{
		irr::s32 group;
		UINT winMessage;
		irr::s32 irrMessage;
	};

	static messageMap mouseMap[] =
	{
		{0, WM_LBUTTONDOWN, irr::EMIE_LMOUSE_PRESSED_DOWN},
		{1, WM_LBUTTONUP,   irr::EMIE_LMOUSE_LEFT_UP},
		{0, WM_RBUTTONDOWN, irr::EMIE_RMOUSE_PRESSED_DOWN},
		{1, WM_RBUTTONUP,   irr::EMIE_RMOUSE_LEFT_UP},
		{0, WM_MBUTTONDOWN, irr::EMIE_MMOUSE_PRESSED_DOWN},
		{1, WM_MBUTTONUP,   irr::EMIE_MMOUSE_LEFT_UP},
		{2, WM_MOUSEMOVE,   irr::EMIE_MOUSE_MOVED},
		{3, WM_MOUSEWHEEL,  irr::EMIE_MOUSE_WHEEL},
		{-1, 0, 0}
	};

	// handle grouped events
	messageMap * m = mouseMap;
	while ( m->group >=0 && m->winMessage != message )
		m += 1;

	if ( m->group >= 0 )
	{
		if ( m->group == 0 )	// down
		{
			ClickCount++;
			SetCapture(hWnd);
		}
		else
		if ( m->group == 1 )	// up
		{
			ClickCount--;
			if (ClickCount<1)
			{
				ClickCount=0;
				ReleaseCapture();
			}
		}

		event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = (irr::EMOUSE_INPUT_EVENT) m->irrMessage;
		event.MouseInput.X = (short)LOWORD(lParam);
		event.MouseInput.Y = (short)HIWORD(lParam);
		event.MouseInput.Shift = ((LOWORD(wParam) & MK_SHIFT) != 0);
		event.MouseInput.Control = ((LOWORD(wParam) & MK_CONTROL) != 0);
		// left and right mouse buttons
		event.MouseInput.ButtonStates = wParam & ( MK_LBUTTON | MK_RBUTTON);
		// middle and extra buttons
		if (wParam & MK_MBUTTON)
			event.MouseInput.ButtonStates |= irr::EMBSM_MIDDLE;
#if(_WIN32_WINNT >= 0x0500)
		if (wParam & MK_XBUTTON1)
			event.MouseInput.ButtonStates |= irr::EMBSM_EXTRA1;
		if (wParam & MK_XBUTTON2)
			event.MouseInput.ButtonStates |= irr::EMBSM_EXTRA2;
#endif
		event.MouseInput.Wheel = 0.f;

		// wheel
		if ( m->group == 3 )
		{
			POINT p; // fixed by jox
			p.x = 0; p.y = 0;
			ClientToScreen(hWnd, &p);
			event.MouseInput.X -= p.x;
			event.MouseInput.Y -= p.y;
			event.MouseInput.Wheel = ((irr::f32)((short)HIWORD(wParam))) / (irr::f32)WHEEL_DELTA;
		}

		dev = getDeviceFromHWnd(hWnd);
		if (dev)
		{
			dev->postEventFromUser(event);

			if ( event.MouseInput.Event >= irr::EMIE_LMOUSE_PRESSED_DOWN && event.MouseInput.Event <= irr::EMIE_MMOUSE_PRESSED_DOWN )
			{
				irr::u32 clicks = dev->checkSuccessiveClicks(event.MouseInput.X, event.MouseInput.Y, event.MouseInput.Event);
				if ( clicks == 2 )
				{
					event.MouseInput.Event = (irr::EMOUSE_INPUT_EVENT)(irr::EMIE_LMOUSE_DOUBLE_CLICK + event.MouseInput.Event-irr::EMIE_LMOUSE_PRESSED_DOWN);
					dev->postEventFromUser(event);
				}
				else if ( clicks == 3 )
				{
					event.MouseInput.Event = (irr::EMOUSE_INPUT_EVENT)(irr::EMIE_LMOUSE_TRIPLE_CLICK + event.MouseInput.Event-irr::EMIE_LMOUSE_PRESSED_DOWN);
					dev->postEventFromUser(event);
				}
			}
		}
		return 0;
	}

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

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
		{
			BYTE allKeys[256];

			event.EventType = irr::EET_KEY_INPUT_EVENT;
			event.KeyInput.Key = (irr::EKEY_CODE)wParam;
			event.KeyInput.PressedDown = (message==WM_KEYDOWN || message == WM_SYSKEYDOWN);

			const UINT MY_MAPVK_VSC_TO_VK_EX = 3; // MAPVK_VSC_TO_VK_EX should be in SDK according to MSDN, but isn't in mine.
			if ( event.KeyInput.Key == irr::KEY_SHIFT )
			{
				// this will fail on systems before windows NT/2000/XP, not sure _what_ will return there instead.
				event.KeyInput.Key = (irr::EKEY_CODE)MapVirtualKey( ((lParam>>16) & 255), MY_MAPVK_VSC_TO_VK_EX );
			}
			if ( event.KeyInput.Key == irr::KEY_CONTROL )
			{
				event.KeyInput.Key = (irr::EKEY_CODE)MapVirtualKey( ((lParam>>16) & 255), MY_MAPVK_VSC_TO_VK_EX );
				// some keyboards will just return LEFT for both - left and right keys. So also check extend bit.
				if (lParam & 0x1000000)
					event.KeyInput.Key = irr::KEY_RCONTROL;
			}
			if ( event.KeyInput.Key == irr::KEY_MENU )
			{
				event.KeyInput.Key = (irr::EKEY_CODE)MapVirtualKey( ((lParam>>16) & 255), MY_MAPVK_VSC_TO_VK_EX );
				if (lParam & 0x1000000)
					event.KeyInput.Key = irr::KEY_RMENU;
			}

			GetKeyboardState(allKeys);

			event.KeyInput.Shift = ((allKeys[VK_SHIFT] & 0x80)!=0);
			event.KeyInput.Control = ((allKeys[VK_CONTROL] & 0x80)!=0);

			// Handle unicode and deadkeys in a way that works since Windows 95 and nt4.0
			// Using ToUnicode instead would be shorter, but would to my knowledge not run on 95 and 98.
			WORD keyChars[2];
			UINT scanCode = HIWORD(lParam);
			int conversionResult = ToAsciiEx(wParam,scanCode,allKeys,keyChars,0,KEYBOARD_INPUT_HKL);
			if (conversionResult == 1)
			{
				WORD unicodeChar;
				MultiByteToWideChar(
						KEYBOARD_INPUT_CODEPAGE,
						MB_PRECOMPOSED, // default
						(LPCSTR)keyChars,
						sizeof(keyChars),
						(WCHAR*)&unicodeChar,
						1 );
				event.KeyInput.Char = unicodeChar;
			}
			else
				event.KeyInput.Char = 0;

			// allow composing characters like '@' with Alt Gr on non-US keyboards
			if ((allKeys[VK_MENU] & 0x80) != 0)
				event.KeyInput.Control = 0;

			dev = getDeviceFromHWnd(hWnd);
			if (dev)
				dev->postEventFromUser(event);

			if (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
				return DefWindowProc(hWnd, message, wParam, lParam);
			else
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

	case WM_SYSCOMMAND:
		// prevent screensaver or monitor powersave mode from starting
		if ((wParam & 0xFFF0) == SC_SCREENSAVE ||
			(wParam & 0xFFF0) == SC_MONITORPOWER ||
			(wParam & 0xFFF0) == SC_KEYMENU
			)
			return 0;

		break;

	case WM_ACTIVATE:
		// we need to take care for screen changes, e.g. Alt-Tab
		dev = getDeviceFromHWnd(hWnd);
		if (dev && dev->isFullscreen())
		{
			if ((wParam&0xFF)==WA_INACTIVE)
			{
				// If losing focus we minimize the app to show other one
				ShowWindow(hWnd,SW_MINIMIZE);
				// and switch back to default resolution
				dev->switchToFullScreen(true);
			}
			else
			{
				// Otherwise we retore the fullscreen Irrlicht app
				SetForegroundWindow(hWnd);
				ShowWindow(hWnd, SW_RESTORE);
				// and set the fullscreen resolution again
				dev->switchToFullScreen();
			}
		}
		break;

	case WM_USER:
		event.EventType = irr::EET_USER_EVENT;
		event.UserEvent.UserData1 = (irr::s32)wParam;
		event.UserEvent.UserData2 = (irr::s32)lParam;
		dev = getDeviceFromHWnd(hWnd);

		if (dev)
			dev->postEventFromUser(event);

		return 0;

	case WM_SETCURSOR:
		// because Windows forgot about that in the meantime
		dev = getDeviceFromHWnd(hWnd);
		if (dev)
		{
			dev->getCursorControl()->setActiveIcon( dev->getCursorControl()->getActiveIcon() );
			dev->getCursorControl()->setVisible( dev->getCursorControl()->isVisible() );
		}
		break;

	case WM_INPUTLANGCHANGE:
		// get the new codepage used for keyboard input
		KEYBOARD_INPUT_HKL = GetKeyboardLayout(0);
		KEYBOARD_INPUT_CODEPAGE = LocaleIdToCodepage( LOWORD(KEYBOARD_INPUT_HKL) );
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}


namespace irr
{

//! constructor
CIrrDeviceWin32::CIrrDeviceWin32(const SIrrlichtCreationParameters& params)
: CIrrDeviceStub(params), HWnd(0), ChangedToFullScreen(false), Resized(false),
	ExternalWindow(false), Win32CursorControl(0), JoyControl(0)
{
	#ifdef _DEBUG
	setDebugName("CIrrDeviceWin32");
	#endif

	// get windows version and create OS operator
	core::stringc winversion;
	getWindowsVersion(winversion);
	Operator = new COSOperator(winversion);
	os::Printer::log(winversion.c_str(), ELL_INFORMATION);

	// get handle to exe file
	HINSTANCE hInstance = GetModuleHandle(0);

	// Store original desktop mode.

	memset(&DesktopMode, 0, sizeof(DesktopMode));
	DesktopMode.dmSize = sizeof(DesktopMode);

	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DesktopMode);

	// create the window if we need to and we do not use the null device
	if (!CreationParams.WindowId && CreationParams.DriverType != video::EDT_NULL)
	{
		const fschar_t* ClassName = __TEXT("CIrrDeviceWin32");

		// Register Class
		WNDCLASSEX wcex;
		wcex.cbSize			= sizeof(WNDCLASSEX);
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= WndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= hInstance;
		wcex.hIcon			= NULL;
		wcex.hCursor		= 0; // LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName	= 0;
		wcex.lpszClassName	= ClassName;
		wcex.hIconSm		= 0;

		// if there is an icon, load it
		wcex.hIcon = (HICON)LoadImage(hInstance, __TEXT("irrlicht.ico"), IMAGE_ICON, 0,0, LR_LOADFROMFILE);

		RegisterClassEx(&wcex);

		// calculate client size

		RECT clientSize;
		clientSize.top = 0;
		clientSize.left = 0;
		clientSize.right = CreationParams.WindowSize.Width;
		clientSize.bottom = CreationParams.WindowSize.Height;

		DWORD style = WS_POPUP;

		if (!CreationParams.Fullscreen)
			style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

		AdjustWindowRect(&clientSize, style, FALSE);

		const s32 realWidth = clientSize.right - clientSize.left;
		const s32 realHeight = clientSize.bottom - clientSize.top;

		s32 windowLeft = (GetSystemMetrics(SM_CXSCREEN) - realWidth) / 2;
		s32 windowTop = (GetSystemMetrics(SM_CYSCREEN) - realHeight) / 2;

		if ( windowLeft < 0 )
			windowLeft = 0;
		if ( windowTop < 0 )
			windowTop = 0;	// make sure window menus are in screen on creation

		if (CreationParams.Fullscreen)
		{
			windowLeft = 0;
			windowTop = 0;
		}

		// create window

		HWnd = CreateWindow( ClassName, __TEXT(""), style, windowLeft, windowTop,
					realWidth, realHeight, NULL, NULL, hInstance, NULL);
		CreationParams.WindowId = HWnd;
//		CreationParams.WindowSize.Width = realWidth;
//		CreationParams.WindowSize.Height = realHeight;

		ShowWindow(HWnd, SW_SHOWNORMAL);
		UpdateWindow(HWnd);

		// fix ugly ATI driver bugs. Thanks to ariaci
		MoveWindow(HWnd, windowLeft, windowTop, realWidth, realHeight, TRUE);

		// make sure everything gets updated to the real sizes
		Resized = true;
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

	Win32CursorControl = new CCursorControl(this, CreationParams.WindowSize, HWnd, CreationParams.Fullscreen);
	CursorControl = Win32CursorControl;
	JoyControl = new SJoystickWin32Control(this);

	// initialize doubleclicks with system values
	MouseMultiClicks.DoubleClickTime = GetDoubleClickTime();

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
	if (!ExternalWindow)
	{
		SetActiveWindow(HWnd);
		SetForegroundWindow(HWnd);
	}

	// get the codepage used for keyboard input
	KEYBOARD_INPUT_HKL = GetKeyboardLayout(0);
	KEYBOARD_INPUT_CODEPAGE = LocaleIdToCodepage( LOWORD(KEYBOARD_INPUT_HKL) );

	// inform driver about the window size etc.
	resizeIfNecessary();
}


//! destructor
CIrrDeviceWin32::~CIrrDeviceWin32()
{
	delete JoyControl;

	// unregister environment

	irr::core::list<SEnvMapper>::Iterator it = EnvMap.begin();
	for (; it!= EnvMap.end(); ++it)
	{
		if ((*it).hWnd == HWnd)
		{
			EnvMap.erase(it);
			break;
		}
	}

	switchToFullScreen(true);
}


//! create the driver
void CIrrDeviceWin32::createDriver()
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
		switchToFullScreen();

		VideoDriver = video::createOpenGLDriver(CreationParams, FileSystem, this);
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
		switchToFullScreen();

		VideoDriver = video::createSoftwareDriver(CreationParams.WindowSize, CreationParams.Fullscreen, FileSystem, this);
		#else
		os::Printer::log("Software driver was not compiled in.", ELL_ERROR);
		#endif

		break;

	case video::EDT_BURNINGSVIDEO:
		#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
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
bool CIrrDeviceWin32::run()
{
	os::Timer::tick();

	static_cast<CCursorControl*>(CursorControl)->update();

	handleSystemMessages();

	if (!Close)
		resizeIfNecessary();

	if(!Close && JoyControl)
		JoyControl->pollJoysticks();

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return !Close;
}


//! Pause the current process for the minimum time allowed only to allow other processes to execute
void CIrrDeviceWin32::yield()
{
	Sleep(1);
}

//! Pause execution and let other processes to run for a specified amount of time.
void CIrrDeviceWin32::sleep(u32 timeMs, bool pauseTimer)
{
	const bool wasStopped = Timer ? Timer->isStopped() : true;
	if (pauseTimer && !wasStopped)
		Timer->stop();

	Sleep(timeMs);

	if (pauseTimer && !wasStopped)
		Timer->start();
}


void CIrrDeviceWin32::resizeIfNecessary()
{
	if (!Resized || !getVideoDriver())
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

		getVideoDriver()->OnResize(irr::core::dimension2du((u32)r.right, (u32)r.bottom));
		getWin32CursorControl()->OnResize(getVideoDriver()->getScreenSize());
	}

	Resized = false;
}


//! sets the caption of the window
void CIrrDeviceWin32::setWindowCaption(const wchar_t* text)
{
	// We use SendMessage instead of SetText to ensure proper
	// function even in cases where the HWND was created in a different thread
	DWORD_PTR dwResult;
	SendMessageTimeoutW(HWnd, WM_SETTEXT, 0,
			reinterpret_cast<LPARAM>(text),
			SMTO_ABORTIFHUNG, 2000, &dwResult);
}


//! presents a surface in the client area
bool CIrrDeviceWin32::present(video::IImage* image, void* windowId, core::rect<s32>* src)
{
	HWND hwnd = HWnd;
	if ( windowId )
		hwnd = reinterpret_cast<HWND>(windowId);

	HDC dc = GetDC(hwnd);

	if ( dc )
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		const void* memory = (const void *)image->lock();

		BITMAPV4HEADER bi;
		ZeroMemory (&bi, sizeof(bi));
		bi.bV4Size = sizeof(BITMAPINFOHEADER);
		bi.bV4BitCount = (WORD)image->getBitsPerPixel();
		bi.bV4Planes = 1;
		bi.bV4Width = image->getDimension().Width;
		bi.bV4Height = -((s32)image->getDimension().Height);
		bi.bV4V4Compression = BI_BITFIELDS;
		bi.bV4AlphaMask = image->getAlphaMask();
		bi.bV4RedMask = image->getRedMask();
		bi.bV4GreenMask = image->getGreenMask();
		bi.bV4BlueMask = image->getBlueMask();

		if ( src )
		{
			StretchDIBits(dc, 0,0, rect.right, rect.bottom,
					src->UpperLeftCorner.X, src->UpperLeftCorner.Y,
					src->getWidth(), src->getHeight(),
					memory, (const BITMAPINFO*)(&bi), DIB_RGB_COLORS, SRCCOPY);
		}
		else
		{
			StretchDIBits(dc, 0,0, rect.right, rect.bottom,
					0, 0, image->getDimension().Width, image->getDimension().Height,
					memory, (const BITMAPINFO*)(&bi), DIB_RGB_COLORS, SRCCOPY);
		}

		image->unlock();

		ReleaseDC(hwnd, dc);
	}
	return true;
}


//! notifies the device that it should close itself
void CIrrDeviceWin32::closeDevice()
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


//! returns if window is active. if not, nothing needs to be drawn
bool CIrrDeviceWin32::isWindowActive() const
{
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return (GetActiveWindow() == HWnd);
}


//! returns if window has focus
bool CIrrDeviceWin32::isWindowFocused() const
{
	bool ret = (GetFocus() == HWnd);
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! returns if window is minimized
bool CIrrDeviceWin32::isWindowMinimized() const
{
	WINDOWPLACEMENT plc;
	plc.length=sizeof(WINDOWPLACEMENT);
	bool ret=false;
	if (GetWindowPlacement(HWnd,&plc))
		ret=(plc.showCmd & SW_SHOWMINIMIZED)!=0;
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! switches to fullscreen
bool CIrrDeviceWin32::switchToFullScreen(bool reset)
{
	if (!CreationParams.Fullscreen)
		return true;

	if (reset)
	{
		if (ChangedToFullScreen)
		{
			return (ChangeDisplaySettings(&DesktopMode,0)==DISP_CHANGE_SUCCESSFUL);
		}
		else
			return true;
	}

	// use default values from current setting

	DEVMODE dm;
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);

	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
	dm.dmPelsWidth = CreationParams.WindowSize.Width;
	dm.dmPelsHeight = CreationParams.WindowSize.Height;
	dm.dmBitsPerPel = CreationParams.Bits;
	dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

	LONG res = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
	if (res != DISP_CHANGE_SUCCESSFUL)
	{ // try again without forcing display frequency
		dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		res = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
	}

	bool ret = false;
	switch(res)
	{
	case DISP_CHANGE_SUCCESSFUL:
		ChangedToFullScreen = true;
		ret = true;
		break;
	case DISP_CHANGE_RESTART:
		os::Printer::log("Switch to fullscreen: The computer must be restarted in order for the graphics mode to work.", ELL_ERROR);
		break;
	case DISP_CHANGE_BADFLAGS:
		os::Printer::log("Switch to fullscreen: An invalid set of flags was passed in.", ELL_ERROR);
		break;
	case DISP_CHANGE_BADPARAM:
		os::Printer::log("Switch to fullscreen: An invalid parameter was passed in. This can include an invalid flag or combination of flags.", ELL_ERROR);
		break;
	case DISP_CHANGE_FAILED:
		os::Printer::log("Switch to fullscreen: The display driver failed the specified graphics mode.", ELL_ERROR);
		break;
	case DISP_CHANGE_BADMODE:
		os::Printer::log("Switch to fullscreen: The graphics mode is not supported.", ELL_ERROR);
		break;
	default:
		os::Printer::log("An unknown error occured while changing to fullscreen.", ELL_ERROR);
		break;
	}
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! returns the win32 cursor control
CIrrDeviceWin32::CCursorControl* CIrrDeviceWin32::getWin32CursorControl()
{
	return Win32CursorControl;
}


//! \return Returns a pointer to a list with all video modes supported
//! by the gfx adapter.
video::IVideoModeList* CIrrDeviceWin32::getVideoModeList()
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

typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
// Needed for old windows apis
// depending on the SDK version and compilers some defines might be available
// or not
#ifndef PRODUCT_ULTIMATE
#define PRODUCT_ULTIMATE	0x00000001
#define PRODUCT_HOME_BASIC	0x00000002
#define PRODUCT_HOME_PREMIUM	0x00000003
#define PRODUCT_ENTERPRISE	0x00000004
#define PRODUCT_HOME_BASIC_N	0x00000005
#define PRODUCT_BUSINESS	0x00000006
#define PRODUCT_STARTER		0x0000000B
#endif
#ifndef PRODUCT_ULTIMATE_N
#define PRODUCT_BUSINESS_N	0x00000010
#define PRODUCT_HOME_PREMIUM_N	0x0000001A
#define PRODUCT_ENTERPRISE_N	0x0000001B
#define PRODUCT_ULTIMATE_N	0x0000001C
#endif
#ifndef PRODUCT_STARTER_N
#define PRODUCT_STARTER_N	0x0000002F
#endif
#ifndef PRODUCT_PROFESSIONAL
#define PRODUCT_PROFESSIONAL	0x00000030
#define PRODUCT_PROFESSIONAL_N	0x00000031
#endif
#ifndef PRODUCT_ULTIMATE_E
#define PRODUCT_STARTER_E	0x00000042
#define PRODUCT_HOME_BASIC_E	0x00000043
#define PRODUCT_HOME_PREMIUM_E	0x00000044
#define PRODUCT_PROFESSIONAL_E	0x00000045
#define PRODUCT_ENTERPRISE_E	0x00000046
#define PRODUCT_ULTIMATE_E	0x00000047
#endif

void CIrrDeviceWin32::getWindowsVersion(core::stringc& out)
{
	OSVERSIONINFOEX osvi;
	PGPI pGPI;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);
	if (!bOsVersionInfoEx)
	{
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (! GetVersionEx((OSVERSIONINFO *) &osvi))
			return;
	}

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		if (osvi.dwMajorVersion <= 4)
			out.append("Microsoft Windows NT ");
		else
		if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
			out.append("Microsoft Windows 2000 ");
		else
		if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
			out.append("Microsoft Windows XP ");
		else
		if (osvi.dwMajorVersion == 6 )
		{
			if (osvi.dwMinorVersion == 0)
			{
				if (osvi.wProductType == VER_NT_WORKSTATION)
					out.append("Microsoft Windows Vista ");
				else
					out.append("Microsoft Windows Server 2008 ");
			}
			else if (osvi.dwMinorVersion == 1)
			{
				if (osvi.wProductType == VER_NT_WORKSTATION)
					out.append("Microsoft Windows 7 ");
				else
					out.append("Microsoft Windows Server 2008 R2 ");
			}
		}

		if (bOsVersionInfoEx)
		{
			if (osvi.dwMajorVersion == 6)
			{
				DWORD dwType;
				pGPI = (PGPI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");
				pGPI(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

				switch (dwType)
				{
				case PRODUCT_ULTIMATE:
				case PRODUCT_ULTIMATE_E:
				case PRODUCT_ULTIMATE_N:
					out.append("Ultimate Edition ");
					break;
				case PRODUCT_PROFESSIONAL:
				case PRODUCT_PROFESSIONAL_E:
				case PRODUCT_PROFESSIONAL_N:
					out.append("Professional Edition ");
					break;
				case PRODUCT_HOME_BASIC:
				case PRODUCT_HOME_BASIC_E:
				case PRODUCT_HOME_BASIC_N:
					out.append("Home Basic Edition ");
					break;
				case PRODUCT_HOME_PREMIUM:
				case PRODUCT_HOME_PREMIUM_E:
				case PRODUCT_HOME_PREMIUM_N:
					out.append("Home Premium Edition ");
					break;
				case PRODUCT_ENTERPRISE:
				case PRODUCT_ENTERPRISE_E:
				case PRODUCT_ENTERPRISE_N:
					out.append("Enterprise Edition ");
					break;
				case PRODUCT_BUSINESS:
				case PRODUCT_BUSINESS_N:
					out.append("Business Edition ");
					break;
				case PRODUCT_STARTER:
				case PRODUCT_STARTER_E:
				case PRODUCT_STARTER_N:
					out.append("Starter Edition ");
					break;
				}
			}
#ifdef VER_SUITE_ENTERPRISE
			else
			if (osvi.wProductType == VER_NT_WORKSTATION)
			{
#ifndef __BORLANDC__
				if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
					out.append("Personal ");
				else
					out.append("Professional ");
#endif
			}
			else if (osvi.wProductType == VER_NT_SERVER)
			{
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					out.append("DataCenter Server ");
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					out.append("Advanced Server ");
				else
					out.append("Server ");
			}
#endif
		}
		else
		{
			HKEY hKey;
			char szProductType[80];
			DWORD dwBufLen;

			RegOpenKeyEx( HKEY_LOCAL_MACHINE,
					__TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
					0, KEY_QUERY_VALUE, &hKey );
			RegQueryValueEx( hKey, __TEXT("ProductType"), NULL, NULL,
					(LPBYTE) szProductType, &dwBufLen);
			RegCloseKey( hKey );

			if (_strcmpi( "WINNT", szProductType) == 0 )
				out.append("Professional ");
			if (_strcmpi( "LANMANNT", szProductType) == 0)
				out.append("Server ");
			if (_strcmpi( "SERVERNT", szProductType) == 0)
				out.append("Advanced Server ");
		}

		// Display version, service pack (if any), and build number.

		char tmp[255];

		if (osvi.dwMajorVersion <= 4 )
		{
			sprintf(tmp, "version %ld.%ld %s (Build %ld)",
					osvi.dwMajorVersion,
					osvi.dwMinorVersion,
					irr::core::stringc(osvi.szCSDVersion).c_str(),
					osvi.dwBuildNumber & 0xFFFF);
		}
		else
		{
			sprintf(tmp, "%s (Build %ld)", irr::core::stringc(osvi.szCSDVersion).c_str(),
			osvi.dwBuildNumber & 0xFFFF);
		}

		out.append(tmp);
		break;

	case VER_PLATFORM_WIN32_WINDOWS:

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
		{
			out.append("Microsoft Windows 95 ");
			if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
				out.append("OSR2 " );
		}

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
		{
			out.append("Microsoft Windows 98 ");
			if ( osvi.szCSDVersion[1] == 'A' )
				out.append( "SE " );
		}

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
			out.append("Microsoft Windows Me ");

		break;

	case VER_PLATFORM_WIN32s:
		out.append("Microsoft Win32s ");
		break;
	}
}

//! Notifies the device, that it has been resized
void CIrrDeviceWin32::OnResized()
{
	Resized = true;
}

//! Sets if the window should be resizable in windowed mode.
void CIrrDeviceWin32::setResizable(bool resize)
{
	if (ExternalWindow || !getVideoDriver() || CreationParams.Fullscreen)
		return;

	LONG_PTR style = WS_POPUP;

	if (!resize)
		style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	else
		style = WS_THICKFRAME | WS_SYSMENU | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

	if (!SetWindowLongPtr(HWnd, GWL_STYLE, style))
		os::Printer::log("Could not change window style.");

	RECT clientSize;
	clientSize.top = 0;
	clientSize.left = 0;
	clientSize.right = getVideoDriver()->getScreenSize().Width;
	clientSize.bottom = getVideoDriver()->getScreenSize().Height;

	AdjustWindowRect(&clientSize, style, FALSE);

	const s32 realWidth = clientSize.right - clientSize.left;
	const s32 realHeight = clientSize.bottom - clientSize.top;

	const s32 windowLeft = (GetSystemMetrics(SM_CXSCREEN) - realWidth) / 2;
	const s32 windowTop = (GetSystemMetrics(SM_CYSCREEN) - realHeight) / 2;

	SetWindowPos(HWnd, HWND_TOP, windowLeft, windowTop, realWidth, realHeight,
		SWP_FRAMECHANGED | SWP_NOMOVE | SWP_SHOWWINDOW);

	static_cast<CCursorControl*>(CursorControl)->updateBorderSize(CreationParams.Fullscreen, resize);
}


//! Minimizes the window.
void CIrrDeviceWin32::minimizeWindow()
{
	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(HWnd, &wndpl);
	wndpl.showCmd = SW_SHOWMINNOACTIVE;
	SetWindowPlacement(HWnd, &wndpl);
}


//! Maximizes the window.
void CIrrDeviceWin32::maximizeWindow()
{
	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(HWnd, &wndpl);
	wndpl.showCmd = SW_SHOWMAXIMIZED;
	SetWindowPlacement(HWnd, &wndpl);
}


//! Restores the window to its original size.
void CIrrDeviceWin32::restoreWindow()
{
	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(HWnd, &wndpl);
	wndpl.showCmd = SW_SHOWNORMAL;
	SetWindowPlacement(HWnd, &wndpl);
}


bool CIrrDeviceWin32::activateJoysticks(core::array<SJoystickInfo> & joystickInfo)
{
	if (JoyControl)
		return JoyControl->activateJoysticks(joystickInfo);
	else
		return false;
}


//! Set the current Gamma Value for the Display
bool CIrrDeviceWin32::setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast )
{
	bool r;
	u16 ramp[3][256];

	calculateGammaRamp( ramp[0], red, brightness, contrast );
	calculateGammaRamp( ramp[1], green, brightness, contrast );
	calculateGammaRamp( ramp[2], blue, brightness, contrast );

	HDC dc = GetDC(0);
	r = SetDeviceGammaRamp ( dc, ramp ) == TRUE;
	ReleaseDC(HWnd, dc);
	return r;
}

//! Get the current Gamma Value for the Display
bool CIrrDeviceWin32::getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast )
{
	bool r;
	u16 ramp[3][256];

	HDC dc = GetDC(0);
	r = GetDeviceGammaRamp ( dc, ramp ) == TRUE;
	ReleaseDC(HWnd, dc);

	if ( r )
	{
		calculateGammaFromRamp(red, ramp[0]);
		calculateGammaFromRamp(green, ramp[1]);
		calculateGammaFromRamp(blue, ramp[2]);
	}

	brightness = 0.f;
	contrast = 0.f;

	return r;

}


//! Process system events
void CIrrDeviceWin32::handleSystemMessages()
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		// No message translation because we don't use WM_CHAR and it would conflict with our
		// deadkey handling.

		if (ExternalWindow && msg.hwnd == HWnd)
			WndProc(HWnd, msg.message, msg.wParam, msg.lParam);
		else
			DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
			Close = true;
	}
}


//! Remove all messages pending in the system message loop
void CIrrDeviceWin32::clearSystemMessages()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
	{}
	while (PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
	{}
}

// shows last error in a messagebox to help internal debugging.
void CIrrDeviceWin32::ReportLastWinApiError()
{
	// (based on code from ovidiucucu from http://www.codeguru.com/forum/showthread.php?t=318721)
	LPCTSTR pszCaption = __TEXT("Windows SDK Error Report");
	DWORD dwError = GetLastError();

	if(NOERROR == dwError)
	{
		MessageBox(NULL, __TEXT("No error"), pszCaption, MB_OK);
	}
	else
	{
		const DWORD dwFormatControl = FORMAT_MESSAGE_ALLOCATE_BUFFER |
										FORMAT_MESSAGE_IGNORE_INSERTS |
										FORMAT_MESSAGE_FROM_SYSTEM;

		LPVOID pTextBuffer = NULL;
		DWORD dwCount = FormatMessage(dwFormatControl,
										NULL,
										dwError,
										0,
										(LPTSTR) &pTextBuffer,
										0,
										NULL);
		if(0 != dwCount)
		{
			MessageBox(NULL, (LPCTSTR)pTextBuffer, pszCaption, MB_OK|MB_ICONERROR);
			LocalFree(pTextBuffer);
		}
		else
		{
			MessageBox(NULL, __TEXT("Unknown error"), pszCaption, MB_OK|MB_ICONERROR);
		}
	}
}

// Convert an Irrlicht texture to a Windows cursor
// Based on http://www.codeguru.com/cpp/w-p/win32/cursors/article.php/c4529/
HCURSOR CIrrDeviceWin32::TextureToCursor(HWND hwnd, irr::video::ITexture * tex, const core::rect<s32>& sourceRect, const core::position2d<s32> &hotspot)
{
	//
	// create the bitmaps needed for cursors from the texture

	HDC dc = GetDC(hwnd);
	HDC andDc = CreateCompatibleDC(dc);
	HDC xorDc = CreateCompatibleDC(dc);
	HBITMAP andBitmap = CreateCompatibleBitmap(dc, sourceRect.getWidth(), sourceRect.getHeight());
	HBITMAP xorBitmap = CreateCompatibleBitmap(dc, sourceRect.getWidth(), sourceRect.getHeight());

	HBITMAP oldAndBitmap = (HBITMAP)SelectObject(andDc, andBitmap);
	HBITMAP oldXorBitmap = (HBITMAP)SelectObject(xorDc, xorBitmap);


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
				SetPixel(andDc, x, y, RGB(255,255,255));
				SetPixel(xorDc, x, y, RGB(0,0,0));
			}
			else	// color
			{
				SetPixel(andDc, x, y, RGB(0,0,0));
				SetPixel(xorDc, x, y, RGB(pixelCol.getRed(), pixelCol.getGreen(), pixelCol.getBlue()));
			}
		}
		data += bytesRightGap;
	}
	tex->unlock();

	SelectObject(andDc, oldAndBitmap);
	SelectObject(xorDc, oldXorBitmap);

	DeleteDC(xorDc);
	DeleteDC(andDc);

	ReleaseDC(hwnd, dc);

	// create the cursor

	ICONINFO iconinfo;
	iconinfo.fIcon = false;	// type is cursor not icon
	iconinfo.xHotspot = hotspot.X;
	iconinfo.yHotspot = hotspot.Y;
	iconinfo.hbmMask = andBitmap;
	iconinfo.hbmColor = xorBitmap;

	HCURSOR cursor = CreateIconIndirect(&iconinfo);

	DeleteObject(andBitmap);
	DeleteObject(xorBitmap);

	return cursor;
}


CIrrDeviceWin32::CCursorControl::CCursorControl(CIrrDeviceWin32* device, const core::dimension2d<u32>& wsize, HWND hwnd, bool fullscreen)
	: Device(device), WindowSize(wsize), InvWindowSize(0.0f, 0.0f),
		HWnd(hwnd), BorderX(0), BorderY(0),
		UseReferenceRect(false), IsVisible(true)
		, ActiveIcon(gui::ECI_NORMAL), ActiveIconStartTime(0)
{
	if (WindowSize.Width!=0)
		InvWindowSize.Width = 1.0f / WindowSize.Width;

	if (WindowSize.Height!=0)
		InvWindowSize.Height = 1.0f / WindowSize.Height;

	updateBorderSize(fullscreen, false);
	initCursors();
}

CIrrDeviceWin32::CCursorControl::~CCursorControl()
{
	for ( u32 i=0; i < Cursors.size(); ++i )
	{
		for ( u32 f=0; f < Cursors[i].Frames.size(); ++f )
		{
			DestroyCursor(Cursors[i].Frames[f].IconHW);
		}
	}
}


void CIrrDeviceWin32::CCursorControl::initCursors()
{
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_ARROW)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_CROSS)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_HAND)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_HELP)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_IBEAM)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_NO)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_WAIT)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_SIZEALL)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_SIZENESW)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_SIZENWSE)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_SIZENS)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_SIZEWE)) );
	Cursors.push_back( CursorW32(LoadCursor(NULL, IDC_UPARROW)) );
}


void CIrrDeviceWin32::CCursorControl::update()
{
	if ( !Cursors[ActiveIcon].Frames.empty() && Cursors[ActiveIcon].FrameTime )
	{
		// update animated cursors. This could also be done by X11 in case someone wants to figure that out (this way was just easier to implement)
		u32 now = Device->getTimer()->getRealTime();
		u32 frame = ((now - ActiveIconStartTime) / Cursors[ActiveIcon].FrameTime) % Cursors[ActiveIcon].Frames.size();
		SetCursor( Cursors[ActiveIcon].Frames[frame].IconHW );
	}
}

//! Sets the active cursor icon
void CIrrDeviceWin32::CCursorControl::setActiveIcon(gui::ECURSOR_ICON iconId)
{
	if ( iconId >= (s32)Cursors.size() )
		return;

	ActiveIcon = iconId;
	ActiveIconStartTime = Device->getTimer()->getRealTime();
	if ( Cursors[ActiveIcon].Frames.size() )
		SetCursor( Cursors[ActiveIcon].Frames[0].IconHW );
}


//! Add a custom sprite as cursor icon.
gui::ECURSOR_ICON CIrrDeviceWin32::CCursorControl::addIcon(const gui::SCursorSprite& icon)
{
	if ( icon.SpriteId >= 0 )
	{
		CursorW32 cW32;
		cW32.FrameTime = icon.SpriteBank->getSprites()[icon.SpriteId].frameTime;

		for ( u32 i=0; i < icon.SpriteBank->getSprites()[icon.SpriteId].Frames.size(); ++i )
		{
			irr::u32 texId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].textureNumber;
			irr::u32 rectId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].rectNumber;
			irr::core::rect<s32> rectIcon = icon.SpriteBank->getPositions()[rectId];

			HCURSOR hc = Device->TextureToCursor(HWnd, icon.SpriteBank->getTexture(texId), rectIcon, icon.HotSpot);
			cW32.Frames.push_back( CursorFrameW32(hc) );
		}

		Cursors.push_back( cW32 );
		return (gui::ECURSOR_ICON)(Cursors.size() - 1);
	}
	return gui::ECI_NORMAL;
}


//! replace the given cursor icon.
void CIrrDeviceWin32::CCursorControl::changeIcon(gui::ECURSOR_ICON iconId, const gui::SCursorSprite& icon)
{
	if ( iconId >= (s32)Cursors.size() )
		return;

	for ( u32 i=0; i < Cursors[iconId].Frames.size(); ++i )
		DestroyCursor(Cursors[iconId].Frames[i].IconHW);

	if ( icon.SpriteId >= 0 )
	{
		CursorW32 cW32;
		cW32.FrameTime = icon.SpriteBank->getSprites()[icon.SpriteId].frameTime;
		for ( u32 i=0; i < icon.SpriteBank->getSprites()[icon.SpriteId].Frames.size(); ++i )
		{
			irr::u32 texId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].textureNumber;
			irr::u32 rectId = icon.SpriteBank->getSprites()[icon.SpriteId].Frames[i].rectNumber;
			irr::core::rect<s32> rectIcon = icon.SpriteBank->getPositions()[rectId];

			HCURSOR hc = Device->TextureToCursor(HWnd, icon.SpriteBank->getTexture(texId), rectIcon, icon.HotSpot);
			cW32.Frames.push_back( CursorFrameW32(hc) );
		}

		Cursors[iconId] = cW32;
	}
}


//! Return a system-specific size which is supported for cursors. Larger icons will fail, smaller icons might work.
core::dimension2di CIrrDeviceWin32::CCursorControl::getSupportedIconSize() const
{
	core::dimension2di result;

	result.Width = GetSystemMetrics(SM_CXCURSOR);
	result.Height = GetSystemMetrics(SM_CYCURSOR);

	return result;
}



} // end namespace

#endif // _IRR_COMPILE_WITH_WINDOWS_DEVICE_
