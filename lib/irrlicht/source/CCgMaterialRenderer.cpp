// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_CG_

#include "CCgMaterialRenderer.h"

namespace irr
{
namespace video
{

CCgUniform::CCgUniform(const CGparameter& parameter, bool global) : Parameter(parameter), Type(CG_UNKNOWN_TYPE)
{
	Name = cgGetParameterName(Parameter);

	if(global)
		Space = CG_GLOBAL;
	else
		Space = CG_PROGRAM;
}

const core::stringc& CCgUniform::getName() const
{
	return Name;
}

const CGparameter& CCgUniform::getParameter() const
{
	return Parameter;
}

CGenum CCgUniform::getSpace() const
{
	return Space;
}

CGtype CCgUniform::getType() const
{
	return Type;
}

CCgUniform1f::CCgUniform1f(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_FLOAT;
}

void CCgUniform1f::update(const void* data, const SMaterial& material) const
{
	f32* Data = (f32*)data;
	cgSetParameter1f(Parameter, *Data);
}

CCgUniform2f::CCgUniform2f(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_FLOAT2;
}

void CCgUniform2f::update(const void* data, const SMaterial& material) const
{
	f32* Data = (f32*)data;
	cgSetParameter2f(Parameter, *Data, *(Data+1));
}

CCgUniform3f::CCgUniform3f(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_FLOAT3;
}

void CCgUniform3f::update(const void* data, const SMaterial& material) const
{
	f32* Data = (f32*)data;
	cgSetParameter3f(Parameter, *Data, *(Data+1), *(Data+2));
}

CCgUniform4f::CCgUniform4f(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_FLOAT4;
}

void CCgUniform4f::update(const void* data, const SMaterial& material) const
{
	f32* Data = (f32*)data;
	cgSetParameter4f(Parameter, *Data, *(Data+1), *(Data+2), *(Data+3));
}

CCgUniform1i::CCgUniform1i(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_INT;
}

void CCgUniform1i::update(const void* data, const SMaterial& material) const
{
	s32* Data = (s32*)data;
	cgSetParameter1i(Parameter, *Data);
}

CCgUniform2i::CCgUniform2i(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_INT2;
}

void CCgUniform2i::update(const void* data, const SMaterial& material) const
{
	s32* Data = (s32*)data;
	cgSetParameter2i(Parameter, *Data, *(Data+1));
}

CCgUniform3i::CCgUniform3i(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_INT3;
}

void CCgUniform3i::update(const void* data, const SMaterial& material) const
{
	s32* Data = (s32*)data;
	cgSetParameter3i(Parameter, *Data, *(Data+1), *(Data+2));
}

CCgUniform4i::CCgUniform4i(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_INT4;
}

void CCgUniform4i::update(const void* data, const SMaterial& material) const
{
	s32* Data = (s32*)data;
	cgSetParameter4i(Parameter, *Data, *(Data+1), *(Data+2), *(Data+3));
}

CCgUniform4x4f::CCgUniform4x4f(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_FLOAT4x4;
}

void CCgUniform4x4f::update(const void* data, const SMaterial& material) const
{
	f32* Data = (f32*)data;
	cgSetMatrixParameterfr(Parameter, Data);
}

CCgUniformSampler2D::CCgUniformSampler2D(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_SAMPLER2D;
}

void CCgUniformSampler2D::update(const void* data, const SMaterial& material) const
{
}

CCgMaterialRenderer::CCgMaterialRenderer(IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData) :
	CallBack(callback), BaseMaterial(baseMaterial), UserData(userData),
	VertexProgram(0), FragmentProgram(0), GeometryProgram(0), VertexProfile(CG_PROFILE_UNKNOWN), FragmentProfile(CG_PROFILE_UNKNOWN), GeometryProfile(CG_PROFILE_UNKNOWN),
	Material(IdentityMaterial), Error(CG_NO_ERROR)
{
	#ifdef _DEBUG
	setDebugName("CCgMaterialRenderer");
	#endif

	if(BaseMaterial)
		BaseMaterial->grab();

	if(CallBack)
		CallBack->grab();
}

CCgMaterialRenderer::~CCgMaterialRenderer()
{
	if(CallBack)
		CallBack->drop();

	if(BaseMaterial)
		BaseMaterial->drop();

	for(unsigned int i = 0; i < UniformInfo.size(); ++i)
		delete UniformInfo[i];

	UniformInfo.clear();
}

bool CCgMaterialRenderer::isTransparent() const
{
	return BaseMaterial ? BaseMaterial->isTransparent() : false;
}

void CCgMaterialRenderer::setVertexShaderConstant(const f32* data, s32 startRegister, s32 constantAmount)
{
	os::Printer::log("Cannot set constant, please use high level shader call instead.", ELL_WARNING);
}

bool CCgMaterialRenderer::setVertexShaderConstant(const c8* name, const f32* floats, int count)
{
	return setPixelShaderConstant(name, floats, count);
}

bool CCgMaterialRenderer::setVertexShaderConstant(const c8* name, const bool* bools, int count)
{
	return setPixelShaderConstant(name, bools, count);
}

bool CCgMaterialRenderer::setVertexShaderConstant(const c8* name, const s32* ints, int count)
{
	return setPixelShaderConstant(name, ints, count);
}

void CCgMaterialRenderer::setPixelShaderConstant(const f32* data, s32 startRegister, s32 constantAmount)
{
	os::Printer::log("Cannot set constant, please use high level shader call instead.", ELL_WARNING);
}

bool CCgMaterialRenderer::setPixelShaderConstant(const c8* name, const f32* floats, int count)
{
	bool Status = false;

	for(unsigned int i = 0; i < UniformInfo.size(); ++i)
	{
		if(UniformInfo[i]->getName() == name)
		{
			UniformInfo[i]->update(floats, Material);

			Status = true;
		}
	}

	return Status;
}

bool CCgMaterialRenderer::setPixelShaderConstant(const c8* name, const s32* ints, int count)
{
	bool Status = false;

	for(unsigned int i = 0; i < UniformInfo.size(); ++i)
	{
		if(UniformInfo[i]->getName() == name)
		{
			UniformInfo[i]->update(ints, Material);

			Status = true;
		}
	}

	return Status;
}

bool CCgMaterialRenderer::setPixelShaderConstant(const c8* name, const bool* bools, int count)
{
	bool Status = false;

	for(unsigned int i = 0; i < UniformInfo.size(); ++i)
	{
		if(UniformInfo[i]->getName() == name)
		{
			UniformInfo[i]->update(bools, Material);

			Status = true;
		}
	}

	return Status;
}

void CCgMaterialRenderer::getUniformList()
{
	for(unsigned int i = 0; i < UniformInfo.size(); ++i)
		delete UniformInfo[i];

	UniformInfo.clear();

	for(unsigned int i = 0; i < 2; ++i)
	{
		CGenum Space = CG_GLOBAL;
		bool IsGlobal = 1;

		if(i == 1)
		{
			Space = CG_PROGRAM;
			IsGlobal = 0;
		}

		for(unsigned int j = 0; j < 3; ++j)
		{
			CGprogram* Program = 0;

			switch(j)
			{
			case 0:
				Program = &VertexProgram;
				break;
			case 1:
				Program = &FragmentProgram;
				break;
			case 2:
				Program = &GeometryProgram;
				break;
			}

			if(*Program)
			{
				CGparameter Parameter = cgGetFirstParameter(*Program, Space);

				while(Parameter)
				{
					if(cgGetParameterVariability(Parameter) == CG_UNIFORM && cgGetParameterDirection(Parameter) == CG_IN)
					{
						CCgUniform* Uniform = 0;

						CGtype Type = cgGetParameterType(Parameter);

						switch(Type)
						{
						case CG_FLOAT:
						case CG_FLOAT1:
							Uniform = new CCgUniform1f(Parameter, IsGlobal);
							break;
						case CG_FLOAT2:
							Uniform = new CCgUniform2f(Parameter, IsGlobal);
							break;
						case CG_FLOAT3:
							Uniform = new CCgUniform3f(Parameter, IsGlobal);
							break;
						case CG_FLOAT4:
							Uniform = new CCgUniform4f(Parameter, IsGlobal);
							break;
						case CG_INT:
						case CG_INT1:
							Uniform = new CCgUniform1i(Parameter, IsGlobal);
							break;
						case CG_INT2:
							Uniform = new CCgUniform2i(Parameter, IsGlobal);
							break;
						case CG_INT3:
							Uniform = new CCgUniform3i(Parameter, IsGlobal);
							break;
						case CG_INT4:
							Uniform = new CCgUniform4i(Parameter, IsGlobal);
							break;
						case CG_FLOAT4x4:
							Uniform = new CCgUniform4x4f(Parameter, IsGlobal);
							break;
						case CG_SAMPLER2D:
							Uniform = new CCgUniformSampler2D(Parameter, IsGlobal);
							break;
						}

						if(Uniform)
							UniformInfo.push_back(Uniform);
					}

					Parameter = cgGetNextParameter(Parameter);
				}
			}
		}
	}
}

}
}

#endif
