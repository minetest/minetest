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

#pragma once

#include <string>

class Clouds;
class MenuTextureSource;
namespace irr{namespace scene{class ISceneManager;}}

struct TextureDefinition {
	video::ITexture *texture = nullptr;
	bool             tile;
	unsigned int     minsize;
};

class StartupScreen {
public:
	StartupScreen();
	~StartupScreen();

	enum class Background {
		COLOR,   // TO
		TEXTURE, // BE
		SKY      // COMMENTED
	};

	enum class Texture {
		BACKGROUND = 0, //
		OVERLAY,        // TO
		HEADER,         // BE
		FOOTER,         // COMMENTED
		PROGRESS_FG,    //
		PROGRESS_BG,    //
		Count
	};

	enum class Color {
		BACKGROUND = 0, //
		SKY,            // TO
		CLOUDS,         // BE
		MESSAGE,        // COMMENTED
		Count           //
	};

	// Startup screen customization
	void setBackgroundType(Background type);
	bool setTexture(Texture type, const std::string &texturepath,
			bool tile_image = false, unsigned int minsize = 0);
	inline void setColor(Color type, const video::SColor &color)
		{ m_colors[static_cast<int>(type)] = color; };

	// Loading message and progress management
	void setMessage(const wchar_t *msg, int percent);
	void setMessage(const char *msg, int percent);
	inline void clearMessage() { setMessage(L"", -1); };

	// Step : draw and eventualy limit FPS
	void step(bool limitFps);

protected:
	// Access to customizations
	inline const TextureDefinition& getTexture(Texture type) 
		{ return m_textures[static_cast<int>(type)]; };
	inline const video::SColor& getColor(Color type)
		{ return m_colors[static_cast<int>(type)]; };

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
	Background        m_background_type = Background::COLOR;
	TextureDefinition m_textures[static_cast<int>(Texture::Count)];
	video::SColor     m_colors[static_cast<int>(Color::Count)];

	// Internal variables
	video::IVideoDriver *m_driver;
	MenuTextureSource *m_tsrc;

	v2u32 m_screensize;
	u32 m_last_time;
};
