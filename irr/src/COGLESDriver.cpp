// Copyright (C) 2002-2008 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "COGLESDriver.h"
#include <cassert>
#include "CNullDriver.h"
#include "IContextManager.h"

#ifdef _IRR_COMPILE_WITH_OGLES1_

#include "COpenGLCoreTexture.h"
#include "COpenGLCoreRenderTarget.h"
#include "COpenGLCoreCacheHandler.h"

#include "COGLESMaterialRenderer.h"

#include "EVertexAttributes.h"
#include "CImage.h"
#include "os.h"

namespace irr
{
namespace video
{

COGLES1Driver::COGLES1Driver(const SIrrlichtCreationParameters &params, io::IFileSystem *io, IContextManager *contextManager) :
		CNullDriver(io, params.WindowSize), COGLES1ExtensionHandler(), CacheHandler(0), CurrentRenderMode(ERM_NONE),
		ResetRenderStates(true), Transformation3DChanged(true), AntiAlias(params.AntiAlias),
		ColorFormat(ECF_R8G8B8), Params(params), ContextManager(contextManager)
{
#ifdef _DEBUG
	setDebugName("COGLESDriver");
#endif

	core::dimension2d<u32> windowSize(0, 0);

	if (!ContextManager)
		return;

	ContextManager->grab();
	ContextManager->generateSurface();
	ContextManager->generateContext();
	ExposedData = ContextManager->getContext();
	ContextManager->activateContext(ExposedData, false);

	windowSize = params.WindowSize;

	genericDriverInit(windowSize, params.Stencilbuffer);
}

COGLES1Driver::~COGLES1Driver()
{
	deleteMaterialRenders();

	CacheHandler->getTextureCache().clear();

	removeAllRenderTargets();
	deleteAllTextures();
	removeAllOcclusionQueries();
	removeAllHardwareBuffers();

	delete CacheHandler;

	if (ContextManager) {
		ContextManager->destroyContext();
		ContextManager->destroySurface();
		ContextManager->terminate();
		ContextManager->drop();
	}
}

// -----------------------------------------------------------------------
// METHODS
// -----------------------------------------------------------------------

bool COGLES1Driver::genericDriverInit(const core::dimension2d<u32> &screenSize, bool stencilBuffer)
{
	Name = glGetString(GL_VERSION);
	printVersion();

	// print renderer information
	VendorName = glGetString(GL_VENDOR);
	os::Printer::log(VendorName.c_str(), ELL_INFORMATION);

	// load extensions
	initExtensions();

	// reset cache handler
	delete CacheHandler;
	CacheHandler = new COGLES1CacheHandler(this);

	StencilBuffer = stencilBuffer;

	DriverAttributes->setAttribute("MaxTextures", (s32)Feature.MaxTextureUnits);
	DriverAttributes->setAttribute("MaxSupportedTextures", (s32)Feature.MaxTextureUnits);
	DriverAttributes->setAttribute("MaxAnisotropy", MaxAnisotropy);
	DriverAttributes->setAttribute("MaxIndices", (s32)MaxIndices);
	DriverAttributes->setAttribute("MaxTextureSize", (s32)MaxTextureSize);
	DriverAttributes->setAttribute("MaxTextureLODBias", MaxTextureLODBias);
	DriverAttributes->setAttribute("Version", Version);
	DriverAttributes->setAttribute("AntiAlias", AntiAlias);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	for (s32 i = 0; i < ETS_COUNT; ++i)
		setTransform(static_cast<E_TRANSFORMATION_STATE>(i), core::IdentityMatrix);

	setAmbientLight(SColorf(0.0f, 0.0f, 0.0f, 0.0f));
	glClearDepthf(1.0f);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
	glDepthFunc(GL_LEQUAL);
	glFrontFace(GL_CW);
	glAlphaFunc(GL_GREATER, 0.f);

	// create material renderers
	createMaterialRenderers();

	// set the renderstates
	setRenderStates3DMode();

	// set fog mode
	setFog(FogColor, FogType, FogStart, FogEnd, FogDensity, PixelFog, RangeFog);

	// create matrix for flipping textures
	TextureFlipMatrix.buildTextureTransform(0.0f, core::vector2df(0, 0), core::vector2df(0, 1.0f), core::vector2df(1.0f, -1.0f));

	// We need to reset once more at the beginning of the first rendering.
	// This fixes problems with intermediate changes to the material during texture load.
	ResetRenderStates = true;

	testGLError(__LINE__);

	return true;
}

void COGLES1Driver::createMaterialRenderers()
{
	addAndDropMaterialRenderer(new COGLES1MaterialRenderer_SOLID(this));
	addAndDropMaterialRenderer(new COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL(this));
	addAndDropMaterialRenderer(new COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL_REF(this));
	addAndDropMaterialRenderer(new COGLES1MaterialRenderer_TRANSPARENT_VERTEX_ALPHA(this));
	addAndDropMaterialRenderer(new COGLES1MaterialRenderer_ONETEXTURE_BLEND(this));
}

bool COGLES1Driver::beginScene(u16 clearFlag, SColor clearColor, f32 clearDepth, u8 clearStencil, const SExposedVideoData &videoData, core::rect<s32> *sourceRect)
{
	CNullDriver::beginScene(clearFlag, clearColor, clearDepth, clearStencil, videoData, sourceRect);

	if (ContextManager)
		ContextManager->activateContext(videoData, true);

	clearBuffers(clearFlag, clearColor, clearDepth, clearStencil);

	return true;
}

bool COGLES1Driver::endScene()
{
	CNullDriver::endScene();

	glFlush();

	if (ContextManager)
		return ContextManager->swapBuffers();

	return false;
}

//! Returns the transformation set by setTransform
const core::matrix4 &COGLES1Driver::getTransform(E_TRANSFORMATION_STATE state) const
{
	return Matrices[state];
}

//! sets transformation
void COGLES1Driver::setTransform(E_TRANSFORMATION_STATE state, const core::matrix4 &mat)
{
	Matrices[state] = mat;
	Transformation3DChanged = true;

	switch (state) {
	case ETS_VIEW:
	case ETS_WORLD: {
		// OGLES1 only has a model matrix, view and world is not existent. so lets fake these two.
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((Matrices[ETS_VIEW] * Matrices[ETS_WORLD]).pointer());
	} break;
	case ETS_PROJECTION: {
		GLfloat glmat[16];
		getGLMatrix(glmat, mat);
		// flip z to compensate OGLES1s right-hand coordinate system
		glmat[12] *= -1.0f;
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(glmat);
	} break;
	default:
		break;
	}
}

bool COGLES1Driver::updateVertexHardwareBuffer(SHWBufferLink_opengl *HWBuffer)
{
	if (!HWBuffer)
		return false;

	const scene::IMeshBuffer *mb = HWBuffer->MeshBuffer;
	const void *vertices = mb->getVertices();
	const u32 vertexCount = mb->getVertexCount();
	const E_VERTEX_TYPE vType = mb->getVertexType();
	const u32 vertexSize = getVertexPitchFromType(vType);

	// buffer vertex data, and convert colours...
	core::array<c8> buffer(vertexSize * vertexCount);
	buffer.set_used(vertexSize * vertexCount);
	memcpy(buffer.pointer(), vertices, vertexSize * vertexCount);

	// in order to convert the colors into opengl format (RGBA)
	switch (vType) {
	case EVT_STANDARD: {
		S3DVertex *pb = reinterpret_cast<S3DVertex *>(buffer.pointer());
		const S3DVertex *po = static_cast<const S3DVertex *>(vertices);
		for (u32 i = 0; i < vertexCount; i++) {
			po[i].Color.toOpenGLColor((u8 *)&(pb[i].Color.color));
		}
	} break;
	case EVT_2TCOORDS: {
		S3DVertex2TCoords *pb = reinterpret_cast<S3DVertex2TCoords *>(buffer.pointer());
		const S3DVertex2TCoords *po = static_cast<const S3DVertex2TCoords *>(vertices);
		for (u32 i = 0; i < vertexCount; i++) {
			po[i].Color.toOpenGLColor((u8 *)&(pb[i].Color.color));
		}
	} break;
	case EVT_TANGENTS: {
		S3DVertexTangents *pb = reinterpret_cast<S3DVertexTangents *>(buffer.pointer());
		const S3DVertexTangents *po = static_cast<const S3DVertexTangents *>(vertices);
		for (u32 i = 0; i < vertexCount; i++) {
			po[i].Color.toOpenGLColor((u8 *)&(pb[i].Color.color));
		}
	} break;
	default: {
		return false;
	}
	}

	// get or create buffer
	bool newBuffer = false;
	if (!HWBuffer->vbo_verticesID) {
		glGenBuffers(1, &HWBuffer->vbo_verticesID);
		if (!HWBuffer->vbo_verticesID)
			return false;
		newBuffer = true;
	} else if (HWBuffer->vbo_verticesSize < vertexCount * vertexSize) {
		newBuffer = true;
	}

	glBindBuffer(GL_ARRAY_BUFFER, HWBuffer->vbo_verticesID);

	// copy data to graphics card
	if (!newBuffer)
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * vertexSize, buffer.const_pointer());
	else {
		HWBuffer->vbo_verticesSize = vertexCount * vertexSize;

		if (HWBuffer->Mapped_Vertex == scene::EHM_STATIC)
			glBufferData(GL_ARRAY_BUFFER, vertexCount * vertexSize, buffer.const_pointer(), GL_STATIC_DRAW);
		else
			glBufferData(GL_ARRAY_BUFFER, vertexCount * vertexSize, buffer.const_pointer(), GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return (!testGLError(__LINE__));
}

bool COGLES1Driver::updateIndexHardwareBuffer(SHWBufferLink_opengl *HWBuffer)
{
	if (!HWBuffer)
		return false;

	const scene::IMeshBuffer *mb = HWBuffer->MeshBuffer;

	const void *indices = mb->getIndices();
	u32 indexCount = mb->getIndexCount();

	GLenum indexSize;
	switch (mb->getIndexType()) {
	case (EIT_16BIT): {
		indexSize = sizeof(u16);
		break;
	}
	case (EIT_32BIT): {
		indexSize = sizeof(u32);
		break;
	}
	default: {
		return false;
	}
	}

	// get or create buffer
	bool newBuffer = false;
	if (!HWBuffer->vbo_indicesID) {
		glGenBuffers(1, &HWBuffer->vbo_indicesID);
		if (!HWBuffer->vbo_indicesID)
			return false;
		newBuffer = true;
	} else if (HWBuffer->vbo_indicesSize < indexCount * indexSize) {
		newBuffer = true;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, HWBuffer->vbo_indicesID);

	// copy data to graphics card
	if (!newBuffer)
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexCount * indexSize, indices);
	else {
		HWBuffer->vbo_indicesSize = indexCount * indexSize;

		if (HWBuffer->Mapped_Index == scene::EHM_STATIC)
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * indexSize, indices, GL_STATIC_DRAW);
		else
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * indexSize, indices, GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return (!testGLError(__LINE__));
}

//! updates hardware buffer if needed
bool COGLES1Driver::updateHardwareBuffer(SHWBufferLink *HWBuffer)
{
	if (!HWBuffer)
		return false;

	if (HWBuffer->Mapped_Vertex != scene::EHM_NEVER) {
		if (HWBuffer->ChangedID_Vertex != HWBuffer->MeshBuffer->getChangedID_Vertex() || !static_cast<SHWBufferLink_opengl *>(HWBuffer)->vbo_verticesID) {

			HWBuffer->ChangedID_Vertex = HWBuffer->MeshBuffer->getChangedID_Vertex();

			if (!updateVertexHardwareBuffer(static_cast<SHWBufferLink_opengl *>(HWBuffer)))
				return false;
		}
	}

	if (HWBuffer->Mapped_Index != scene::EHM_NEVER) {
		if (HWBuffer->ChangedID_Index != HWBuffer->MeshBuffer->getChangedID_Index() || !((SHWBufferLink_opengl *)HWBuffer)->vbo_indicesID) {

			HWBuffer->ChangedID_Index = HWBuffer->MeshBuffer->getChangedID_Index();

			if (!updateIndexHardwareBuffer(static_cast<SHWBufferLink_opengl *>(HWBuffer)))
				return false;
		}
	}

	return true;
}

//! Create hardware buffer from meshbuffer
COGLES1Driver::SHWBufferLink *COGLES1Driver::createHardwareBuffer(const scene::IMeshBuffer *mb)
{
	if (!mb || (mb->getHardwareMappingHint_Index() == scene::EHM_NEVER && mb->getHardwareMappingHint_Vertex() == scene::EHM_NEVER))
		return 0;

	SHWBufferLink_opengl *HWBuffer = new SHWBufferLink_opengl(mb);

	// add to map
	HWBuffer->listPosition = HWBufferList.insert(HWBufferList.end(), HWBuffer);

	HWBuffer->ChangedID_Vertex = HWBuffer->MeshBuffer->getChangedID_Vertex();
	HWBuffer->ChangedID_Index = HWBuffer->MeshBuffer->getChangedID_Index();
	HWBuffer->Mapped_Vertex = mb->getHardwareMappingHint_Vertex();
	HWBuffer->Mapped_Index = mb->getHardwareMappingHint_Index();
	HWBuffer->vbo_verticesID = 0;
	HWBuffer->vbo_indicesID = 0;
	HWBuffer->vbo_verticesSize = 0;
	HWBuffer->vbo_indicesSize = 0;

	if (!updateHardwareBuffer(HWBuffer)) {
		deleteHardwareBuffer(HWBuffer);
		return 0;
	}

	return HWBuffer;
}

void COGLES1Driver::deleteHardwareBuffer(SHWBufferLink *_HWBuffer)
{
	if (!_HWBuffer)
		return;

	SHWBufferLink_opengl *HWBuffer = static_cast<SHWBufferLink_opengl *>(_HWBuffer);
	if (HWBuffer->vbo_verticesID) {
		glDeleteBuffers(1, &HWBuffer->vbo_verticesID);
		HWBuffer->vbo_verticesID = 0;
	}
	if (HWBuffer->vbo_indicesID) {
		glDeleteBuffers(1, &HWBuffer->vbo_indicesID);
		HWBuffer->vbo_indicesID = 0;
	}

	CNullDriver::deleteHardwareBuffer(_HWBuffer);
}

//! Draw hardware buffer
void COGLES1Driver::drawHardwareBuffer(SHWBufferLink *_HWBuffer)
{
	if (!_HWBuffer)
		return;

	SHWBufferLink_opengl *HWBuffer = static_cast<SHWBufferLink_opengl *>(_HWBuffer);

	updateHardwareBuffer(HWBuffer); // check if update is needed

	const scene::IMeshBuffer *mb = HWBuffer->MeshBuffer;
	const void *vertices = mb->getVertices();
	const void *indexList = mb->getIndices();

	if (HWBuffer->Mapped_Vertex != scene::EHM_NEVER) {
		glBindBuffer(GL_ARRAY_BUFFER, HWBuffer->vbo_verticesID);
		vertices = 0;
	}

	if (HWBuffer->Mapped_Index != scene::EHM_NEVER) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, HWBuffer->vbo_indicesID);
		indexList = 0;
	}

