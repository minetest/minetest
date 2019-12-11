#pragma once

#ifdef __ANDROID__
#error formspectestexecutor.h should not be included on Android
#endif

#include <string>
#include "guiFormSpecMenu.h"
#include "guiEngine.h"

class StandaloneFormspecRenderer
{
	ISimpleTextureSource *m_texture_source;
	FormspecFormSource *m_formspecgui;
	TextDest *m_buttonhandler;
	IMenuManager *menuManager;
	IGUIElement *root;
	GUIFormSpecMenu *m_menu;

public:
	StandaloneFormspecRenderer();
	~StandaloneFormspecRenderer();

	/// @param path Path to a file containing formspec code
	/// @param outputPath Where to save the screenshot
	bool renderPath(const std::string &testPath, const std::string &outputPath);

	/// @param source The formspec source
	/// @param outputPath Where to save the screenshot
	bool renderSource(const std::string &source, const std::string &outputPath);
};
