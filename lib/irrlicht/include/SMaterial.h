// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __S_MATERIAL_H_INCLUDED__
#define __S_MATERIAL_H_INCLUDED__

#include "SColor.h"
#include "matrix4.h"
#include "irrArray.h"
#include "irrMath.h"
#include "EMaterialTypes.h"
#include "EMaterialFlags.h"
#include "SMaterialLayer.h"

namespace irr
{
namespace video
{
	class ITexture;

	//! Flag for EMT_ONETEXTURE_BLEND, ( BlendFactor ) BlendFunc = source * sourceFactor + dest * destFactor
	enum E_BLEND_FACTOR
	{
		EBF_ZERO	= 0,			//!< src & dest	(0, 0, 0, 0)
		EBF_ONE,					//!< src & dest	(1, 1, 1, 1)
		EBF_DST_COLOR, 				//!< src	(destR, destG, destB, destA)
		EBF_ONE_MINUS_DST_COLOR,	//!< src	(1-destR, 1-destG, 1-destB, 1-destA)
		EBF_SRC_COLOR,				//!< dest	(srcR, srcG, srcB, srcA)
		EBF_ONE_MINUS_SRC_COLOR, 	//!< dest	(1-srcR, 1-srcG, 1-srcB, 1-srcA)
		EBF_SRC_ALPHA,				//!< src & dest	(srcA, srcA, srcA, srcA)
		EBF_ONE_MINUS_SRC_ALPHA,	//!< src & dest	(1-srcA, 1-srcA, 1-srcA, 1-srcA)
		EBF_DST_ALPHA,				//!< src & dest	(destA, destA, destA, destA)
		EBF_ONE_MINUS_DST_ALPHA,	//!< src & dest	(1-destA, 1-destA, 1-destA, 1-destA)
		EBF_SRC_ALPHA_SATURATE		//!< src	(min(srcA, 1-destA), idem, ...)
	};

	//! Values defining the blend operation used when blend is enabled
	enum E_BLEND_OPERATION
	{
		EBO_NONE = 0,	//!< No blending happens
		EBO_ADD,		//!< Default blending adds the color values
		EBO_SUBTRACT,	//!< This mode subtracts the color values
		EBO_REVSUBTRACT,//!< This modes subtracts destination from source
		EBO_MIN,		//!< Choose minimum value of each color channel
		EBO_MAX,		//!< Choose maximum value of each color channel
		EBO_MIN_FACTOR,	//!< Choose minimum value of each color channel after applying blend factors, not widely supported
		EBO_MAX_FACTOR,	//!< Choose maximum value of each color channel after applying blend factors, not widely supported
		EBO_MIN_ALPHA,	//!< Choose minimum value of each color channel based on alpha value, not widely supported
		EBO_MAX_ALPHA	//!< Choose maximum value of each color channel based on alpha value, not widely supported
	};

	//! MaterialTypeParam: e.g. DirectX: D3DTOP_MODULATE, D3DTOP_MODULATE2X, D3DTOP_MODULATE4X
	enum E_MODULATE_FUNC
	{
		EMFN_MODULATE_1X	= 1,
		EMFN_MODULATE_2X	= 2,
		EMFN_MODULATE_4X	= 4
	};

	//! Comparison function, e.g. for depth buffer test
	enum E_COMPARISON_FUNC
	{
		//! Test never succeeds, this equals disable
		ECFN_NEVER=0,
		//! <= test, default for e.g. depth test
		ECFN_LESSEQUAL=1,
		//! Exact equality
		ECFN_EQUAL=2,
		//! exclusive less comparison, i.e. <
		ECFN_LESS,
		//! Succeeds almost always, except for exact equality
		ECFN_NOTEQUAL,
		//! >= test
		ECFN_GREATEREQUAL,
		//! inverse of <=
		ECFN_GREATER,
		//! test succeeds always
		ECFN_ALWAYS
	};

