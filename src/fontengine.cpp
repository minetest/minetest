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
#include "config.h"
#include "porting.h"
#include "constants.h"
#include "filesys.h"

#if USE_FREETYPE
#include "gettext.h"
#include "xCGUITTFont.h"
#endif

/** maximum size distance for getting a "similar" font size */
#define MAX_FONT_SIZE_OFFSET 10

/** reference to access font engine, has to be initialized by main */
FontEngine* g_fontengine = NULL;

/** callback to be used on change of font size setting */
static void font_setting_changed(const std::string &name, void *userdata)
{
	g_fontengine->readSettings();
}

/******************************************************************************/
FontEngine::FontEngine(Settings* main_settings, gui::IGUIEnvironment* env) :
	m_settings(main_settings),
	m_env(env),
	m_font_cache(),
	m_currentMode(FM_Standard),
	m_lastMode(),
	m_lastSize(0),
	m_lastFont(NULL)
{

	for (unsigned int i = 0; i < FM_MaxMode; i++) {
		m_default_size[i] = (FontMode) FONT_SIZE_UNSPECIFIED;
	}

	assert(m_settings != NULL); // pre-condition
	assert(m_env != NULL); // pre-condition
	assert(m_env->getSkin() != NULL); // pre-condition

	m_currentMode = FM_Simple;

#if USE_FREETYPE
	if (g_settings->getBool("freetype")) {
		m_default_size[FM_Standard] = m_settings->getU16("font_size");
		m_default_size[FM_Fallback] = m_settings->getU16("fallback_font_size");
		m_default_size[FM_Mono]     = m_settings->getU16("mono_font_size");

		if (is_yes(gettext("needs_fallback_font"))) {
			m_currentMode = FM_Fallback;
		}
		else {
			m_currentMode = FM_Standard;
		}
	}

	// having freetype but not using it is quite a strange case so we need to do
	// special handling for it
	if (m_currentMode == FM_Simple) {
		std::stringstream fontsize;
		fontsize << DEFAULT_FONT_SIZE;
		m_settings->setDefault("font_size", fontsize.str());
		m_settings->setDefault("mono_font_size", fontsize.str());
	}
#endif

	m_default_size[FM_Simple]       = m_settings->getU16("font_size");
	m_default_size[FM_SimpleMono]   = m_settings->getU16("mono_font_size");

	updateSkin();

	if (m_currentMode == FM_Standard) {
		m_settings->registerChangedCallback("font_size", font_setting_changed, NULL);
		m_settings->registerChangedCallback("font_path", font_setting_changed, NULL);
		m_settings->registerChangedCallback("font_shadow", font_setting_changed, NULL);
		m_settings->registerChangedCallback("font_shadow_alpha", font_setting_changed, NULL);
	}
	else if (m_currentMode == FM_Fallback) {
		m_settings->registerChangedCallback("fallback_font_size", font_setting_changed, NULL);
		m_settings->registerChangedCallback("fallback_font_path", font_setting_changed, NULL);
		m_settings->registerChangedCallback("fallback_font_shadow", font_setting_changed, NULL);
		m_settings->registerChangedCallback("fallback_font_shadow_alpha", font_setting_changed, NULL);
	}

	m_settings->registerChangedCallback("mono_font_path", font_setting_changed, NULL);
	m_settings->registerChangedCallback("mono_font_size", font_setting_changed, NULL);
	m_settings->registerChangedCallback("screen_dpi", font_setting_changed, NULL);
	m_settings->registerChangedCallback("gui_scaling", font_setting_changed, NULL);
}

/******************************************************************************/
FontEngine::~FontEngine()
{
	cleanCache();
}

/******************************************************************************/
void FontEngine::cleanCache()
{
	for ( unsigned int i = 0; i < FM_MaxMode; i++) {

		for (std::map<unsigned int, irr::gui::IGUIFont*>::iterator iter
				= m_font_cache[i].begin();
				iter != m_font_cache[i].end(); ++iter) {
			iter->second->drop();
			iter->second = NULL;
		}
		m_font_cache[i].clear();
	}
}

/******************************************************************************/
irr::gui::IGUIFont* FontEngine::getFont(unsigned int font_size, FontMode mode)
{
	if (mode == FM_Unspecified) {
		mode = m_currentMode;
	}
	else if ((mode == FM_Mono) && (m_currentMode == FM_Simple)) {
		mode = FM_SimpleMono;
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
	irr::gui::IGUIFont* font = getFont(font_size, mode);

	// use current skin font as fallback
	if (font == NULL) {
		font = m_env->getSkin()->getFont();
	}
	FATAL_ERROR_IF(font == NULL, "Could not get skin font");

	return font->getDimension(L"Some unimportant example String").Height;
}

/******************************************************************************/
unsigned int FontEngine::getTextWidth(const std::wstring& text,
		unsigned int font_size, FontMode mode)
{
	irr::gui::IGUIFont* font = getFont(font_size, mode);

	// use current skin font as fallback
	if (font == NULL) {
		font = m_env->getSkin()->getFont();
	}
	FATAL_ERROR_IF(font == NULL, "Could not get font");

	return font->getDimension(text.c_str()).Width;
}


/** get line height for a specific font (including empty room between lines) */
unsigned int FontEngine::getLineHeight(unsigned int font_size, FontMode mode)
{
	irr::gui::IGUIFont* font = getFont(font_size, mode);

	// use current skin font as fallback
	if (font == NULL) {
		font = m_env->getSkin()->getFont();
	}
	FATAL_ERROR_IF(font == NULL, "Could not get font");

	return font->getDimension(L"Some unimportant example String").Height
			+ font->getKerningHeight();
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
		m_default_size[FM_Standard] = m_settings->getU16("font_size");
		m_default_size[FM_Fallback] = m_settings->getU16("fallback_font_size");
		m_default_size[FM_Mono]     = m_settings->getU16("mono_font_size");

		if (is_yes(gettext("needs_fallback_font"))) {
			m_currentMode = FM_Fallback;
		}
		else {
			m_currentMode = FM_Standard;
		}
	}
#endif
	m_default_size[FM_Simple]       = m_settings->getU16("font_size");
	m_default_size[FM_SimpleMono]   = m_settings->getU16("mono_font_size");

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
		errorstream << "FontEngine: Default font file: " <<
				"\n\t\"" << m_settings->get("font_path") << "\"" <<
				"\n\trequired for current screen configuration was not found" <<
				" or was invalid file format." <<
				"\n\tUsing irrlicht default font." << std::endl;

	// If we did fail to create a font our own make irrlicht find a default one
	font = m_env->getSkin()->getFont();
	FATAL_ERROR_IF(font == NULL, "Could not create/get font");

	u32 text_height = font->getDimension(L"Hello, world!").Height;
	infostream << "text_height=" << text_height << std::endl;
}

