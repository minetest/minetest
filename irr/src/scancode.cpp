// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "os.h"
#include "scancode.h"

#ifndef _WIN32
#include <SDL_keyboard.h>
#endif
#include <SDL_scancode.h>


#ifdef _WIN32
#include <windows.h>
#include <winuser.h>

#include <array>
#endif

namespace {

constexpr unsigned int windows_to_unicode_flag_no_side_effects{2};

} // end anonymous namespace

wchar_t irr::get_modified_char_from_scancode(SDL_Scancode SDL_scancode)
{
#ifdef _WIN32
	std::array<unsigned char, 256> keyStates;
	if (!GetKeyboardState(keyStates.data())) {
		os::Printer::log("[get_modified_char_from_scancode] Failed to get Windows keyboard state.", ELL_ERROR);
	}

	unsigned int windowsScancode{convert_to_windows_scancode(SDL_scancode)};
	unsigned int windowsVirtualKey{MapVirtualKeyA(windowsScancode, MAPVK_VSC_TO_VK)};
	wchar_t      result{};
	int          written{ToUnicode(windowsVirtualKey, windowsScancode, keyStates.data(), &result, 1, windows_to_unicode_flag_no_side_effects)};

	if (written <= 0) {
		os::Printer::log("[get_modified_char_from_scancode] Failed to get char from scancode.", ELL_ERROR);
	}

	return result;
#else
	return SDL_GetKeyFromScancode(SDL_scancode);
#endif
}

#ifdef _WIN32
unsigned int irr::convert_to_windows_scancode(SDL_Scancode SDL_scancode)
{
	switch (SDL_scancode) {
	case SDL_SCANCODE_A:
		return 30;
	case SDL_SCANCODE_B:
		return 48;
	case SDL_SCANCODE_C:
		return 46;
	case SDL_SCANCODE_D:
		return 32;
	case SDL_SCANCODE_E:
		return 18;
	case SDL_SCANCODE_F:
		return 33;
	case SDL_SCANCODE_G:
		return 34;
	case SDL_SCANCODE_H:
		return 35;
	case SDL_SCANCODE_I:
		return 23;
	case SDL_SCANCODE_J:
		return 36;
	case SDL_SCANCODE_K:
		return 37;
	case SDL_SCANCODE_L:
		return 38;
	case SDL_SCANCODE_M:
		return 50;
	case SDL_SCANCODE_N:
		return 49;
	case SDL_SCANCODE_O:
		return 24;
	case SDL_SCANCODE_P:
		return 25;
	case SDL_SCANCODE_Q:
		return 16;
	case SDL_SCANCODE_R:
		return 19;
	case SDL_SCANCODE_S:
		return 31;
	case SDL_SCANCODE_T:
		return 20;
	case SDL_SCANCODE_U:
		return 22;
	case SDL_SCANCODE_V:
		return 47;
	case SDL_SCANCODE_W:
		return 17;
	case SDL_SCANCODE_X:
		return 45;
	case SDL_SCANCODE_Y:
		return 21;
	case SDL_SCANCODE_Z:
		return 44;
	case SDL_SCANCODE_1:
	case SDL_SCANCODE_2:
	case SDL_SCANCODE_3:
	case SDL_SCANCODE_4:
	case SDL_SCANCODE_5:
	case SDL_SCANCODE_6:
	case SDL_SCANCODE_7:
	case SDL_SCANCODE_8:
	case SDL_SCANCODE_9:
	case SDL_SCANCODE_0:
		return SDL_scancode - 28;
	case SDL_SCANCODE_SPACE:
		return 61; // seems this may be the wrong code
	case SDL_SCANCODE_LEFTBRACKET:
		return 26;
	case SDL_SCANCODE_RIGHTBRACKET:
		return 27;
	case SDL_SCANCODE_BACKSLASH:
		return 28;
	case SDL_SCANCODE_SEMICOLON:
		return 39;
	case SDL_SCANCODE_APOSTROPHE:
		return 40;
	case SDL_SCANCODE_GRAVE:
		return 1;
	case SDL_SCANCODE_COMMA:
		return 51;
	case SDL_SCANCODE_PERIOD:
		return 52;
	case SDL_SCANCODE_SLASH:
		return 53;
	case SDL_SCANCODE_CAPSLOCK:
		return 29;
	case SDL_SCANCODE_F1:
	case SDL_SCANCODE_F2:
	case SDL_SCANCODE_F3:
	case SDL_SCANCODE_F4:
	case SDL_SCANCODE_F5:
	case SDL_SCANCODE_F6:
	case SDL_SCANCODE_F7:
	case SDL_SCANCODE_F8:
	case SDL_SCANCODE_F9:
	case SDL_SCANCODE_F10:
	case SDL_SCANCODE_F11:
	case SDL_SCANCODE_F12:
		return SDL_scancode + 54;

	default:
		os::Printer::log("[convert_to_windows_scancode] unsupported scancode", ELL_ERROR);
		return 0;
	}
}
#endif // _WIN32
