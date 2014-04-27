/*
Minetest
Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "drawscene.h"
#include "main.h" // for g_settings
#include "settings.h"
#include "util/timetaker.h"

void draw_selectionbox(video::IVideoDriver* driver, Hud& hud,
		std::vector<aabb3f>& hilightboxes, bool show_hud)
{
	if (!show_hud)
		return;

	video::SMaterial oldmaterial = driver->getMaterial2D();
	video::SMaterial m;
	m.Thickness = 3;
	m.Lighting = false;
	driver->setMaterial(m);
	hud.drawSelectionBoxes(hilightboxes);
	driver->setMaterial(oldmaterial);
}

void draw_anaglyph_3d_mode(Camera& camera, bool show_hud, Hud hud,
		std::vector<aabb3f> hilightboxes, video::IVideoDriver* driver,
		scene::ISceneManager* smgr, bool draw_wield_tool, Client& client,
		gui::IGUIEnvironment* guienv )
{

	/* preserve old setup*/
	irr::core::vector3df oldPosition = camera.getCameraNode()->getPosition();
	irr::core::vector3df oldTarget   = camera.getCameraNode()->getTarget();

	irr::core::matrix4 startMatrix =
			camera.getCameraNode()->getAbsoluteTransformation();
	irr::core::vector3df focusPoint = (camera.getCameraNode()->getTarget()
			- camera.getCameraNode()->getAbsolutePosition()).setLength(1)
			+ camera.getCameraNode()->getAbsolutePosition();


	//Left eye...
	irr::core::vector3df leftEye;
	irr::core::matrix4 leftMove;
	leftMove.setTranslation(
			irr::core::vector3df(-g_settings->getFloat("3d_paralax_strength"),
					0.0f, 0.0f));
	leftEye = (startMatrix * leftMove).getTranslation();

	//clear the depth buffer, and color
	driver->beginScene( true, true, irr::video::SColor(0, 200, 200, 255));
	driver->getOverrideMaterial().Material.ColorMask = irr::video::ECP_RED;
	driver->getOverrideMaterial().EnableFlags = irr::video::EMF_COLOR_MASK;
	driver->getOverrideMaterial().EnablePasses = irr::scene::ESNRP_SKY_BOX
			+ irr::scene::ESNRP_SOLID + irr::scene::ESNRP_TRANSPARENT
			+ irr::scene::ESNRP_TRANSPARENT_EFFECT + irr::scene::ESNRP_SHADOW;
	camera.getCameraNode()->setPosition(leftEye);
	camera.getCameraNode()->setTarget(focusPoint);
	smgr->drawAll();
	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (show_hud)
	{
		draw_selectionbox(driver, hud, hilightboxes, show_hud);

		if (draw_wield_tool)
			camera.drawWieldedTool(&leftMove);
	}

	guienv->drawAll();

	//Right eye...
	irr::core::vector3df rightEye;
	irr::core::matrix4 rightMove;
	rightMove.setTranslation(
			irr::core::vector3df(g_settings->getFloat("3d_paralax_strength"),
					0.0f, 0.0f));
	rightEye = (startMatrix * rightMove).getTranslation();

	//clear the depth buffer
	driver->clearZBuffer();
	driver->getOverrideMaterial().Material.ColorMask = irr::video::ECP_GREEN
			+ irr::video::ECP_BLUE;
	driver->getOverrideMaterial().EnableFlags = irr::video::EMF_COLOR_MASK;
	driver->getOverrideMaterial().EnablePasses = irr::scene::ESNRP_SKY_BOX
			+ irr::scene::ESNRP_SOLID + irr::scene::ESNRP_TRANSPARENT
			+ irr::scene::ESNRP_TRANSPARENT_EFFECT + irr::scene::ESNRP_SHADOW;
	camera.getCameraNode()->setPosition(rightEye);
	camera.getCameraNode()->setTarget(focusPoint);
	smgr->drawAll();
	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (show_hud)
	{
		draw_selectionbox(driver, hud, hilightboxes, show_hud);

		if (draw_wield_tool)
			camera.drawWieldedTool(&rightMove);
	}

	guienv->drawAll();

	driver->getOverrideMaterial().Material.ColorMask = irr::video::ECP_ALL;
	driver->getOverrideMaterial().EnableFlags = 0;
	driver->getOverrideMaterial().EnablePasses = 0;
	camera.getCameraNode()->setPosition(oldPosition);
	camera.getCameraNode()->setTarget(oldTarget);
}