	drawVertexPrimitiveList(vertices, mb->getVertexCount(), indexList,
			mb->getPrimitiveCount(), mb->getVertexType(),
			mb->getPrimitiveType(), mb->getIndexType());

	if (HWBuffer->Mapped_Vertex != scene::EHM_NEVER)
		glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (HWBuffer->Mapped_Index != scene::EHM_NEVER)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

IRenderTarget *COGLES1Driver::addRenderTarget()
{
	COGLES1RenderTarget *renderTarget = new COGLES1RenderTarget(this);
	RenderTargets.push_back(renderTarget);

	return renderTarget;
}

// small helper function to create vertex buffer object adress offsets
static inline u8 *buffer_offset(const long offset)
{
	return ((u8 *)0 + offset);
}

//! draws a vertex primitive list
void COGLES1Driver::drawVertexPrimitiveList(const void *vertices, u32 vertexCount,
		const void *indexList, u32 primitiveCount,
		E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType)
{
	if (!checkPrimitiveCount(primitiveCount))
		return;

	setRenderStates3DMode();

	drawVertexPrimitiveList2d3d(vertices, vertexCount, (const u16 *)indexList, primitiveCount, vType, pType, iType);
}

void COGLES1Driver::drawVertexPrimitiveList2d3d(const void *vertices, u32 vertexCount,
		const void *indexList, u32 primitiveCount,
		E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType, bool threed)
{
	if (!primitiveCount || !vertexCount)
		return;

	if (!threed && !checkPrimitiveCount(primitiveCount))
		return;

	CNullDriver::drawVertexPrimitiveList(vertices, vertexCount, indexList, primitiveCount, vType, pType, iType);

	if (vertices) {
		// convert colors to gl color format.
		vertexCount *= 4; // reused as color component count
		ColorBuffer.set_used(vertexCount);
		u32 i;

		switch (vType) {
		case EVT_STANDARD: {
			const S3DVertex *p = static_cast<const S3DVertex *>(vertices);
			for (i = 0; i < vertexCount; i += 4) {
				p->Color.toOpenGLColor(&ColorBuffer[i]);
				++p;
			}
		} break;
		case EVT_2TCOORDS: {
			const S3DVertex2TCoords *p = static_cast<const S3DVertex2TCoords *>(vertices);
			for (i = 0; i < vertexCount; i += 4) {
				p->Color.toOpenGLColor(&ColorBuffer[i]);
				++p;
			}
		} break;
		case EVT_TANGENTS: {
			const S3DVertexTangents *p = static_cast<const S3DVertexTangents *>(vertices);
			for (i = 0; i < vertexCount; i += 4) {
				p->Color.toOpenGLColor(&ColorBuffer[i]);
				++p;
			}
		} break;
		}
	}

	// draw everything
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	if ((pType != scene::EPT_POINTS) && (pType != scene::EPT_POINT_SPRITES))
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#ifdef GL_OES_point_size_array
	else if (FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_point_size_array] && (Material.Thickness == 0.0f))
		glEnableClientState(GL_POINT_SIZE_ARRAY_OES);
#endif
	if (threed && (pType != scene::EPT_POINTS) && (pType != scene::EPT_POINT_SPRITES))
		glEnableClientState(GL_NORMAL_ARRAY);

	if (vertices)
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, &ColorBuffer[0]);

	switch (vType) {
	case EVT_STANDARD:
		if (vertices) {
			if (threed)
				glNormalPointer(GL_FLOAT, sizeof(S3DVertex), &(static_cast<const S3DVertex *>(vertices))[0].Normal);
			glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex), &(static_cast<const S3DVertex *>(vertices))[0].TCoords);
			glVertexPointer((threed ? 3 : 2), GL_FLOAT, sizeof(S3DVertex), &(static_cast<const S3DVertex *>(vertices))[0].Pos);
		} else {
			glNormalPointer(GL_FLOAT, sizeof(S3DVertex), buffer_offset(12));
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(S3DVertex), buffer_offset(24));
			glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex), buffer_offset(28));
			glVertexPointer(3, GL_FLOAT, sizeof(S3DVertex), 0);
		}

		if (Feature.MaxTextureUnits > 0 && CacheHandler->getTextureCache().get(1)) {
			glClientActiveTexture(GL_TEXTURE0 + 1);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			if (vertices)
				glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex), &(static_cast<const S3DVertex *>(vertices))[0].TCoords);
			else
				glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex), buffer_offset(28));
		}
		break;
	case EVT_2TCOORDS:
		if (vertices) {
			if (threed)
				glNormalPointer(GL_FLOAT, sizeof(S3DVertex2TCoords), &(static_cast<const S3DVertex2TCoords *>(vertices))[0].Normal);
			glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex2TCoords), &(static_cast<const S3DVertex2TCoords *>(vertices))[0].TCoords);
			glVertexPointer((threed ? 3 : 2), GL_FLOAT, sizeof(S3DVertex2TCoords), &(static_cast<const S3DVertex2TCoords *>(vertices))[0].Pos);
		} else {
			glNormalPointer(GL_FLOAT, sizeof(S3DVertex2TCoords), buffer_offset(12));
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(S3DVertex2TCoords), buffer_offset(24));
			glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex2TCoords), buffer_offset(28));
			glVertexPointer(3, GL_FLOAT, sizeof(S3DVertex2TCoords), buffer_offset(0));
		}

		if (Feature.MaxTextureUnits > 0) {
			glClientActiveTexture(GL_TEXTURE0 + 1);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			if (vertices)
				glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex2TCoords), &(static_cast<const S3DVertex2TCoords *>(vertices))[0].TCoords2);
			else
				glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex2TCoords), buffer_offset(36));
		}
		break;
	case EVT_TANGENTS:
		if (vertices) {
			if (threed)
				glNormalPointer(GL_FLOAT, sizeof(S3DVertexTangents), &(static_cast<const S3DVertexTangents *>(vertices))[0].Normal);
			glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertexTangents), &(static_cast<const S3DVertexTangents *>(vertices))[0].TCoords);
			glVertexPointer((threed ? 3 : 2), GL_FLOAT, sizeof(S3DVertexTangents), &(static_cast<const S3DVertexTangents *>(vertices))[0].Pos);
		} else {
			glNormalPointer(GL_FLOAT, sizeof(S3DVertexTangents), buffer_offset(12));
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(S3DVertexTangents), buffer_offset(24));
			glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertexTangents), buffer_offset(28));
			glVertexPointer(3, GL_FLOAT, sizeof(S3DVertexTangents), buffer_offset(0));
		}

		if (Feature.MaxTextureUnits > 0) {
			glClientActiveTexture(GL_TEXTURE0 + 1);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			if (vertices)
				glTexCoordPointer(3, GL_FLOAT, sizeof(S3DVertexTangents), &(static_cast<const S3DVertexTangents *>(vertices))[0].Tangent);
			else
				glTexCoordPointer(3, GL_FLOAT, sizeof(S3DVertexTangents), buffer_offset(36));

			glClientActiveTexture(GL_TEXTURE0 + 2);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			if (vertices)
				glTexCoordPointer(3, GL_FLOAT, sizeof(S3DVertexTangents), &(static_cast<const S3DVertexTangents *>(vertices))[0].Binormal);
			else
				glTexCoordPointer(3, GL_FLOAT, sizeof(S3DVertexTangents), buffer_offset(48));
		}
		break;
	}

	GLenum indexSize = 0;

	switch (iType) {
	case (EIT_16BIT): {
		indexSize = GL_UNSIGNED_SHORT;
		break;
	}
	case (EIT_32BIT): {
#ifdef GL_OES_element_index_uint
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
		if (FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_element_index_uint])
			indexSize = GL_UNSIGNED_INT;
		else
#endif
			indexSize = GL_UNSIGNED_SHORT;
		break;
	}
	}

	switch (pType) {
	case scene::EPT_POINTS:
	case scene::EPT_POINT_SPRITES: {
#ifdef GL_OES_point_sprite
		if (pType == scene::EPT_POINT_SPRITES && FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_point_sprite])
			glEnable(GL_POINT_SPRITE_OES);
#endif
		// if ==0 we use the point size array
		if (Material.Thickness != 0.f) {
			float quadratic[] = {0.0f, 0.0f, 10.01f};
			glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, quadratic);
			float maxParticleSize = 1.0f;
			glGetFloatv(GL_POINT_SIZE_MAX, &maxParticleSize);
			//				maxParticleSize=maxParticleSize<Material.Thickness?maxParticleSize:Material.Thickness;
			//				extGlPointParameterf(GL_POINT_SIZE_MAX,maxParticleSize);
			//				extGlPointParameterf(GL_POINT_SIZE_MIN,Material.Thickness);
			glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 60.0f);
			glPointSize(Material.Thickness);
		}
