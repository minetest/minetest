#include "renderingcore.h"
#include "camera.h"
#include "client.h"
#include "clientmap.h"
#include "hud.h"
#include "localplayer.h"
#include "minimap.h"
#include "settings.h"

RenderingCore::RenderingCore(irr::IrrlichtDevice *_device) :
	device(_device),
	driver(device->getVideoDriver()),
	smgr(device->getSceneManager())
{
}

void RenderingCore::setup(Camera *_camera, Client *_client, LocalPlayer *_player,
		  Hud *_hud, Minimap *_mapper, gui::IGUIEnvironment *_guienv,
		  const v2u32 &_screensize, const video::SColor &_skycolor,
		  bool _show_hud, bool _show_minimap)
{
	camera = _camera;
	client = _client;
	player = _player;
	hud = _hud;
	mapper = _mapper;
	guienv = _guienv;

	screensize = _screensize;
	skycolor = _skycolor;
	show_hud = _show_hud;
	show_minimap = _show_minimap;

	draw_wield_tool = (show_hud && (player->hud_flags & HUD_FLAG_WIELDITEM_VISIBLE) &&
			camera->getCameraMode() < CAMERA_MODE_THIRD);

	draw_crosshair = ((player->hud_flags & HUD_FLAG_CROSSHAIR_VISIBLE) &&
			(camera->getCameraMode() != CAMERA_MODE_THIRD_FRONT));
#ifdef HAVE_TOUCHSCREENGUI
	try {
		draw_crosshair = !g_settings->getBool("touchtarget");
	} catch (SettingNotFoundException) {
	}
#endif
}

void RenderingCore::draw_3d()
{
	smgr->drawAll();
	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (!show_hud)
		return;
	hud->drawSelectionMesh();
	if (draw_wield_tool)
		camera->drawWieldedTool();
}

void RenderingCore::draw_hud()
{
	if (!show_hud)
		return;
	if (draw_crosshair)
		hud->drawCrosshair();
	hud->drawHotbar(client->getPlayerItem());
	hud->drawLuaElements(camera->getOffset());
	camera->drawNametags();
	if (mapper && show_minimap)
		mapper->drawMinimap();
	guienv->drawAll();
}

void RenderingCore::draw_last_fx()
{
	client->getEnv().getClientMap().renderPostFx(camera->getCameraMode());
}

RenderingCorePlain::RenderingCorePlain(irr::IrrlichtDevice *_device) :
	RenderingCore(_device)
{
}

void RenderingCorePlain::draw()
{
// TODO prepare undersamped rendering
	draw_3d();
	draw_last_fx();
// TODO upscale undersampled render
	draw_hud();
}

RenderingCoreStereo::RenderingCoreStereo(irr::IrrlichtDevice *_device) :
	RenderingCore(_device)
{
}

RenderingCoreAnaglyph::RenderingCoreAnaglyph(irr::IrrlichtDevice *_device) :
	RenderingCoreStereo(_device)
{
}

void RenderingCoreAnaglyph::draw()
{
}

RenderingCoreDouble::RenderingCoreDouble(irr::IrrlichtDevice *_device, RenderingCoreDouble::Mode _mode) :
	RenderingCoreStereo(_device)
{
}

void RenderingCoreDouble::draw()
{
}

RenderingCoreInterlaced::RenderingCoreInterlaced(irr::IrrlichtDevice *_device) :
	RenderingCoreStereo(_device)
{
}

void RenderingCoreInterlaced::draw()
{
}
