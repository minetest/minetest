// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CIrrDeviceStub.h"
#include "ISceneManager.h"
#include "IEventReceiver.h"
#include "IFileSystem.h"
#include "IGUIEnvironment.h"
#include "os.h"
#include "IrrCompileConfig.h"
#include "CTimer.h"
#include "CLogger.h"
#include "irrString.h"
#include "IRandomizer.h"

namespace irr
{
//! constructor
CIrrDeviceStub::CIrrDeviceStub(const SIrrlichtCreationParameters& params)
: IrrlichtDevice(), VideoDriver(0), GUIEnvironment(0), SceneManager(0),
	Timer(0), CursorControl(0), UserReceiver(params.EventReceiver),
	Logger(0), Operator(0), Randomizer(0), FileSystem(0),
	InputReceivingSceneManager(0), VideoModeList(0),
	CreationParams(params), Close(false)
{
	Timer = new CTimer(params.UsePerformanceTimer);
	if (os::Printer::Logger)
	{
		os::Printer::Logger->grab();
		Logger = (CLogger*)os::Printer::Logger;
		Logger->setReceiver(UserReceiver);
	}
	else
	{
		Logger = new CLogger(UserReceiver);
		os::Printer::Logger = Logger;
	}
	Logger->setLogLevel(CreationParams.LoggingLevel);

	os::Printer::Logger = Logger;
	Randomizer = createDefaultRandomizer();

	FileSystem = io::createFileSystem();
	VideoModeList = new video::CVideoModeList();

	core::stringc s = "Irrlicht Engine version ";
	s.append(getVersion());
	os::Printer::log(s.c_str(), ELL_INFORMATION);

	checkVersion(params.SDK_version_do_not_use);
}


CIrrDeviceStub::~CIrrDeviceStub()
{
	VideoModeList->drop();
	FileSystem->drop();

	if (GUIEnvironment)
		GUIEnvironment->drop();

	if (VideoDriver)
		VideoDriver->drop();

	if (SceneManager)
		SceneManager->drop();

	if (InputReceivingSceneManager)
		InputReceivingSceneManager->drop();

	if (CursorControl)
		CursorControl->drop();

	if (Operator)
		Operator->drop();

	if (Randomizer)
		Randomizer->drop();

	CursorControl = 0;

	if (Timer)
		Timer->drop();

	if (Logger->drop())
		os::Printer::Logger = 0;
}


void CIrrDeviceStub::createGUIAndScene()
{
	#ifdef _IRR_COMPILE_WITH_GUI_
	// create gui environment
	GUIEnvironment = gui::createGUIEnvironment(FileSystem, VideoDriver, Operator);
	#endif

	// create Scene manager
	SceneManager = scene::createSceneManager(VideoDriver, FileSystem, CursorControl, GUIEnvironment);

	setEventReceiver(UserReceiver);
}


//! returns the video driver
video::IVideoDriver* CIrrDeviceStub::getVideoDriver()
{
	return VideoDriver;
}



//! return file system
io::IFileSystem* CIrrDeviceStub::getFileSystem()
{
	return FileSystem;
}



//! returns the gui environment
gui::IGUIEnvironment* CIrrDeviceStub::getGUIEnvironment()
{
	return GUIEnvironment;
}



//! returns the scene manager
scene::ISceneManager* CIrrDeviceStub::getSceneManager()
{
	return SceneManager;
}


//! \return Returns a pointer to the ITimer object. With it the
//! current Time can be received.
ITimer* CIrrDeviceStub::getTimer()
{
	return Timer;
}


//! Returns the version of the engine.
const char* CIrrDeviceStub::getVersion() const
{
	return IRRLICHT_SDK_VERSION;
}

//! \return Returns a pointer to the mouse cursor control interface.
gui::ICursorControl* CIrrDeviceStub::getCursorControl()
{
	return CursorControl;
}


//! \return Returns a pointer to a list with all video modes supported
//! by the gfx adapter.
video::IVideoModeList* CIrrDeviceStub::getVideoModeList()
{
	return VideoModeList;
}


//! checks version of sdk and prints warning if there might be a problem
bool CIrrDeviceStub::checkVersion(const char* version)
{
	if (strcmp(getVersion(), version))
	{
		core::stringc w;
		w = "Warning: The library version of the Irrlicht Engine (";
		w += getVersion();
		w += ") does not match the version the application was compiled with (";
		w += version;
		w += "). This may cause problems.";
		os::Printer::log(w.c_str(), ELL_WARNING);
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}

	return true;
}


//! Compares to the last call of this function to return double and triple clicks.
u32 CIrrDeviceStub::checkSuccessiveClicks(s32 mouseX, s32 mouseY, EMOUSE_INPUT_EVENT inputEvent )
{
	const s32 MAX_MOUSEMOVE = 3;

	irr::u32 clickTime = getTimer()->getRealTime();

	if ( (clickTime-MouseMultiClicks.LastClickTime) < MouseMultiClicks.DoubleClickTime
		&& core::abs_(MouseMultiClicks.LastClick.X - mouseX ) <= MAX_MOUSEMOVE
		&& core::abs_(MouseMultiClicks.LastClick.Y - mouseY ) <= MAX_MOUSEMOVE
		&& MouseMultiClicks.CountSuccessiveClicks < 3
		&& MouseMultiClicks.LastMouseInputEvent == inputEvent
	   )
	{
		++MouseMultiClicks.CountSuccessiveClicks;
	}
	else
	{
		MouseMultiClicks.CountSuccessiveClicks = 1;
	}

	MouseMultiClicks.LastMouseInputEvent = inputEvent;
	MouseMultiClicks.LastClickTime = clickTime;
	MouseMultiClicks.LastClick.X = mouseX;
	MouseMultiClicks.LastClick.Y = mouseY;

	return MouseMultiClicks.CountSuccessiveClicks;
}


//! send the event to the right receiver
bool CIrrDeviceStub::postEventFromUser(const SEvent& event)
{
	bool absorbed = false;

	if (UserReceiver)
		absorbed = UserReceiver->OnEvent(event);

	if (!absorbed && GUIEnvironment)
		absorbed = GUIEnvironment->postEventFromUser(event);

	scene::ISceneManager* inputReceiver = InputReceivingSceneManager;
	if (!inputReceiver)
		inputReceiver = SceneManager;

	if (!absorbed && inputReceiver)
		absorbed = inputReceiver->postEventFromUser(event);

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return absorbed;
}


//! Sets a new event receiver to receive events
void CIrrDeviceStub::setEventReceiver(IEventReceiver* receiver)
{
	UserReceiver = receiver;
	Logger->setReceiver(receiver);
	if (GUIEnvironment)
		GUIEnvironment->setUserEventReceiver(receiver);
}


//! Returns poinhter to the current event receiver. Returns 0 if there is none.
IEventReceiver* CIrrDeviceStub::getEventReceiver()
{
	return UserReceiver;
}


//! \return Returns a pointer to the logger.
ILogger* CIrrDeviceStub::getLogger()
{
	return Logger;
}


//! Returns the operation system opertator object.
IOSOperator* CIrrDeviceStub::getOSOperator()
{
	return Operator;
}


//! Provides access to the engine's currently set randomizer.
IRandomizer* CIrrDeviceStub::getRandomizer() const
{
	return Randomizer;
}

//! Sets a new randomizer.
void CIrrDeviceStub::setRandomizer(IRandomizer* r)
{
	if (r!=Randomizer)
	{
		if (Randomizer)
			Randomizer->drop();
		Randomizer=r;
		if (Randomizer)
			Randomizer->grab();
	}
}

namespace
{
	struct SDefaultRandomizer : public IRandomizer
	{
		virtual void reset(s32 value=0x0f0f0f0f)
		{
			os::Randomizer::reset(value);
		}