#ifdef GL_OES_point_sprite
		if (pType == scene::EPT_POINT_SPRITES && FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_point_sprite])
			glTexEnvf(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
#endif
		glDrawArrays(GL_POINTS, 0, primitiveCount);
#ifdef GL_OES_point_sprite
		if (pType == scene::EPT_POINT_SPRITES && FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_point_sprite]) {
			glDisable(GL_POINT_SPRITE_OES);
			glTexEnvf(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_FALSE);
		}
#endif
	} break;
	case scene::EPT_LINE_STRIP:
		glDrawElements(GL_LINE_STRIP, primitiveCount + 1, indexSize, indexList);
		break;
	case scene::EPT_LINE_LOOP:
		glDrawElements(GL_LINE_LOOP, primitiveCount, indexSize, indexList);
		break;
	case scene::EPT_LINES:
		glDrawElements(GL_LINES, primitiveCount * 2, indexSize, indexList);
		break;
	case scene::EPT_TRIANGLE_STRIP:
		glDrawElements(GL_TRIANGLE_STRIP, primitiveCount + 2, indexSize, indexList);
		break;
	case scene::EPT_TRIANGLE_FAN:
		glDrawElements(GL_TRIANGLE_FAN, primitiveCount + 2, indexSize, indexList);
		break;
	case scene::EPT_TRIANGLES:
		glDrawElements((LastMaterial.Wireframe) ? GL_LINES : (LastMaterial.PointCloud) ? GL_POINTS
																					   : GL_TRIANGLES,
				primitiveCount * 3, indexSize, indexList);
		break;
	}

	if (Feature.MaxTextureUnits > 0) {
		if (vType == EVT_TANGENTS) {
			glClientActiveTexture(GL_TEXTURE0 + 2);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		if ((vType != EVT_STANDARD) || CacheHandler->getTextureCache().get(1)) {
			glClientActiveTexture(GL_TEXTURE0 + 1);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		glClientActiveTexture(GL_TEXTURE0);
	}

#ifdef GL_OES_point_size_array
	if (FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_point_size_array] && (Material.Thickness == 0.0f))
		glDisableClientState(GL_POINT_SIZE_ARRAY_OES);
#endif

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

//! draws a 2d image, using a color and the alpha channel of the texture
void COGLES1Driver::draw2DImage(const video::ITexture *texture,
		const core::position2d<s32> &pos,
		const core::rect<s32> &sourceRect,
		const core::rect<s32> *clipRect, SColor color,
		bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	if (!sourceRect.isValid())
		return;

	core::position2d<s32> targetPos(pos);
	core::position2d<s32> sourcePos(sourceRect.UpperLeftCorner);
	core::dimension2d<s32> sourceSize(sourceRect.getSize());
	if (clipRect) {
		if (targetPos.X < clipRect->UpperLeftCorner.X) {
			sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
			if (sourceSize.Width <= 0)
				return;

			sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
			targetPos.X = clipRect->UpperLeftCorner.X;
		}

		if (targetPos.X + sourceSize.Width > clipRect->LowerRightCorner.X) {
			sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
			if (sourceSize.Width <= 0)
				return;
		}

		if (targetPos.Y < clipRect->UpperLeftCorner.Y) {
			sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
			if (sourceSize.Height <= 0)
				return;

			sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
			targetPos.Y = clipRect->UpperLeftCorner.Y;
		}

		if (targetPos.Y + sourceSize.Height > clipRect->LowerRightCorner.Y) {
			sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
			if (sourceSize.Height <= 0)
				return;
		}
	}

	// clip these coordinates

	if (targetPos.X < 0) {
		sourceSize.Width += targetPos.X;
		if (sourceSize.Width <= 0)
			return;

		sourcePos.X -= targetPos.X;
		targetPos.X = 0;
	}

	const core::dimension2d<u32> &renderTargetSize = getCurrentRenderTargetSize();

	if (targetPos.X + sourceSize.Width > (s32)renderTargetSize.Width) {
		sourceSize.Width -= (targetPos.X + sourceSize.Width) - renderTargetSize.Width;
		if (sourceSize.Width <= 0)
			return;
	}

	if (targetPos.Y < 0) {
		sourceSize.Height += targetPos.Y;
		if (sourceSize.Height <= 0)
			return;

		sourcePos.Y -= targetPos.Y;
		targetPos.Y = 0;
	}

	if (targetPos.Y + sourceSize.Height > (s32)renderTargetSize.Height) {
		sourceSize.Height -= (targetPos.Y + sourceSize.Height) - renderTargetSize.Height;
		if (sourceSize.Height <= 0)
			return;
	}

	// ok, we've clipped everything.
	// now draw it.

	// texcoords need to be flipped horizontally for RTTs
	const bool isRTT = texture->isRenderTarget();
	const core::dimension2d<u32> &ss = texture->getOriginalSize();
	const f32 invW = 1.f / static_cast<f32>(ss.Width);
	const f32 invH = 1.f / static_cast<f32>(ss.Height);
	const core::rect<f32> tcoords(
			sourcePos.X * invW,
			(isRTT ? (sourcePos.Y + sourceSize.Height) : sourcePos.Y) * invH,
			(sourcePos.X + sourceSize.Width) * invW,
			(isRTT ? sourcePos.Y : (sourcePos.Y + sourceSize.Height)) * invH);

	const core::rect<s32> poss(targetPos, sourceSize);

	if (!CacheHandler->getTextureCache().set(0, texture))
		return;

	setRenderStates2DMode(color.getAlpha() < 255, true, useAlphaChannelOfTexture);

	u16 indices[] = {0, 1, 2, 3};
	S3DVertex vertices[4];
	vertices[0] = S3DVertex((f32)poss.UpperLeftCorner.X, (f32)poss.UpperLeftCorner.Y, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
	vertices[1] = S3DVertex((f32)poss.LowerRightCorner.X, (f32)poss.UpperLeftCorner.Y, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
	vertices[2] = S3DVertex((f32)poss.LowerRightCorner.X, (f32)poss.LowerRightCorner.Y, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
	vertices[3] = S3DVertex((f32)poss.UpperLeftCorner.X, (f32)poss.LowerRightCorner.Y, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
	drawVertexPrimitiveList2d3d(vertices, 4, indices, 2, video::EVT_STANDARD, scene::EPT_TRIANGLE_FAN, EIT_16BIT, false);
}

//! The same, but with a four element array of colors, one for each vertex
void COGLES1Driver::draw2DImage(const video::ITexture *texture, const core::rect<s32> &destRect,
		const core::rect<s32> &sourceRect, const core::rect<s32> *clipRect,
		const video::SColor *const colors, bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	// texcoords need to be flipped horizontally for RTTs
	const bool isRTT = texture->isRenderTarget();
	const core::dimension2du &ss = texture->getOriginalSize();
	const f32 invW = 1.f / static_cast<f32>(ss.Width);
	const f32 invH = 1.f / static_cast<f32>(ss.Height);
	const core::rect<f32> tcoords(
			sourceRect.UpperLeftCorner.X * invW,
			(isRTT ? sourceRect.LowerRightCorner.Y : sourceRect.UpperLeftCorner.Y) * invH,
			sourceRect.LowerRightCorner.X * invW,
			(isRTT ? sourceRect.UpperLeftCorner.Y : sourceRect.LowerRightCorner.Y) * invH);

	const video::SColor temp[4] = {
			0xFFFFFFFF,
			0xFFFFFFFF,
			0xFFFFFFFF,
			0xFFFFFFFF,
		};

	const video::SColor *const useColor = colors ? colors : temp;

	if (!CacheHandler->getTextureCache().set(0, texture))
		return;

	setRenderStates2DMode(useColor[0].getAlpha() < 255 || useColor[1].getAlpha() < 255 ||
								  useColor[2].getAlpha() < 255 || useColor[3].getAlpha() < 255,
			true, useAlphaChannelOfTexture);

	if (clipRect) {
		if (!clipRect->isValid())
			return;

		glEnable(GL_SCISSOR_TEST);
		const core::dimension2d<u32> &renderTargetSize = getCurrentRenderTargetSize();
		glScissor(clipRect->UpperLeftCorner.X, renderTargetSize.Height - clipRect->LowerRightCorner.Y,
				clipRect->getWidth(), clipRect->getHeight());
	}

	u16 indices[] = {0, 1, 2, 3};
	S3DVertex vertices[4];
	vertices[0] = S3DVertex((f32)destRect.UpperLeftCorner.X, (f32)destRect.UpperLeftCorner.Y, 0, 0, 0, 1, useColor[0], tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
	vertices[1] = S3DVertex((f32)destRect.LowerRightCorner.X, (f32)destRect.UpperLeftCorner.Y, 0, 0, 0, 1, useColor[3], tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
	vertices[2] = S3DVertex((f32)destRect.LowerRightCorner.X, (f32)destRect.LowerRightCorner.Y, 0, 0, 0, 1, useColor[2], tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
	vertices[3] = S3DVertex((f32)destRect.UpperLeftCorner.X, (f32)destRect.LowerRightCorner.Y, 0, 0, 0, 1, useColor[1], tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
	drawVertexPrimitiveList2d3d(vertices, 4, indices, 2, video::EVT_STANDARD, scene::EPT_TRIANGLE_FAN, EIT_16BIT, false);

	if (clipRect)
		glDisable(GL_SCISSOR_TEST);
}

void COGLES1Driver::draw2DImage(const video::ITexture *texture, u32 layer, bool flip)
{
	if (!texture || !CacheHandler->getTextureCache().set(0, texture))
		return;

	setRenderStates2DMode(false, true, true);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Transformation3DChanged = true;

	u16 indices[] = {0, 1, 2, 3};
	S3DVertex vertices[4];

	vertices[0].Pos = core::vector3df(-1.f, 1.f, 0.f);
	vertices[1].Pos = core::vector3df(1.f, 1.f, 0.f);
	vertices[2].Pos = core::vector3df(1.f, -1.f, 0.f);
	vertices[3].Pos = core::vector3df(-1.f, -1.f, 0.f);

	f32 modificator = (flip) ? 1.f : 0.f;

	vertices[0].TCoords = core::vector2df(0.f, 0.f + modificator);
	vertices[1].TCoords = core::vector2df(1.f, 0.f + modificator);
	vertices[2].TCoords = core::vector2df(1.f, 1.f - modificator);
	vertices[3].TCoords = core::vector2df(0.f, 1.f - modificator);

	vertices[0].Color = 0xFFFFFFFF;
	vertices[1].Color = 0xFFFFFFFF;
	vertices[2].Color = 0xFFFFFFFF;
	vertices[3].Color = 0xFFFFFFFF;

	drawVertexPrimitiveList2d3d(vertices, 4, indices, 2, video::EVT_STANDARD, scene::EPT_TRIANGLE_FAN, EIT_16BIT, false);
}

//! draws a set of 2d images, using a color and the alpha channel of the texture if desired.
void COGLES1Driver::draw2DImageBatch(const video::ITexture *texture,
		const core::array<core::position2d<s32>> &positions,
		const core::array<core::rect<s32>> &sourceRects,
		const core::rect<s32> *clipRect,
		SColor color,
		bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	const u32 drawCount = core::min_<u32>(positions.size(), sourceRects.size());
	if (!drawCount)
		return;

	const core::dimension2d<u32> &ss = texture->getOriginalSize();
	if (!ss.Width || !ss.Height)
		return;
	const f32 invW = 1.f / static_cast<f32>(ss.Width);
	const f32 invH = 1.f / static_cast<f32>(ss.Height);
	const core::dimension2d<u32> &renderTargetSize = getCurrentRenderTargetSize();

	if (!CacheHandler->getTextureCache().set(0, texture))
		return;

	setRenderStates2DMode(color.getAlpha() < 255, true, useAlphaChannelOfTexture);

	core::array<S3DVertex> vertices;
	core::array<u16> quadIndices;
	vertices.reallocate(drawCount * 4);
	quadIndices.reallocate(drawCount * 6);

	for (u32 i = 0; i < drawCount; ++i) {
		if (!sourceRects[i].isValid())
			continue;

		core::position2d<s32> targetPos(positions[i]);
		core::position2d<s32> sourcePos(sourceRects[i].UpperLeftCorner);
		// This needs to be signed as it may go negative.
		core::dimension2d<s32> sourceSize(sourceRects[i].getSize());
		if (clipRect) {
			if (targetPos.X < clipRect->UpperLeftCorner.X) {
				sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
				if (sourceSize.Width <= 0)
					continue;

				sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
				targetPos.X = clipRect->UpperLeftCorner.X;
			}

			if (targetPos.X + sourceSize.Width > clipRect->LowerRightCorner.X) {
				sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
				if (sourceSize.Width <= 0)
					continue;
			}

			if (targetPos.Y < clipRect->UpperLeftCorner.Y) {
				sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
				if (sourceSize.Height <= 0)
					continue;

				sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
				targetPos.Y = clipRect->UpperLeftCorner.Y;
			}

			if (targetPos.Y + sourceSize.Height > clipRect->LowerRightCorner.Y) {
				sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
				if (sourceSize.Height <= 0)
					continue;
			}
		}

		// clip these coordinates

		if (targetPos.X < 0) {
			sourceSize.Width += targetPos.X;
			if (sourceSize.Width <= 0)
				continue;

			sourcePos.X -= targetPos.X;
			targetPos.X = 0;
		}

		if (targetPos.X + sourceSize.Width > (s32)renderTargetSize.Width) {
			sourceSize.Width -= (targetPos.X + sourceSize.Width) - renderTargetSize.Width;
			if (sourceSize.Width <= 0)
				continue;
		}

		if (targetPos.Y < 0) {
			sourceSize.Height += targetPos.Y;
			if (sourceSize.Height <= 0)
				continue;

			sourcePos.Y -= targetPos.Y;
			targetPos.Y = 0;
		}

		if (targetPos.Y + sourceSize.Height > (s32)renderTargetSize.Height) {
			sourceSize.Height -= (targetPos.Y + sourceSize.Height) - renderTargetSize.Height;
			if (sourceSize.Height <= 0)
				continue;
		}

		// ok, we've clipped everything.

		const core::rect<f32> tcoords(
				sourcePos.X * invW,
				sourcePos.Y * invH,
				(sourcePos.X + sourceSize.Width) * invW,
				(sourcePos.Y + sourceSize.Height) * invH);

		const core::rect<s32> poss(targetPos, sourceSize);

		const u32 vstart = vertices.size();

		vertices.push_back(S3DVertex((f32)poss.UpperLeftCorner.X, (f32)poss.UpperLeftCorner.Y, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y));
		vertices.push_back(S3DVertex((f32)poss.LowerRightCorner.X, (f32)poss.UpperLeftCorner.Y, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y));
		vertices.push_back(S3DVertex((f32)poss.LowerRightCorner.X, (f32)poss.LowerRightCorner.Y, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y));
		vertices.push_back(S3DVertex((f32)poss.UpperLeftCorner.X, (f32)poss.LowerRightCorner.Y, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y));

		quadIndices.push_back(vstart);
		quadIndices.push_back(vstart + 1);
		quadIndices.push_back(vstart + 2);
		quadIndices.push_back(vstart);
		quadIndices.push_back(vstart + 2);
		quadIndices.push_back(vstart + 3);
	}
	if (vertices.size())
		drawVertexPrimitiveList2d3d(vertices.pointer(), vertices.size(),
				quadIndices.pointer(), vertices.size() / 2,
				video::EVT_STANDARD, scene::EPT_TRIANGLES,
				EIT_16BIT, false);
}

//! draw a 2d rectangle
void COGLES1Driver::draw2DRectangle(SColor color, const core::rect<s32> &position,
		const core::rect<s32> *clip)
{
	setRenderStates2DMode(color.getAlpha() < 255, false, false);

	core::rect<s32> pos = position;

	if (clip)
		pos.clipAgainst(*clip);

	if (!pos.isValid())
		return;

	u16 indices[] = {0, 1, 2, 3};
	S3DVertex vertices[4];
	vertices[0] = S3DVertex((f32)pos.UpperLeftCorner.X, (f32)pos.UpperLeftCorner.Y, 0, 0, 0, 1, color, 0, 0);
	vertices[1] = S3DVertex((f32)pos.LowerRightCorner.X, (f32)pos.UpperLeftCorner.Y, 0, 0, 0, 1, color, 0, 0);
	vertices[2] = S3DVertex((f32)pos.LowerRightCorner.X, (f32)pos.LowerRightCorner.Y, 0, 0, 0, 1, color, 0, 0);
	vertices[3] = S3DVertex((f32)pos.UpperLeftCorner.X, (f32)pos.LowerRightCorner.Y, 0, 0, 0, 1, color, 0, 0);
	drawVertexPrimitiveList2d3d(vertices, 4, indices, 2, video::EVT_STANDARD, scene::EPT_TRIANGLE_FAN, EIT_16BIT, false);
}

//! draw an 2d rectangle
void COGLES1Driver::draw2DRectangle(const core::rect<s32> &position,
		SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
		const core::rect<s32> *clip)
{
	core::rect<s32> pos = position;

	if (clip)
		pos.clipAgainst(*clip);

	if (!pos.isValid())
		return;

	setRenderStates2DMode(colorLeftUp.getAlpha() < 255 ||
								  colorRightUp.getAlpha() < 255 ||
								  colorLeftDown.getAlpha() < 255 ||
								  colorRightDown.getAlpha() < 255,
			false, false);

	u16 indices[] = {0, 1, 2, 3};
	S3DVertex vertices[4];
	vertices[0] = S3DVertex((f32)pos.UpperLeftCorner.X, (f32)pos.UpperLeftCorner.Y, 0, 0, 0, 1, colorLeftUp, 0, 0);
	vertices[1] = S3DVertex((f32)pos.LowerRightCorner.X, (f32)pos.UpperLeftCorner.Y, 0, 0, 0, 1, colorRightUp, 0, 0);
	vertices[2] = S3DVertex((f32)pos.LowerRightCorner.X, (f32)pos.LowerRightCorner.Y, 0, 0, 0, 1, colorRightDown, 0, 0);
	vertices[3] = S3DVertex((f32)pos.UpperLeftCorner.X, (f32)pos.LowerRightCorner.Y, 0, 0, 0, 1, colorLeftDown, 0, 0);
	drawVertexPrimitiveList2d3d(vertices, 4, indices, 2, video::EVT_STANDARD, scene::EPT_TRIANGLE_FAN, EIT_16BIT, false);
}

//! Draws a 2d line.
void COGLES1Driver::draw2DLine(const core::position2d<s32> &start,
		const core::position2d<s32> &end,
		SColor color)
{
	setRenderStates2DMode(color.getAlpha() < 255, false, false);

	u16 indices[] = {0, 1};
	S3DVertex vertices[2];
	vertices[0] = S3DVertex((f32)start.X, (f32)start.Y, 0, 0, 0, 1, color, 0, 0);
	vertices[1] = S3DVertex((f32)end.X, (f32)end.Y, 0, 0, 0, 1, color, 1, 1);
	drawVertexPrimitiveList2d3d(vertices, 2, indices, 1, video::EVT_STANDARD, scene::EPT_LINES, EIT_16BIT, false);
}

//! creates a matrix in supplied GLfloat array to pass to OGLES1
inline void COGLES1Driver::getGLMatrix(GLfloat gl_matrix[16], const core::matrix4 &m)
{
	memcpy(gl_matrix, m.pointer(), 16 * sizeof(f32));
}

//! creates a opengltexturematrix from a D3D style texture matrix
inline void COGLES1Driver::getGLTextureMatrix(GLfloat *o, const core::matrix4 &m)
{
	o[0] = m[0];
	o[1] = m[1];
	o[2] = 0.f;
	o[3] = 0.f;

	o[4] = m[4];
	o[5] = m[5];
	o[6] = 0.f;
	o[7] = 0.f;

	o[8] = 0.f;
	o[9] = 0.f;
	o[10] = 1.f;
	o[11] = 0.f;

	o[12] = m[8];
	o[13] = m[9];
	o[14] = 0.f;
	o[15] = 1.f;
}

ITexture *COGLES1Driver::createDeviceDependentTexture(const io::path &name, IImage *image)
{
	std::vector<IImage*> tmp { image };

	COGLES1Texture *texture = new COGLES1Texture(name, tmp, ETT_2D, this);

	return texture;
}

ITexture *COGLES1Driver::createDeviceDependentTextureCubemap(const io::path &name, const std::vector<IImage *> &image)
{
	COGLES1Texture *texture = new COGLES1Texture(name, image, ETT_CUBEMAP, this);

	return texture;
}

//! Sets a material. All 3d drawing functions draw geometry now using this material.
void COGLES1Driver::setMaterial(const SMaterial &material)
{
	Material = material;
	OverrideMaterial.apply(Material);

	for (u32 i = 0; i < Feature.MaxTextureUnits; ++i)
		setTransform((E_TRANSFORMATION_STATE)(ETS_TEXTURE_0 + i), material.getTextureMatrix(i));
}

//! prints error if an error happened.
bool COGLES1Driver::testGLError(int code)
{
	if (!Params.DriverDebug)
		return false;

	GLenum g = glGetError();
	switch (g) {
	case GL_NO_ERROR:
		return false;
	case GL_INVALID_ENUM:
		os::Printer::log("GL_INVALID_ENUM", core::stringc(code).c_str(), ELL_ERROR);
		break;
	case GL_INVALID_VALUE:
		os::Printer::log("GL_INVALID_VALUE", core::stringc(code).c_str(), ELL_ERROR);
		break;
	case GL_INVALID_OPERATION:
		os::Printer::log("GL_INVALID_OPERATION", core::stringc(code).c_str(), ELL_ERROR);
		break;
	case GL_STACK_OVERFLOW:
		os::Printer::log("GL_STACK_OVERFLOW", core::stringc(code).c_str(), ELL_ERROR);
		break;
	case GL_STACK_UNDERFLOW:
		os::Printer::log("GL_STACK_UNDERFLOW", core::stringc(code).c_str(), ELL_ERROR);
		break;
	case GL_OUT_OF_MEMORY:
		os::Printer::log("GL_OUT_OF_MEMORY", core::stringc(code).c_str(), ELL_ERROR);
		break;
	};
	return true;
}

//! sets the needed renderstates
void COGLES1Driver::setRenderStates3DMode()
{
	if (CurrentRenderMode != ERM_3D) {
		// Reset Texture Stages
		CacheHandler->setBlend(false);
		glDisable(GL_ALPHA_TEST);
		CacheHandler->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// switch back the matrices
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((Matrices[ETS_VIEW] * Matrices[ETS_WORLD]).pointer());

		GLfloat glmat[16];
		getGLMatrix(glmat, Matrices[ETS_PROJECTION]);
		glmat[12] *= -1.0f;
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(glmat);

		ResetRenderStates = true;
	}

	if (ResetRenderStates || LastMaterial != Material) {
		// unset old material

		if (LastMaterial.MaterialType != Material.MaterialType &&
				static_cast<u32>(LastMaterial.MaterialType) < MaterialRenderers.size())
			MaterialRenderers[LastMaterial.MaterialType].Renderer->OnUnsetMaterial();

		// set new material.
		if (static_cast<u32>(Material.MaterialType) < MaterialRenderers.size())
			MaterialRenderers[Material.MaterialType].Renderer->OnSetMaterial(
					Material, LastMaterial, ResetRenderStates, this);

		LastMaterial = Material;
		CacheHandler->correctCacheMaterial(LastMaterial);
		ResetRenderStates = false;
	}

	if (static_cast<u32>(Material.MaterialType) < MaterialRenderers.size())
		MaterialRenderers[Material.MaterialType].Renderer->OnRender(this, video::EVT_STANDARD);

	CurrentRenderMode = ERM_3D;
}

GLint COGLES1Driver::getTextureWrapMode(u8 clamp) const
{
	switch (clamp) {
	case ETC_CLAMP:
		//	return GL_CLAMP; not supported in ogl-es
		return GL_CLAMP_TO_EDGE;
		break;
	case ETC_CLAMP_TO_EDGE:
		return GL_CLAMP_TO_EDGE;
		break;
	case ETC_CLAMP_TO_BORDER:
		//	return GL_CLAMP_TO_BORDER; not supported in ogl-es
		return GL_CLAMP_TO_EDGE;
		break;
	case ETC_MIRROR:
#ifdef GL_OES_texture_mirrored_repeat
		if (FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_texture_mirrored_repeat])
			return GL_MIRRORED_REPEAT_OES;
		else
#endif
			return GL_REPEAT;
		break;
	// the next three are not yet supported at all
	case ETC_MIRROR_CLAMP:
	case ETC_MIRROR_CLAMP_TO_EDGE:
	case ETC_MIRROR_CLAMP_TO_BORDER:
#ifdef GL_OES_texture_mirrored_repeat
		if (FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_texture_mirrored_repeat])
			return GL_MIRRORED_REPEAT_OES;
		else
#endif
			return GL_CLAMP_TO_EDGE;
		break;
	case ETC_REPEAT:
	default:
		return GL_REPEAT;
		break;
	}
}

//! Can be called by an IMaterialRenderer to make its work easier.
void COGLES1Driver::setBasicRenderStates(const SMaterial &material, const SMaterial &lastmaterial,
		bool resetAllRenderStates)
{
	if (resetAllRenderStates ||
			lastmaterial.ColorMaterial != material.ColorMaterial) {
		// we only have diffuse_and_ambient in ogl-es
		if (material.ColorMaterial == ECM_DIFFUSE_AND_AMBIENT)
			glEnable(GL_COLOR_MATERIAL);
		else
			glDisable(GL_COLOR_MATERIAL);
	}

	if (resetAllRenderStates ||
			lastmaterial.AmbientColor != material.AmbientColor ||
			lastmaterial.DiffuseColor != material.DiffuseColor ||
			lastmaterial.EmissiveColor != material.EmissiveColor ||
			lastmaterial.ColorMaterial != material.ColorMaterial) {
		GLfloat color[4];

		const f32 inv = 1.0f / 255.0f;

		if ((material.ColorMaterial != video::ECM_AMBIENT) &&
				(material.ColorMaterial != video::ECM_DIFFUSE_AND_AMBIENT)) {
			color[0] = material.AmbientColor.getRed() * inv;
			color[1] = material.AmbientColor.getGreen() * inv;
			color[2] = material.AmbientColor.getBlue() * inv;
			color[3] = material.AmbientColor.getAlpha() * inv;
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
		}

		if ((material.ColorMaterial != video::ECM_DIFFUSE) &&
				(material.ColorMaterial != video::ECM_DIFFUSE_AND_AMBIENT)) {
			color[0] = material.DiffuseColor.getRed() * inv;
			color[1] = material.DiffuseColor.getGreen() * inv;
			color[2] = material.DiffuseColor.getBlue() * inv;
			color[3] = material.DiffuseColor.getAlpha() * inv;
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
		}

		if (material.ColorMaterial != video::ECM_EMISSIVE) {
			color[0] = material.EmissiveColor.getRed() * inv;
			color[1] = material.EmissiveColor.getGreen() * inv;
			color[2] = material.EmissiveColor.getBlue() * inv;
			color[3] = material.EmissiveColor.getAlpha() * inv;
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
		}
	}

	if (resetAllRenderStates ||
			lastmaterial.SpecularColor != material.SpecularColor ||
			lastmaterial.Shininess != material.Shininess) {
		GLfloat color[] = {0.f, 0.f, 0.f, 1.f};
		const f32 inv = 1.0f / 255.0f;

		// disable Specular colors if no shininess is set
		if ((material.Shininess != 0.0f) &&
				(material.ColorMaterial != video::ECM_SPECULAR)) {
#ifdef GL_EXT_separate_specular_color
			if (FeatureAvailable[IRR_EXT_separate_specular_color])
				glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
#endif
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material.Shininess);
			color[0] = material.SpecularColor.getRed() * inv;
			color[1] = material.SpecularColor.getGreen() * inv;
			color[2] = material.SpecularColor.getBlue() * inv;
			color[3] = material.SpecularColor.getAlpha() * inv;
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
		}
#ifdef GL_EXT_separate_specular_color
		else if (FeatureAvailable[IRR_EXT_separate_specular_color])
			glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
#endif
	}

	// TODO ogl-es
	// fillmode
	//	if (resetAllRenderStates || (lastmaterial.Wireframe != material.Wireframe) || (lastmaterial.PointCloud != material.PointCloud))
	//		glPolygonMode(GL_FRONT_AND_BACK, material.Wireframe ? GL_LINE : material.PointCloud? GL_POINT : GL_FILL);

	// shademode
	if (resetAllRenderStates || (lastmaterial.GouraudShading != material.GouraudShading)) {
		if (material.GouraudShading)
			glShadeModel(GL_SMOOTH);
		else
			glShadeModel(GL_FLAT);
	}

	// lighting
	if (resetAllRenderStates || (lastmaterial.Lighting != material.Lighting)) {
		if (material.Lighting)
			glEnable(GL_LIGHTING);
		else
			glDisable(GL_LIGHTING);
	}

	// zbuffer
	if (resetAllRenderStates || lastmaterial.ZBuffer != material.ZBuffer) {
		switch (material.ZBuffer) {
		case ECFN_DISABLED:
			glDisable(GL_DEPTH_TEST);
			break;
		case ECFN_LESSEQUAL:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			break;
		case ECFN_EQUAL:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_EQUAL);
			break;
		case ECFN_LESS:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			break;
		case ECFN_NOTEQUAL:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_NOTEQUAL);
			break;
		case ECFN_GREATEREQUAL:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_GEQUAL);
			break;
		case ECFN_GREATER:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_GREATER);
			break;
		case ECFN_ALWAYS:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_ALWAYS);
			break;
		case ECFN_NEVER:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_NEVER);
			break;
		}
	}

	// zwrite
	if (getWriteZBuffer(material)) {
		glDepthMask(GL_TRUE);
	} else {
		glDepthMask(GL_FALSE);
	}

	// back face culling
	if (resetAllRenderStates || (lastmaterial.FrontfaceCulling != material.FrontfaceCulling) || (lastmaterial.BackfaceCulling != material.BackfaceCulling)) {
		if ((material.FrontfaceCulling) && (material.BackfaceCulling)) {
			glCullFace(GL_FRONT_AND_BACK);
			glEnable(GL_CULL_FACE);
		} else if (material.BackfaceCulling) {
			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
		} else if (material.FrontfaceCulling) {
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);
		} else
			glDisable(GL_CULL_FACE);
	}

	// fog
	if (resetAllRenderStates || lastmaterial.FogEnable != material.FogEnable) {
		if (material.FogEnable)
			glEnable(GL_FOG);
		else
			glDisable(GL_FOG);
	}

	// normalization
	if (resetAllRenderStates || lastmaterial.NormalizeNormals != material.NormalizeNormals) {
		if (material.NormalizeNormals)
			glEnable(GL_NORMALIZE);
		else
			glDisable(GL_NORMALIZE);
	}

	// Color Mask
	if (resetAllRenderStates || lastmaterial.ColorMask != material.ColorMask) {
		glColorMask(
				(material.ColorMask & ECP_RED) ? GL_TRUE : GL_FALSE,
				(material.ColorMask & ECP_GREEN) ? GL_TRUE : GL_FALSE,
				(material.ColorMask & ECP_BLUE) ? GL_TRUE : GL_FALSE,
				(material.ColorMask & ECP_ALPHA) ? GL_TRUE : GL_FALSE);
	}

	// Blend Equation
	if (material.BlendOperation == EBO_NONE)
		CacheHandler->setBlend(false);
	else {
		CacheHandler->setBlend(true);

		if (queryFeature(EVDF_BLEND_OPERATIONS)) {
			switch (material.BlendOperation) {
			case EBO_ADD:
#if defined(GL_OES_blend_subtract)
				CacheHandler->setBlendEquation(GL_FUNC_ADD_OES);
#endif
				break;
			case EBO_SUBTRACT:
#if defined(GL_OES_blend_subtract)
				CacheHandler->setBlendEquation(GL_FUNC_SUBTRACT_OES);
#endif
				break;
			case EBO_REVSUBTRACT:
#if defined(GL_OES_blend_subtract)
				CacheHandler->setBlendEquation(GL_FUNC_REVERSE_SUBTRACT_OES);
#endif
				break;
			default:
				break;
			}
		}
	}

	// Blend Factor
	if (IR(material.BlendFactor) & 0xFFFFFFFF // TODO: why the & 0xFFFFFFFF?
			&& material.MaterialType != EMT_ONETEXTURE_BLEND) {
		E_BLEND_FACTOR srcRGBFact = EBF_ZERO;
		E_BLEND_FACTOR dstRGBFact = EBF_ZERO;
		E_BLEND_FACTOR srcAlphaFact = EBF_ZERO;
		E_BLEND_FACTOR dstAlphaFact = EBF_ZERO;
		E_MODULATE_FUNC modulo = EMFN_MODULATE_1X;
		u32 alphaSource = 0;

		unpack_textureBlendFuncSeparate(srcRGBFact, dstRGBFact, srcAlphaFact, dstAlphaFact, modulo, alphaSource, material.BlendFactor);

		if (queryFeature(EVDF_BLEND_SEPARATE)) {
			CacheHandler->setBlendFuncSeparate(getGLBlend(srcRGBFact), getGLBlend(dstRGBFact),
					getGLBlend(srcAlphaFact), getGLBlend(dstAlphaFact));
		} else {
			CacheHandler->setBlendFunc(getGLBlend(srcRGBFact), getGLBlend(dstRGBFact));
		}
	}

	// TODO: Polygon Offset. Not sure if it was left out deliberately or if it won't work with this driver.

	// thickness
	if (resetAllRenderStates || lastmaterial.Thickness != material.Thickness) {
		if (AntiAlias) {
			//			glPointSize(core::clamp(static_cast<GLfloat>(material.Thickness), DimSmoothedPoint[0], DimSmoothedPoint[1]));
			// we don't use point smoothing
			glPointSize(core::clamp(static_cast<GLfloat>(material.Thickness), DimAliasedPoint[0], DimAliasedPoint[1]));
		} else {
			glPointSize(core::clamp(static_cast<GLfloat>(material.Thickness), DimAliasedPoint[0], DimAliasedPoint[1]));
			glLineWidth(core::clamp(static_cast<GLfloat>(material.Thickness), DimAliasedLine[0], DimAliasedLine[1]));
		}
	}

	// Anti aliasing
	if (resetAllRenderStates || lastmaterial.AntiAliasing != material.AntiAliasing) {
		if (material.AntiAliasing & EAAM_ALPHA_TO_COVERAGE)
			glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		else if (lastmaterial.AntiAliasing & EAAM_ALPHA_TO_COVERAGE)
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		if ((AntiAlias >= 2) && (material.AntiAliasing & (EAAM_SIMPLE | EAAM_QUALITY)))
			glEnable(GL_MULTISAMPLE);
		else
			glDisable(GL_MULTISAMPLE);
	}

	// Texture parameters
	setTextureRenderStates(material, resetAllRenderStates);
}

