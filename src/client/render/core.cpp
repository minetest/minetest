/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachesky Vitaly <numzer0@yandex.ru>

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

#include "core.h"
#include "camera.h"
#include "client.h"
#include "clientmap.h"
#include "hud.h"
#include "minimap.h"
#include "sky.h"

#if !defined(_IRR_OSX_PLATFORM_)
#ifdef _IRR_WINDOWS_API_
PFNGLACTIVETEXTUREPROC glActiveTexture;
#endif
// ARB framebuffer object
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
// EXT framebuffer object
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
#endif

RenderingCore::RenderingCore(IrrlichtDevice *_device, Client *_client, Hud *_hud, Sky *_sky)
	: device(_device), driver(device->getVideoDriver()), smgr(device->getSceneManager()),
	guienv(device->getGUIEnvironment()), client(_client), camera(client->getCamera()),
	mapper(client->getMinimap()), hud(_hud), sky(_sky)
{
	screensize = driver->getScreenSize();
	virtual_size = screensize;
}

RenderingCore::~RenderingCore()
{
	clearTextures();
}

void RenderingCore::initialize()
{
	// have to be called late as the VMT is not ready in the constructor:
	initTextures();
}

void RenderingCore::updateScreenSize()
{
	virtual_size = screensize;
	clearTextures();
	initTextures();
}

void RenderingCore::draw(video::SColor _skycolor, bool _show_hud, bool _show_minimap,
		bool _draw_wield_tool, bool _draw_crosshair, Sky &sky, Clouds &clouds)
{
	v2u32 ss = driver->getScreenSize();
	if (screensize != ss) {
		screensize = ss;
		updateScreenSize();
	}
	skycolor = _skycolor;
	show_hud = _show_hud;
	show_minimap = _show_minimap;
	draw_wield_tool = _draw_wield_tool;
	draw_crosshair = _draw_crosshair;

	beforeDraw();

	// todo - use a simpler shader for shadow map generation
	static u32 shadow_shader;
	if (shadow_shader == 0) {
		IShaderSource *shdsrc = client->getShaderSource();
		shadow_shader = shdsrc->getShader("shadow_shader", 1, 1);
	}

	client->getEnv().getClientMap().depthTexture = NULL;

	static COpenGLRenderTarget *lightScene;
	if (lightScene == NULL)
	{
#ifdef _IRR_WINDOWS_API_
		glActiveTexture = (PFNGLACTIVETEXTUREPROC)IRR_OGL_LOAD_EXTENSION("glActiveTexture");
#endif
		// ARB FrameBufferObjects
		glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)IRR_OGL_LOAD_EXTENSION("glBindFramebuffer");
		glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)IRR_OGL_LOAD_EXTENSION("glDeleteFramebuffers");
		glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)IRR_OGL_LOAD_EXTENSION("glGenFramebuffers");
		glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)IRR_OGL_LOAD_EXTENSION("glCheckFramebufferStatus");
		glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)IRR_OGL_LOAD_EXTENSION("glFramebufferTexture2D");

		lightScene = new COpenGLRenderTarget(v2u32(1024, 1024), "lightscene", true, driver);
	}
	//driver->setRenderTarget(0, true, true, video::SColor(0xFFFFFFFF));

	lightScene->bindRTT(true, true);
	{
		// Render
		sky.OnAnimate(porting::getTimeMs());
		client->getCamera()->getCameraNode()->OnAnimate(porting::getTimeMs());
		client->getEnv().getClientMap().OnAnimate(porting::getTimeMs());
		client->getEnv().getClientMap().depthTexture = NULL;

		v3f sunPosition = sky.getSunPosition();
		//sunPosition = client.getEnv().getLocalPlayer()->getEyePosition();
		//sunPosition.Y = sunPosition.Y;//*10.0f;// +900.0f;
		v3f sunDirection = v3f(0.0f, 0.0f, 0.0f) - sunPosition;//v3f(0.0f, -1.0f, 0.0f).normalize();
															   //sunPosition += sunDirection * -300.f;
		irr::core::matrix4 projectionMatrix, viewMatrix;
		//viewMatrix.buildCameraLookAtMatrixLH(irr::core::vector3df(-1000, 300, 1394), irr::core::vector3df(-581, 32, 1394), irr::core::vector3df(0.0f, 1.f, 0.0f));
		viewMatrix.buildCameraLookAtMatrixLH(sunPosition, sunPosition + sunDirection, irr::core::vector3df(0.0f, 1.f, 0.0f));
		//projectionMatrix.buildProjectionMatrixPerspectiveFovLH(0.71f, 1.0f, 1.f, 10000.0f);
		projectionMatrix.buildProjectionMatrixOrthoLH(1024.0f, 1024.0f, 1.0f, 10000.0f);
		driver->setTransform(video::ETS_PROJECTION, projectionMatrix);
		driver->setTransform(video::ETS_VIEW, viewMatrix);

		driver->setTransform(video::ETS_WORLD, client->getEnv().getClientMap().getAbsoluteTransformation());
		
		core::matrix4 shadowMatrix(core::matrix4::EM4CONST_IDENTITY);
		shadowMatrix.setScale(v3f(0.5f));
		shadowMatrix.setTranslation(v3f(0.5f));
		shadowMatrix *= projectionMatrix;
		shadowMatrix *= viewMatrix;
		shadowMatrix *= client->getEnv().getClientMap().getAbsoluteTransformation();

		m_shadow_matrix = shadowMatrix;

		client->getEnv().getClientMap().renderMapToShadowMap(driver, scene::ESNRP_SOLID, sky);
		// TODO: find a better way to do this
		client->getEnv().getClientMap().depthTexture = lightScene->DepthTexture;
	}

	lightScene->unbindRTT(screensize);
	//driver->setRenderTarget(0, true, true, sky.getSkyColor());

	drawAll();
}

