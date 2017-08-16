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
	screensize = driver->getScreenSize();
}

void RenderingCore::update_screen_size()
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

	if (screensize != _screensize) {
		screensize = _screensize;
		update_screen_size();
	}
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
	parallax_strength = g_settings->getFloat("3d_paralax_strength");
}

void RenderingCoreStereo::pre_draw()
{
	cam = camera->getCameraNode();
	base_transform = cam->getRelativeTransformation();
}

void RenderingCoreStereo::use_eye(bool right)
{
	irr::core::matrix4 move;
	move.setTranslation(irr::core::vector3df(right ? parallax_strength : -parallax_strength, 0.0f, 0.0f));
	cam->setPosition((base_transform * move).getTranslation());
}

void RenderingCoreStereo::reset_eye()
{
	cam->setPosition(base_transform.getTranslation());
}

void RenderingCoreStereo::draw_two()
{
	use_eye(false);
	draw_3d();
	reset_eye();
	use_eye(true);
	draw_3d();
	reset_eye();
}

RenderingCoreAnaglyph::RenderingCoreAnaglyph(irr::IrrlichtDevice *_device) :
	RenderingCoreStereo(_device)
{
}

void RenderingCoreAnaglyph::draw()
{
	pre_draw();
	draw_two();
	draw_last_fx();
	draw_hud();
}

void RenderingCoreAnaglyph::use_eye(bool right)
{
	RenderingCoreStereo::use_eye(right);
	driver->clearZBuffer();
	irr::video::SOverrideMaterial &mat = driver->getOverrideMaterial();
	mat.Material.ColorMask = right ? irr::video::ECP_GREEN | irr::video::ECP_BLUE : irr::video::ECP_RED;
	mat.EnableFlags = irr::video::EMF_COLOR_MASK;
	mat.EnablePasses =
			irr::scene::ESNRP_SKY_BOX | irr::scene::ESNRP_SOLID |
			irr::scene::ESNRP_TRANSPARENT |
			irr::scene::ESNRP_TRANSPARENT_EFFECT | irr::scene::ESNRP_SHADOW;
}

void RenderingCoreAnaglyph::reset_eye()
{
	irr::video::SOverrideMaterial &mat = driver->getOverrideMaterial();
	mat.Material.ColorMask = irr::video::ECP_ALL;
	mat.EnableFlags = irr::video::EMF_COLOR_MASK;
	mat.EnablePasses =
			irr::scene::ESNRP_SKY_BOX + irr::scene::ESNRP_SOLID +
			irr::scene::ESNRP_TRANSPARENT +
			irr::scene::ESNRP_TRANSPARENT_EFFECT + irr::scene::ESNRP_SHADOW;
}

RenderingCoreSideBySide::RenderingCoreSideBySide(irr::IrrlichtDevice *_device) :
	RenderingCoreStereo(_device)
{
	init_textures();
}

RenderingCoreSideBySide::~RenderingCoreSideBySide()
{
	clear_textures();
}

void RenderingCoreSideBySide::init_textures()
{
	image_size = v2u32(screensize.X / 2, screensize.Y);
	left = driver->addRenderTargetTexture(image_size, "3d_render_left", irr::video::ECF_A8R8G8B8);
	right = driver->addRenderTargetTexture(image_size, "3d_render_right", irr::video::ECF_A8R8G8B8);
	hud = driver->addRenderTargetTexture(screensize, "3d_render_hud", irr::video::ECF_A8R8G8B8);
}

void RenderingCoreSideBySide::clear_textures()
{
	driver->removeTexture(left);
	driver->removeTexture(right);
	driver->removeTexture(hud);
}

void RenderingCoreSideBySide::update_screen_size()
{
	clear_textures();
	init_textures();
}

void RenderingCoreSideBySide::draw()
{
	pre_draw();
	draw_two();
	driver->setRenderTarget(hud, true, true, video::SColor(0, 0, 0, 0));
	draw_hud();
	driver->setRenderTarget(nullptr, false, false, skycolor);

	driver->draw2DImage(left, v2s32(0, 0));
	driver->draw2DImage(right, v2s32(screensize.X / 2, 0));

	driver->draw2DImage(hud,
			irr::core::rect<s32>(0, 0, screensize.X / 2, screensize.Y),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			true);

	driver->draw2DImage(hud,
			irr::core::rect<s32>(screensize.X / 2, 0, screensize.X, screensize.Y),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			true);
}

void RenderingCoreSideBySide::use_eye(bool _right)
{
	driver->setRenderTarget(_right ? right : left, true, true, skycolor);
	RenderingCoreStereo::use_eye(_right);
}

void RenderingCoreSideBySide::reset_eye()
{
	driver->setRenderTarget(nullptr, false, false, skycolor);
	RenderingCoreStereo::reset_eye();
}

RenderingCorePageflip::RenderingCorePageflip(irr::IrrlichtDevice *_device) :
	RenderingCoreStereo(_device)
{
	init_textures();
}

RenderingCorePageflip::~RenderingCorePageflip()
{
	clear_textures();
}

void RenderingCorePageflip::init_textures()
{
	hud = driver->addRenderTargetTexture(screensize, "3d_render_hud", irr::video::ECF_A8R8G8B8);
}

