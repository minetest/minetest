/*
Minetest
Copyright (C) 2022 x2048, Dmitry Kostenko <codeforsmile@gmail.com>
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

#include "cameras.h"
#include "client/client.h"
#include "client/content_cao.h"
#include "client/universalcamera.h"
#include <unordered_map>


/// @brief ITextureSource implementation that overrides selected textures from a different ITextureSource
class DynamicTextureSource : public ITextureSource, public RenderPipelineObject
{
public:
	DynamicTextureSource(ITextureSource *_source)
			: source(_source) {}

	virtual ~DynamicTextureSource() = default;

	virtual u32 getTextureId(const std::string &name) { return source->getTextureId(name); }
	virtual std::string getTextureName(u32 id) { return source->getTextureName(id); }
	virtual video::ITexture *getTexture(u32 id) { return source->getTexture(id); }
	virtual video::ITexture *getTexture(const std::string &name, u32 *id = nullptr)
	{
		const auto &it = mappings.find(name);
		if (it != mappings.end())
			return it->second;
		return source->getTexture(name, id);
	}
	virtual video::ITexture *getTextureForMesh(const std::string &name, u32 *id = nullptr)
	{
		const auto &it = mappings.find(name);
		if (it != mappings.end())
			return it->second;
		return source->getTextureForMesh(name, id);
	}

	/*!
	 *  Returns a palette from the given texture name.
	 *  The pointer is valid until the texture source is destructed.
	 *  Should be called from the main thread.
	 */
	virtual Palette *getPalette(const std::string &name) { return nullptr; }

	virtual bool isKnownSourceImage(const std::string &name) { return false; }
	virtual video::ITexture *getNormalTexture(const std::string &name) { return nullptr; }
	virtual video::SColor getTextureAverageColor(const std::string &name) { return video::SColor(0); }
	virtual video::ITexture *getShaderFlagsTexture(bool normalmap_present) { return nullptr; }

	void mapName(const std::string &name, video::ITexture *texture) { mappings[name] = texture; }
	void clearMappings() { mappings.clear(); }
private:
	ITextureSource *source;
	std::unordered_map<std::string, video::ITexture *> mappings;
};

void SelectPrimaryCamera::run(PipelineContext &context)
{
	auto camera = context.client->getCamera()->getCameraNode();

	if (!camera->isVisible())
	{
		infostream << "SelectPrimaryCamera::run(): stop the run for the main camera since it is invisible" << std::endl;
		return;
	}

	auto smgr = context.client->getSceneManager();
	smgr->setActiveCamera(camera);

	auto cao = context.client->getEnv().getLocalPlayer()->getCAO();
	if (cao)
		cao->updateMeshCulling();
}

DrawSecondaryCameras::DrawSecondaryCameras(RenderStep *_draw, Client *client)
		: textures(new TextureBuffer()), draw(_draw), overrides(new DynamicTextureSource(client->getTextureSource()))
{
	client->setOverrideTextureSource(overrides);
}

DrawSecondaryCameras::~DrawSecondaryCameras()
{
	delete overrides;
	overrides = nullptr;
}

void DrawSecondaryCameras::reset(PipelineContext &context)
{
	textures->reset(context);
}

void DrawSecondaryCameras::run(PipelineContext &context)
{
	auto smgr = context.client->getSceneManager();
	auto driver = context.device->getVideoDriver();

	const auto &cameras = context.client->getEnv().getCameras();
	auto cao = context.client->getEnv().getLocalPlayer()->getCAO();

	for (const auto &it : cameras) {
		auto idx = it.first;
		UniversalCamera *cam = it.second;

		if (!cam->isVisible())
			continue;

		auto name = std::string("camera_rt_") + std::to_string(idx);
		auto format = video::ECF_A8R8G8B8;

		if (cam->getRenderTextureName().size() > 0) {
			auto resolution = g_settings->get("render_target_resolution");
			u16 res = MYMAX(stoul(resolution), 64);
			core::dimension2du size(res, res / cam->getRenderTextureAspectRatio());
			textures->setTexture(idx, size, name, format);
		} else {
			const auto &vp = cam->getViewPortF32();
			auto scale = v2f(vp.getWidth(), vp.getHeight());
			textures->setTexture(idx, scale, name, format);
		}
	}

	// Ensure and resize textures
	textures->reset(context);
	overrides->clearMappings();

	// Force temporarily the player CAO to be visible on screen
	if (cao)
		cao->updateMeshCulling(CameraMode::CAMERA_MODE_THIRD);

	// Hardcode flags for the nested context
	PipelineContext nested_context(context);
	nested_context.show_hud = false;

	for (const auto &it : cameras) {
		auto idx = it.first;

		UniversalCamera *cam = it.second;

		if (!cam->isVisible())
			continue;

		assert(textures->getTexture(idx) != nullptr);

		TextureBufferOutput rt(textures.get(), idx);
		draw->setRenderTarget(&rt);

		smgr->setActiveCamera(cam);
		draw->run(nested_context);

		draw->setRenderTarget(target);
	}

	if (target)
		target->activate(context);

	for (const auto &it : cameras) {
		auto idx = it.first;

		UniversalCamera *cam = it.second;

		if (!cam->isVisible())
			continue;

		if (cam->getRenderTextureName().size() != 0)
			overrides->mapName(cam->getRenderTextureName(), textures->getTexture(idx));
		else {
			const auto &vp = cam->getViewPort();
			auto texture = textures->getTexture(idx);

			// Draw the custom cameras as images
			driver->draw2DImage(textures->getTexture(idx),
				core::recti(vp.UpperLeftCorner, vp.LowerRightCorner),
				core::recti(0, 0, texture->getSize().Width, texture->getSize().Height)
			);
		}
	}
}