//! Compare in SMaterial doesn't check texture parameters, so we should call this on each OnRender call.
void COGLES1Driver::setTextureRenderStates(const SMaterial &material, bool resetAllRenderstates)
{
	// Set textures to TU/TIU and apply filters to them

	for (s32 i = Feature.MaxTextureUnits - 1; i >= 0; --i) {
		CacheHandler->getTextureCache().set(i, material.TextureLayers[i].Texture);

		const COGLES1Texture *tmpTexture = CacheHandler->getTextureCache().get(i);

		if (!tmpTexture)
			continue;

		GLenum tmpTextureType = tmpTexture->getOpenGLTextureType();

		CacheHandler->setActiveTexture(GL_TEXTURE0 + i);

		{
			const bool isRTT = tmpTexture->isRenderTarget();

			glMatrixMode(GL_TEXTURE);

			if (!isRTT && Matrices[ETS_TEXTURE_0 + i].isIdentity())
				glLoadIdentity();
			else {
				GLfloat glmat[16];
				if (isRTT)
					getGLTextureMatrix(glmat, Matrices[ETS_TEXTURE_0 + i] * TextureFlipMatrix);
				else
					getGLTextureMatrix(glmat, Matrices[ETS_TEXTURE_0 + i]);
				glLoadMatrixf(glmat);
			}
		}

		COGLES1Texture::SStatesCache &statesCache = tmpTexture->getStatesCache();

		if (resetAllRenderstates)
			statesCache.IsCached = false;

#if defined(GL_EXT_texture_lod_bias)
		if (FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_EXT_texture_lod_bias]) {
			if (material.TextureLayers[i].LODBias) {
				const float tmp = core::clamp(material.TextureLayers[i].LODBias * 0.125f, -MaxTextureLODBias, MaxTextureLODBias);
				glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, tmp);
			} else
				glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, 0.f);
		}
