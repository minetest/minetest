// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "EShaderTypes.h"
#include "EMaterialTypes.h"
#include "EPrimitiveTypes.h"
#include "path.h"

namespace irr
{

namespace io
{
class IReadFile;
} // end namespace io

namespace video
{

class IVideoDriver;
class IShaderConstantSetCallBack;

//! Interface making it possible to create and use programs running on the GPU.
class IGPUProgrammingServices
{
public:
	//! Destructor
	virtual ~IGPUProgrammingServices() {}

	//! Adds a new high-level shading material renderer to the VideoDriver.
	/** Currently only HLSL/D3D9 and GLSL/OpenGL are supported.
	\param vertexShaderProgram String containing the source of the vertex
	shader program. This can be 0 if no vertex program shall be used.
	\param vertexShaderEntryPointName Name of the entry function of the
	vertexShaderProgram (p.e. "main")
	\param vsCompileTarget Vertex shader version the high level shader
	shall be compiled to.
	\param pixelShaderProgram String containing the source of the pixel
	shader program. This can be 0 if no pixel shader shall be used.
	\param pixelShaderEntryPointName Entry name of the function of the
	pixelShaderProgram (p.e. "main")
	\param psCompileTarget Pixel shader version the high level shader
	shall be compiled to.
	\param geometryShaderProgram String containing the source of the
	geometry shader program. This can be 0 if no geometry shader shall be
	used.
	\param geometryShaderEntryPointName Entry name of the function of the
	geometryShaderProgram (p.e. "main")
	\param gsCompileTarget Geometry shader version the high level shader
	shall be compiled to.
	\param inType Type of vertices passed to geometry shader
	\param outType Type of vertices created by geometry shader
	\param verticesOut Maximal number of vertices created by geometry
	shader. If 0, maximal number supported is assumed.
	\param callback Pointer to an implementation of
	IShaderConstantSetCallBack in which you can set the needed vertex,
	pixel, and geometry shader program constants. Set this to 0 if you
	don't need this.
	\param baseMaterial Base material which renderstates will be used to
	shade the material.
	\param userData a user data int. This int can be set to any value and
	will be set as parameter in the callback method when calling
	OnSetConstants(). In this way it is easily possible to use the same
	callback method for multiple materials and distinguish between them
	during the call.
	\return Number of the material type which can be set in
	SMaterial::MaterialType to use the renderer. -1 is returned if an error
	occurred, e.g. if a shader program could not be compiled or a compile
	target is not reachable. The error strings are then printed to the
	error log and can be caught with a custom event receiver. */
	virtual s32 addHighLevelShaderMaterial(
			const c8 *vertexShaderProgram,
			const c8 *vertexShaderEntryPointName,
			E_VERTEX_SHADER_TYPE vsCompileTarget,
			const c8 *pixelShaderProgram,
			const c8 *pixelShaderEntryPointName,
			E_PIXEL_SHADER_TYPE psCompileTarget,
			const c8 *geometryShaderProgram,
			const c8 *geometryShaderEntryPointName = "main",
			E_GEOMETRY_SHADER_TYPE gsCompileTarget = EGST_GS_4_0,
			scene::E_PRIMITIVE_TYPE inType = scene::EPT_TRIANGLES,
			scene::E_PRIMITIVE_TYPE outType = scene::EPT_TRIANGLE_STRIP,
			u32 verticesOut = 0,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0) = 0;

	//! convenience function for use without geometry shaders
	s32 addHighLevelShaderMaterial(
			const c8 *vertexShaderProgram,
			const c8 *vertexShaderEntryPointName = "main",
			E_VERTEX_SHADER_TYPE vsCompileTarget = EVST_VS_1_1,
			const c8 *pixelShaderProgram = 0,
			const c8 *pixelShaderEntryPointName = "main",
			E_PIXEL_SHADER_TYPE psCompileTarget = EPST_PS_1_1,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0)
	{
		return addHighLevelShaderMaterial(
				vertexShaderProgram, vertexShaderEntryPointName,
				vsCompileTarget, pixelShaderProgram,
				pixelShaderEntryPointName, psCompileTarget,
				0, "main", EGST_GS_4_0,
				scene::EPT_TRIANGLES, scene::EPT_TRIANGLE_STRIP, 0,
				callback, baseMaterial, userData);
	}

