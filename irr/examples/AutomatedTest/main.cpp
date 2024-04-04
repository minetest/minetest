#include <iostream>
#include <irrlicht.h>
#include "exampleHelper.h"

using namespace irr;

static IrrlichtDevice *device = nullptr;
static int test_fail = 0;

void test_irr_array();
void test_irr_string();

static video::E_DRIVER_TYPE chooseDriver(core::stringc arg_)
{
	if (arg_ == "null")
		return video::EDT_NULL;
	if (arg_ == "ogles1")
		return video::EDT_OGLES1;
	if (arg_ == "ogles2")
		return video::EDT_OGLES2;
	if (arg_ == "opengl")
		return video::EDT_OPENGL;
	if (arg_ == "opengl3")
		return video::EDT_OPENGL3;
	std::cerr << "Unknown driver type: " << arg_.c_str() << ". Trying OpenGL." << std::endl;
	return video::EDT_OPENGL;
}

static inline void check(bool ok, const char *msg)
{
	if (!ok) {
		test_fail++;
		device->getLogger()->log((core::stringc("FAILED TEST: ") + msg).c_str(), ELL_ERROR);
	}
}

void run_unit_tests()
{
	std::cout << "Running unit tests:" << std::endl;
	try {
		test_irr_array();
		test_irr_string();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		test_fail++;
	}
	std::cout << std::endl;
}

int main(int argc, char *argv[])
{
	run_unit_tests();

	SIrrlichtCreationParameters p;
	p.DriverType = chooseDriver(argc > 1 ? argv[1] : "");
	p.WindowSize = core::dimension2du(640, 480);
	p.Vsync = true;
	p.LoggingLevel = ELL_DEBUG;

	device = createDeviceEx(p);
	if (!device)
		return 1;

	{
		u32 total = 0;
		device->getOSOperator()->getSystemMemory(&total, nullptr);
		core::stringc message = core::stringc("Total RAM in MiB: ") + core::stringc(total >> 10);
		device->getLogger()->log(message.c_str(), ELL_INFORMATION);
		check(total > 130 * 1024, "RAM amount");
	}

	device->setWindowCaption(L"Hello World!");
	device->setResizable(true);

	video::IVideoDriver *driver = device->getVideoDriver();
	scene::ISceneManager *smgr = device->getSceneManager();
	gui::IGUIEnvironment *guienv = device->getGUIEnvironment();

	guienv->addStaticText(L"sample text", core::rect<s32>(10, 10, 110, 22), false);

	gui::IGUIButton *button = guienv->addButton(
			core::rect<s32>(10, 30, 110, 30 + 32), 0, -1, L"sample button",
			L"sample tooltip");

	gui::IGUIEditBox *editbox = guienv->addEditBox(L"",
			core::rect<s32>(10, 70, 60, 70 + 16));

	const io::path mediaPath = getExampleMediaPath();

	auto mesh_file = device->getFileSystem()->createAndOpenFile(mediaPath + "coolguy_opt.x");
	check(mesh_file, "mesh file loading");
	scene::IAnimatedMesh *mesh = smgr->getMesh(mesh_file);
	check(mesh, "mesh loading");
	if (mesh_file)
		mesh_file->drop();
	if (mesh) {
		video::ITexture *tex = driver->getTexture(mediaPath + "cooltexture.png");
		check(tex, "texture loading");
		scene::IAnimatedMeshSceneNode *node = smgr->addAnimatedMeshSceneNode(mesh);
		if (node) {
			node->forEachMaterial([tex](video::SMaterial &mat) {
				mat.Lighting = false;
				mat.setTexture(0, tex);
			});
			node->setFrameLoop(0, 29);
			node->setAnimationSpeed(30);
		}
	}

	smgr->addCameraSceneNode(0, core::vector3df(0, 4, 5), core::vector3df(0, 2, 0));

	s32 n = 0;
	SEvent event;
	device->getTimer()->start();

	while (device->run()) {
		if (device->getTimer()->getTime() >= 1000) {
			device->getTimer()->setTime(0);
			++n;
			if (n == 1) { // Tooltip display
				bzero(&event, sizeof(SEvent));
				event.EventType = irr::EET_MOUSE_INPUT_EVENT;
				event.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
				event.MouseInput.X = button->getAbsolutePosition().getCenter().X;
				event.MouseInput.Y = button->getAbsolutePosition().getCenter().Y;
				device->postEventFromUser(event);
			} else if (n == 2) // Text input focus
				guienv->setFocus(editbox);
			else if (n == 3) { // Keypress for Text input
				bzero(&event, sizeof(SEvent));
				event.EventType = irr::EET_KEY_INPUT_EVENT;
				event.KeyInput.Char = L'a';
				event.KeyInput.Key = KEY_KEY_A;
				event.KeyInput.PressedDown = true;
				device->postEventFromUser(event);
				event.KeyInput.PressedDown = false;
				device->postEventFromUser(event);
			} else
				device->closeDevice();
		}

		driver->beginScene(video::ECBF_COLOR | video::ECBF_DEPTH,
				video::SColor(255, 100, 100, 150));
		smgr->drawAll();
		guienv->drawAll();
		driver->endScene();
	}

	check(core::stringw(L"a") == editbox->getText(), "EditBox text");

	device->getLogger()->log("Done.", ELL_INFORMATION);
	device->drop();
	return test_fail > 0 ? 1 : 0;
}
