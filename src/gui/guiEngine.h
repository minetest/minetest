/*
Minetest
Copyright (C) 2013 sapier

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

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "irrlichttypes.h"
#include "guiFormSpecMenu.h"
#include "client/sound.h"
#include "client/tile.h"
#include "util/enriched_string.h"

/******************************************************************************/
/* forward declarations                                                       */
/******************************************************************************/
class GUIEngine;
class MainMenuScripting;
class Clouds;
struct MainMenuData;

/******************************************************************************/
/* declarations                                                               */
/******************************************************************************/

/** GUIEngine specific implementation of TextDest used within guiFormSpecMenu */
class TextDestGuiEngine : public TextDest
{
public:
	/**
	 * default constructor
	 * @param engine the engine data is transmitted for further processing
	 */
	TextDestGuiEngine(GUIEngine* engine) : m_engine(engine) {};

	/**
	 * receive fields transmitted by guiFormSpecMenu
	 * @param fields map containing formspec field elements currently active
	 */
	void gotText(const StringMap &fields);

	/**
	 * receive text/events transmitted by guiFormSpecMenu
	 * @param text textual representation of event
	 */
	void gotText(const std::wstring &text);

private:
	/** target to transmit data to */
	GUIEngine *m_engine = nullptr;
};

/** GUIEngine specific implementation of ISimpleTextureSource */
class MenuTextureSource : public ISimpleTextureSource
{
public:
	/**
	 * default constructor
	 * @param driver the video driver to load textures from
	 */
	MenuTextureSource(video::IVideoDriver *driver) : m_driver(driver) {};

	/**
	 * destructor, removes all loaded textures
	 */
	virtual ~MenuTextureSource();

	/**
	 * get a texture, loading it if required
	 * @param name path to the texture
	 * @param id receives the texture ID, always 0 in this implementation
	 */
	video::ITexture *getTexture(const std::string &name, u32 *id = NULL);

private:
	/** driver to get textures from */
	video::IVideoDriver *m_driver = nullptr;
	/** set of texture names to delete */
	std::set<std::string> m_to_delete;
};

/** GUIEngine specific implementation of OnDemandSoundFetcher */
class MenuMusicFetcher: public OnDemandSoundFetcher
{
public:
	/**
	 * get sound file paths according to sound name
	 * @param name sound name
	 * @param dst_paths receives possible paths to sound files
	 * @param dst_datas receives binary sound data (not used here)
	 */
	void fetchSounds(const std::string &name,
			std::set<std::string> &dst_paths,
			std::set<std::string> &dst_datas);

private:
	/** set of fetched sound names */
	std::set<std::string> m_fetched;
};

/** implementation of main menu based uppon formspecs */
class GUIEngine {
	/** grant ModApiMainMenu access to private members */
	friend class ModApiMainMenu;
	friend class ModApiSound;

public:
	/**
	 * default constructor
	 * @param dev device to draw at
	 * @param parent parent gui element
	 * @param menumgr manager to add menus to
	 * @param smgr scene manager to add scene elements to
	 * @param data struct to transfer data to main game handling
	 */
	GUIEngine(JoystickController *joystick,
			gui::IGUIElement *parent,
			IMenuManager *menumgr,
			MainMenuData *data,
			bool &kill);

	/** default destructor */
	virtual ~GUIEngine();

	/**
	 * return MainMenuScripting interface
	 */
	MainMenuScripting *getScriptIface()
	{
		return m_script;
	}

	/**
	 * return dir of current menuscript
	 */
	std::string getScriptDir()
	{
		return m_scriptdir;
	}

	/** pass async callback to scriptengine **/
	unsigned int queueAsync(const std::string &serialized_fct,
			const std::string &serialized_params);

private:

	/** find and run the main menu script */
	bool loadMainMenuScript();

	/** run main menu loop */
	void run();

	/** update size of topleftext element */
	void updateTopLeftTextSize();

	/** parent gui element */
	gui::IGUIElement        *m_parent = nullptr;
	/** manager to add menus to */
	IMenuManager            *m_menumanager = nullptr;
	/** scene manager to add scene elements to */
	scene::ISceneManager    *m_smgr = nullptr;
	/** pointer to data beeing transfered back to main game handling */
	MainMenuData            *m_data = nullptr;
	/** pointer to texture source */
	ISimpleTextureSource    *m_texture_source = nullptr;
	/** pointer to soundmanager*/
	ISoundManager           *m_sound_manager = nullptr;

	/** representation of form source to be used in mainmenu formspec */
	FormspecFormSource      *m_formspecgui = nullptr;
	/** formspec input receiver */
	TextDestGuiEngine       *m_buttonhandler = nullptr;
	/** the formspec menu */
	GUIFormSpecMenu         *m_menu = nullptr;

	/** reference to kill variable managed by SIGINT handler */
	bool                    &m_kill;

	/** variable used to abort menu and return back to main game handling */
	bool                     m_startgame = false;

	/** scripting interface */
	MainMenuScripting       *m_script = nullptr;

	/** script basefolder */
	std::string              m_scriptdir = "";

	void setFormspecPrepend(const std::string &fs);

	/**
	 * download a file using curl
	 * @param url url to download
	 * @param target file to store to
	 */
	static bool downloadFile(const std::string &url, const std::string &target);

	/**
	 * specify text to appear as top left string
	 * @param text to set
	 */
	void setTopleftText(const std::string &text);

	/** pointer to gui element shown at topleft corner */
	irr::gui::IGUIStaticText *m_irr_toplefttext = nullptr;
	/** and text that is in it */
	EnrichedString m_toplefttext;

	/** is drawing of clouds enabled atm */
//	bool        m_clouds_enabled = true;

	/** start playing a sound and return handle */
	s32 playSound(const SimpleSoundSpec &spec, bool looped);
	/** stop playing a sound started with playSound() */
	void stopSound(s32 handle);


};
