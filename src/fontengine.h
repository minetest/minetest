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
#ifndef __FONTENGINE_H__
#define __FONTENGINE_H__

#include <map>
#include <vector>
#include "IGUIFont.h"
#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "settings.h"

#define FONT_SIZE_UNSPECIFIED 0xFFFFFFFF

enum FontMode {
	Standard = 0,
	Mono,
	Fallback,
	Simple,
	SimpleMono,
	MaxMode,
	Unspecified
};

class FontEngine
{
public:

	FontEngine(Settings* main_settings, gui::IGUIEnvironment* env);

	~FontEngine();

	/** get Font */
	irr::gui::IGUIFont* getFont(unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=Unspecified);

	/** get text height for a specific font */
	unsigned int getTextHeight(unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=Unspecified);

	/** get default font size */
	unsigned int getDefaultFontSize();

	/** initialize font engine */
	void initialize(Settings* main_settings, gui::IGUIEnvironment* env);

	/** update internal parameters from settings */
	void readSettings();

private:
	/** disable copy constructor */
	FontEngine() :
		m_settings(NULL),
		m_env(NULL),
		m_font_cache(),
		m_default_size(),
		m_currentMode(Standard),
		m_lastMode(),
		m_lastSize(0),
		m_lastFont(NULL)
	{};

	/** update content of font cache in case of a setting change made it invalid */
	void updateFontCache();

	/** initialize a new font */
	void initFont(unsigned int basesize, FontMode mode=Unspecified);

	/** initialize a font without freetype */
	void initSimpleFont(unsigned int basesize, FontMode mode);

	/** update current minetest skin with font changes */
	void updateSkin();

	/** clean cache */
	void cleanCache();

	/** pointer to settings for registering callbacks or reading config */
	Settings* m_settings;

	/** pointer to irrlicht gui environment */
	gui::IGUIEnvironment* m_env;

	/** internal storage for caching fonts of different size */
	std::map<unsigned int, irr::gui::IGUIFont*> m_font_cache[MaxMode];

	/** default font size to use */
	unsigned int m_default_size[MaxMode];

	/** current font engine mode */
	FontMode m_currentMode;

	/** font mode of last request */
	FontMode m_lastMode;

	/** size of last request */
	unsigned int m_lastSize;

	/** last font returned */
	irr::gui::IGUIFont* m_lastFont;

};

/** interface to access main font engine*/
extern FontEngine* fe;

#endif
