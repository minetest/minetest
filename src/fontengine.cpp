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
#include "fontengine.h"
#include "log.h"
#include "main.h"
#include "config.h"
#include "porting.h"
#include "constants.h"
#if USE_FREETYPE
#include "gettext.h"
#include "filesys.h"
#include "xCGUITTFont.h"
#endif


/** minimum font size supported */
#define MIN_FONT_SIZE 4

/** maximum size distance for getting a "similar" font size */
#define MAX_FONT_SIZE_OFFSET 10

/** reference to access font engine, has to be initialized by main */
FontEngine* fe = NULL;

/** callback to be used on change of font size setting */
static void font_setting_changed() {
	fe->readSettings();
}

/******************************************************************************/
FontEngine::FontEngine(Settings* main_settings, gui::IGUIEnvironment* env) :
	m_settings(NULL),
	m_env(NULL),
	m_font_cache(),
	m_default_size(),
	m_currentMode(Standard),
	m_lastMode(),
	m_lastSize(0),
	m_lastFont(NULL)
{

	for ( unsigned int i = 0; i < MaxMode; i++) {
		m_default_size[i] = (FontMode) FONT_SIZE_UNSPECIFIED;
	}

	m_settings = main_settings;
	m_env     = env;

	assert(m_settings != NULL);
	assert(m_env != NULL);
	assert(m_env->getSkin() != NULL);

	m_currentMode = Simple;

#if USE_FREETYPE
	if (g_settings->getBool("freetype")) {
		m_default_size[Standard] = m_settings->getU16("font_size");
		m_default_size[Fallback] = m_settings->getU16("fallback_font_size");
		m_default_size[Mono]     = m_settings->getU16("mono_font_size");

		if (is_yes(gettext("needs_fallback_font"))) {
			m_currentMode = Fallback;
		}
		else {
			m_currentMode = Standard;
		}
	}

	// having freetype but not using it is quite a strange case so we need to do
	// special handling for it
	if (m_currentMode == Simple) {
		std::stringstream fontsize;
		fontsize << DEFAULT_FONT_SIZE;
		m_settings->setDefault("font_size", fontsize.str());
		m_settings->setDefault("mono_font_size", fontsize.str());
	}
#endif

	m_default_size[Simple]       = m_settings->getU16("font_size");
	m_default_size[SimpleMono]   = m_settings->getU16("mono_font_size");

	updateSkin();

	if (m_currentMode == Standard) {
		m_settings->registerChangedCallback("font_size", font_setting_changed);
		m_settings->registerChangedCallback("font_path", font_setting_changed);
		m_settings->registerChangedCallback("font_shadow", font_setting_changed);
		m_settings->registerChangedCallback("font_shadow_alpha", font_setting_changed);
	}
	else if (m_currentMode == Fallback) {
		m_settings->registerChangedCallback("fallback_font_size", font_setting_changed);
		m_settings->registerChangedCallback("fallback_font_path", font_setting_changed);
		m_settings->registerChangedCallback("fallback_font_shadow", font_setting_changed);
		m_settings->registerChangedCallback("fallback_font_shadow_alpha", font_setting_changed);
	}

	m_settings->registerChangedCallback("mono_font_path", font_setting_changed);
	m_settings->registerChangedCallback("mono_font_size", font_setting_changed);
	m_settings->registerChangedCallback("screen_dpi", font_setting_changed);
	m_settings->registerChangedCallback("gui_scaling", font_setting_changed);
}

/******************************************************************************/
FontEngine::~FontEngine()
{
	cleanCache();
}

/******************************************************************************/
void FontEngine::cleanCache()
{
	for ( unsigned int i = 0; i < MaxMode; i++) {

		for (std::map<unsigned int, irr::gui::IGUIFont*>::iterator iter
				= m_font_cache[i].begin();
				iter != m_font_cache[i].end(); iter++) {
			iter->second->drop();
			iter->second = NULL;
		}
		m_font_cache[i].clear();
	}
}