	//! Enum values for enabling/disabling color planes for rendering
	enum E_COLOR_PLANE
	{
		//! No color enabled
		ECP_NONE=0,
		//! Alpha enabled
		ECP_ALPHA=1,
		//! Red enabled
		ECP_RED=2,
		//! Green enabled
		ECP_GREEN=4,
		//! Blue enabled
		ECP_BLUE=8,
		//! All colors, no alpha
		ECP_RGB=14,
		//! All planes enabled
		ECP_ALL=15
	};

	//! Source of the alpha value to take
	/** This is currently only supported in EMT_ONETEXTURE_BLEND. You can use an
	or'ed combination of values. Alpha values are modulated (multiplicated). */
	enum E_ALPHA_SOURCE
	{
		//! Use no alpha, somewhat redundant with other settings
		EAS_NONE=0,
		//! Use vertex color alpha
		EAS_VERTEX_COLOR,
		//! Use texture alpha channel
		EAS_TEXTURE
	};

	//! EMT_ONETEXTURE_BLEND: pack srcFact, dstFact, Modulate and alpha source to MaterialTypeParam
	/** alpha source can be an OR'ed combination of E_ALPHA_SOURCE values. */
	inline f32 pack_textureBlendFunc ( const E_BLEND_FACTOR srcFact, const E_BLEND_FACTOR dstFact, const E_MODULATE_FUNC modulate=EMFN_MODULATE_1X, const u32 alphaSource=EAS_TEXTURE )
	{
		const u32 tmp = (alphaSource << 12) | (modulate << 8) | (srcFact << 4) | dstFact;
		return FR(tmp);
	}

	//! EMT_ONETEXTURE_BLEND: unpack srcFact & dstFact and Modulo to MaterialTypeParam
	/** The fields don't use the full byte range, so we could pack even more... */
	inline void unpack_textureBlendFunc ( E_BLEND_FACTOR &srcFact, E_BLEND_FACTOR &dstFact,
			E_MODULATE_FUNC &modulo, u32& alphaSource, const f32 param )
	{
		const u32 state = IR(param);
		alphaSource = (state & 0x0000F000) >> 12;
		modulo	= E_MODULATE_FUNC( ( state & 0x00000F00 ) >> 8 );
		srcFact = E_BLEND_FACTOR ( ( state & 0x000000F0 ) >> 4 );
		dstFact = E_BLEND_FACTOR ( ( state & 0x0000000F ) );
	}

	//! EMT_ONETEXTURE_BLEND: has BlendFactor Alphablending
	inline bool textureBlendFunc_hasAlpha ( const E_BLEND_FACTOR factor )
	{
		switch ( factor )
		{
			case EBF_SRC_ALPHA:
			case EBF_ONE_MINUS_SRC_ALPHA:
			case EBF_DST_ALPHA:
			case EBF_ONE_MINUS_DST_ALPHA:
			case EBF_SRC_ALPHA_SATURATE:
				return true;
			default:
				return false;
		}
	}


	//! These flags are used to specify the anti-aliasing and smoothing modes
	/** Techniques supported are multisampling, geometry smoothing, and alpha
	to coverage.
	Some drivers don't support a per-material setting of the anti-aliasing
	modes. In those cases, FSAA/multisampling is defined by the device mode
	chosen upon creation via irr::SIrrCreationParameters.
	*/
	enum E_ANTI_ALIASING_MODE
	{
		//! Use to turn off anti-aliasing for this material
		EAAM_OFF=0,
		//! Default anti-aliasing mode
		EAAM_SIMPLE=1,
		//! High-quality anti-aliasing, not always supported, automatically enables SIMPLE mode
		EAAM_QUALITY=3,
		//! Line smoothing
		EAAM_LINE_SMOOTH=4,
		//! point smoothing, often in software and slow, only with OpenGL
		EAAM_POINT_SMOOTH=8,
		//! All typical anti-alias and smooth modes
		EAAM_FULL_BASIC=15,
		//! Enhanced anti-aliasing for transparent materials
		/** Usually used with EMT_TRANSPARENT_ALPHA_REF and multisampling. */
		EAAM_ALPHA_TO_COVERAGE=16
	};

