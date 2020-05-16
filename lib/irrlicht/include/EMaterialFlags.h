// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_MATERIAL_FLAGS_H_INCLUDED__
#define __E_MATERIAL_FLAGS_H_INCLUDED__

namespace irr
{
namespace video
{

	//! Material flags
	enum E_MATERIAL_FLAG
	{
		//! Draw as wireframe or filled triangles? Default: false
		EMF_WIREFRAME = 0x1,

		//! Draw as point cloud or filled triangles? Default: false
		EMF_POINTCLOUD = 0x2,

		//! Flat or Gouraud shading? Default: true
		EMF_GOURAUD_SHADING = 0x4,

		//! Will this material be lighted? Default: true
		EMF_LIGHTING = 0x8,

		//! Is the ZBuffer enabled? Default: true
		EMF_ZBUFFER = 0x10,

		//! May be written to the zbuffer or is it readonly. Default: true
		/** This flag is ignored, if the material type is a transparent type. */
		EMF_ZWRITE_ENABLE = 0x20,

		//! Is backface culling enabled? Default: true
		EMF_BACK_FACE_CULLING = 0x40,

		//! Is frontface culling enabled? Default: false
		/** Overrides EMF_BACK_FACE_CULLING if both are enabled. */
		EMF_FRONT_FACE_CULLING = 0x80,

		//! Is bilinear filtering enabled? Default: true
		EMF_BILINEAR_FILTER = 0x100,

		//! Is trilinear filtering enabled? Default: false
		/** If the trilinear filter flag is enabled,
		the bilinear filtering flag is ignored. */
		EMF_TRILINEAR_FILTER = 0x200,

		//! Is anisotropic filtering? Default: false
		/** In Irrlicht you can use anisotropic texture filtering in
		conjunction with bilinear or trilinear texture filtering
		to improve rendering results. Primitives will look less
		blurry with this flag switched on. */
		EMF_ANISOTROPIC_FILTER = 0x400,

		//! Is fog enabled? Default: false
		EMF_FOG_ENABLE = 0x800,

		//! Normalizes normals. Default: false
		/** You can enable this if you need to scale a dynamic lighted
		model. Usually, its normals will get scaled too then and it
		will get darker. If you enable the EMF_NORMALIZE_NORMALS flag,
		the normals will be normalized again, and the model will look
		as bright as it should. */
		EMF_NORMALIZE_NORMALS = 0x1000,

		//! Access to all layers texture wrap settings. Overwrites separate layer settings.
		EMF_TEXTURE_WRAP = 0x2000,

		//! AntiAliasing mode
		EMF_ANTI_ALIASING = 0x4000,

		//! ColorMask bits, for enabling the color planes
		EMF_COLOR_MASK = 0x8000,

		//! ColorMaterial enum for vertex color interpretation
		EMF_COLOR_MATERIAL = 0x10000,

		//! Flag for enabling/disabling mipmap usage
		EMF_USE_MIP_MAPS = 0x20000,

		//! Flag for blend operation
		EMF_BLEND_OPERATION = 0x40000,

		//! Flag for polygon offset
		EMF_POLYGON_OFFSET = 0x80000
	};

} // end namespace video
} // end namespace irr


#endif // __E_MATERIAL_FLAGS_H_INCLUDED__