#endif

		if (!statesCache.IsCached || material.TextureLayers[i].MagFilter != statesCache.MagFilter) {
			E_TEXTURE_MAG_FILTER magFilter = material.TextureLayers[i].MagFilter;
			glTexParameteri(tmpTextureType, GL_TEXTURE_MAG_FILTER,
					magFilter == ETMAGF_NEAREST ? GL_NEAREST : (assert(magFilter == ETMAGF_LINEAR), GL_LINEAR));

			statesCache.MagFilter = magFilter;
		}

		if (material.UseMipMaps && tmpTexture->hasMipMaps()) {
			if (!statesCache.IsCached || material.TextureLayers[i].MinFilter != statesCache.MinFilter ||
					!statesCache.MipMapStatus) {
				E_TEXTURE_MIN_FILTER minFilter = material.TextureLayers[i].MinFilter;
				glTexParameteri(tmpTextureType, GL_TEXTURE_MIN_FILTER,
						minFilter == ETMINF_NEAREST_MIPMAP_NEAREST ? GL_NEAREST_MIPMAP_NEAREST : minFilter == ETMINF_LINEAR_MIPMAP_NEAREST ? GL_LINEAR_MIPMAP_NEAREST
																						 : minFilter == ETMINF_NEAREST_MIPMAP_LINEAR       ? GL_NEAREST_MIPMAP_LINEAR
																																		   : (assert(minFilter == ETMINF_LINEAR_MIPMAP_LINEAR), GL_LINEAR_MIPMAP_LINEAR));

				statesCache.MinFilter = minFilter;
				statesCache.MipMapStatus = true;
			}
		} else {
			if (!statesCache.IsCached || material.TextureLayers[i].MinFilter != statesCache.MinFilter ||
					statesCache.MipMapStatus) {
				E_TEXTURE_MIN_FILTER minFilter = material.TextureLayers[i].MinFilter;
				glTexParameteri(tmpTextureType, GL_TEXTURE_MIN_FILTER,
						(minFilter == ETMINF_NEAREST_MIPMAP_NEAREST || minFilter == ETMINF_NEAREST_MIPMAP_LINEAR) ? GL_NEAREST : (assert(minFilter == ETMINF_LINEAR_MIPMAP_NEAREST || minFilter == ETMINF_LINEAR_MIPMAP_LINEAR), GL_LINEAR));

				statesCache.MinFilter = minFilter;
				statesCache.MipMapStatus = false;
			}
		}