	//! These flags allow to define the interpretation of vertex color when lighting is enabled
	/** Without lighting being enabled the vertex color is the only value defining the fragment color.
	Once lighting is enabled, the four values for diffuse, ambient, emissive, and specular take over.
	With these flags it is possible to define which lighting factor shall be defined by the vertex color
	instead of the lighting factor which is the same for all faces of that material.
	The default is to use vertex color for the diffuse value, another pretty common value is to use
	vertex color for both diffuse and ambient factor. */
	enum E_COLOR_MATERIAL
	{
		//! Don't use vertex color for lighting
		ECM_NONE=0,
		//! Use vertex color for diffuse light, this is default
		ECM_DIFFUSE,
		//! Use vertex color for ambient light
		ECM_AMBIENT,
		//! Use vertex color for emissive light
		ECM_EMISSIVE,
		//! Use vertex color for specular light
		ECM_SPECULAR,
		//! Use vertex color for both diffuse and ambient light
		ECM_DIFFUSE_AND_AMBIENT
	};

	//! Flags for the definition of the polygon offset feature
	/** These flags define whether the offset should be into the screen, or towards the eye. */
	enum E_POLYGON_OFFSET
	{
		//! Push pixel towards the far plane, away from the eye
		/** This is typically used for rendering inner areas. */
		EPO_BACK=0,
		//! Pull pixels towards the camera.
		/** This is typically used for polygons which should appear on top
		of other elements, such as decals. */
		EPO_FRONT=1
	};

	//! Names for polygon offset direction
	const c8* const PolygonOffsetDirectionNames[] =
	{
		"Back",
		"Front",
		0
	};


	//! Maximum number of texture an SMaterial can have.
	const u32 MATERIAL_MAX_TEXTURES = _IRR_MATERIAL_MAX_TEXTURES_;

	//! Struct for holding parameters for a material renderer
	class SMaterial
	{
	public:
		//! Default constructor. Creates a solid, lit material with white colors
		SMaterial()
		: MaterialType(EMT_SOLID), AmbientColor(255,255,255,255), DiffuseColor(255,255,255,255),
			EmissiveColor(0,0,0,0), SpecularColor(255,255,255,255),
			Shininess(0.0f), MaterialTypeParam(0.0f), MaterialTypeParam2(0.0f), Thickness(1.0f),
			ZBuffer(ECFN_LESSEQUAL), AntiAliasing(EAAM_SIMPLE), ColorMask(ECP_ALL),
			ColorMaterial(ECM_DIFFUSE), BlendOperation(EBO_NONE),
			PolygonOffsetFactor(0), PolygonOffsetDirection(EPO_FRONT),
			Wireframe(false), PointCloud(false), GouraudShading(true),
			Lighting(true), ZWriteEnable(true), BackfaceCulling(true), FrontfaceCulling(false),
			FogEnable(false), NormalizeNormals(false), UseMipMaps(true)
		{ }

		//! Copy constructor
		/** \param other Material to copy from. */
		SMaterial(const SMaterial& other)
		{
			// These pointers are checked during assignment
			for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
				TextureLayer[i].TextureMatrix = 0;
			*this = other;
		}