void RenderingCorePageflip::clear_textures()
{
	driver->removeTexture(hud);
}

void RenderingCorePageflip::update_screen_size()
{
	clear_textures();
	init_textures();
}

void RenderingCorePageflip::draw()
{
	pre_draw();
	driver->setRenderTarget(hud, true, true, video::SColor(0, 0, 0, 0));
	draw_hud();
	driver->setRenderTarget(nullptr, false, false, skycolor);
	draw_two();
}

void RenderingCorePageflip::use_eye(bool _right)
{
	driver->setRenderTarget(
		_right ? irr::video::ERT_STEREO_RIGHT_BUFFER : irr::video::ERT_STEREO_LEFT_BUFFER,
		 true, true, skycolor);
	RenderingCoreStereo::use_eye(_right);
}

void RenderingCorePageflip::reset_eye()
{
	driver->draw2DImage(hud, v2s32(0, 0));
	driver->setRenderTarget(irr::video::ERT_FRAME_BUFFER, false, false, skycolor);
	RenderingCoreStereo::reset_eye();
}

RenderingCoreInterlaced::RenderingCoreInterlaced(irr::IrrlichtDevice *_device) :
	RenderingCoreStereo(_device)
{
	mat.MaterialType = (video::E_MATERIAL_TYPE)0;
	init_textures();
}

RenderingCoreInterlaced::~RenderingCoreInterlaced()
{
	clear_textures();
}

void RenderingCoreInterlaced::init_material()
{
	if (mat.MaterialType)
		return;
	IShaderSource *s = client->getShaderSource();
	mat.UseMipMaps = false;
	mat.ZBuffer = false;
	mat.ZWriteEnable = false;
	u32 shader = s->getShader("3d_interlaced_merge", TILE_MATERIAL_BASIC, 0);
	mat.MaterialType = s->getShaderInfo(shader).material;
	for (int k = 0; k != 3; ++k) {
		mat.TextureLayer[k].AnisotropicFilter = false;
		mat.TextureLayer[k].BilinearFilter = false;
		mat.TextureLayer[k].TrilinearFilter = false;
		mat.TextureLayer[k].TextureWrapU = irr::video::ETC_CLAMP_TO_EDGE;
		mat.TextureLayer[k].TextureWrapV = irr::video::ETC_CLAMP_TO_EDGE;
	}
}

void RenderingCoreInterlaced::init_textures()
{
	image_size = v2u32(screensize.X, screensize.Y / 2);
	left = driver->addRenderTargetTexture(image_size, "3d_render_left", irr::video::ECF_A8R8G8B8);
	right = driver->addRenderTargetTexture(image_size, "3d_render_right", irr::video::ECF_A8R8G8B8);
	mask = driver->addTexture(screensize, "3d_render_mask", irr::video::ECF_A8R8G8B8);
	init_mask();
	mat.TextureLayer[0].Texture = left;
	mat.TextureLayer[1].Texture = right;
	mat.TextureLayer[2].Texture = mask;
}

void RenderingCoreInterlaced::clear_textures()
{
	driver->removeTexture(left);
	driver->removeTexture(right);
	driver->removeTexture(mask);
}

void RenderingCoreInterlaced::init_mask()
{
	u8 *data = reinterpret_cast<u8 *>(mask->lock());
	for (unsigned j = 0; j < screensize.Y; j++) {
		u8 val = j % 2 ? 0xff : 0x00;
		memset(data, val, 4 * screensize.X);
		data += 4 * screensize.X;
	}
	mask->unlock();
}

void RenderingCoreInterlaced::update_screen_size()
{
	clear_textures();
	init_textures();
}

void RenderingCoreInterlaced::draw()
{
	pre_draw();
	draw_two();
	merge();
	draw_hud();
}

void RenderingCoreInterlaced::merge()
{
	static const video::S3DVertex vertices[4] = {
		video::S3DVertex(1.0, -1.0, 0.0, 0.0, 0.0, -1.0, video::SColor(255, 0, 255, 255), 1.0, 0.0),
		video::S3DVertex(-1.0, -1.0, 0.0, 0.0, 0.0, -1.0, video::SColor(255, 255, 0, 255), 0.0, 0.0),
		video::S3DVertex(-1.0, 1.0, 0.0, 0.0, 0.0, -1.0, video::SColor(255, 255, 255, 0), 0.0, 1.0),
		video::S3DVertex(1.0, 1.0, 0.0, 0.0, 0.0, -1.0, video::SColor(255, 255, 255, 255), 1.0, 1.0),
	};
	static const u16 indices[6] = { 0, 1, 2, 2, 3, 0 };
	init_material();
	driver->setMaterial(mat);
	driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);
}

void RenderingCoreInterlaced::use_eye(bool _right)
{
	driver->setRenderTarget(_right ? right : left, true, true, skycolor);
	RenderingCoreStereo::use_eye(_right);
}

void RenderingCoreInterlaced::reset_eye()
{
	driver->setRenderTarget(nullptr, false, false, skycolor);
	RenderingCoreStereo::reset_eye();
}