/******************************************************************************/
void FontEngine::updateFontCache()
{
	/* the only font to be initialized is default one,
	 * all others are re-initialized on demand */
	initFont(m_default_size[m_currentMode], m_currentMode);

	/* reset font quick access */
	m_lastMode = FM_Unspecified;
	m_lastSize = 0;
	m_lastFont = NULL;
}

/******************************************************************************/
void FontEngine::initFont(unsigned int basesize, FontMode mode)
{

	std::string font_config_prefix;

	if (mode == FM_Unspecified) {
		mode = m_currentMode;
	}

	switch (mode) {

		case FM_Standard:
			font_config_prefix = "";
			break;

		case FM_Fallback:
			font_config_prefix = "fallback_";
			break;

		case FM_Mono:
			font_config_prefix = "mono_";
			if (m_currentMode == FM_Simple)
				mode = FM_SimpleMono;
			break;

		case FM_Simple: /* Fallthrough */
		case FM_SimpleMono: /* Fallthrough */
		default:
			font_config_prefix = "";

	}

	if (m_font_cache[mode].find(basesize) != m_font_cache[mode].end())
		return;

	if ((mode == FM_Simple) || (mode == FM_SimpleMono)) {
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
			font_shadow =
					g_settings->getU16(font_config_prefix + "font_shadow");
		} catch (SettingNotFoundException&) {}
		try {
			font_shadow_alpha =
					g_settings->getU16(font_config_prefix + "font_shadow_alpha");
		} catch (SettingNotFoundException&) {}

		std::string font_path = g_settings->get(font_config_prefix + "font_path");

		irr::gui::IGUIFont* font = gui::CGUITTFont::createTTFont(m_env,
				font_path.c_str(), size, true, true, font_shadow,
				font_shadow_alpha);

		if (font) {
			m_font_cache[mode][basesize] = font;
			return;
		}

		if (font_config_prefix == "mono_") {
			const std::string &mono_font_path = m_settings->getDefault("mono_font_path");

			if (font_path != mono_font_path) {
				// try original mono font
				errorstream << "FontEngine: failed to load custom mono "
						"font: " << font_path << ", trying to fall back to "
						"original mono font" << std::endl;

				font = gui::CGUITTFont::createTTFont(m_env,
					mono_font_path.c_str(), size, true, true,
					font_shadow, font_shadow_alpha);

				if (font) {
					m_font_cache[mode][basesize] = font;
					return;
				}
			}
		} else {
			// try fallback font
			errorstream << "FontEngine: failed to load: " << font_path <<
					", trying to fall back to fallback font" << std::endl;

			font_path = g_settings->get(font_config_prefix + "fallback_font_path");

			font = gui::CGUITTFont::createTTFont(m_env,
				font_path.c_str(), size, true, true, font_shadow,
				font_shadow_alpha);

			if (font) {
				m_font_cache[mode][basesize] = font;
				return;
			}

			const std::string &fallback_font_path = m_settings->getDefault("fallback_font_path");

			if (font_path != fallback_font_path) {
				// try original fallback font
				errorstream << "FontEngine: failed to load custom fallback "
						"font: " << font_path << ", trying to fall back to "
						"original fallback font" << std::endl;

				font = gui::CGUITTFont::createTTFont(m_env,
					fallback_font_path.c_str(), size, true, true,
					font_shadow, font_shadow_alpha);

				if (font) {
					m_font_cache[mode][basesize] = font;
					return;
				}
			}
		}

		// give up
		errorstream << "FontEngine: failed to load freetype font: "
				<< font_path << std::endl;
		errorstream << "minetest can not continue without a valid font. "
				"Please correct the 'font_path' setting or install the font "
				"file in the proper location" << std::endl;
		abort();
	}
#endif
}

/** initialize a font without freetype */
void FontEngine::initSimpleFont(unsigned int basesize, FontMode mode)
{
	assert(mode == FM_Simple || mode == FM_SimpleMono); // pre-condition

	std::string font_path = "";
	if (mode == FM_Simple) {
		font_path = m_settings->get("font_path");
	} else {
		font_path = m_settings->get("mono_font_path");
	}
	std::string basename = font_path;
	std::string ending = font_path.substr(font_path.length() -4);

	if (ending == ".ttf") {
		errorstream << "FontEngine: Not trying to open \"" << font_path
				<< "\" which seems to be a truetype font." << std::endl;
		return;
	}

	if ((ending == ".xml") || (ending == ".png")) {
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

	if (font) {
		font->grab();
		m_font_cache[mode][basesize] = font;
	}
}