		//! Assignment operator
		/** \param other Material to copy from. */
		SMaterial& operator=(const SMaterial& other)
		{
			// Check for self-assignment!
			if (this == &other)
				return *this;

			MaterialType = other.MaterialType;

			AmbientColor = other.AmbientColor;
			DiffuseColor = other.DiffuseColor;
			EmissiveColor = other.EmissiveColor;
			SpecularColor = other.SpecularColor;
			Shininess = other.Shininess;
			MaterialTypeParam = other.MaterialTypeParam;
			MaterialTypeParam2 = other.MaterialTypeParam2;
			Thickness = other.Thickness;
			for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
			{
				TextureLayer[i] = other.TextureLayer[i];
			}

			Wireframe = other.Wireframe;
			PointCloud = other.PointCloud;
			GouraudShading = other.GouraudShading;
			Lighting = other.Lighting;
			ZWriteEnable = other.ZWriteEnable;
			BackfaceCulling = other.BackfaceCulling;
			FrontfaceCulling = other.FrontfaceCulling;
			FogEnable = other.FogEnable;
			NormalizeNormals = other.NormalizeNormals;
			ZBuffer = other.ZBuffer;
			AntiAliasing = other.AntiAliasing;
			ColorMask = other.ColorMask;
			ColorMaterial = other.ColorMaterial;
			BlendOperation = other.BlendOperation;
			PolygonOffsetFactor = other.PolygonOffsetFactor;
			PolygonOffsetDirection = other.PolygonOffsetDirection;
			UseMipMaps = other.UseMipMaps;

			return *this;
		}

		//! Texture layer array.
		SMaterialLayer TextureLayer[MATERIAL_MAX_TEXTURES];

		//! Type of the material. Specifies how everything is blended together
		E_MATERIAL_TYPE MaterialType;

		//! How much ambient light (a global light) is reflected by this material.
		/** The default is full white, meaning objects are completely
		globally illuminated. Reduce this if you want to see diffuse
		or specular light effects. */
		SColor AmbientColor;

		//! How much diffuse light coming from a light source is reflected by this material.
		/** The default is full white. */
		SColor DiffuseColor;

		//! Light emitted by this material. Default is to emit no light.
		SColor EmissiveColor;

		//! How much specular light (highlights from a light) is reflected.
		/** The default is to reflect white specular light. See
		SMaterial::Shininess on how to enable specular lights. */
		SColor SpecularColor;

		//! Value affecting the size of specular highlights.
		/** A value of 20 is common. If set to 0, no specular
		highlights are being used. To activate, simply set the
		shininess of a material to a value in the range [0.5;128]:
		\code
		sceneNode->getMaterial(0).Shininess = 20.0f;
		\endcode

		You can change the color of the highlights using
		\code
		sceneNode->getMaterial(0).SpecularColor.set(255,255,255,255);
		\endcode

		The specular color of the dynamic lights
		(SLight::SpecularColor) will influence the the highlight color
		too, but they are set to a useful value by default when
		creating the light scene node. Here is a simple example on how
		to use specular highlights:
		\code
		// load and display mesh
		scene::IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode(
		smgr->getMesh("data/faerie.md2"));
		node->setMaterialTexture(0, driver->getTexture("data/Faerie2.pcx")); // set diffuse texture
		node->setMaterialFlag(video::EMF_LIGHTING, true); // enable dynamic lighting
		node->getMaterial(0).Shininess = 20.0f; // set size of specular highlights

		// add white light
		scene::ILightSceneNode* light = smgr->addLightSceneNode(0,
			core::vector3df(5,5,5), video::SColorf(1.0f, 1.0f, 1.0f));
		\endcode */
		f32 Shininess;

		//! Free parameter, dependent on the material type.
		/** Mostly ignored, used for example in EMT_PARALLAX_MAP_SOLID
		and EMT_TRANSPARENT_ALPHA_CHANNEL. */
		f32 MaterialTypeParam;

		//! Second free parameter, dependent on the material type.
		/** Mostly ignored. */
		f32 MaterialTypeParam2;

		//! Thickness of non-3dimensional elements such as lines and points.
		f32 Thickness;

		//! Is the ZBuffer enabled? Default: ECFN_LESSEQUAL
		/** Values are from E_COMPARISON_FUNC. */
		u8 ZBuffer;

		//! Sets the antialiasing mode
		/** Values are chosen from E_ANTI_ALIASING_MODE. Default is 
		EAAM_SIMPLE|EAAM_LINE_SMOOTH, i.e. simple multi-sample
		anti-aliasing and lime smoothing is enabled. */
		u8 AntiAliasing;