#ifdef GL_EXT_texture_filter_anisotropic
		if (FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_EXT_texture_filter_anisotropic] &&
				(!statesCache.IsCached || material.TextureLayers[i].AnisotropicFilter != statesCache.AnisotropicFilter)) {
			glTexParameteri(tmpTextureType, GL_TEXTURE_MAX_ANISOTROPY_EXT,
					material.TextureLayers[i].AnisotropicFilter > 1 ? core::min_(MaxAnisotropy, material.TextureLayers[i].AnisotropicFilter) : 1);

			statesCache.AnisotropicFilter = material.TextureLayers[i].AnisotropicFilter;
		}
#endif

		if (!statesCache.IsCached || material.TextureLayers[i].TextureWrapU != statesCache.WrapU) {
			glTexParameteri(tmpTextureType, GL_TEXTURE_WRAP_S, getTextureWrapMode(material.TextureLayers[i].TextureWrapU));
			statesCache.WrapU = material.TextureLayers[i].TextureWrapU;
		}

		if (!statesCache.IsCached || material.TextureLayers[i].TextureWrapV != statesCache.WrapV) {
			glTexParameteri(tmpTextureType, GL_TEXTURE_WRAP_T, getTextureWrapMode(material.TextureLayers[i].TextureWrapV));
			statesCache.WrapV = material.TextureLayers[i].TextureWrapV;
		}

		statesCache.IsCached = true;
	}

	// be sure to leave in texture stage 0
	CacheHandler->setActiveTexture(GL_TEXTURE0);
}

//! sets the needed renderstates
void COGLES1Driver::setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel)
{
	if (CurrentRenderMode != ERM_2D || Transformation3DChanged) {
		// unset last 3d material
		if (CurrentRenderMode == ERM_3D) {
			if (static_cast<u32>(LastMaterial.MaterialType) < MaterialRenderers.size())
				MaterialRenderers[LastMaterial.MaterialType].Renderer->OnUnsetMaterial();
		}
		if (Transformation3DChanged) {
			glMatrixMode(GL_PROJECTION);

			const core::dimension2d<u32> &renderTargetSize = getCurrentRenderTargetSize();
			core::matrix4 m(core::matrix4::EM4CONST_NOTHING);
			m.buildProjectionMatrixOrthoLH(f32(renderTargetSize.Width), f32(-(s32)(renderTargetSize.Height)), -1.0f, 1.0f);
			m.setTranslation(core::vector3df(-1, 1, 0));
			glLoadMatrixf(m.pointer());

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			Transformation3DChanged = false;
		}
	}

	Material = (OverrideMaterial2DEnabled) ? OverrideMaterial2D : InitMaterial2D;
	Material.Lighting = false;
	Material.TextureLayers[0].Texture = (texture) ? const_cast<COGLES1Texture *>(CacheHandler->getTextureCache().get(0)) : 0;
	setTransform(ETS_TEXTURE_0, core::IdentityMatrix);

	setBasicRenderStates(Material, LastMaterial, false);

	LastMaterial = Material;
	CacheHandler->correctCacheMaterial(LastMaterial);

	// no alphaChannel without texture
	alphaChannel &= texture;

	if (alphaChannel || alpha) {
		CacheHandler->setBlend(true);
		CacheHandler->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		CacheHandler->setBlendEquation(GL_FUNC_ADD);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.f);
	} else {
		CacheHandler->setBlend(false);
		glDisable(GL_ALPHA_TEST);
	}

	if (texture) {
		// Due to the transformation change, the previous line would call a reset each frame
		// but we can safely reset the variable as it was false before
		Transformation3DChanged = false;

		if (alphaChannel) {
			// if alpha and alpha texture just modulate, otherwise use only the alpha channel
			if (alpha) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			} else {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
				glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
				// rgb always modulates
				glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
				glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
				glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
			}
		} else {
			if (alpha) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
				glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR);
				// rgb always modulates
				glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
				glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
				glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
			} else {
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			}
		}
	}

	CurrentRenderMode = ERM_2D;
}

