/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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

#include "sidebyside.h"
#include "client/hud.h"
#include "client/camera.h"

class DrawImageStep : public RenderStep
{
public:
	DrawImageStep(video::IVideoDriver *driver, u8 texture_index, v2s32 *pos) :
		driver(driver), texture_index(texture_index), pos(pos)
	{}

	void setRenderSource(RenderSource *_source) override
	{
		source = _source;
	}

	void setRenderTarget(RenderTarget *_target) override
	{
		target = _target;
	}

	void reset() override {}

	void run() override
	{
		if (target)
			target->activate();
		
		auto texture = source->getTexture(texture_index);
		driver->draw2DImage(texture, pos ? *pos : v2s32 {});
	}
private:
	video::IVideoDriver *driver;
	u8 texture_index;
	v2s32 *pos;
	RenderSource *source;
	RenderTarget *target;
};

RenderingCoreSideBySide::RenderingCoreSideBySide(
	IrrlichtDevice *_device, Client *_client, Hud *_hud, bool _horizontal, bool _flipped)
	: RenderingCoreStereo(_device, _client, _hud), buffer(driver), horizontal(_horizontal), flipped(_flipped)
{
}

void RenderingCoreSideBySide::initTextures()
{
	core::dimension2du image_size;
	if (horizontal) {
		image_size = {screensize.X, screensize.Y / 2};
		rpos = v2s32(0, screensize.Y / 2);
	} else {
		image_size = {screensize.X / 2, screensize.Y};
		rpos = v2s32(screensize.X / 2, 0);
	}
	virtual_size = image_size;
	buffer.setTexture(TEXTURE_LEFT, image_size.Width, image_size.Height, "3d_render_left", video::ECF_A8R8G8B8);
	buffer.setTexture(TEXTURE_RIGHT, image_size.Width, image_size.Height, "3d_render_right", video::ECF_A8R8G8B8);
}

void RenderingCoreSideBySide::createPipeline()
{
	auto cam_node = camera->getCameraNode();

	// eyes
	for (bool right : { false, true }) {
		pipeline.addStep(pipeline.own(new OffsetCameraStep(cam_node, flipped ? !right : right)));
		auto step3D = new Draw3D(&pipelineState, smgr, driver, hud, camera);
		pipeline.addStep(pipeline.own(step3D));
		auto output = new TextureBufferOutput(driver, &buffer, right ? TEXTURE_RIGHT : TEXTURE_LEFT);
		output->setClearColor(&skycolor);
		step3D->setRenderTarget(pipeline.own(output));
		pipeline.addStep(pipeline.own(new TrampolineStep<RenderingCoreSideBySide>(this, &RenderingCoreSideBySide::drawPostFx)));
		pipeline.addStep(stepHUD);
	}

	for (bool right : { false, true }) {
		auto step = new DrawImageStep(driver, right ? TEXTURE_RIGHT : TEXTURE_LEFT, right ? &rpos : nullptr);
		step->setRenderSource(&buffer);
		step->setRenderTarget(screen);
		pipeline.addStep(pipeline.own(step));
	}
}