video::ITexture* update_line_masks(const v2u32& screensize,
		video::IVideoDriver* driver, video::ITexture*& even_mask,
		video::ITexture*& odd_mask)
{
	static v2u32 last_screen_size = v2u32(0, 0);

	/* prepare masks */
	if (last_screen_size != screensize) {
		if (even_mask != 0) {
			driver->removeTexture(even_mask);
		}
		if (odd_mask != 0) {
			driver->removeTexture(odd_mask);
		}
		last_screen_size = screensize;

		/* create new even line mask */
		even_mask = driver->addRenderTargetTexture(last_screen_size,
				"even_mask", irr::video::ECF_A8R8G8B8);
		driver->setRenderTarget(even_mask, true, true,
				irr::video::SColor(0, 0, 0, 0));
		for (unsigned int i = 0; i < screensize.Y; i += 2) {
			driver->draw2DLine(irr::core::vector2d<int>(0, i),
					irr::core::vector2d<int>(screensize.X, i),
					irr::video::SColor(255, 0, 0, 0));
		}

		/* create new odd line mask */
		odd_mask = driver->addRenderTargetTexture(last_screen_size, "odd_mask",
				irr::video::ECF_A8R8G8B8);
		driver->setRenderTarget(odd_mask, true, true,
				irr::video::SColor(0, 0, 0, 0));
		for (unsigned int i = 1; i < screensize.Y; i += 2) {
			driver->draw2DLine(irr::core::vector2d<int>(0, i),
					irr::core::vector2d<int>(screensize.X, i),
					irr::video::SColor(255, 0, 0, 0));
		}
	}
	return even_mask;
}

video::ITexture* draw_left_eye_image(const v2u32& screensize,
		const irr::core::matrix4& startMatrix,
		const irr::core::vector3df& focusPoint, bool show_hud,
		video::IVideoDriver* driver, Camera& camera, scene::ISceneManager* smgr,
		Hud& hud, std::vector<aabb3f>& hilightboxes, video::ITexture*& odd_mask,
		bool draw_wield_tool, Client& client, gui::IGUIEnvironment* guienv )
{
	static v2u32 last_screensize = v2u32(0,0);
	static video::ITexture* image = NULL;


	if (( image == NULL ) || (screensize != last_screensize)) {
		if (image != NULL) {
			driver->removeTexture(image);
		}
		image = driver->addRenderTargetTexture(
				core::dimension2d<u32>(screensize.X, screensize.Y));
		last_screensize = screensize;
	}
	driver->setRenderTarget(image, true, true,
			irr::video::SColor(255, 200, 200, 255));

	irr::core::vector3df leftEye;
	irr::core::matrix4 leftMove;
	leftMove.setTranslation(
			irr::core::vector3df(-g_settings->getFloat("3d_paralax_strength"),
					0.0f, 0.0f));
	leftEye = (startMatrix * leftMove).getTranslation();

	//clear the depth buffer
	driver->clearZBuffer();
	camera.getCameraNode()->setPosition(leftEye);
	camera.getCameraNode()->setTarget(focusPoint);
	smgr->drawAll();

	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);

	if (show_hud)
	{
		draw_selectionbox(driver, hud, hilightboxes, show_hud);
		camera.drawWieldedTool(&leftMove);
	}

	guienv->drawAll();

	/* mask odd lines */
	driver->draw2DImage(odd_mask, irr::core::position2d<s32>(0, 0),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0,
			irr::video::SColor(255, 255, 255, 255),
			true);

	/* switch back to real renderer */
	driver->setRenderTarget(0, true, true,
			irr::video::SColor(0, 200, 200, 255));

	return image;
}