void RenderingCore::draw3D()
{
	smgr->drawAll();
	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (!show_hud)
		return;
	hud->drawSelectionMesh();
	if (draw_wield_tool)
		camera->drawWieldedTool();
}

void RenderingCore::drawHUD()
{
	if (show_hud) {
		if (draw_crosshair)
			hud->drawCrosshair();
		hud->drawHotbar(client->getPlayerItem());
		hud->drawLuaElements(camera->getOffset());
		camera->drawNametags();
		if (mapper && show_minimap)
			mapper->drawMinimap();
	}
	guienv->drawAll();
}

void RenderingCore::drawPostFx()
{
	client->getEnv().getClientMap().renderPostFx(camera->getCameraMode());
}

#ifdef _IRR_COMPILE_WITH_OPENGL_

COpenGLRenderSurface::COpenGLRenderSurface(const core::dimension2d<u32> &size, const io::path &name, const bool isDepth, irr::video::ECOLOR_FORMAT format) : ITexture(name)
{
	ImageSize = size;
	TextureSize = size;
	InternalFormat = GL_RGBA;
	PixelFormat = GL_RGBA; // GL_BGRA_EXT ?
	PixelType = GL_UNSIGNED_BYTE;

	// generate color texture
	glGenTextures(1, &TextureName);
	glBindTexture(GL_TEXTURE_2D, TextureName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (!isDepth) {
		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, ImageSize.Width,
			ImageSize.Height, 0, PixelFormat, PixelType, 0);
	}
	else {
		// generate depth texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8_EXT/*GL_DEPTH_COMPONENT24*/, ImageSize.Width,
			ImageSize.Height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

COpenGLRenderSurface::~COpenGLRenderSurface()
{
	if (TextureName) {
		glDeleteTextures(1, &TextureName);
	}
	TextureName = 0;
}


COpenGLRenderTarget::COpenGLRenderTarget(const core::dimension2d<u32> &size, const io::path &name, const bool withDepth,
	irr::video::IVideoDriver* driver, irr::video::ECOLOR_FORMAT format)
{
	Driver = driver;
	ColorFrameBuffer = 0;
	ColorTexture = NULL;
	DepthTexture = NULL;

	// generate frame buffer
	glGenFramebuffers(1, &ColorFrameBuffer);
	bindRTT();

	ColorTexture = new COpenGLRenderSurface(size, name, false, format);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ColorTexture->TextureName);
	// attach color texture to frame buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
		GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D,
		ColorTexture->TextureName,
		0);

	if (withDepth)
	{
		DepthTexture = new COpenGLRenderSurface(size, name, true, format);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, DepthTexture->TextureName);
		// attach depth texture to frame buffer
		glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
			GL_DEPTH_ATTACHMENT_EXT,
			GL_TEXTURE_2D,
			DepthTexture->TextureName,
			0);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	//GLenum check = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	//assert(check == GL_FRAMEBUFFER_COMPLETE);

	unbindRTT(size);
}

COpenGLRenderTarget::~COpenGLRenderTarget()
{
	if (ColorTexture) {
		delete ColorTexture;
		ColorTexture = NULL;
	}

	if (DepthTexture) {
		delete DepthTexture;
		DepthTexture = NULL;
	}
}

void COpenGLRenderTarget::bindRTT(bool clearColor, bool clearDepth, video::SColor color)
{
#if defined(GL_ARB_framebuffer_object)
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, ColorFrameBuffer);
#elif defined(GL_EXT_framebuffer_object)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ColorFrameBuffer);
#else
#error "no framebuffer support !"
#endif

	if (ColorTexture || DepthTexture)
	{
		glViewport(0, 0, ColorTexture->getSize().Width, ColorTexture->getSize().Height);
		const f32 inv = 1.0f / 255.0f;
		glClearColor(color.getRed() * inv, color.getGreen() * inv,
			color.getBlue() * inv, color.getAlpha() * inv);
		if (clearColor && clearDepth) {
			glDepthMask(GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		else if (clearColor)
			glClear(GL_COLOR_BUFFER_BIT);
		else if (clearDepth) {
			glDepthMask(GL_TRUE);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	}
}

void COpenGLRenderTarget::unbindRTT(const v2u32 &screensize)
{
#if defined(GL_ARB_framebuffer_object)
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
#elif defined(GL_EXT_framebuffer_object)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#else
#error "no framebuffer support !"
#endif
	//Driver->setRenderTarget(NULL);
	glViewport(0, 0, screensize.X, screensize.Y);
}
#endif // _IRR_COMPILE_WITH_OPENGL_