//! \return Returns the name of the video driver.
const char *COGLES1Driver::getName() const
{
	return Name.c_str();
}

//! Sets the dynamic ambient light color.
void COGLES1Driver::setAmbientLight(const SColorf &color)
{
	CNullDriver::setAmbientLight(color);
	GLfloat data[4] = {color.r, color.g, color.b, color.a};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, data);
}

// this code was sent in by Oliver Klems, thank you
void COGLES1Driver::setViewPort(const core::rect<s32> &area)
{
	core::rect<s32> vp = area;
	core::rect<s32> rendert(0, 0, getCurrentRenderTargetSize().Width, getCurrentRenderTargetSize().Height);
	vp.clipAgainst(rendert);

	if (vp.getHeight() > 0 && vp.getWidth() > 0)
		CacheHandler->setViewport(vp.UpperLeftCorner.X, getCurrentRenderTargetSize().Height - vp.UpperLeftCorner.Y - vp.getHeight(), vp.getWidth(), vp.getHeight());

	ViewPort = vp;
}

void COGLES1Driver::setViewPortRaw(u32 width, u32 height)
{
	CacheHandler->setViewport(0, 0, width, height);
	ViewPort = core::recti(0, 0, width, height);
}

//! Sets the fog mode.
void COGLES1Driver::setFog(SColor c, E_FOG_TYPE fogType, f32 start,
		f32 end, f32 density, bool pixelFog, bool rangeFog)
{
	CNullDriver::setFog(c, fogType, start, end, density, pixelFog, rangeFog);

	glFogf(GL_FOG_MODE, GLfloat((fogType == EFT_FOG_LINEAR) ? GL_LINEAR : (fogType == EFT_FOG_EXP) ? GL_EXP
																								   : GL_EXP2));

#ifdef GL_EXT_fog_coord
	if (FeatureAvailable[IRR_EXT_fog_coord])
		glFogi(GL_FOG_COORDINATE_SOURCE, GL_FRAGMENT_DEPTH);
#endif

	if (fogType == EFT_FOG_LINEAR) {
		glFogf(GL_FOG_START, start);
		glFogf(GL_FOG_END, end);
	} else
		glFogf(GL_FOG_DENSITY, density);

	if (pixelFog)
		glHint(GL_FOG_HINT, GL_NICEST);
	else
		glHint(GL_FOG_HINT, GL_FASTEST);

	SColorf color(c);
	GLfloat data[4] = {color.r, color.g, color.b, color.a};
	glFogfv(GL_FOG_COLOR, data);
}

//! Draws a 3d line.
void COGLES1Driver::draw3DLine(const core::vector3df &start,
		const core::vector3df &end, SColor color)
{
	setRenderStates3DMode();

	u16 indices[] = {0, 1};
	S3DVertex vertices[2];
	vertices[0] = S3DVertex(start.X, start.Y, start.Z, 0, 0, 1, color, 0, 0);
	vertices[1] = S3DVertex(end.X, end.Y, end.Z, 0, 0, 1, color, 0, 0);
	drawVertexPrimitiveList2d3d(vertices, 2, indices, 1, video::EVT_STANDARD, scene::EPT_LINES);
}

//! Only used by the internal engine. Used to notify the driver that
//! the window was resized.
void COGLES1Driver::OnResize(const core::dimension2d<u32> &size)
{
	CNullDriver::OnResize(size);
	CacheHandler->setViewport(0, 0, size.Width, size.Height);
	Transformation3DChanged = true;
}

//! Returns type of video driver
E_DRIVER_TYPE COGLES1Driver::getDriverType() const
{
	return EDT_OGLES1;
}

//! returns color format
ECOLOR_FORMAT COGLES1Driver::getColorFormat() const
{
	return ColorFormat;
}

//! Get a vertex shader constant index.
s32 COGLES1Driver::getVertexShaderConstantID(const c8 *name)
{
	return getPixelShaderConstantID(name);
}

//! Get a pixel shader constant index.
s32 COGLES1Driver::getPixelShaderConstantID(const c8 *name)
{
	os::Printer::log("Error: Please use IMaterialRendererServices from IShaderConstantSetCallBack::OnSetConstants not VideoDriver->getPixelShaderConstantID().");
	return -1;
}

//! Sets a constant for the vertex shader based on an index.
bool COGLES1Driver::setVertexShaderConstant(s32 index, const f32 *floats, int count)
{
	// pass this along, as in GLSL the same routine is used for both vertex and fragment shaders
	return setPixelShaderConstant(index, floats, count);
}

//! Int interface for the above.
bool COGLES1Driver::setVertexShaderConstant(s32 index, const s32 *ints, int count)
{
	return setPixelShaderConstant(index, ints, count);
}

bool COGLES1Driver::setVertexShaderConstant(s32 index, const u32 *ints, int count)
{
	return setPixelShaderConstant(index, ints, count);
}

//! Sets a constant for the pixel shader based on an index.
bool COGLES1Driver::setPixelShaderConstant(s32 index, const f32 *floats, int count)
{
	os::Printer::log("Error: Please use IMaterialRendererServices from IShaderConstantSetCallBack::OnSetConstants not VideoDriver->setPixelShaderConstant().");
	return false;
}

//! Int interface for the above.
bool COGLES1Driver::setPixelShaderConstant(s32 index, const s32 *ints, int count)
{
	os::Printer::log("Error: Please use IMaterialRendererServices from IShaderConstantSetCallBack::OnSetConstants not VideoDriver->setPixelShaderConstant().");
	return false;
}

bool COGLES1Driver::setPixelShaderConstant(s32 index, const u32 *ints, int count)
{
	os::Printer::log("Error: Please use IMaterialRendererServices from IShaderConstantSetCallBack::OnSetConstants not VideoDriver->setPixelShaderConstant().");
	return false;
}

//! Adds a new material renderer to the VideoDriver, using GLSL to render geometry.
s32 COGLES1Driver::addHighLevelShaderMaterial(
		const c8 *vertexShaderProgram,
		const c8 *vertexShaderEntryPointName,
		E_VERTEX_SHADER_TYPE vsCompileTarget,
		const c8 *pixelShaderProgram,
		const c8 *pixelShaderEntryPointName,
		E_PIXEL_SHADER_TYPE psCompileTarget,
		const c8 *geometryShaderProgram,
		const c8 *geometryShaderEntryPointName,
		E_GEOMETRY_SHADER_TYPE gsCompileTarget,
		scene::E_PRIMITIVE_TYPE inType,
		scene::E_PRIMITIVE_TYPE outType,
		u32 verticesOut,
		IShaderConstantSetCallBack *callback,
		E_MATERIAL_TYPE baseMaterial,
		s32 userData)
{
	os::Printer::log("No shader support.");
	return -1;
}

//! Returns a pointer to the IVideoDriver interface. (Implementation for
//! IMaterialRendererServices)
IVideoDriver *COGLES1Driver::getVideoDriver()
{
	return this;
}

//! Returns pointer to the IGPUProgrammingServices interface.
IGPUProgrammingServices *COGLES1Driver::getGPUProgrammingServices()
{
	return this;
}

ITexture *COGLES1Driver::addRenderTargetTexture(const core::dimension2d<u32> &size,
		const io::path &name, const ECOLOR_FORMAT format)
{
	// disable mip-mapping
	bool generateMipLevels = getTextureCreationFlag(ETCF_CREATE_MIP_MAPS);
	setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false);

	bool supportForFBO = (Feature.ColorAttachment > 0);

	core::dimension2du destSize(size);

	if (!supportForFBO) {
		destSize = core::dimension2d<u32>(core::min_(size.Width, ScreenSize.Width), core::min_(size.Height, ScreenSize.Height));
		destSize = destSize.getOptimalSize((size == size.getOptimalSize()), false, false);
	}

	COGLES1Texture *renderTargetTexture = new COGLES1Texture(name, destSize, ETT_2D, format, this);
	addTexture(renderTargetTexture);
	renderTargetTexture->drop();

	// restore mip-mapping
	setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, generateMipLevels);

	return renderTargetTexture;
}

ITexture *COGLES1Driver::addRenderTargetTextureCubemap(const irr::u32 sideLen, const io::path &name, const ECOLOR_FORMAT format)
{
	// disable mip-mapping
	bool generateMipLevels = getTextureCreationFlag(ETCF_CREATE_MIP_MAPS);
	setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false);

	bool supportForFBO = (Feature.ColorAttachment > 0);

	const core::dimension2d<u32> size(sideLen, sideLen);
	core::dimension2du destSize(size);

	if (!supportForFBO) {
		destSize = core::dimension2d<u32>(core::min_(size.Width, ScreenSize.Width), core::min_(size.Height, ScreenSize.Height));
		destSize = destSize.getOptimalSize((size == size.getOptimalSize()), false, false);
	}

	COGLES1Texture *renderTargetTexture = new COGLES1Texture(name, destSize, ETT_CUBEMAP, format, this);
	addTexture(renderTargetTexture);
	renderTargetTexture->drop();

	// restore mip-mapping
	setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, generateMipLevels);

	return renderTargetTexture;
}

//! Returns the maximum amount of primitives
u32 COGLES1Driver::getMaximalPrimitiveCount() const
{
	return 65535;
}

bool COGLES1Driver::setRenderTargetEx(IRenderTarget *target, u16 clearFlag, SColor clearColor, f32 clearDepth, u8 clearStencil)
{
	if (target && target->getDriverType() != EDT_OGLES1) {
		os::Printer::log("Fatal Error: Tried to set a render target not owned by OpenGL driver.", ELL_ERROR);
		return false;
	}

	bool supportForFBO = (Feature.ColorAttachment > 0);

	core::dimension2d<u32> destRenderTargetSize(0, 0);

	if (target) {
		COGLES1RenderTarget *renderTarget = static_cast<COGLES1RenderTarget *>(target);

		if (supportForFBO) {
			CacheHandler->setFBO(renderTarget->getBufferID());
			renderTarget->update();
		}

		destRenderTargetSize = renderTarget->getSize();

		setViewPortRaw(destRenderTargetSize.Width, destRenderTargetSize.Height);
	} else {
		if (supportForFBO)
			CacheHandler->setFBO(0);
		else {
			COGLES1RenderTarget *prevRenderTarget = static_cast<COGLES1RenderTarget *>(CurrentRenderTarget);
			COGLES1Texture *renderTargetTexture = static_cast<COGLES1Texture *>(prevRenderTarget->getTexture());

			if (renderTargetTexture) {
				const COGLES1Texture *prevTexture = CacheHandler->getTextureCache().get(0);

				CacheHandler->getTextureCache().set(0, renderTargetTexture);

				const core::dimension2d<u32> size = renderTargetTexture->getSize();
				glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, size.Width, size.Height);

				CacheHandler->getTextureCache().set(0, prevTexture);
			}
		}

		destRenderTargetSize = core::dimension2d<u32>(0, 0);

		setViewPortRaw(ScreenSize.Width, ScreenSize.Height);
	}

	if (CurrentRenderTargetSize != destRenderTargetSize) {
		CurrentRenderTargetSize = destRenderTargetSize;

		Transformation3DChanged = true;
	}

	CurrentRenderTarget = target;

	if (!supportForFBO) {
		clearFlag |= ECBF_COLOR;
		clearFlag |= ECBF_DEPTH;
	}

	clearBuffers(clearFlag, clearColor, clearDepth, clearStencil);

	return true;
}

