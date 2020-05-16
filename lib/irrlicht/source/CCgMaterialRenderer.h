// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_CG_MATERIAL_RENDERER_H_INCLUDED__
#define __C_CG_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_CG_

#include "IMaterialRenderer.h"
#include "IMaterialRendererServices.h"
#include "IShaderConstantSetCallBack.h"
#include "IGPUProgrammingServices.h"
#include "irrArray.h"
#include "irrString.h"
#include "IVideoDriver.h"
#include "os.h"
#include "Cg/cg.h"

#ifdef _MSC_VER
	#pragma comment(lib, "cg.lib")
#endif

namespace irr
{
namespace video
{

class CCgUniform
{
public:
	CCgUniform(const CGparameter& parameter, bool global);

	const core::stringc& getName() const;
	const CGparameter& getParameter() const;
	CGenum getSpace() const;
	CGtype getType() const;

	virtual void update(const void* data, const SMaterial& material) const = 0;

protected:
	core::stringc Name;
	CGparameter Parameter;
	CGenum Space;
	CGtype Type;
};

class CCgUniform1f : public CCgUniform
{
public:
	CCgUniform1f(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniform2f : public CCgUniform
{
public:
	CCgUniform2f(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniform3f : public CCgUniform
{
public:
	CCgUniform3f(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniform4f : public CCgUniform
{
public:
	CCgUniform4f(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniform1i : public CCgUniform
{
public:
	CCgUniform1i(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniform2i : public CCgUniform
{
public:
	CCgUniform2i(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniform3i : public CCgUniform
{
public:
	CCgUniform3i(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniform4i : public CCgUniform
{
public:
	CCgUniform4i(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniform4x4f : public CCgUniform
{
public:
	CCgUniform4x4f(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgUniformSampler2D : public CCgUniform
{
public:
	CCgUniformSampler2D(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class CCgMaterialRenderer : public IMaterialRenderer, public IMaterialRendererServices
{
public:
	CCgMaterialRenderer(IShaderConstantSetCallBack* callback = 0, IMaterialRenderer* baseMaterial = 0, s32 userData = 0);
	virtual ~CCgMaterialRenderer();

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates, IMaterialRendererServices* services) = 0;
	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype) = 0;
	virtual void OnUnsetMaterial() = 0;

	virtual bool isTransparent() const;

	virtual void setBasicRenderStates(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates) = 0;
	virtual bool setVertexShaderConstant(const c8* name, const f32* floats, int count);
	virtual bool setVertexShaderConstant(const c8* name, const bool* bools, int count);
	virtual bool setVertexShaderConstant(const c8* name, const s32* ints, int count);
	virtual void setVertexShaderConstant(const f32* data, s32 startRegister, s32 constantAmount=1);
	virtual bool setPixelShaderConstant(const c8* name, const f32* floats, int count);
	virtual bool setPixelShaderConstant(const c8* name, const bool* bools, int count);
	virtual bool setPixelShaderConstant(const c8* name, const s32* ints, int count);
	virtual void setPixelShaderConstant(const f32* data, s32 startRegister, s32 constantAmount=1);
	virtual IVideoDriver* getVideoDriver() = 0;

protected:
	void getUniformList();

	IShaderConstantSetCallBack* CallBack;
	IMaterialRenderer* BaseMaterial;
	s32 UserData;

	core::array<CCgUniform*> UniformInfo;

	CGprogram VertexProgram;
	CGprogram FragmentProgram;
	CGprogram GeometryProgram;
	CGprofile VertexProfile;
	CGprofile FragmentProfile;
	CGprofile GeometryProfile;

	SMaterial Material;
	CGerror Error;
};

}
}

#endif
#endif