		virtual s32 rand() const
		{
			return os::Randomizer::rand();
		}

		virtual f32 frand() const
		{
			return os::Randomizer::frand();
		}

		virtual s32 randMax() const
		{
			return os::Randomizer::randMax();
		}
	};
}

//! Creates a new default randomizer.
IRandomizer* CIrrDeviceStub::createDefaultRandomizer() const
{
	IRandomizer* r = new SDefaultRandomizer();
	if (r)
		r->reset();
	return r;
}


//! Sets the input receiving scene manager.
void CIrrDeviceStub::setInputReceivingSceneManager(scene::ISceneManager* sceneManager)
{
	if (sceneManager)
		sceneManager->grab();
	if (InputReceivingSceneManager)
		InputReceivingSceneManager->drop();

	InputReceivingSceneManager = sceneManager;
}


//! Checks if the window is running in fullscreen mode
bool CIrrDeviceStub::isFullscreen() const
{
	return CreationParams.Fullscreen;
}


//! returns color format
video::ECOLOR_FORMAT CIrrDeviceStub::getColorFormat() const
{
	return video::ECF_R5G6B5;
}

//! No-op in this implementation
bool CIrrDeviceStub::activateJoysticks(core::array<SJoystickInfo> & joystickInfo)
{
	return false;
}

/*!
*/
void CIrrDeviceStub::calculateGammaRamp ( u16 *ramp, f32 gamma, f32 relativebrightness, f32 relativecontrast )
{
	s32 i;
	s32 value;
	s32 rbright = (s32) ( relativebrightness * (65535.f / 4 ) );
	f32 rcontrast = 1.f / (255.f - ( relativecontrast * 127.5f ) );

	gamma = gamma > 0.f ? 1.0f / gamma : 0.f;

	for ( i = 0; i < 256; ++i )
	{
		value = (s32)(pow( rcontrast * i, gamma)*65535.f + 0.5f );
		ramp[i] = (u16) core::s32_clamp ( value + rbright, 0, 65535 );
	}

}

void CIrrDeviceStub::calculateGammaFromRamp ( f32 &gamma, const u16 *ramp )
{
	/* The following is adapted from a post by Garrett Bass on OpenGL
	Gamedev list, March 4, 2000.
	*/
	f32 sum = 0.0;
	s32 i, count = 0;

	gamma = 1.0;
	for ( i = 1; i < 256; ++i ) {
		if ( (ramp[i] != 0) && (ramp[i] != 65535) ) {
			f32 B = (f32)i / 256.f;
			f32 A = ramp[i] / 65535.f;
			sum += (f32) ( logf(A) / logf(B) );
			count++;
		}
	}
	if ( count && sum ) {
		gamma = 1.0f / (sum / count);
	}

}

//! Set the current Gamma Value for the Display
bool CIrrDeviceStub::setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast )
{
	return false;
}

//! Get the current Gamma Value for the Display
bool CIrrDeviceStub::getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast )
{
	return false;
}

//! Set the maximal elapsed time between 2 clicks to generate doubleclicks for the mouse. It also affects tripleclick behavior.
void CIrrDeviceStub::setDoubleClickTime( u32 timeMs )
{
	MouseMultiClicks.DoubleClickTime = timeMs;
}

//! Get the maximal elapsed time between 2 clicks to generate double- and tripleclicks for the mouse.
u32 CIrrDeviceStub::getDoubleClickTime() const
{
	return MouseMultiClicks.DoubleClickTime;
}

//! Remove all messages pending in the system message loop
void CIrrDeviceStub::clearSystemMessages()
{
}



} // end namespace irr

