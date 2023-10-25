// Copyright (C) 2002-2007 Nikolaus Gebhardt
// Copyright (C) 2007-2011 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CIrrDeviceAndroid.h"

#ifdef _IRR_COMPILE_WITH_ANDROID_DEVICE_

#include "os.h"
#include "CFileSystem.h"
#include "CAndroidAssetReader.h"
#include "CAndroidAssetFileArchive.h"
#include "CKeyEventWrapper.h"
#include "CEGLManager.h"
#include "ISceneManager.h"
#include "IGUIEnvironment.h"
#include "CEGLManager.h"

namespace irr
{
	namespace video
	{
		IVideoDriver* createOGLES1Driver(const SIrrlichtCreationParameters& params,
			io::IFileSystem* io, video::IContextManager* contextManager);

		IVideoDriver* createOGLES2Driver(const SIrrlichtCreationParameters& params,
			io::IFileSystem* io, video::IContextManager* contextManager);
	}
}

namespace irr
{

CIrrDeviceAndroid::CIrrDeviceAndroid(const SIrrlichtCreationParameters& param)
	: CIrrDeviceStub(param), Accelerometer(0), Gyroscope(0), Focused(false), Initialized(false), Paused(true), JNIEnvAttachedToVM(0)
{
#ifdef _DEBUG
	setDebugName("CIrrDeviceAndroid");
#endif

	// Get the interface to the native Android activity.
	Android = (android_app*)(param.PrivateData);

	// Set the private data so we can use it in any static callbacks.
	Android->userData = this;

	// Set the default command handler. This is a callback function that the Android
	// OS invokes to send the native activity messages.
	Android->onAppCmd = handleAndroidCommand;

	createKeyMap();

	// Create a sensor manager to receive touch screen events from the java activity.
	SensorManager = ASensorManager_getInstance();
	SensorEventQueue = ASensorManager_createEventQueue(SensorManager, Android->looper, LOOPER_ID_USER, 0, 0);
	Android->onInputEvent = handleInput;

	// Create EGL manager.
	ContextManager = new video::CEGLManager();

	os::Printer::log("Waiting for Android activity window to be created.", ELL_DEBUG);

	do
	{
		s32 Events = 0;
		android_poll_source* Source = 0;

		while ((ALooper_pollAll(((Focused && !Paused) || !Initialized) ? 0 : -1, 0, &Events, (void**)&Source)) >= 0)
		{
			if(Source)
				Source->process(Android, Source);
		}
	}
	while(!Initialized);
}


CIrrDeviceAndroid::~CIrrDeviceAndroid()
{
	if (GUIEnvironment)
	{
		GUIEnvironment->drop();
		GUIEnvironment = 0;
	}

	if (SceneManager)
	{
		SceneManager->drop();
		SceneManager = 0;
	}

	if (VideoDriver)
	{
		VideoDriver->drop();
		VideoDriver = 0;
	}
}

bool CIrrDeviceAndroid::run()
{
	if (!Initialized)
		return false;

	os::Timer::tick();

	s32 id;
	s32 Events = 0;
	android_poll_source* Source = 0;

	while ((id = ALooper_pollAll(((Focused && !Paused) || !Initialized) ? 0 : -1, 0, &Events, (void**)&Source)) >= 0)
	{
		if(Source)
			Source->process(Android, Source);

		// if a sensor has data, we'll process it now.
        if (id == LOOPER_ID_USER)
		{
			ASensorEvent sensorEvent;
			while (ASensorEventQueue_getEvents(SensorEventQueue, &sensorEvent, 1) > 0)
			{
				switch (sensorEvent.type)
				{
					case ASENSOR_TYPE_ACCELEROMETER:
						SEvent accEvent;
						accEvent.EventType = EET_ACCELEROMETER_EVENT;
						accEvent.AccelerometerEvent.X = sensorEvent.acceleration.x;
						accEvent.AccelerometerEvent.Y = sensorEvent.acceleration.y;
						accEvent.AccelerometerEvent.Z = sensorEvent.acceleration.z;

						postEventFromUser(accEvent);
					break;

					case ASENSOR_TYPE_GYROSCOPE:
						SEvent gyroEvent;
						gyroEvent.EventType = EET_GYROSCOPE_EVENT;
						gyroEvent.GyroscopeEvent.X = sensorEvent.vector.x;
						gyroEvent.GyroscopeEvent.Y = sensorEvent.vector.y;
						gyroEvent.GyroscopeEvent.Z = sensorEvent.vector.z;

						postEventFromUser(gyroEvent);
					break;
					default:
					break;
				}
			}
		}

		if(!Initialized)
			break;
	}

	return Initialized;
}

void CIrrDeviceAndroid::yield()
{
	struct timespec ts = {0,1};
	nanosleep(&ts, NULL);
}

void CIrrDeviceAndroid::sleep(u32 timeMs, bool pauseTimer)
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

void CIrrDeviceAndroid::setWindowCaption(const wchar_t* text)
{
}

bool CIrrDeviceAndroid::isWindowActive() const
{
	return (Focused && !Paused);
}

bool CIrrDeviceAndroid::isWindowFocused() const
{
	return Focused;
}

bool CIrrDeviceAndroid::isWindowMinimized() const
{
	return !Focused;
}

void CIrrDeviceAndroid::closeDevice()
{
	ANativeActivity_finish(Android->activity);
}

void CIrrDeviceAndroid::setResizable(bool resize)
{
}

void CIrrDeviceAndroid::minimizeWindow()
{
}

void CIrrDeviceAndroid::maximizeWindow()
{
}

void CIrrDeviceAndroid::restoreWindow()
{
}

core::position2di CIrrDeviceAndroid::getWindowPosition()
{
	return core::position2di(0, 0);
}

E_DEVICE_TYPE CIrrDeviceAndroid::getType() const
{
	return EIDT_ANDROID;
}

void CIrrDeviceAndroid::handleAndroidCommand(android_app* app, int32_t cmd)
{
	CIrrDeviceAndroid* device = (CIrrDeviceAndroid*)app->userData;

	SEvent event;
	event.EventType = EET_SYSTEM_EVENT;
	event.SystemEvent.EventType = ESET_ANDROID_CMD;
	event.SystemEvent.AndroidCmd.Cmd = cmd;
	if ( device->postEventFromUser(event) )
		return;

	switch (cmd)
	{
		case APP_CMD_INPUT_CHANGED:
			os::Printer::log("Android command APP_CMD_INPUT_CHANGED", ELL_DEBUG);
		break;
		case APP_CMD_WINDOW_RESIZED:
			os::Printer::log("Android command APP_CMD_WINDOW_RESIZED", ELL_DEBUG);
		break;
		case APP_CMD_WINDOW_REDRAW_NEEDED:
			os::Printer::log("Android command APP_CMD_WINDOW_REDRAW_NEEDED", ELL_DEBUG);
		break;
		case APP_CMD_SAVE_STATE:
			os::Printer::log("Android command APP_CMD_SAVE_STATE", ELL_DEBUG);
		break;
		case APP_CMD_CONTENT_RECT_CHANGED:
			os::Printer::log("Android command APP_CMD_CONTENT_RECT_CHANGED", ELL_DEBUG);
		break;
		case APP_CMD_CONFIG_CHANGED:
			os::Printer::log("Android command APP_CMD_CONFIG_CHANGED", ELL_DEBUG);
		break;
		case APP_CMD_LOW_MEMORY:
			os::Printer::log("Android command APP_CMD_LOW_MEMORY", ELL_DEBUG);
		break;
		case APP_CMD_START:
			os::Printer::log("Android command APP_CMD_START", ELL_DEBUG);
		break;
		case APP_CMD_INIT_WINDOW:
			os::Printer::log("Android command APP_CMD_INIT_WINDOW", ELL_DEBUG);
			device->getExposedVideoData().OGLESAndroid.Window = app->window;

			if (device->CreationParams.WindowSize.Width == 0 || device->CreationParams.WindowSize.Height == 0)
			{
				device->CreationParams.WindowSize.Width = ANativeWindow_getWidth(app->window);
				device->CreationParams.WindowSize.Height = ANativeWindow_getHeight(app->window);
			}

			device->getContextManager()->initialize(device->CreationParams, device->ExposedVideoData);
			device->getContextManager()->generateSurface();
			device->getContextManager()->generateContext();
			device->getContextManager()->activateContext(device->getContextManager()->getContext());

			if (!device->Initialized)
			{
				io::CAndroidAssetFileArchive* assets = new io::CAndroidAssetFileArchive( device->Android->activity->assetManager, false, false);
				assets->addDirectoryToFileList("");
				device->FileSystem->addFileArchive(assets);
				assets->drop();

				device->createDriver();

				if (device->VideoDriver)
					device->createGUIAndScene();
			}
			device->Initialized = true;
		break;
		case APP_CMD_TERM_WINDOW:
			os::Printer::log("Android command APP_CMD_TERM_WINDOW", ELL_DEBUG);
			device->getContextManager()->destroySurface();
		break;
		case APP_CMD_GAINED_FOCUS:
			os::Printer::log("Android command APP_CMD_GAINED_FOCUS", ELL_DEBUG);
			device->Focused = true;
		break;
		case APP_CMD_LOST_FOCUS:
			os::Printer::log("Android command APP_CMD_LOST_FOCUS", ELL_DEBUG);
			device->Focused = false;
		break;
		case APP_CMD_DESTROY:
			os::Printer::log("Android command APP_CMD_DESTROY", ELL_DEBUG);
			if ( device->JNIEnvAttachedToVM )
			{
				device->JNIEnvAttachedToVM  = 0;
				device->Android->activity->vm->DetachCurrentThread();
			}
			device->Initialized = false;
			break;
		case APP_CMD_PAUSE:
			os::Printer::log("Android command APP_CMD_PAUSE", ELL_DEBUG);
			device->Paused = true;
			break;
		case APP_CMD_STOP:
			os::Printer::log("Android command APP_CMD_STOP", ELL_DEBUG);
			break;
		case APP_CMD_RESUME:
			os::Printer::log("Android command APP_CMD_RESUME", ELL_DEBUG);
			device->Paused = false;
			break;
		default:
			break;
	}
}

s32 CIrrDeviceAndroid::handleInput(android_app* app, AInputEvent* androidEvent)
{
	CIrrDeviceAndroid* device = (CIrrDeviceAndroid*)app->userData;
	s32 status = 0;

	switch ( AInputEvent_getType(androidEvent) )
	{
		case AINPUT_EVENT_TYPE_MOTION:
		{
			SEvent event;
			event.EventType = EET_TOUCH_INPUT_EVENT;

			s32 eventAction = AMotionEvent_getAction(androidEvent);
			s32 eventType =  eventAction & AMOTION_EVENT_ACTION_MASK;

#if 0
			// Useful for debugging. We might have to pass some of those infos on at some point.
			// but preferably device independent (so iphone can use same irrlicht flags).
			int32_t flags = AMotionEvent_getFlags(androidEvent);
			os::Printer::log("flags: ", core::stringc(flags).c_str(), ELL_DEBUG);
			int32_t metaState = AMotionEvent_getMetaState(androidEvent);
			os::Printer::log("metaState: ", core::stringc(metaState).c_str(), ELL_DEBUG);
			int32_t edgeFlags = AMotionEvent_getEdgeFlags(androidEvent);
			os::Printer::log("edgeFlags: ", core::stringc(flags).c_str(), ELL_DEBUG);
#endif

			bool touchReceived = true;

			switch (eventType)
			{
			case AMOTION_EVENT_ACTION_DOWN:
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				event.TouchInput.Event = ETIE_PRESSED_DOWN;
				break;
			case AMOTION_EVENT_ACTION_MOVE:
				event.TouchInput.Event = ETIE_MOVED;
				break;
			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_POINTER_UP:
			case AMOTION_EVENT_ACTION_CANCEL:
				event.TouchInput.Event = ETIE_LEFT_UP;
				break;
			default:
				touchReceived = false;
				break;
			}

			if (touchReceived)
			{
				// Process all touches for move action.
				if (event.TouchInput.Event == ETIE_MOVED)
				{
					s32 pointerCount = AMotionEvent_getPointerCount(androidEvent);

					for (s32 i = 0; i < pointerCount; ++i)
					{
						event.TouchInput.ID = AMotionEvent_getPointerId(androidEvent, i);
						event.TouchInput.X = AMotionEvent_getX(androidEvent, i);
						event.TouchInput.Y = AMotionEvent_getY(androidEvent, i);
						event.TouchInput.touchedCount = AMotionEvent_getPointerCount(androidEvent);

						device->postEventFromUser(event);
					}
				}
				else // Process one touch for other actions.
				{
					s32 pointerIndex = (eventAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

					event.TouchInput.ID = AMotionEvent_getPointerId(androidEvent, pointerIndex);
					event.TouchInput.X = AMotionEvent_getX(androidEvent, pointerIndex);
					event.TouchInput.Y = AMotionEvent_getY(androidEvent, pointerIndex);
					event.TouchInput.touchedCount = AMotionEvent_getPointerCount(androidEvent);

					device->postEventFromUser(event);
				}

				status = 1;
			}
		}
		break;
		case AINPUT_EVENT_TYPE_KEY:
		{
			SEvent event;
			event.EventType = EET_KEY_INPUT_EVENT;

			int32_t keyCode = AKeyEvent_getKeyCode(androidEvent);
			// os::Printer::log("keyCode: ", core::stringc(keyCode).c_str(), ELL_DEBUG);

			int32_t keyAction = AKeyEvent_getAction(androidEvent);
			int32_t keyMetaState = AKeyEvent_getMetaState(androidEvent);

			if ( keyCode >= 0 && (u32)keyCode < device->KeyMap.size() )
				event.KeyInput.Key = device->KeyMap[keyCode];
			else
				event.KeyInput.Key = KEY_UNKNOWN;
			event.KeyInput.SystemKeyCode = (u32)keyCode;
			if ( keyAction == AKEY_EVENT_ACTION_DOWN )
				event.KeyInput.PressedDown = true;
			else if ( keyAction == AKEY_EVENT_ACTION_UP )
				event.KeyInput.PressedDown = false;
			else if ( keyAction == AKEY_EVENT_ACTION_MULTIPLE )
			{
				// TODO: Multiple duplicate key events have occurred in a row,
				// or a complex string is being delivered. The repeat_count
				// property of the key event contains the number of times the
				// given key code should be executed.
				// I guess this might necessary for more complicated i18n key input,
				// but don't see yet how to handle this correctly.
			}

			/* no use for meta keys so far.
			if (   keyMetaState & AMETA_ALT_ON
				|| keyMetaState & AMETA_ALT_LEFT_ON
				|| keyMetaState & AMETA_ALT_RIGHT_ON )
				;
			// what is a sym?
			if (   keyMetaState & AMETA_SYM_ON )
				;
			*/
			if (   keyMetaState & AMETA_SHIFT_ON
				|| keyMetaState & AMETA_SHIFT_LEFT_ON
				|| keyMetaState & AMETA_SHIFT_RIGHT_ON )
				event.KeyInput.Shift = true;
			else
				event.KeyInput.Shift = false;
			event.KeyInput.Control = false;

			// Having memory allocations + going through JNI for each key-press is pretty bad (slow).
			// So we do it only for those keys which are likely text-characters and avoid it for all other keys.
			// So it's fast for keys like game controller input and special keys. And text keys are typically
			// only used or entering text and not for gaming on Android, so speed likely doesn't matter there too much.
			if ( event.KeyInput.Key > 0 )
			{
				// TODO:
				// Not sure why we have to attach a JNIEnv here, but it won't work when doing that in the constructor or
				// trying to use the activity->env. My best guess is that the event-handling happens in an own thread.
				// It means JNIEnvAttachedToVM will never get detached as I don't know a safe way where to do that
				// (we could attach & detach each time, but that would probably be slow)
				// Also - it has to be each time as it get's invalid when the application mode changes.
				if ( device->Initialized && device->Android && device->Android->activity && device->Android->activity->vm )
				{
					JavaVMAttachArgs attachArgs;
					attachArgs.version = JNI_VERSION_1_6;
					attachArgs.name = 0;
					attachArgs.group = NULL;

					// Not a big problem calling it each time - it's a no-op when the thread already is attached.
					// And we have to do that as someone else can have detached the thread in the meantime.
					jint result = device->Android->activity->vm->AttachCurrentThread(&device->JNIEnvAttachedToVM, &attachArgs);
					if(result == JNI_ERR)
					{
						os::Printer::log("AttachCurrentThread for the JNI environment failed.", ELL_WARNING);
						device->JNIEnvAttachedToVM = 0;
					}

					if ( device->JNIEnvAttachedToVM )
					{
						jni::CKeyEventWrapper * keyEventWrapper = new jni::CKeyEventWrapper(device->JNIEnvAttachedToVM, keyAction, keyCode);
						event.KeyInput.Char = keyEventWrapper->getUnicodeChar(keyMetaState);
						delete keyEventWrapper;
					}
				}
				if ( event.KeyInput.Key == KEY_BACK )
				{
					event.KeyInput.Char =  0x08;	// same key-code as on other operating systems. Otherwise we have to handle too much system specific stuff in the editbox.
				}
				//os::Printer::log("char-code: ", core::stringc((int)event.KeyInput.Char).c_str(), ELL_DEBUG);

			}
			else
			{
				// os::Printer::log("keyCode: ", core::stringc(keyCode).c_str(), ELL_DEBUG);
				event.KeyInput.Char = 0;
			}

			status = device->postEventFromUser(event);
		}
		break;
		default:
		break;
	}

	return status;
}

void CIrrDeviceAndroid::createDriver()
{
	switch(CreationParams.DriverType)
	{
	case video::EDT_OGLES1:
#ifdef _IRR_COMPILE_WITH_OGLES1_
		VideoDriver = video::createOGLES1Driver(CreationParams, FileSystem, ContextManager);
#else
		os::Printer::log("No OpenGL ES 1.0 support compiled in.", ELL_ERROR);
#endif
		break;
	case video::EDT_OGLES2:
#ifdef _IRR_COMPILE_WITH_OGLES2_
		VideoDriver = video::createOGLES2Driver(CreationParams, FileSystem, ContextManager);
#else
		os::Printer::log("No OpenGL ES 2.0 support compiled in.", ELL_ERROR);
#endif
		break;
	case video::EDT_NULL:
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;
	case video::EDT_OPENGL:
		os::Printer::log("This driver is not available in Android. Try OpenGL ES 1.0 or ES 2.0.", ELL_ERROR);
		break;
	default:
		os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
		break;
	}
}

video::SExposedVideoData& CIrrDeviceAndroid::getExposedVideoData()
{
	return ExposedVideoData;
}

void CIrrDeviceAndroid::createKeyMap()
{
	KeyMap.set_used(223);

    KeyMap[0] = KEY_UNKNOWN; // AKEYCODE_UNKNOWN
    KeyMap[1] = KEY_LBUTTON; // AKEYCODE_SOFT_LEFT
    KeyMap[2] = KEY_RBUTTON; // AKEYCODE_SOFT_RIGHT
    KeyMap[3] = KEY_HOME; // AKEYCODE_HOME
    KeyMap[4] = KEY_CANCEL; // AKEYCODE_BACK
    KeyMap[5] = KEY_UNKNOWN; // AKEYCODE_CALL
    KeyMap[6] = KEY_UNKNOWN; // AKEYCODE_ENDCALL
    KeyMap[7] = KEY_KEY_0; // AKEYCODE_0
    KeyMap[8] = KEY_KEY_1; // AKEYCODE_1
    KeyMap[9] = KEY_KEY_2; // AKEYCODE_2
    KeyMap[10] = KEY_KEY_3; // AKEYCODE_3
    KeyMap[11] = KEY_KEY_4; // AKEYCODE_4
    KeyMap[12] = KEY_KEY_5; // AKEYCODE_5
    KeyMap[13] = KEY_KEY_6; // AKEYCODE_6
    KeyMap[14] = KEY_KEY_7; // AKEYCODE_7
    KeyMap[15] = KEY_KEY_8; // AKEYCODE_8
    KeyMap[16] = KEY_KEY_9; // AKEYCODE_9
    KeyMap[17] = KEY_UNKNOWN; // AKEYCODE_STAR
    KeyMap[18] = KEY_UNKNOWN; // AKEYCODE_POUND
    KeyMap[19] = KEY_UP; // AKEYCODE_DPAD_UP
    KeyMap[20] = KEY_DOWN; // AKEYCODE_DPAD_DOWN
    KeyMap[21] = KEY_LEFT; // AKEYCODE_DPAD_LEFT
    KeyMap[22] = KEY_RIGHT; // AKEYCODE_DPAD_RIGHT
    KeyMap[23] = KEY_SELECT; // AKEYCODE_DPAD_CENTER
    KeyMap[24] = KEY_VOLUME_DOWN; // AKEYCODE_VOLUME_UP
    KeyMap[25] = KEY_VOLUME_UP; // AKEYCODE_VOLUME_DOWN
    KeyMap[26] = KEY_UNKNOWN; // AKEYCODE_POWER
    KeyMap[27] = KEY_UNKNOWN; // AKEYCODE_CAMERA
    KeyMap[28] = KEY_CLEAR; // AKEYCODE_CLEAR
    KeyMap[29] = KEY_KEY_A; // AKEYCODE_A
    KeyMap[30] = KEY_KEY_B; // AKEYCODE_B
    KeyMap[31] = KEY_KEY_C; // AKEYCODE_C
    KeyMap[32] = KEY_KEY_D; // AKEYCODE_D
    KeyMap[33] = KEY_KEY_E; // AKEYCODE_E
    KeyMap[34] = KEY_KEY_F; // AKEYCODE_F
    KeyMap[35] = KEY_KEY_G; // AKEYCODE_G
    KeyMap[36] = KEY_KEY_H; // AKEYCODE_H
    KeyMap[37] = KEY_KEY_I; // AKEYCODE_I
    KeyMap[38] = KEY_KEY_J; // AKEYCODE_J
    KeyMap[39] = KEY_KEY_K; // AKEYCODE_K
    KeyMap[40] = KEY_KEY_L; // AKEYCODE_L
    KeyMap[41] = KEY_KEY_M; // AKEYCODE_M
    KeyMap[42] = KEY_KEY_N; // AKEYCODE_N
    KeyMap[43] = KEY_KEY_O; // AKEYCODE_O
    KeyMap[44] = KEY_KEY_P; // AKEYCODE_P
    KeyMap[45] = KEY_KEY_Q; // AKEYCODE_Q
    KeyMap[46] = KEY_KEY_R; // AKEYCODE_R
    KeyMap[47] = KEY_KEY_S; // AKEYCODE_S
    KeyMap[48] = KEY_KEY_T; // AKEYCODE_T
    KeyMap[49] = KEY_KEY_U; // AKEYCODE_U
    KeyMap[50] = KEY_KEY_V; // AKEYCODE_V
    KeyMap[51] = KEY_KEY_W; // AKEYCODE_W
    KeyMap[52] = KEY_KEY_X; // AKEYCODE_X
    KeyMap[53] = KEY_KEY_Y; // AKEYCODE_Y
    KeyMap[54] = KEY_KEY_Z; // AKEYCODE_Z
    KeyMap[55] = KEY_COMMA; // AKEYCODE_COMMA
    KeyMap[56] = KEY_PERIOD; // AKEYCODE_PERIOD
    KeyMap[57] = KEY_MENU; // AKEYCODE_ALT_LEFT
    KeyMap[58] = KEY_MENU; // AKEYCODE_ALT_RIGHT
    KeyMap[59] = KEY_LSHIFT; // AKEYCODE_SHIFT_LEFT
    KeyMap[60] = KEY_RSHIFT; // AKEYCODE_SHIFT_RIGHT
    KeyMap[61] = KEY_TAB; // AKEYCODE_TAB
    KeyMap[62] = KEY_SPACE; // AKEYCODE_SPACE
    KeyMap[63] = KEY_UNKNOWN; // AKEYCODE_SYM
    KeyMap[64] = KEY_UNKNOWN; // AKEYCODE_EXPLORER
    KeyMap[65] = KEY_UNKNOWN; // AKEYCODE_ENVELOPE
    KeyMap[66] = KEY_RETURN; // AKEYCODE_ENTER
    KeyMap[67] = KEY_BACK; // AKEYCODE_DEL
    KeyMap[68] = KEY_OEM_3; // AKEYCODE_GRAVE
    KeyMap[69] = KEY_MINUS; // AKEYCODE_MINUS
    KeyMap[70] = KEY_UNKNOWN; // AKEYCODE_EQUALS
    KeyMap[71] = KEY_UNKNOWN; // AKEYCODE_LEFT_BRACKET
    KeyMap[72] = KEY_UNKNOWN; // AKEYCODE_RIGHT_BRACKET
    KeyMap[73] = KEY_UNKNOWN; // AKEYCODE_BACKSLASH
    KeyMap[74] = KEY_UNKNOWN; // AKEYCODE_SEMICOLON
    KeyMap[75] = KEY_UNKNOWN; // AKEYCODE_APOSTROPHE
    KeyMap[76] = KEY_UNKNOWN; // AKEYCODE_SLASH
    KeyMap[77] = KEY_UNKNOWN; // AKEYCODE_AT
    KeyMap[78] = KEY_UNKNOWN; // AKEYCODE_NUM
    KeyMap[79] = KEY_UNKNOWN; // AKEYCODE_HEADSETHOOK
    KeyMap[80] = KEY_UNKNOWN; // AKEYCODE_FOCUS (*Camera* focus)
    KeyMap[81] = KEY_PLUS; // AKEYCODE_PLUS
    KeyMap[82] = KEY_MENU; // AKEYCODE_MENU
    KeyMap[83] = KEY_UNKNOWN; // AKEYCODE_NOTIFICATION
    KeyMap[84] = KEY_UNKNOWN; // AKEYCODE_SEARCH
    KeyMap[85] = KEY_MEDIA_PLAY_PAUSE; // AKEYCODE_MEDIA_PLAY_PAUSE
    KeyMap[86] = KEY_MEDIA_STOP; // AKEYCODE_MEDIA_STOP
    KeyMap[87] = KEY_MEDIA_NEXT_TRACK; // AKEYCODE_MEDIA_NEXT
    KeyMap[88] = KEY_MEDIA_PREV_TRACK; // AKEYCODE_MEDIA_PREVIOUS
    KeyMap[89] = KEY_UNKNOWN; // AKEYCODE_MEDIA_REWIND
    KeyMap[90] = KEY_UNKNOWN; // AKEYCODE_MEDIA_FAST_FORWARD
    KeyMap[91] = KEY_VOLUME_MUTE; // AKEYCODE_MUTE
    KeyMap[92] = KEY_PRIOR; // AKEYCODE_PAGE_UP
    KeyMap[93] = KEY_NEXT; // AKEYCODE_PAGE_DOWN
    KeyMap[94] = KEY_UNKNOWN; // AKEYCODE_PICTSYMBOLS
    KeyMap[95] = KEY_UNKNOWN; // AKEYCODE_SWITCH_CHARSET

    // following look like controller inputs
    KeyMap[96] = KEY_UNKNOWN; // AKEYCODE_BUTTON_A
    KeyMap[97] = KEY_UNKNOWN; // AKEYCODE_BUTTON_B
    KeyMap[98] = KEY_UNKNOWN; // AKEYCODE_BUTTON_C
    KeyMap[99] = KEY_UNKNOWN; // AKEYCODE_BUTTON_X
    KeyMap[100] = KEY_UNKNOWN; // AKEYCODE_BUTTON_Y
    KeyMap[101] = KEY_UNKNOWN; // AKEYCODE_BUTTON_Z
    KeyMap[102] = KEY_UNKNOWN; // AKEYCODE_BUTTON_L1
    KeyMap[103] = KEY_UNKNOWN; // AKEYCODE_BUTTON_R1
    KeyMap[104] = KEY_UNKNOWN; // AKEYCODE_BUTTON_L2
    KeyMap[105] = KEY_UNKNOWN; // AKEYCODE_BUTTON_R2
    KeyMap[106] = KEY_UNKNOWN; // AKEYCODE_BUTTON_THUMBL
    KeyMap[107] = KEY_UNKNOWN; // AKEYCODE_BUTTON_THUMBR
    KeyMap[108] = KEY_UNKNOWN; // AKEYCODE_BUTTON_START
    KeyMap[109] = KEY_UNKNOWN; // AKEYCODE_BUTTON_SELECT
    KeyMap[110] = KEY_UNKNOWN; // AKEYCODE_BUTTON_MODE

    KeyMap[111] = KEY_ESCAPE; // AKEYCODE_ESCAPE
    KeyMap[112] = KEY_DELETE; // AKEYCODE_FORWARD_DEL
    KeyMap[113] = KEY_CONTROL; // AKEYCODE_CTRL_LEFT
    KeyMap[114] = KEY_CONTROL; // AKEYCODE_CTRL_RIGHT
    KeyMap[115] = KEY_CAPITAL; // AKEYCODE_CAPS_LOCK
    KeyMap[116] = KEY_SCROLL; // AKEYCODE_SCROLL_LOCK
    KeyMap[117] = KEY_UNKNOWN; // AKEYCODE_META_LEFT
    KeyMap[118] = KEY_UNKNOWN; // AKEYCODE_META_RIGHT
    KeyMap[119] = KEY_UNKNOWN; // AKEYCODE_FUNCTION
    KeyMap[120] = KEY_SNAPSHOT; // AKEYCODE_SYSRQ
    KeyMap[121] = KEY_PAUSE; // AKEYCODE_BREAK
    KeyMap[122] = KEY_HOME; // AKEYCODE_MOVE_HOME
    KeyMap[123] = KEY_END; // AKEYCODE_MOVE_END
    KeyMap[124] = KEY_INSERT; // AKEYCODE_INSERT
    KeyMap[125] = KEY_UNKNOWN; // AKEYCODE_FORWARD
    KeyMap[126] = KEY_PLAY; // AKEYCODE_MEDIA_PLAY
    KeyMap[127] = KEY_MEDIA_PLAY_PAUSE; // AKEYCODE_MEDIA_PAUSE
    KeyMap[128] = KEY_UNKNOWN; // AKEYCODE_MEDIA_CLOSE
    KeyMap[129] = KEY_UNKNOWN; // AKEYCODE_MEDIA_EJECT
    KeyMap[130] = KEY_UNKNOWN; // AKEYCODE_MEDIA_RECORD
    KeyMap[131] = KEY_F1; // AKEYCODE_F1
    KeyMap[132] = KEY_F2; // AKEYCODE_F2
    KeyMap[133] = KEY_F3; // AKEYCODE_F3
    KeyMap[134] = KEY_F4; // AKEYCODE_F4
    KeyMap[135] = KEY_F5; // AKEYCODE_F5
    KeyMap[136] = KEY_F6; // AKEYCODE_F6
    KeyMap[137] = KEY_F7; // AKEYCODE_F7
    KeyMap[138] = KEY_F8; // AKEYCODE_F8
    KeyMap[139] = KEY_F9; // AKEYCODE_F9
    KeyMap[140] = KEY_F10; // AKEYCODE_F10
    KeyMap[141] = KEY_F11; // AKEYCODE_F11
    KeyMap[142] = KEY_F12; // AKEYCODE_F12
    KeyMap[143] = KEY_NUMLOCK; // AKEYCODE_NUM_LOCK
    KeyMap[144] = KEY_NUMPAD0; // AKEYCODE_NUMPAD_0
    KeyMap[145] = KEY_NUMPAD1; // AKEYCODE_NUMPAD_1
    KeyMap[146] = KEY_NUMPAD2; // AKEYCODE_NUMPAD_2
    KeyMap[147] = KEY_NUMPAD3; // AKEYCODE_NUMPAD_3
    KeyMap[148] = KEY_NUMPAD4; // AKEYCODE_NUMPAD_4
    KeyMap[149] = KEY_NUMPAD5; // AKEYCODE_NUMPAD_5
    KeyMap[150] = KEY_NUMPAD6; // AKEYCODE_NUMPAD_6
    KeyMap[151] = KEY_NUMPAD7; // AKEYCODE_NUMPAD_7
    KeyMap[152] = KEY_NUMPAD8; // AKEYCODE_NUMPAD_8
    KeyMap[153] = KEY_NUMPAD9; // AKEYCODE_NUMPAD_9
    KeyMap[154] = KEY_DIVIDE; // AKEYCODE_NUMPAD_DIVIDE
    KeyMap[155] = KEY_MULTIPLY; // AKEYCODE_NUMPAD_MULTIPLY
    KeyMap[156] = KEY_SUBTRACT; // AKEYCODE_NUMPAD_SUBTRACT
    KeyMap[157] = KEY_ADD; // AKEYCODE_NUMPAD_ADD
    KeyMap[158] = KEY_UNKNOWN; // AKEYCODE_NUMPAD_DOT
    KeyMap[159] = KEY_COMMA; // AKEYCODE_NUMPAD_COMMA
    KeyMap[160] = KEY_RETURN; // AKEYCODE_NUMPAD_ENTER
    KeyMap[161] = KEY_UNKNOWN; // AKEYCODE_NUMPAD_EQUALS
    KeyMap[162] = KEY_UNKNOWN; // AKEYCODE_NUMPAD_LEFT_PAREN
    KeyMap[163] = KEY_UNKNOWN; // AKEYCODE_NUMPAD_RIGHT_PAREN
    KeyMap[164] = KEY_VOLUME_MUTE; // AKEYCODE_VOLUME_MUTE
    KeyMap[165] = KEY_UNKNOWN; // AKEYCODE_INFO
    KeyMap[166] = KEY_UNKNOWN; // AKEYCODE_CHANNEL_UP
    KeyMap[167] = KEY_UNKNOWN; // AKEYCODE_CHANNEL_DOWN
    KeyMap[168] = KEY_ZOOM; // AKEYCODE_ZOOM_IN
    KeyMap[169] = KEY_UNKNOWN; // AKEYCODE_ZOOM_OUT
    KeyMap[170] = KEY_UNKNOWN; // AKEYCODE_TV
    KeyMap[171] = KEY_UNKNOWN; // AKEYCODE_WINDOW
    KeyMap[172] = KEY_UNKNOWN; // AKEYCODE_GUIDE
    KeyMap[173] = KEY_UNKNOWN; // AKEYCODE_DVR
    KeyMap[174] = KEY_UNKNOWN; // AKEYCODE_BOOKMARK
    KeyMap[175] = KEY_UNKNOWN; // AKEYCODE_CAPTIONS
    KeyMap[176] = KEY_UNKNOWN; // AKEYCODE_SETTINGS
    KeyMap[177] = KEY_UNKNOWN; // AKEYCODE_TV_POWER
    KeyMap[178] = KEY_UNKNOWN; // AKEYCODE_TV_INPUT
    KeyMap[179] = KEY_UNKNOWN; // AKEYCODE_STB_POWER
    KeyMap[180] = KEY_UNKNOWN; // AKEYCODE_STB_INPUT
    KeyMap[181] = KEY_UNKNOWN; // AKEYCODE_AVR_POWER
    KeyMap[182] = KEY_UNKNOWN; // AKEYCODE_AVR_INPUT
    KeyMap[183] = KEY_UNKNOWN; // AKEYCODE_PROG_RED
    KeyMap[184] = KEY_UNKNOWN; // AKEYCODE_PROG_GREEN
    KeyMap[185] = KEY_UNKNOWN; // AKEYCODE_PROG_YELLOW
    KeyMap[186] = KEY_UNKNOWN; // AKEYCODE_PROG_BLUE
    KeyMap[187] = KEY_UNKNOWN; // AKEYCODE_APP_SWITCH
    KeyMap[188] = KEY_UNKNOWN; // AKEYCODE_BUTTON_1
    KeyMap[189] = KEY_UNKNOWN; // AKEYCODE_BUTTON_2
    KeyMap[190] = KEY_UNKNOWN; // AKEYCODE_BUTTON_3
    KeyMap[191] = KEY_UNKNOWN; // AKEYCODE_BUTTON_4
    KeyMap[192] = KEY_UNKNOWN; // AKEYCODE_BUTTON_5
    KeyMap[193] = KEY_UNKNOWN; // AKEYCODE_BUTTON_6
    KeyMap[194] = KEY_UNKNOWN; // AKEYCODE_BUTTON_7
    KeyMap[195] = KEY_UNKNOWN; // AKEYCODE_BUTTON_8
    KeyMap[196] = KEY_UNKNOWN; // AKEYCODE_BUTTON_9
    KeyMap[197] = KEY_UNKNOWN; // AKEYCODE_BUTTON_10
    KeyMap[198] = KEY_UNKNOWN; // AKEYCODE_BUTTON_11
    KeyMap[199] = KEY_UNKNOWN; // AKEYCODE_BUTTON_12
    KeyMap[200] = KEY_UNKNOWN; // AKEYCODE_BUTTON_13
    KeyMap[201] = KEY_UNKNOWN; // AKEYCODE_BUTTON_14
    KeyMap[202] = KEY_UNKNOWN; // AKEYCODE_BUTTON_15
    KeyMap[203] = KEY_UNKNOWN; // AKEYCODE_BUTTON_16
    KeyMap[204] = KEY_UNKNOWN; // AKEYCODE_LANGUAGE_SWITCH
    KeyMap[205] = KEY_UNKNOWN; // AKEYCODE_MANNER_MODE
    KeyMap[206] = KEY_UNKNOWN; // AKEYCODE_3D_MODE
    KeyMap[207] = KEY_UNKNOWN; // AKEYCODE_CONTACTS
    KeyMap[208] = KEY_UNKNOWN; // AKEYCODE_CALENDAR
    KeyMap[209] = KEY_UNKNOWN; // AKEYCODE_MUSIC
    KeyMap[210] = KEY_UNKNOWN; // AKEYCODE_CALCULATOR
    KeyMap[211] = KEY_UNKNOWN; // AKEYCODE_ZENKAKU_HANKAKU
    KeyMap[212] = KEY_UNKNOWN; // AKEYCODE_EISU
    KeyMap[213] = KEY_UNKNOWN; // AKEYCODE_MUHENKAN
    KeyMap[214] = KEY_UNKNOWN; // AKEYCODE_HENKAN
    KeyMap[215] = KEY_UNKNOWN; // AKEYCODE_KATAKANA_HIRAGANA
    KeyMap[216] = KEY_UNKNOWN; // AKEYCODE_YEN
    KeyMap[217] = KEY_UNKNOWN; // AKEYCODE_RO
    KeyMap[218] = KEY_UNKNOWN; // AKEYCODE_KANA
    KeyMap[219] = KEY_UNKNOWN; // AKEYCODE_ASSIST
    KeyMap[220] = KEY_UNKNOWN; // AKEYCODE_BRIGHTNESS_DOWN
    KeyMap[221] = KEY_UNKNOWN; // AKEYCODE_BRIGHTNESS_UP ,
    KeyMap[222] = KEY_UNKNOWN; // AKEYCODE_MEDIA_AUDIO_TRACK
}

bool CIrrDeviceAndroid::activateAccelerometer(float updateInterval)
{
	if (!isAccelerometerAvailable())
		return false;

	ASensorEventQueue_enableSensor(SensorEventQueue, Accelerometer);
	ASensorEventQueue_setEventRate(SensorEventQueue, Accelerometer, (int32_t)(updateInterval*1000.f*1000.f)); // in microseconds

	os::Printer::log("Activated accelerometer", ELL_DEBUG);
	return true;
}

bool CIrrDeviceAndroid::deactivateAccelerometer()
{
	if (Accelerometer)
	{
		ASensorEventQueue_disableSensor(SensorEventQueue, Accelerometer);
		Accelerometer = 0;
		os::Printer::log("Deactivated accelerometer", ELL_DEBUG);
		return true;
	}

	return false;
}

bool CIrrDeviceAndroid::isAccelerometerActive()
{
	return (Accelerometer != 0);
}

bool CIrrDeviceAndroid::isAccelerometerAvailable()
{
	if (!Accelerometer)
		Accelerometer = ASensorManager_getDefaultSensor(SensorManager, ASENSOR_TYPE_ACCELEROMETER);

	return (Accelerometer != 0);
}

bool CIrrDeviceAndroid::activateGyroscope(float updateInterval)
{
	if (!isGyroscopeAvailable())
		return false;

	ASensorEventQueue_enableSensor(SensorEventQueue, Gyroscope);
	ASensorEventQueue_setEventRate(SensorEventQueue, Gyroscope, (int32_t)(updateInterval*1000.f*1000.f)); // in microseconds

	os::Printer::log("Activated gyroscope", ELL_DEBUG);
	return true;
}

bool CIrrDeviceAndroid::deactivateGyroscope()
{
	if (Gyroscope)
	{
		ASensorEventQueue_disableSensor(SensorEventQueue, Gyroscope);
		Gyroscope = 0;
		os::Printer::log("Deactivated gyroscope", ELL_DEBUG);
		return true;
	}

	return false;
}

bool CIrrDeviceAndroid::isGyroscopeActive()
{
	return (Gyroscope != 0);
}

bool CIrrDeviceAndroid::isGyroscopeAvailable()
{
	if (!Gyroscope)
		Gyroscope = ASensorManager_getDefaultSensor(SensorManager, ASENSOR_TYPE_GYROSCOPE);

	return (Gyroscope != 0);
}

} // end namespace irr

#endif