	//! convenience function for use with many defaults, without geometry shader
	/** All shader names are set to "main" and compile targets are shader
	type 1.1.
	*/
	s32 addHighLevelShaderMaterial(
			const c8 *vertexShaderProgram,
			const c8 *pixelShaderProgram = 0,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0)
	{
		return addHighLevelShaderMaterial(
				vertexShaderProgram, "main",
				EVST_VS_1_1, pixelShaderProgram,
				"main", EPST_PS_1_1,
				0, "main", EGST_GS_4_0,
				scene::EPT_TRIANGLES, scene::EPT_TRIANGLE_STRIP, 0,
				callback, baseMaterial, userData);
	}

	//! convenience function for use with many defaults, with geometry shader
	/** All shader names are set to "main" and compile targets are shader
	type 1.1 and geometry shader 4.0.
	*/
	s32 addHighLevelShaderMaterial(
			const c8 *vertexShaderProgram,
			const c8 *pixelShaderProgram = 0,
			const c8 *geometryShaderProgram = 0,
			scene::E_PRIMITIVE_TYPE inType = scene::EPT_TRIANGLES,
			scene::E_PRIMITIVE_TYPE outType = scene::EPT_TRIANGLE_STRIP,
			u32 verticesOut = 0,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0)
	{
		return addHighLevelShaderMaterial(
				vertexShaderProgram, "main",
				EVST_VS_1_1, pixelShaderProgram,
				"main", EPST_PS_1_1,
				geometryShaderProgram, "main", EGST_GS_4_0,
				inType, outType, verticesOut,
				callback, baseMaterial, userData);
	}

	//! Like addHighLevelShaderMaterial(), but loads from files.
	/** \param vertexShaderProgramFileName Text file containing the source
	of the vertex shader program. Set to empty string if no vertex shader
	shall be created.
	\param vertexShaderEntryPointName Name of the entry function of the
	vertexShaderProgram  (p.e. "main")
	\param vsCompileTarget Vertex shader version the high level shader
	shall be compiled to.
	\param pixelShaderProgramFileName Text file containing the source of
	the pixel shader program. Set to empty string if no pixel shader shall
	be created.
	\param pixelShaderEntryPointName Entry name of the function of the
	pixelShaderProgram (p.e. "main")
	\param psCompileTarget Pixel shader version the high level shader
	shall be compiled to.
	\param geometryShaderProgramFileName Name of the source of
	the geometry shader program. Set to empty string if no geometry shader
	shall be created.
	\param geometryShaderEntryPointName Entry name of the function of the
	geometryShaderProgram (p.e. "main")
	\param gsCompileTarget Geometry shader version the high level shader
	shall be compiled to.
	\param inType Type of vertices passed to geometry shader
	\param outType Type of vertices created by geometry shader
	\param verticesOut Maximal number of vertices created by geometry
	shader. If 0, maximal number supported is assumed.
	\param callback Pointer to an implementation of
	IShaderConstantSetCallBack in which you can set the needed vertex,
	pixel, and geometry shader program constants. Set this to 0 if you
	don't need this.
	\param baseMaterial Base material which renderstates will be used to
	shade the material.
	\param userData a user data int. This int can be set to any value and
	will be set as parameter in the callback method when calling
	OnSetConstants(). In this way it is easily possible to use the same
	callback method for multiple materials and distinguish between them
	during the call.
	\return Number of the material type which can be set in
	SMaterial::MaterialType to use the renderer. -1 is returned if an error
	occurred, e.g. if a shader program could not be compiled or a compile
	target is not reachable. The error strings are then printed to the
	error log and can be caught with a custom event receiver. */
	virtual s32 addHighLevelShaderMaterialFromFiles(
			const io::path &vertexShaderProgramFileName,
			const c8 *vertexShaderEntryPointName,
			E_VERTEX_SHADER_TYPE vsCompileTarget,
			const io::path &pixelShaderProgramFileName,
			const c8 *pixelShaderEntryPointName,
			E_PIXEL_SHADER_TYPE psCompileTarget,
			const io::path &geometryShaderProgramFileName,
			const c8 *geometryShaderEntryPointName = "main",
			E_GEOMETRY_SHADER_TYPE gsCompileTarget = EGST_GS_4_0,
			scene::E_PRIMITIVE_TYPE inType = scene::EPT_TRIANGLES,
			scene::E_PRIMITIVE_TYPE outType = scene::EPT_TRIANGLE_STRIP,
			u32 verticesOut = 0,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0) = 0;