		//! Defines the enabled color planes
		/** Values are defined as or'ed values of the E_COLOR_PLANE enum.
		Only enabled color planes will be rendered to the current render
		target. Typical use is to disable all colors when rendering only to
		depth or stencil buffer, or using Red and Green for Stereo rendering. */
		u8 ColorMask:4;

		//! Defines the interpretation of vertex color in the lighting equation
		/** Values should be chosen from E_COLOR_MATERIAL.
		When lighting is enabled, vertex color can be used instead of the 
		material values for light modulation. This allows to easily change e.g. the
		diffuse light behavior of each face. The default, ECM_DIFFUSE, will result in
		a very similar rendering as with lighting turned off, just with light shading. */
		u8 ColorMaterial:3;

		//! Store the blend operation of choice
		/** Values to be chosen from E_BLEND_OPERATION. The actual way to use this value
		is not yet determined, so ignore it for now. */
		E_BLEND_OPERATION BlendOperation:4;

		//! Factor specifying how far the polygon offset should be made
		/** Specifying 0 disables the polygon offset. The direction is specified spearately.
		The factor can be from 0 to 7.*/
		u8 PolygonOffsetFactor:3;

		//! Flag defining the direction the polygon offset is applied to.
		/** Can be to front or to back, specififed by values from E_POLYGON_OFFSET. */
		E_POLYGON_OFFSET PolygonOffsetDirection:1;

		//! Draw as wireframe or filled triangles? Default: false
		/** The user can access a material flag using
		\code material.Wireframe=true \endcode
		or \code material.setFlag(EMF_WIREFRAME, true); \endcode */
		bool Wireframe:1;

		//! Draw as point cloud or filled triangles? Default: false
		bool PointCloud:1;

		//! Flat or Gouraud shading? Default: true
		bool GouraudShading:1;

		//! Will this material be lighted? Default: true
		bool Lighting:1;

		//! Is the zbuffer writeable or is it read-only. Default: true.
		/** This flag is forced to false if the MaterialType is a
		transparent type and the scene parameter
		ALLOW_ZWRITE_ON_TRANSPARENT is not set. */
		bool ZWriteEnable:1;

		//! Is backface culling enabled? Default: true
		bool BackfaceCulling:1;

		//! Is frontface culling enabled? Default: false
		bool FrontfaceCulling:1;

		//! Is fog enabled? Default: false
		bool FogEnable:1;

		//! Should normals be normalized?
		/** Always use this if the mesh lit and scaled. Default: false */
		bool NormalizeNormals:1;

		//! Shall mipmaps be used if available
		/** Sometimes, disabling mipmap usage can be useful. Default: true */
		bool UseMipMaps:1;

		//! Gets the texture transformation matrix for level i
		/** \param i The desired level. Must not be larger than MATERIAL_MAX_TEXTURES.
		\return Texture matrix for texture level i. */
		core::matrix4& getTextureMatrix(u32 i)
		{
			return TextureLayer[i].getTextureMatrix();
		}

		//! Gets the immutable texture transformation matrix for level i
		/** \param i The desired level.
		\return Texture matrix for texture level i, or identity matrix for levels larger than MATERIAL_MAX_TEXTURES. */
		const core::matrix4& getTextureMatrix(u32 i) const
		{
			if (i<MATERIAL_MAX_TEXTURES)
				return TextureLayer[i].getTextureMatrix();
			else
				return core::IdentityMatrix;
		}

		//! Sets the i-th texture transformation matrix
		/** \param i The desired level.
		\param mat Texture matrix for texture level i. */
		void setTextureMatrix(u32 i, const core::matrix4& mat)
		{
			if (i>=MATERIAL_MAX_TEXTURES)
				return;
			TextureLayer[i].setTextureMatrix(mat);
		}