/******************************************************************************/
irr::gui::IGUIFont* FontEngine::getFont(unsigned int font_size, FontMode mode)
{
	if (mode == Unspecified) {
		mode = m_currentMode;
	}
	else if ((mode == Mono) && (m_currentMode == Simple)) {
		mode = SimpleMono;
	}

	if (font_size == FONT_SIZE_UNSPECIFIED) {
		font_size = m_default_size[mode];
	}

	if ((font_size == m_lastSize) && (mode == m_lastMode)) {
		return m_lastFont;
	}

	if (m_font_cache[mode].find(font_size) == m_font_cache[mode].end()) {
		initFont(font_size, mode);
	}

	if (m_font_cache[mode].find(font_size) == m_font_cache[mode].end()) {
		return NULL;
	}

	m_lastSize = font_size;
	m_lastMode = mode;
	m_lastFont = m_font_cache[mode][font_size];

	return m_font_cache[mode][font_size];
}

/******************************************************************************/
unsigned int FontEngine::getTextHeight(unsigned int font_size, FontMode mode)
{
	irr::gui::IGUIFont* font = getFont(font_size,mode);

	if (font == NULL) return MIN_FONT_SIZE;

	return font->getDimension(L"Some unimportant example String").Height;
}

/******************************************************************************/
unsigned int FontEngine::getDefaultFontSize()
{
	return m_default_size[m_currentMode];
}

/******************************************************************************/
void FontEngine::readSettings()
{
#if USE_FREETYPE
	if (g_settings->getBool("freetype")) {
		m_default_size[Standard] = m_settings->getU16("font_size");
		m_default_size[Fallback] = m_settings->getU16("fallback_font_size");
		m_default_size[Mono]     = m_settings->getU16("mono_font_size");

		if (is_yes(gettext("needs_fallback_font"))) {
			m_currentMode = Fallback;
		}
		else {
			m_currentMode = Standard;
		}
	}
#endif
	m_default_size[Simple]       = m_settings->getU16("font_size");
	m_default_size[SimpleMono]   = m_settings->getU16("mono_font_size");

	cleanCache();
	updateFontCache();
	updateSkin();
}

/******************************************************************************/
void FontEngine::updateSkin()
{
	gui::IGUIFont *font = getFont();

	if (font)
		m_env->getSkin()->setFont(font);
	else
		errorstream << "WARNING: Font file was not found."
				<< " Using irrlicht default font." << std::endl;

	// If we did fail to create a font our own make irrlicht find a default one
	font = m_env->getSkin()->getFont();
	assert(font);

	u32 text_height = font->getDimension(L"Hello, world!").Height;
	infostream << "text_height=" << text_height << std::endl;
}

/******************************************************************************/
void FontEngine::updateFontCache()
{
	for (unsigned int i = 0; i < MaxMode; i++) {
		/* only font to be initialized is default one,
		 * all others are re-initialized on demand */
		initFont(m_default_size[i], (FontMode) i);
	}

	/* reset font quick access */
	m_lastMode = Unspecified;
	m_lastSize = 0;
	m_lastFont = NULL;
}