void COGLES1Driver::clearBuffers(u16 flag, SColor color, f32 depth, u8 stencil)
{
	GLbitfield mask = 0;

	if (flag & ECBF_COLOR) {
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		const f32 inv = 1.0f / 255.0f;
		glClearColor(color.getRed() * inv, color.getGreen() * inv,
				color.getBlue() * inv, color.getAlpha() * inv);

		mask |= GL_COLOR_BUFFER_BIT;
	}

	if (flag & ECBF_DEPTH) {
		glDepthMask(GL_TRUE);
		glClearDepthf(depth);
		mask |= GL_DEPTH_BUFFER_BIT;
	}

	if (flag & ECBF_STENCIL) {
		glClearStencil(stencil);
		mask |= GL_STENCIL_BUFFER_BIT;
	}

	if (mask)
		glClear(mask);
}

//! Returns an image created from the last rendered frame.
// We want to read the front buffer to get the latest render finished.
// This is not possible under ogl-es, though, so one has to call this method
// outside of the render loop only.
IImage *COGLES1Driver::createScreenShot(video::ECOLOR_FORMAT format, video::E_RENDER_TARGET target)
{
	if (target == video::ERT_MULTI_RENDER_TEXTURES || target == video::ERT_RENDER_TEXTURE || target == video::ERT_STEREO_BOTH_BUFFERS)
		return 0;
	GLint internalformat = GL_RGBA;
	GLint type = GL_UNSIGNED_BYTE;
	if (false && (FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_IMG_read_format] || FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_OES_read_format] || FeatureAvailable[COGLESCoreExtensionHandler::IRR_GL_EXT_read_format_bgra])) {
#ifdef GL_IMPLEMENTATION_COLOR_READ_TYPE_OES
		glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, &internalformat);
		glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES, &type);
#endif
		// there are formats we don't support ATM
		if (GL_UNSIGNED_SHORT_4_4_4_4 == type)
			type = GL_UNSIGNED_SHORT_5_5_5_1;
#ifdef GL_EXT_read_format_bgra
		else if (GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT == type)
			type = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
#endif
	}

	IImage *newImage = 0;
	if ((GL_RGBA == internalformat)
#ifdef GL_EXT_read_format_bgra
			|| (GL_BGRA_EXT == internalformat)
#endif
	) {
		if (GL_UNSIGNED_BYTE == type)
			newImage = new CImage(ECF_A8R8G8B8, ScreenSize);
		else
			newImage = new CImage(ECF_A1R5G5B5, ScreenSize);
	} else {
		if (GL_UNSIGNED_BYTE == type)
			newImage = new CImage(ECF_R8G8B8, ScreenSize);
		else
			newImage = new CImage(ECF_R5G6B5, ScreenSize);
	}

	u8 *pixels = static_cast<u8 *>(newImage->getData());
	if (!pixels) {
		newImage->drop();
		return 0;
	}

	glReadPixels(0, 0, ScreenSize.Width, ScreenSize.Height, internalformat, type, pixels);

	// opengl images are horizontally flipped, so we have to fix that here.
	const s32 pitch = newImage->getPitch();
	u8 *p2 = pixels + (ScreenSize.Height - 1) * pitch;
	u8 *tmpBuffer = new u8[pitch];
	for (u32 i = 0; i < ScreenSize.Height; i += 2) {
		memcpy(tmpBuffer, pixels, pitch);
		memcpy(pixels, p2, pitch);
		memcpy(p2, tmpBuffer, pitch);
		pixels += pitch;
		p2 -= pitch;
	}
	delete[] tmpBuffer;

	if (testGLError(__LINE__)) {
		newImage->drop();
		return 0;
	}

	return newImage;
}

void COGLES1Driver::removeTexture(ITexture *texture)
{
	CacheHandler->getTextureCache().remove(texture);
	CNullDriver::removeTexture(texture);
}

core::dimension2du COGLES1Driver::getMaxTextureSize() const
{
	return core::dimension2du(MaxTextureSize, MaxTextureSize);
}

GLenum COGLES1Driver::getGLBlend(E_BLEND_FACTOR factor) const
{
	static GLenum const blendTable[] = {
			GL_ZERO,
			GL_ONE,
			GL_DST_COLOR,
			GL_ONE_MINUS_DST_COLOR,
			GL_SRC_COLOR,
			GL_ONE_MINUS_SRC_COLOR,
			GL_SRC_ALPHA,
			GL_ONE_MINUS_SRC_ALPHA,
			GL_DST_ALPHA,
			GL_ONE_MINUS_DST_ALPHA,
			GL_SRC_ALPHA_SATURATE,
		};

	return blendTable[factor];
}

GLenum COGLES1Driver::getZBufferBits() const
{
	GLenum bits = 0;

	switch (Params.ZBufferBits) {
	case 24:
#if defined(GL_OES_depth24)
		if (queryGLESFeature(COGLESCoreExtensionHandler::IRR_GL_OES_depth24))
			bits = GL_DEPTH_COMPONENT24_OES;
		else
#endif
			bits = GL_DEPTH_COMPONENT16;
		break;
	case 32:
#if defined(GL_OES_depth32)
		if (queryGLESFeature(COGLESCoreExtensionHandler::IRR_GL_OES_depth32))
			bits = GL_DEPTH_COMPONENT32_OES;
		else
#endif
			bits = GL_DEPTH_COMPONENT16;
		break;
	default:
		bits = GL_DEPTH_COMPONENT16;
		break;
	}

	return bits;
}

bool COGLES1Driver::getColorFormatParameters(ECOLOR_FORMAT format, GLint &internalFormat, GLenum &pixelFormat,
		GLenum &pixelType, void (**converter)(const void *, s32, void *)) const
{
	bool supported = false;
	internalFormat = GL_RGBA;
	pixelFormat = GL_RGBA;
	pixelType = GL_UNSIGNED_BYTE;
	*converter = 0;

	switch (format) {
	case ECF_A1R5G5B5:
		supported = true;
		internalFormat = GL_RGBA;
		pixelFormat = GL_RGBA;
		pixelType = GL_UNSIGNED_SHORT_5_5_5_1;
		*converter = CColorConverter::convert_A1R5G5B5toR5G5B5A1;
		break;
	case ECF_R5G6B5:
		supported = true;
		internalFormat = GL_RGB;
		pixelFormat = GL_RGB;
		pixelType = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case ECF_R8G8B8:
		supported = true;
		internalFormat = GL_RGB;
		pixelFormat = GL_RGB;
		pixelType = GL_UNSIGNED_BYTE;
		break;
	case ECF_A8R8G8B8:
		supported = true;
		if (queryGLESFeature(COGLESCoreExtensionHandler::IRR_GL_IMG_texture_format_BGRA8888) ||
				queryGLESFeature(COGLESCoreExtensionHandler::IRR_GL_EXT_texture_format_BGRA8888) ||
				queryGLESFeature(COGLESCoreExtensionHandler::IRR_GL_APPLE_texture_format_BGRA8888)) {
			internalFormat = GL_BGRA;
			pixelFormat = GL_BGRA;
		} else {
			internalFormat = GL_RGBA;
			pixelFormat = GL_RGBA;
			*converter = CColorConverter::convert_A8R8G8B8toA8B8G8R8;
		}
		pixelType = GL_UNSIGNED_BYTE;
		break;
	case ECF_D16:
		supported = true;
		internalFormat = GL_DEPTH_COMPONENT16;
		pixelFormat = GL_DEPTH_COMPONENT;
		pixelType = GL_UNSIGNED_SHORT;
		break;
	case ECF_D32:
#if defined(GL_OES_depth32)
		if (queryGLESFeature(COGLESCoreExtensionHandler::IRR_GL_OES_depth32)) {
			supported = true;
			internalFormat = GL_DEPTH_COMPONENT32_OES;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_INT;
		}
#endif
		break;
	case ECF_D24S8:
#ifdef GL_OES_packed_depth_stencil
		if (queryGLESFeature(COGLESCoreExtensionHandler::IRR_GL_OES_packed_depth_stencil)) {
			supported = true;
			internalFormat = GL_DEPTH24_STENCIL8_OES;
			pixelFormat = GL_DEPTH_STENCIL_OES;
			pixelType = GL_UNSIGNED_INT_24_8_OES;
		}
#endif
		break;
	case ECF_R8:
		break;
	case ECF_R8G8:
		break;
	case ECF_R16:
		break;
	case ECF_R16G16:
		break;
	case ECF_R16F:
		break;
	case ECF_G16R16F:
		break;
	case ECF_A16B16G16R16F:
		break;
	case ECF_R32F:
		break;
	case ECF_G32R32F:
		break;
	case ECF_A32B32G32R32F:
		break;
	default:
		break;
	}

#ifdef _IRR_IOS_PLATFORM_
	if (internalFormat == GL_BGRA)
		internalFormat = GL_RGBA;
#endif

	return supported;
}

bool COGLES1Driver::queryTextureFormat(ECOLOR_FORMAT format) const
{
	GLint dummyInternalFormat;
	GLenum dummyPixelFormat;
	GLenum dummyPixelType;
	void (*dummyConverter)(const void *, s32, void *);
	return getColorFormatParameters(format, dummyInternalFormat, dummyPixelFormat, dummyPixelType, &dummyConverter);
}

bool COGLES1Driver::needsTransparentRenderPass(const irr::video::SMaterial &material) const
{
	return CNullDriver::needsTransparentRenderPass(material) || material.isAlphaBlendOperation();
}

COGLES1CacheHandler *COGLES1Driver::getCacheHandler() const
{
	return CacheHandler;
}

} // end namespace
} // end namespace

#endif // _IRR_COMPILE_WITH_OGLES1_

namespace irr
{
namespace video
{

#ifndef _IRR_COMPILE_WITH_OGLES1_
class IVideoDriver;
class IContextManager;
#endif

IVideoDriver *createOGLES1Driver(const SIrrlichtCreationParameters &params, io::IFileSystem *io, IContextManager *contextManager)
{
#ifdef _IRR_COMPILE_WITH_OGLES1_
	return new COGLES1Driver(params, io, contextManager);
#else
	return 0;
#endif //  _IRR_COMPILE_WITH_OGLES1_
}

} // end namespace
} // end namespace
