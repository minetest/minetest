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
#include "clouds.h"
#include "guiLuaApi.h"
#include "guiFormSpecMenu.h"

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/
#define MAX_MENUBAR_BTN_COUNT	10
#define MAX_MENUBAR_BTN_ID		256
#define MIN_MENUBAR_BTN_ID		(MAX_MENUBAR_BTN_ID - MAX_MENUBAR_BTN_COUNT)

/** texture layer ids */
typedef enum {
	TEX_LAYER_BACKGROUND = 0,
	TEX_LAYER_OVERLAY,
	TEX_LAYER_HEADER,
	TEX_LAYER_FOOTER,
	TEX_LAYER_MAX
} texture_layer;

/******************************************************************************/
/* forward declarations                                                       */
/******************************************************************************/
class GUIEngine;
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


/** implementation of main menu based uppon formspecs */
class GUIEngine {
public:
	/** TextDestGuiEngine needs to transfer data to engine */
	friend class TextDestGuiEngine;
	/** guiLuaApi is used to initialize contained stack */
	friend class guiLuaApi;

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
				MainMenuData* data);

	/** default destructor */
	virtual ~GUIEngine();

protected:
	/**
	 * process field data recieved from formspec
	 * @param fields data in field format
	 */
	void handleButtons(std::map<std::string, std::string> fields);
	/**
	 * process events received from formspec
	 * @param text events in textual form
	 */
	void handleEvent(std::string text);

	/**
	 * return dir of current menuscript
	 */
	std::string getScriptDir() {
		return m_scriptdir;
	}

private:

	/* run main menu loop */
	void run();

	/** handler to limit frame rate within main menu */
	void limitFrameRate();

	/** device to draw at */
	irr::IrrlichtDevice*	m_device;
	/** parent gui element */
	gui::IGUIElement*		m_parent;
	/** manager to add menus to */
	IMenuManager*			m_menumanager;
	/** scene manager to add scene elements to */
	scene::ISceneManager*	m_smgr;
	/** pointer to data beeing transfered back to main game handling */
	MainMenuData*			m_data;

	/** representation of form source to be used in mainmenu formspec */
	FormspecFormSource*		m_formspecgui;
	/** formspec input receiver */
	TextDestGuiEngine*		m_buttonhandler;
	/** the formspec menu */
	GUIFormSpecMenu*		m_menu;

	/** variable used to abort menu and return back to main game handling */
	bool					m_startgame;

	/**
	 * initialize lua stack
	 * @param L stack to initialize
	 */
	void initalize_api(lua_State * L);

	/**
	 * run a lua script
	 * @param script full path to script to run
	 */
	bool runScript(std::string script);

	/**
	 * script error handler to process errors within lua
	 */
	void scriptError(const char *fmt, ...);

	/** lua stack */
	lua_State*				m_engineluastack;
	/** lua internal stack number of error handler*/
	int						m_luaerrorhandler;

	/** script basefolder */
	std::string				m_scriptdir;

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
	bool setTexture(texture_layer layer,std::string texturepath);

	/**
	 * download a file using curl
	 * @param url url to download
	 * @param target file to store to
	 */
	bool downloadFile(std::string url,std::string target);

	/** array containing pointers to current specified texture layers */
	video::ITexture*		m_textures[TEX_LAYER_MAX];

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
		f32		dtime;
		/** absolute time of last cloud processing */
		u32		lasttime;
		/** pointer to cloud class */
		Clouds*	clouds;
		/** camera required for drawing clouds */
		scene::ICameraSceneNode* camera;
	};

	/** is drawing of clouds enabled atm */
	bool					m_clouds_enabled;
	/** data used to draw clouds */
	clouddata 				m_cloud;

};



#endif /* GUI_ENGINE_H_ */