void draw_odd_even_3d_mode(Camera& camera, bool show_hud,
		Hud hud, std::vector<aabb3f> hilightboxes, video::IVideoDriver* driver,
		scene::ISceneManager* smgr, const v2u32& screensize,
		bool draw_wield_tool, Client& client, gui::IGUIEnvironment* guienv )
{
	static video::ITexture* odd_mask = 0;
	static video::ITexture* even_mask = 0;

	/* prepare masks */
	even_mask = update_line_masks(screensize, driver, even_mask, odd_mask);

	/* save current info */
	irr::core::vector3df oldPosition = camera.getCameraNode()->getPosition();
	irr::core::vector3df oldTarget = camera.getCameraNode()->getTarget();
	irr::core::matrix4 startMatrix =
			camera.getCameraNode()->getAbsoluteTransformation();
	irr::core::vector3df focusPoint = (camera.getCameraNode()->getTarget()
			- camera.getCameraNode()->getAbsolutePosition()).setLength(1)
			+ camera.getCameraNode()->getAbsolutePosition();

	/* create left view */
	video::ITexture* left_image = draw_left_eye_image(screensize, startMatrix,
			focusPoint, show_hud, driver, camera, smgr, hud, hilightboxes,
			odd_mask, draw_wield_tool, client, guienv);

	//Right eye...
	irr::core::vector3df rightEye;
	irr::core::matrix4 rightMove;
	rightMove.setTranslation(
			irr::core::vector3df(g_settings->getFloat("3d_paralax_strength"),
					0.0f, 0.0f));
	rightEye = (startMatrix * rightMove).getTranslation();

	//clear the depth buffer
	driver->clearZBuffer();
	camera.getCameraNode()->setPosition(rightEye);
	camera.getCameraNode()->setTarget(focusPoint);
	smgr->drawAll();

	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);

	if (show_hud)
	{
		draw_selectionbox(driver, hud, hilightboxes, show_hud);

		if(draw_wield_tool)
			camera.drawWieldedTool(&rightMove);
	}

	guienv->drawAll();

	/* mask even lines */
	driver->draw2DImage(even_mask, irr::core::position2d<s32>(0, 0),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0,
			irr::video::SColor(255, 255, 255, 255),
			true);

	/* draw left eye image */
	driver->makeColorKeyTexture(left_image, irr::video::SColor(255, 0, 0, 0));
	//makeColorKeyTexture mirrors texture so we do it twice to get it right again
	driver->makeColorKeyTexture(left_image, irr::video::SColor(255, 0, 0, 0));
	driver->draw2DImage(left_image, irr::core::position2d<s32>(0, 0),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0,
			irr::video::SColor(255, 255, 255, 255),
			true);

	/* cleanup */
	camera.getCameraNode()->setPosition(oldPosition);
	camera.getCameraNode()->setTarget(oldTarget);
}

void draw_plain(Camera& camera, bool show_hud, Hud hud,
		std::vector<aabb3f> hilightboxes, video::IVideoDriver* driver,
		bool draw_wield_tool, Client& client, gui::IGUIEnvironment* guienv)
{

	draw_selectionbox(driver, hud, hilightboxes, show_hud);

	if(draw_wield_tool)
		camera.drawWieldedTool();
}

void draw_scene(Camera& camera, bool show_hud, Hud hud,
		std::vector<aabb3f> hilightboxes, video::IVideoDriver* driver,
		scene::ISceneManager* smgr,const v2u32& screensize,
		video::SColor skycolor, CameraMode cur_cam_mode, LocalPlayer* player,
		Client& client, gui::IGUIEnvironment* guienv)
{
	//TODO check if usefull
	u32 scenetime = 0;
	{
		TimeTaker timer("smgr");

		bool draw_wield_tool = (show_hud &&
				(player->hud_flags & HUD_FLAG_WIELDITEM_VISIBLE) &&
				cur_cam_mode < CAMERA_MODE_THIRD );

		std::string draw_mode = g_settings->get("3d_mode");

		smgr->drawAll();

		if (draw_mode == "anaglyph")
		{
			draw_anaglyph_3d_mode(camera, show_hud, hud, hilightboxes, driver,
					smgr, draw_wield_tool, client, guienv);
		}
		else if (g_settings->get("3d_mode") == "odd/even")
		{
			draw_odd_even_3d_mode(camera, show_hud, hud, hilightboxes, driver,
					smgr, screensize, draw_wield_tool, client, guienv);
		}
		else {
			draw_plain(camera, show_hud, hud, hilightboxes, driver,
					draw_wield_tool, client, guienv);
		}

		//TODO how to make those 3d too
		if (show_hud) {
			hud.drawCrosshair();
			hud.drawHotbar(client.getHP(), client.getPlayerItem(), client.getBreath());
			hud.drawLuaElements();
		}

		guienv->drawAll();
		scenetime = timer.stop(true);
	}

}