	//! convenience function for use without geometry shaders
	s32 addHighLevelShaderMaterialFromFiles(
			const io::path &vertexShaderProgramFileName,
			const c8 *vertexShaderEntryPointName = "main",
			E_VERTEX_SHADER_TYPE vsCompileTarget = EVST_VS_1_1,
			const io::path &pixelShaderProgramFileName = "",
			const c8 *pixelShaderEntryPointName = "main",
			E_PIXEL_SHADER_TYPE psCompileTarget = EPST_PS_1_1,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0)
	{
		return addHighLevelShaderMaterialFromFiles(
				vertexShaderProgramFileName, vertexShaderEntryPointName,
				vsCompileTarget, pixelShaderProgramFileName,
				pixelShaderEntryPointName, psCompileTarget,
				"", "main", EGST_GS_4_0,
				scene::EPT_TRIANGLES, scene::EPT_TRIANGLE_STRIP, 0,
				callback, baseMaterial, userData);
	}

	//! convenience function for use with many defaults, without geometry shader
	/** All shader names are set to "main" and compile targets are shader
	type 1.1.
	*/
	s32 addHighLevelShaderMaterialFromFiles(
			const io::path &vertexShaderProgramFileName,
			const io::path &pixelShaderProgramFileName = "",
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0)
	{
		return addHighLevelShaderMaterialFromFiles(
				vertexShaderProgramFileName, "main",
				EVST_VS_1_1, pixelShaderProgramFileName,
				"main", EPST_PS_1_1,
				"", "main", EGST_GS_4_0,
				scene::EPT_TRIANGLES, scene::EPT_TRIANGLE_STRIP, 0,
				callback, baseMaterial, userData);
	}

	//! convenience function for use with many defaults, with geometry shader
	/** All shader names are set to "main" and compile targets are shader
	type 1.1 and geometry shader 4.0.
	*/
	s32 addHighLevelShaderMaterialFromFiles(
			const io::path &vertexShaderProgramFileName,
			const io::path &pixelShaderProgramFileName = "",
			const io::path &geometryShaderProgramFileName = "",
			scene::E_PRIMITIVE_TYPE inType = scene::EPT_TRIANGLES,
			scene::E_PRIMITIVE_TYPE outType = scene::EPT_TRIANGLE_STRIP,
			u32 verticesOut = 0,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0)
	{
		return addHighLevelShaderMaterialFromFiles(
				vertexShaderProgramFileName, "main",
				EVST_VS_1_1, pixelShaderProgramFileName,
				"main", EPST_PS_1_1,
				geometryShaderProgramFileName, "main", EGST_GS_4_0,
				inType, outType, verticesOut,
				callback, baseMaterial, userData);
	}