		//! Gets the i-th texture
		/** \param i The desired level.
		\return Texture for texture level i, if defined, else 0. */
		ITexture* getTexture(u32 i) const
		{
			return i < MATERIAL_MAX_TEXTURES ? TextureLayer[i].Texture : 0;
		}

		//! Sets the i-th texture
		/** If i>=MATERIAL_MAX_TEXTURES this setting will be ignored.
		\param i The desired level.
		\param tex Texture for texture level i. */
		void setTexture(u32 i, ITexture* tex)
		{
			if (i>=MATERIAL_MAX_TEXTURES)
				return;
			TextureLayer[i].Texture = tex;
		}

		//! Sets the Material flag to the given value
		/** \param flag The flag to be set.
		\param value The new value for the flag. */
		void setFlag(E_MATERIAL_FLAG flag, bool value)
		{
			switch (flag)
			{
				case EMF_WIREFRAME:
					Wireframe = value; break;
				case EMF_POINTCLOUD:
					PointCloud = value; break;
				case EMF_GOURAUD_SHADING:
					GouraudShading = value; break;
				case EMF_LIGHTING:
					Lighting = value; break;
				case EMF_ZBUFFER:
					ZBuffer = value; break;
				case EMF_ZWRITE_ENABLE:
					ZWriteEnable = value; break;
				case EMF_BACK_FACE_CULLING:
					BackfaceCulling = value; break;
				case EMF_FRONT_FACE_CULLING:
					FrontfaceCulling = value; break;
				case EMF_BILINEAR_FILTER:
				{
					for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
						TextureLayer[i].BilinearFilter = value;
				}
				break;
				case EMF_TRILINEAR_FILTER:
				{
					for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
						TextureLayer[i].TrilinearFilter = value;
				}
				break;
				case EMF_ANISOTROPIC_FILTER:
				{
					if (value)
						for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
							TextureLayer[i].AnisotropicFilter = 0xFF;
					else
						for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
							TextureLayer[i].AnisotropicFilter = 0;
				}
				break;
				case EMF_FOG_ENABLE:
					FogEnable = value; break;
				case EMF_NORMALIZE_NORMALS:
					NormalizeNormals = value; break;
				case EMF_TEXTURE_WRAP:
				{
					for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
					{
						TextureLayer[i].TextureWrapU = (E_TEXTURE_CLAMP)value;
						TextureLayer[i].TextureWrapV = (E_TEXTURE_CLAMP)value;
					}
				}
				break;
				case EMF_ANTI_ALIASING:
					AntiAliasing = value?EAAM_SIMPLE:EAAM_OFF; break;
				case EMF_COLOR_MASK:
					ColorMask = value?ECP_ALL:ECP_NONE; break;
				case EMF_COLOR_MATERIAL:
					ColorMaterial = value?ECM_DIFFUSE:ECM_NONE; break;
				case EMF_USE_MIP_MAPS:
					UseMipMaps = value; break;
				case EMF_BLEND_OPERATION:
					BlendOperation = value?EBO_ADD:EBO_NONE; break;
				case EMF_POLYGON_OFFSET:
					PolygonOffsetFactor = value?1:0;
					PolygonOffsetDirection = EPO_BACK;
					break;
				default:
					break;
			}
		}