/******************************************************************************/
void FontEngine::initFont(unsigned int basesize, FontMode mode)
{

	std::string font_config_prefix;

	if (mode == Unspecified) {
		mode = m_currentMode;
	}

	switch (mode) {

		case Standard:
			font_config_prefix = "";
			break;

		case Fallback:
			font_config_prefix = "fallback_";
			break;

		case Mono:
			font_config_prefix = "mono_";
			if (m_currentMode == Simple)
				mode = SimpleMono;
			break;

		case Simple: /* Fallthrough */
		case SimpleMono: /* Fallthrough */
		default:
			font_config_prefix = "";

	}

	if (m_font_cache[mode].find(basesize) != m_font_cache[mode].end())
		return;

	if ((mode == Simple) || (mode == SimpleMono)) {
		initSimpleFont(basesize, mode);
		return;
	}
#if USE_FREETYPE
	else {
		if (! is_yes(m_settings->get("freetype"))) {
			return;
		}
		unsigned int size = floor(
				porting::getDisplayDensity() *
				m_settings->getFloat("gui_scaling") *
				basesize);
		u32 font_shadow       = 0;
		u32 font_shadow_alpha = 0;

		try {
			g_settings->getU16(font_config_prefix + "font_shadow");
		} catch (SettingNotFoundException&) {}
		try {
			g_settings->getU16(font_config_prefix + "font_shadow_alpha");
		} catch (SettingNotFoundException&) {}

		std::string font_path = g_settings->get(font_config_prefix + "font_path");

		irr::gui::IGUIFont* font = gui::CGUITTFont::createTTFont(m_env,
				font_path.c_str(), size, true, true, font_shadow,
				font_shadow_alpha);

		if (font != NULL) {
			font->grab();
			m_font_cache[mode][basesize] = font;
		}
		else {
			errorstream << "FontEngine: failed to load freetype font: "
					<< font_path << std::endl;
		}
	}
#endif
}

/** initialize a font without freetype */
void FontEngine::initSimpleFont(unsigned int basesize, FontMode mode)
{
	assert((mode == Simple) || (mode == SimpleMono));

	std::string font_path = "";
	if (mode == Simple) {
		font_path = m_settings->get("font_path");
	} else {
		font_path = m_settings->get("mono_font_path");
	}
	std::string basename = font_path;
	std::string ending = font_path.substr(font_path.length() -4);

	if ((ending == ".xml") || ( ending == ".png") || ( ending == ".ttf")) {
		basename = font_path.substr(0,font_path.length()-4);
	}

	if (basesize == FONT_SIZE_UNSPECIFIED)
		basesize = DEFAULT_FONT_SIZE;

	unsigned int size = floor(
			porting::getDisplayDensity() *
			m_settings->getFloat("gui_scaling") *
			basesize);

	irr::gui::IGUIFont* font = NULL;

	for(unsigned int offset = 0; offset < MAX_FONT_SIZE_OFFSET; offset++) {

		// try opening positive offset
		std::stringstream fontsize_plus_png;
		fontsize_plus_png << basename << "_" << (size + offset) << ".png";

		if (fs::PathExists(fontsize_plus_png.str())) {
			font = m_env->getFont(fontsize_plus_png.str().c_str());

			if (font) {
				verbosestream << "FontEngine: found font: " << fontsize_plus_png.str() << std::endl;
				break;
			}
		}

		std::stringstream fontsize_plus_xml;
		fontsize_plus_xml << basename << "_" << (size + offset) << ".xml";

		if (fs::PathExists(fontsize_plus_xml.str())) {
			font = m_env->getFont(fontsize_plus_xml.str().c_str());

			if (font) {
				verbosestream << "FontEngine: found font: " << fontsize_plus_xml.str() << std::endl;
				break;
			}
		}

		// try negative offset
		std::stringstream fontsize_minus_png;
		fontsize_minus_png << basename << "_" << (size - offset) << ".png";

		if (fs::PathExists(fontsize_minus_png.str())) {
			font = m_env->getFont(fontsize_minus_png.str().c_str());

			if (font) {
				verbosestream << "FontEngine: found font: " << fontsize_minus_png.str() << std::endl;
				break;
			}
		}

		std::stringstream fontsize_minus_xml;
		fontsize_minus_xml << basename << "_" << (size - offset) << ".xml";

		if (fs::PathExists(fontsize_minus_xml.str())) {
			font = m_env->getFont(fontsize_minus_xml.str().c_str());

			if (font) {
				verbosestream << "FontEngine: found font: " << fontsize_minus_xml.str() << std::endl;
				break;
			}
		}
	}

	// try name direct
	if (font == NULL) {
		if (fs::PathExists(font_path)) {
			font = m_env->getFont(font_path.c_str());
			if (font)
				verbosestream << "FontEngine: found font: " << font_path << std::endl;
		}
	}

	if (font != NULL) {
		font->grab();
		m_font_cache[mode][basesize] = font;
	}
}
