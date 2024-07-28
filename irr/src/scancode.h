// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include <SDL_scancode.h>

namespace irr {

/**
 * Convert a scancode to to a character taking modifiers into account.
 *
 * On Windows, modifiers will be applied. On all other platforms modifiers
 * will not be applied. Generating more than one wchar_t for a single scancode
 * is not supported.
 *
 * @param SDL_scancode The SDL scancode.
 * @return Returns the modified character on Windows, the unmodified character
 * otherwise.
 */
wchar_t get_modified_char_from_scancode(SDL_Scancode SDL_scancode);

#ifdef _WIN32
/**
 * Get the Win32 scancode corresponding to the given SDL scancode.
 *
 * @param SDL_scancode The SDL scancode.
 * @returns The Win32 scancode if a mapping exists, 0 otherwise.
 */
unsigned int convert_to_windows_scancode(SDL_Scancode SDL_scancode);
#endif

} // end namespace irr
