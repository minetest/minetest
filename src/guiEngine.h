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

#ifndef GUI_ENGINE_H_
#define GUI_ENGINE_H_

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "irrlichttypes.h"
#include "modalMenu.h"
#include "guiFormSpecMenu.h"
#include "sound.h"
#include "tile.h"

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/
/** texture layer ids */
typedef enum {
	TEX_LAYER_BACKGROUND = 0,
	TEX_LAYER_OVERLAY,
	TEX_LAYER_HEADER,
	TEX_LAYER_FOOTER,
	TEX_LAYER_MAX
} texture_layer;

typedef struct {
	video::ITexture* texture;
	bool             tile;
	unsigned int     minsize;
} image_definition;

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
	TextDestGuiEngine(GUIEngine* engine);

	/**
	 * receive fields transmitted by guiFormSpecMenu
	 * @param fields map containing formspec field elements currently active
	 */
	void gotText(std::map<std::string, std::string> fields);

	/**
	 * receive text/events transmitted by guiFormSpecMenu
	 * @param text textual representation of event
	 */
	void gotText(std::wstring text);

private:
	/** target to transmit data to */
	GUIEngine* m_engine;
};

/** GUIEngine specific implementation of ISimpleTextureSource */
class MenuTextureSource : public ISimpleTextureSource
{
public:
	/**
	 * default constructor
	 * @param driver the video driver to load textures from
	 */
	MenuTextureSource(video::IVideoDriver *driver);

	/**
	 * destructor, removes all loaded textures
	 */
	virtual ~MenuTextureSource();

	/**
	 * get a texture, loading it if required
	 * @param name path to the texture
	 * @param id receives the texture ID, always 0 in this implementation
	 */
	video::ITexture* getTexture(const std::string &name, u32 *id = NULL);

private:
	/** driver to get textures from */
	video::IVideoDriver *m_driver;
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

public:
	/**
	 * default constructor
	 * @param dev device to draw at
	 * @param parent parent gui element
	 * @param menumgr manager to add menus to
	 * @param smgr scene manager to add scene elements to
	 * @param data struct to transfer data to main game handling
	 */
	GUIEngine(	irr::IrrlichtDevice* dev,
			gui::IGUIElement* parent,
			IMenuManager *menumgr,
			scene::ISceneManager* smgr,
			MainMenuData* data,
			bool& kill);

	/** default destructor */
	virtual ~GUIEngine();

	/**
	 * return MainMenuScripting interface
	 */
	MainMenuScripting* getScriptIface()
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
	unsigned int queueAsync(std::string serialized_fct,std::string serialized_params);

private:

	/** find and run the main menu script */
	bool loadMainMenuScript();

	/** run main menu loop */
	void run();

	/** handler to limit frame rate within main menu */
	void limitFrameRate();

	/** device to draw at */
	irr::IrrlichtDevice*     m_device;
	/** parent gui element */
	gui::IGUIElement*        m_parent;
	/** manager to add menus to */
	IMenuManager*            m_menumanager;
	/** scene manager to add scene elements to */
	scene::ISceneManager*    m_smgr;
	/** pointer to data beeing transfered back to main game handling */
	MainMenuData*            m_data;
	/** pointer to texture source */
	ISimpleTextureSource*    m_texture_source;
	/** pointer to soundmanager*/
	ISoundManager*           m_sound_manager;

	/** representation of form source to be used in mainmenu formspec */
	FormspecFormSource*      m_formspecgui;
	/** formspec input receiver */
	TextDestGuiEngine*       m_buttonhandler;
	/** the formspec menu */
	GUIFormSpecMenu*         m_menu;

	/** reference to kill variable managed by SIGINT handler */
	bool&                    m_kill;

	/** variable used to abort menu and return back to main game handling */
	bool                     m_startgame;

	/** scripting interface */
	MainMenuScripting*       m_script;

	/** script basefolder */
	std::string              m_scriptdir;

	/**
	 * draw background layer
	 * @param driver to use for drawing
	 */
	void drawBackground(video::IVideoDriver* driver);
	/**
	 * draw overlay layer
	 * @param driver to use for drawing
	 */
	void drawOverlay(video::IVideoDriver* driver);
	/**
	 * draw header layer
	 * @param driver to use for drawing
	 */
	void drawHeader(video::IVideoDriver* driver);
	/**
	 * draw footer layer
	 * @param driver to use for drawing
	 */
	void drawFooter(video::IVideoDriver* driver);

	/**
	 * load a texture for a specified layer
	 * @param layer draw layer to specify texture
	 * @param texturepath full path of texture to load
	 */
	bool setTexture(texture_layer layer, std::string texturepath,
			bool tile_image, unsigned int minsize);

	/**
	 * download a file using curl
	 * @param url url to download
	 * @param target file to store to
	 */
	static bool downloadFile(std::string url,std::string target);

	/** array containing pointers to current specified texture layers */
	image_definition m_textures[TEX_LAYER_MAX];

	/** draw version string in topleft corner */
	void drawVersion();

	/**
	 * specify text to be appended to version string
	 * @param text to set
	 */
	void setTopleftText(std::string append);

	/** pointer to gui element shown at topleft corner */
	irr::gui::IGUIStaticText*	m_irr_toplefttext;

	/** initialize cloud subsystem */
	void cloudInit();
	/** do preprocessing for cloud subsystem */
	void cloudPreProcess();
	/** do postprocessing for cloud subsystem */
	void cloudPostProcess();

	/** internam data required for drawing clouds */
	struct clouddata {
		/** delta time since last cloud processing */
		f32     dtime;
		/** absolute time of last cloud processing */
		u32     lasttime;
		/** pointer to cloud class */
		Clouds* clouds;
		/** camera required for drawing clouds */
		scene::ICameraSceneNode* camera;
	};

	/** is drawing of clouds enabled atm */
	bool        m_clouds_enabled;
	/** data used to draw clouds */
	clouddata   m_cloud;

	/** start playing a sound and return handle */
	s32 playSound(SimpleSoundSpec spec, bool looped);
	/** stop playing a sound started with playSound() */
	void stopSound(s32 handle);


};



#endif /* GUI_ENGINE_H_ */