		//! Gets the Material flag
		/** \param flag The flag to query.
		\return The current value of the flag. */
		bool getFlag(E_MATERIAL_FLAG flag) const
		{
			switch (flag)
			{
				case EMF_WIREFRAME:
					return Wireframe;
				case EMF_POINTCLOUD:
					return PointCloud;
				case EMF_GOURAUD_SHADING:
					return GouraudShading;
				case EMF_LIGHTING:
					return Lighting;
				case EMF_ZBUFFER:
					return ZBuffer!=ECFN_NEVER;
				case EMF_ZWRITE_ENABLE:
					return ZWriteEnable;
				case EMF_BACK_FACE_CULLING:
					return BackfaceCulling;
				case EMF_FRONT_FACE_CULLING:
					return FrontfaceCulling;
				case EMF_BILINEAR_FILTER:
					return TextureLayer[0].BilinearFilter;
				case EMF_TRILINEAR_FILTER:
					return TextureLayer[0].TrilinearFilter;
				case EMF_ANISOTROPIC_FILTER:
					return TextureLayer[0].AnisotropicFilter!=0;
				case EMF_FOG_ENABLE:
					return FogEnable;
				case EMF_NORMALIZE_NORMALS:
					return NormalizeNormals;
				case EMF_TEXTURE_WRAP:
					return !(TextureLayer[0].TextureWrapU ||
							TextureLayer[0].TextureWrapV ||
							TextureLayer[1].TextureWrapU ||
							TextureLayer[1].TextureWrapV ||
							TextureLayer[2].TextureWrapU ||
							TextureLayer[2].TextureWrapV ||
							TextureLayer[3].TextureWrapU ||
							TextureLayer[3].TextureWrapV);
				case EMF_ANTI_ALIASING:
					return (AntiAliasing==1);
				case EMF_COLOR_MASK:
					return (ColorMask!=ECP_NONE);
				case EMF_COLOR_MATERIAL:
					return (ColorMaterial != ECM_NONE);
				case EMF_USE_MIP_MAPS:
					return UseMipMaps;
				case EMF_BLEND_OPERATION:
					return BlendOperation != EBO_NONE;
				case EMF_POLYGON_OFFSET:
					return PolygonOffsetFactor != 0;
			}

			return false;
		}

		//! Inequality operator
		/** \param b Material to compare to.
		\return True if the materials differ, else false. */
		inline bool operator!=(const SMaterial& b) const
		{
			bool different =
				MaterialType != b.MaterialType ||
				AmbientColor != b.AmbientColor ||
				DiffuseColor != b.DiffuseColor ||
				EmissiveColor != b.EmissiveColor ||
				SpecularColor != b.SpecularColor ||
				Shininess != b.Shininess ||
				MaterialTypeParam != b.MaterialTypeParam ||
				MaterialTypeParam2 != b.MaterialTypeParam2 ||
				Thickness != b.Thickness ||
				Wireframe != b.Wireframe ||
				PointCloud != b.PointCloud ||
				GouraudShading != b.GouraudShading ||
				Lighting != b.Lighting ||
				ZBuffer != b.ZBuffer ||
				ZWriteEnable != b.ZWriteEnable ||
				BackfaceCulling != b.BackfaceCulling ||
				FrontfaceCulling != b.FrontfaceCulling ||
				FogEnable != b.FogEnable ||
				NormalizeNormals != b.NormalizeNormals ||
				AntiAliasing != b.AntiAliasing ||
				ColorMask != b.ColorMask ||
				ColorMaterial != b.ColorMaterial ||
				BlendOperation != b.BlendOperation ||
				PolygonOffsetFactor != b.PolygonOffsetFactor ||
				PolygonOffsetDirection != b.PolygonOffsetDirection ||
				UseMipMaps != b.UseMipMaps;
			for (u32 i=0; (i<MATERIAL_MAX_TEXTURES) && !different; ++i)
			{
				different |= (TextureLayer[i] != b.TextureLayer[i]);
			}
			return different;
		}

		//! Equality operator
		/** \param b Material to compare to.
		\return True if the materials are equal, else false. */
		inline bool operator==(const SMaterial& b) const
		{ return !(b!=*this); }

		bool isTransparent() const
		{
			return MaterialType==EMT_TRANSPARENT_ADD_COLOR ||
				MaterialType==EMT_TRANSPARENT_ALPHA_CHANNEL ||
				MaterialType==EMT_TRANSPARENT_VERTEX_ALPHA ||
				MaterialType==EMT_TRANSPARENT_REFLECTION_2_LAYER;
		}
	};

	//! global const identity Material
	IRRLICHT_API extern SMaterial IdentityMaterial;

} // end namespace video
} // end namespace irr

#endif
