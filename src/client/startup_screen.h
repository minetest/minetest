/*
Minetest
Copyright (C) 2020 Pierre-Yves Rollo <dev@pyrollo.com>

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

#include <string>

class Clouds;
class MenuTextureSource;
namespace irr{namespace scene{class ISceneManager;}}

class StartupScreen;
extern StartupScreen *g_startup_screen;

typedef struct {
	video::ITexture *texture = nullptr;
	bool             tile;
	unsigned int     minsize;
} image_definition;

class StartupScreen {
public:
	StartupScreen();
	~StartupScreen();

	enum backgroundType {
		BT_COLOR,
		BT_TEXTURE,
		BT_SKY
	};

	enum textureLayer {
		TEX_LAYER_BACKGROUND = 0,
		TEX_LAYER_OVERLAY,
		TEX_LAYER_HEADER,
		TEX_LAYER_FOOTER,
		TEX_LAYER_PROGRESS_FG,
		TEX_LAYER_PROGRESS_BG,
		TEX_LAYER_MAX
	};

	enum colorType {
		COLOR_BACKGROUND = 0,
		COLOR_SKY,
		COLOR_CLOUDS,
		COLOR_MESSAGE,
		COLOR_MAX
	};

	// Startup screen customization
	void setBackgroundType(backgroundType background_type);
	bool setTexture(textureLayer layer, const std::string &texturepath,
			bool tile_image = false, unsigned int minsize = 0);
	inline void setColor(colorType type, const video::SColor &color) { m_colors[type] = color; };

	// Loading message and progress management
	void setMessage(const wchar_t *msg, int percent);
	void setMessage(const char *msg, int percent);
	inline void clearMessage() { setMessage(L"", -1); };

	// Step : draw and eventualy limit FPS
	void step(bool limitFps);

protected:

	// Clouds scene
	void startupClouds();
	void shutdownClouds();
	void drawClouds(float dtime);

	Clouds *m_clouds = nullptr;
	irr::scene::ISceneManager *m_cloudsmgr = nullptr;

	// Progress bar stuff
	void drawProgressBar();

	std::wstring m_message;
	int m_percent = -1;

	// Other internal drawings
	void drawBackgroundTexture();
	void drawOverlay();
	void drawHeader();
	void drawFooter();

	void drawAll(float dtime);

	void setWindowsCaption();

	// Background type, custom textures and colors
	backgroundType m_background_type = BT_COLOR;
	image_definition m_textures[TEX_LAYER_MAX];
	video::SColor m_colors[COLOR_MAX];

	// Internal variables
	video::IVideoDriver *m_driver;
	MenuTextureSource *m_tsrc;

	v2u32 m_screensize;
	u32 m_last_time;
};
