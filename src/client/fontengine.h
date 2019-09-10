/*
Minetest
Copyright (C) 2010-2014 sapier <sapier at gmx dot net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <map>
#include <vector>
#include "util/basic_macros.h"
#include <IGUIFont.h>
#include <IGUISkin.h>
#include <IGUIEnvironment.h>
#include "settings.h"

#define FONT_SIZE_UNSPECIFIED 0xFFFFFFFF

enum FontMode {
	FM_Standard = 0,
	FM_Mono,
	FM_Fallback,
	FM_Simple,
	FM_SimpleMono,
	FM_MaxMode,
	FM_Unspecified
};

class FontEngine
{
public:

	FontEngine(Settings* main_settings, gui::IGUIEnvironment* env);

	~FontEngine();

	/** get Font */
	irr::gui::IGUIFont *getFont(unsigned int font_size, FontMode mode,
			bool bold, bool italic);

	irr::gui::IGUIFont *getFont(unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		return getFont(font_size, mode, m_default_bold, m_default_italic);
	}

	/** get text height for a specific font */
	unsigned int getTextHeight(unsigned int font_size, FontMode mode,
			bool bold, bool italic);

	/** get text width if a text for a specific font */
	unsigned int getTextHeight(
			unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		return getTextHeight(font_size, mode, m_default_bold, m_default_italic);
	}

	unsigned int getTextWidth(const std::wstring& text,
			unsigned int font_size, FontMode mode, bool bold, bool italic);

	/** get text width if a text for a specific font */
	unsigned int getTextWidth(const std::wstring& text,
			unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		return getTextWidth(text, font_size, mode, m_default_bold,
				m_default_italic);
	}

	unsigned int getTextWidth(const std::string& text,
			unsigned int font_size, FontMode mode, bool bold, bool italic)
	{
		return getTextWidth(utf8_to_wide(text), font_size, mode, bold, italic);
	}

	unsigned int getTextWidth(const std::string& text,
			unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		return getTextWidth(utf8_to_wide(text), font_size, mode, m_default_bold,
				m_default_italic);
	}

	/** get line height for a specific font (including empty room between lines) */
	unsigned int getLineHeight(unsigned int font_size, FontMode mode, bool bold,
			bool italic);

	unsigned int getLineHeight(unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		return getLineHeight(font_size, mode, m_default_bold, m_default_italic);
	}

	/** get default font size */
	unsigned int getDefaultFontSize();

	/** initialize font engine */
	void initialize(Settings* main_settings, gui::IGUIEnvironment* env);

	/** update internal parameters from settings */
	void readSettings();

private:
	/** update content of font cache in case of a setting change made it invalid */
	void updateFontCache();

	/** initialize a new font */
	void initFont(
		unsigned int basesize,
		FontMode mode,
		bool bold,
		bool italic);

	/** initialize a font without freetype */
	void initSimpleFont(unsigned int basesize, FontMode mode);

	/** update current minetest skin with font changes */
	void updateSkin();

	/** clean cache */
	void cleanCache();

	/** pointer to settings for registering callbacks or reading config */
	Settings* m_settings = nullptr;

	/** pointer to irrlicht gui environment */
	gui::IGUIEnvironment* m_env = nullptr;

	/** internal storage for caching fonts of different size */
	std::map<unsigned int, irr::gui::IGUIFont*> m_font_cache[FM_MaxMode << 2];

	/** default font size to use */
	unsigned int m_default_size[FM_MaxMode];

	/** default bold and italic */
	bool m_default_bold;
	bool m_default_italic;

	/** current font engine mode */
	FontMode m_currentMode = FM_Standard;

	DISABLE_CLASS_COPY(FontEngine);
};

/** interface to access main font engine*/
extern FontEngine* g_fontengine;
