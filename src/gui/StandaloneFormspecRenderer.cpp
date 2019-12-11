#include "StandaloneFormspecRenderer.h"

#include <iostream>

#include "porting.h"
#include "client/renderingengine.h"
#include "settings.h"

// Anonymous namespace
namespace
{

class MockTextDest : public TextDest
{
public:
	/**
	 * receive fields transmitted by guiFormSpecMenu
	 * @param fields map containing formspec field elements currently active
	 */
	void gotText(const StringMap &fields) override {}

	/**
	 * receive text/events transmitted by guiFormSpecMenu
	 * @param text textual representation of event
	 */
	void gotText(const std::wstring &text) override {}
};

class MockMenuManager : public IMenuManager {
	void createdMenu(gui::IGUIElement *menu) override {}
	void deletingMenu(gui::IGUIElement *menu) override {}
};

} // End anonymous namespace


StandaloneFormspecRenderer::StandaloneFormspecRenderer()
{
	// Deleted by formspec menu
	m_buttonhandler = new MockTextDest();

	m_texture_source = new MenuTextureSource(RenderingEngine::get_video_driver());
	m_formspecgui = new FormspecFormSource("");
	menuManager = new MockMenuManager();

	root = RenderingEngine::get_gui_env()->addStaticText(
			L"", core::rect<s32>(0, 0, 10000, 10000));

	/* Create menu */
	m_menu = new GUIFormSpecMenu(nullptr, root, -1, menuManager, NULL /* &client */,
			m_texture_source, m_formspecgui, m_buttonhandler, "", false);

	m_menu->allowClose(false);
	m_menu->lockSize(true, v2u32(800,600));
}

StandaloneFormspecRenderer::~StandaloneFormspecRenderer()
{
	// TODO: delete things

	// delete m_menu;
	// delete root;
	// delete menuManager;
	// delete m_formspecgui;
	// delete m_texture_source;
}

bool StandaloneFormspecRenderer::renderPath(
		const std::string &path, const std::string &outputPath)
{
	std::ifstream stream(path);
	if (!stream.good()) {
		errorstream << "Unable to read from path: " << path << std::endl;
		return false;
	}

	std::string source((std::istreambuf_iterator<char>(stream)),
			std::istreambuf_iterator<char>());

	return renderSource(source, outputPath);
}

bool takeScreenshot(video::IVideoDriver *driver, const std::string &outputPath)
{
	irr::video::IImage *const raw_image = driver->createScreenShot();
	if (!raw_image) {
		errorstream << "Unable to take screenshot" << std::endl;
		return false;
	}

	irr::video::IImage *const image =
			driver->createImage(video::ECF_R8G8B8, raw_image->getDimension());
	if (!image) {
		errorstream << "Unable to convert screenshot to image" << std::endl;
		return false;
	}

	raw_image->copyTo(image);

	u32 quality = (u32)g_settings->getS32("screenshot_quality");
	quality = MYMIN(MYMAX(quality, 0), 100) / 100.0 * 255;

	if (driver->writeImageToFile(image, outputPath.c_str(), quality)) {
		infostream << "Saved screenshot to '" << outputPath << "'";
	} else {
		errorstream << "Failed to save screenshot '" << outputPath << "'";
		return false;
	}

	image->drop();

	return true;
}

bool StandaloneFormspecRenderer::renderSource(
		const std::string &source, const std::string &outputPath)
{
	m_formspecgui->setForm(source);

	bool *kill = porting::signal_handler_killstatus();

	const video::SColor sky_color(255, 140, 186, 250);
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

	assert(RenderingEngine::run() && !(*kill));

	driver->beginScene(true, true, sky_color);
	RenderingEngine::get_gui_env()->drawAll();
	driver->endScene();

	return takeScreenshot(driver, outputPath);
}