	//! Like addHighLevelShaderMaterial(), but loads from files.
	/** \param vertexShaderProgram Text file handle containing the source
	of the vertex shader program. Set to 0 if no vertex shader shall be
	created.
	\param vertexShaderEntryPointName Name of the entry function of the
	vertexShaderProgram
	\param vsCompileTarget Vertex shader version the high level shader
	shall be compiled to.
	\param pixelShaderProgram Text file handle containing the source of
	the pixel shader program. Set to 0 if no pixel shader shall be created.
	\param pixelShaderEntryPointName Entry name of the function of the
	pixelShaderProgram (p.e. "main")
	\param psCompileTarget Pixel shader version the high level shader
	shall be compiled to.
	\param geometryShaderProgram Text file handle containing the source of
	the geometry shader program. Set to 0 if no geometry shader shall be
	created.
	\param geometryShaderEntryPointName Entry name of the function of the
	geometryShaderProgram (p.e. "main")
	\param gsCompileTarget Geometry shader version the high level shader
	shall be compiled to.
	\param inType Type of vertices passed to geometry shader
	\param outType Type of vertices created by geometry shader
	\param verticesOut Maximal number of vertices created by geometry
	shader. If 0, maximal number supported is assumed.
	\param callback Pointer to an implementation of
	IShaderConstantSetCallBack in which you can set the needed vertex and
	pixel shader program constants. Set this to 0 if you don't need this.
	\param baseMaterial Base material which renderstates will be used to
	shade the material.
	\param userData a user data int. This int can be set to any value and
	will be set as parameter in the callback method when calling
	OnSetConstants(). In this way it is easily possible to use the same
	callback method for multiple materials and distinguish between them
	during the call.
	\return Number of the material type which can be set in
	SMaterial::MaterialType to use the renderer. -1 is returned if an
	error occurred, e.g. if a shader program could not be compiled or a
	compile target is not reachable. The error strings are then printed to
	the error log and can be caught with a custom event receiver. */
	virtual s32 addHighLevelShaderMaterialFromFiles(
			io::IReadFile *vertexShaderProgram,
			const c8 *vertexShaderEntryPointName,
			E_VERTEX_SHADER_TYPE vsCompileTarget,
			io::IReadFile *pixelShaderProgram,
			const c8 *pixelShaderEntryPointName,
			E_PIXEL_SHADER_TYPE psCompileTarget,
			io::IReadFile *geometryShaderProgram,
			const c8 *geometryShaderEntryPointName = "main",
			E_GEOMETRY_SHADER_TYPE gsCompileTarget = EGST_GS_4_0,
			scene::E_PRIMITIVE_TYPE inType = scene::EPT_TRIANGLES,
			scene::E_PRIMITIVE_TYPE outType = scene::EPT_TRIANGLE_STRIP,
			u32 verticesOut = 0,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0) = 0;

	//! convenience function for use without geometry shaders
	s32 addHighLevelShaderMaterialFromFiles(
			io::IReadFile *vertexShaderProgram,
			const c8 *vertexShaderEntryPointName = "main",
			E_VERTEX_SHADER_TYPE vsCompileTarget = EVST_VS_1_1,
			io::IReadFile *pixelShaderProgram = 0,
			const c8 *pixelShaderEntryPointName = "main",
			E_PIXEL_SHADER_TYPE psCompileTarget = EPST_PS_1_1,
			IShaderConstantSetCallBack *callback = 0,
			E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
			s32 userData = 0)
	{
		return addHighLevelShaderMaterialFromFiles(
				vertexShaderProgram, vertexShaderEntryPointName,
				vsCompileTarget, pixelShaderProgram,
				pixelShaderEntryPointName, psCompileTarget,
				0, "main", EGST_GS_4_0,
				scene::EPT_TRIANGLES, scene::EPT_TRIANGLE_STRIP, 0,
				callback, baseMaterial, userData);
	}

	//! Delete a shader material and associated data.
	/**
	After you have deleted a material it is invalid to still use and doing
	so might result in a crash. The ID may be reused in the future when new
	materials are added.
	\param material Number of the material type. Must not be a built-in
	material. */
	virtual void deleteShaderMaterial(s32 material) = 0;
};

} // end namespace video
} // end namespace irr
