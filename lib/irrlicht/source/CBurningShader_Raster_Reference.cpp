// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "IBurningShader.h"

#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_


namespace irr
{

namespace video
{

	/*! Render states define set-up states for all kinds of vertex and pixel processing.
		Some render states set up vertex processing, and some set up pixel processing (see Render States).
		Render states can be saved and restored using stateblocks (see State Blocks Save and Restore State).
	*/
	enum BD3DRENDERSTATETYPE
	{
		/*!	BD3DRS_ZENABLE 
			Depth-buffering state as one member of the BD3DZBUFFERTYPE enumerated type.
			Set this state to D3DZB_TRUE to enable z-buffering, 
			D3DZB_USEW to enable w-buffering, or D3DZB_FALSE to disable depth buffering. 
			The default value for this render state is D3DZB_TRUE if a depth stencil was created 
			along with the swap chain by setting the EnableAutoDepthStencil member of the
			D3DPRESENT_PARAMETERS structure to TRUE, and D3DZB_FALSE otherwise.
		*/
		BD3DRS_ZENABLE,

		/*!	BD3DRS_FILLMODE 
			One or more members of the D3DFILLMODE enumerated type. The default value is D3DFILL_SOLID. 
		*/
		BD3DRS_FILLMODE,

		/*!	BD3DRS_SHADEMODE 
			One or more members of the D3DSHADEMODE enumerated type. The default value is D3DSHADE_GOURAUD. 
		*/
		BD3DRS_SHADEMODE,

		/*!	BD3DRS_ZWRITEENABLE 
			TRUE to enable the application to write to the depth buffer. The default value is TRUE. 
			This member enables an application to prevent the system from updating the depth buffer with
			new depth values. If FALSE, depth comparisons are still made according to the render state
			D3DRS_ZFUNC, assuming that depth buffering is taking place, but depth values are not written
			to the buffer. 
		*/
		BD3DRS_ZWRITEENABLE,

		/*!	BD3DRS_ALPHATESTENABLE 
			TRUE to enable per pixel alpha testing. If the test passes, the pixel is processed by the frame
			buffer. Otherwise, all frame-buffer processing is skipped for the pixel. The test is done by 
			comparing the incoming alpha value with the reference alpha value, using the comparison function
			provided by the D3DRS_ALPHAFUNC render state. The reference alpha value is determined by the value
			set for D3DRS_ALPHAREF. For more information, see Alpha Testing State.
			The default value of this parameter is FALSE.
		*/
		BD3DRS_ALPHATESTENABLE,

		/*!	BD3DRS_SRCBLEND 
			One member of the BD3DBLEND enumerated type. The default value is BD3DBLEND_ONE.
		*/
		BD3DRS_SRCBLEND,

		/*!	BD3DRS_DESTBLEND
			One member of the BD3DBLEND enumerated type. The default value is BD3DBLEND_ZERO.
		*/
		BD3DRS_DESTBLEND,

		/*!	BD3DRS_CULLMODE
			Specifies how back-facing triangles are culled, if at all. This can be set to one 
			member of the BD3DCULL enumerated type. The default value is BD3DCULL_CCW.
		*/
		BD3DRS_CULLMODE,

		/*!	BD3DRS_ZFUNC 
			One member of the BD3DCMPFUNC enumerated type. The default value is BD3DCMP_LESSEQUAL. 
			This member enables an application to accept or reject a pixel, based on its distance from 
			the camera. The depth value of the pixel is compared with the depth-buffer value. If the depth
			value of the pixel passes the comparison function, the pixel is written.

			The depth value is written to the depth buffer only if the render state is TRUE.
			Software rasterizers and many hardware accelerators work faster if the depth test fails,
			because there is no need to filter and modulate the texture if the pixel is not going to be
			rendered.
		*/
		BD3DRS_ZFUNC,

		/*!	BD3DRS_ALPHAREF 
			Value that specifies a reference alpha value against which pixels are tested when alpha testing
			is enabled. This is an 8-bit value placed in the low 8 bits of the DWORD render-state value.
			Values can range from 0x00000000 through 0x000000FF. The default value is 0. 
		*/
		BD3DRS_ALPHAREF,

		/*!	BD3DRS_ALPHAFUNC 
			One member of the BD3DCMPFUNC enumerated type. The default value is BD3DCMP_ALWAYS. 
			This member enables an application to accept or reject a pixel, based on its alpha value.
		*/
		BD3DRS_ALPHAFUNC,

		/*!	BD3DRS_DITHERENABLE 
			TRUE to enable dithering. The default value is FALSE. 
		*/
		BD3DRS_DITHERENABLE,

		/*!	BD3DRS_ALPHABLENDENABLE 
			TRUE to enable alpha-blended transparency. The default value is FALSE. 
			The type of alpha blending is determined by the BD3DRS_SRCBLEND and BD3DRS_DESTBLEND render states.
		*/
		BD3DRS_ALPHABLENDENABLE,

		/*! BD3DRS_FOGENABLE 
			TRUE to enable fog blending. The default value is FALSE. For more information about using fog
			blending, see Fog. 
		*/
		BD3DRS_FOGENABLE,

		/*!	BD3DRS_SPECULARENABLE 
			TRUE to enable specular highlights. The default value is FALSE.
			Specular highlights are calculated as though every vertex in the object being lit is at the 
			object's origin. This gives the expected results as long as the object is modeled around the
			origin and the distance from the light to the object is relatively large. In other cases, the
			results as undefined.
			When this member is set to TRUE, the specular color is added to the base color after the 
			texture cascade but before alpha blending.
		*/
		BD3DRS_SPECULARENABLE,

		/*! BD3DRS_FOGCOLOR 
			Value whose type is D3DCOLOR. The default value is 0. For more information about fog color,
			see Fog Color. 
		*/
		BD3DRS_FOGCOLOR,

		/*!	BD3DRS_FOGTABLEMODE 
			The fog formula to be used for pixel fog. Set to one of the members of the D3DFOGMODE
			enumerated type. The default value is D3DFOG_NONE. For more information about pixel fog, 
			see Pixel Fog. 
		*/
		BD3DRS_FOGTABLEMODE,

		/*! BD3DRS_FOGSTART 
			Depth at which pixel or vertex fog effects begin for linear fog mode. The default value is 0.0f.
			Depth is specified in world space for vertex fog and either device space [0.0, 1.0] or world 
			space for pixel fog. For pixel fog, these values are in device space when the system uses z for 
			fog calculations and world-world space when the system is using eye-relative fog (w-fog). For 
			more information, see Fog Parameters and Eye-Relative vs. Z-based Depth. 
			Values for the this render state are floating-point values.
			Because the IDirect3DDevice9::SetRenderState method accepts DWORD values, your 
			application must cast a variable that contains the value, as shown in the following code example.
			pDevice9->SetRenderState( BD3DRS_FOGSTART, *((DWORD*) (&fFogStart)));
		*/
		BD3DRS_FOGSTART,

		/*! BD3DRS_FOGEND 
			Depth at which pixel or vertex fog effects end for linear fog mode. The default value is 1.0f.
			Depth is specified in world space for vertex fog and either device space [0.0, 1.0] or world space
			for pixel fog. For pixel fog, these values are in device space when the system uses z for fog
			calculations and in world space when the system is using eye-relative fog (w-fog). For more 
			information, see Fog Parameters and Eye-Relative vs. Z-based Depth. 
			Values for this render state are floating-point values. 
		*/
		BD3DRS_FOGEND,

		/*! BD3DRS_FOGDENSITY
			Fog density for pixel or vertex fog used in the exponential fog modes (D3DFOG_EXP and D3DFOG_EXP2).
			Valid density values range from 0.0 through 1.0. The default value is 1.0. For more information,
			see Fog Parameters. 
			Values for this render state are floating-point values.
		*/
		BD3DRS_FOGDENSITY,


		/*! BD3DRS_RANGEFOGENABLE 
			TRUE to enable range-based vertex fog. The default value is FALSE, in which case the system
			uses depth-based fog. In range-based fog, the distance of an object from the viewer is used
			to compute fog effects, not the depth of the object (that is, the z-coordinate) in the scene.
			In range-based fog, all fog methods work as usual, except that they use range instead of depth
			in the computations. 
			Range is the correct factor to use for fog computations, but depth is commonly used instead
			because range is time-consuming to compute and depth is generally already available. Using depth
			to calculate fog has the undesirable effect of having the fogginess of peripheral objects change
			as the viewer's eye moves - in this case, the depth changes and the range remains constant.

			Because no hardware currently supports per-pixel range-based fog, range correction is offered
			only for vertex fog.
			For more information, see Vertex Fog.
		*/
		BD3DRS_RANGEFOGENABLE = 48,

		/*! BD3DRS_STENCILENABLE 
			TRUE to enable stenciling, or FALSE to disable stenciling. The default value is FALSE.
			For more information, see Stencil Buffer Techniques. 
		*/
		BD3DRS_STENCILENABLE = 52,

		/*!	BD3DRS_STENCILFAIL 
			Stencil operation to perform if the stencil test fails. Values are from the D3DSTENCILOP 
			enumerated type. The default value is D3DSTENCILOP_KEEP. 
		*/
		BD3DRS_STENCILFAIL = 53,

		/*!	BD3DRS_STENCILZFAIL 
			Stencil operation to perform if the stencil test passes and the depth test (z-test) fails.
			Values are from the D3DSTENCILOP enumerated type. The default value is D3DSTENCILOP_KEEP. 
		*/
		BD3DRS_STENCILZFAIL = 54,

		/*!	BD3DRS_STENCILPASS 
			Stencil operation to perform if both the stencil and the depth (z) tests pass. Values are
			from the D3DSTENCILOP enumerated type. The default value is D3DSTENCILOP_KEEP. 
		*/
		BD3DRS_STENCILPASS = 55,

		/*!	BD3DRS_STENCILFUNC 
			Comparison function for the stencil test. Values are from the D3DCMPFUNC enumerated type.
			The default value is D3DCMP_ALWAYS. 
			The comparison function is used to compare the reference value to a stencil buffer entry.
			This comparison applies only to the bits in the reference value and stencil buffer entry that
			are set in the stencil mask (set by the D3DRS_STENCILMASK render state). If TRUE, the stencil 
			test passes.
		*/
		BD3DRS_STENCILFUNC = 56,

		/*! BD3DRS_STENCILREF 
			An int reference value for the stencil test. The default value is 0. 
		*/
		BD3DRS_STENCILREF = 57,

		/*! BD3DRS_STENCILMASK 
			Mask applied to the reference value and each stencil buffer entry to determine the significant
			bits for the stencil test. The default mask is 0xFFFFFFFF. 
		*/
		BD3DRS_STENCILMASK = 58,

		/*! BD3DRS_STENCILWRITEMASK 
			Write mask applied to values written into the stencil buffer. The default mask is 0xFFFFFFFF. 
		*/
		BD3DRS_STENCILWRITEMASK = 59,

		/*!	BD3DRS_TEXTUREFACTOR 
			Color used for multiple-texture blending with the D3DTA_TFACTOR texture-blending argument or the
			D3DTOP_BLENDFACTORALPHA texture-blending operation. The associated value is a D3DCOLOR variable.
			The default value is opaque white (0xFFFFFFFF). 
		*/
		BD3DRS_TEXTUREFACTOR = 60,

		/*! BD3DRS_WRAP0 
			Texture-wrapping behavior for multiple sets of texture coordinates. Valid values for this 
			render state can be any combination of the D3DWRAPCOORD_0 (or D3DWRAP_U), D3DWRAPCOORD_1
			(or D3DWRAP_V), D3DWRAPCOORD_2 (or D3DWRAP_W), and D3DWRAPCOORD_3 flags. These cause the system 
			to wrap in the direction of the first, second, third, and fourth dimensions, sometimes referred
			to as the s, t, r, and q directions, for a given texture. The default value for this render state
			is 0 (wrapping disabled in all directions). 
		*/
		BD3DRS_WRAP0 = 128,
		BD3DRS_WRAP1 = 129,
		BD3DRS_WRAP2 = 130,
		BD3DRS_WRAP3 = 131,
		BD3DRS_WRAP4 = 132,
		BD3DRS_WRAP5 = 133,
		BD3DRS_WRAP6 = 134,
		BD3DRS_WRAP7 = 135,

		/*! BD3DRS_CLIPPING 
			TRUE to enable primitive clipping by Direct3D, or FALSE to disable it. The default value is TRUE. 
		*/
		BD3DRS_CLIPPING = 136,

		/*! BD3DRS_LIGHTING 
			TRUE to enable Direct3D lighting, or FALSE to disable it. The default value is TRUE. Only 
			vertices that include a vertex normal are properly lit; vertices that do not contain a normal
			employ a dot product of 0 in all lighting calculations. 
		*/
		BD3DRS_LIGHTING = 137,

		/*! D3DRS_AMBIENT 
			Ambient light color. This value is of type D3DCOLOR. The default value is 0. 
		*/
		BD3DRS_AMBIENT = 139,

		/*! BD3DRS_FOGVERTEXMODE 
			Fog formula to be used for vertex fog. Set to one member of the BD3DFOGMODE enumerated type.
			The default value is D3DFOG_NONE. 
		*/
		BD3DRS_FOGVERTEXMODE = 140,

		/*! BD3DRS_COLORVERTEX 
			TRUE to enable per-vertex color or FALSE to disable it. The default value is TRUE. Enabling
			per-vertex color allows the system to include the color defined for individual vertices in its
			lighting calculations. 
			For more information, see the following render states:
				BD3DRS_DIFFUSEMATERIALSOURCE 
				BD3DRS_SPECULARMATERIALSOURCE 
				BD3DRS_AMBIENTMATERIALSOURCE 
				BD3DRS_EMISSIVEMATERIALSOURCE 
		*/
		BD3DRS_COLORVERTEX = 141,

		/*! BD3DRS_LOCALVIEWER 
			TRUE to enable camera-relative specular highlights, or FALSE to use orthogonal specular 
			highlights. The default value is TRUE. Applications that use orthogonal projection should 
			specify false. 
		*/
		BD3DRS_LOCALVIEWER = 142,

		/*! BD3DRS_NORMALIZENORMALS 
			TRUE to enable automatic normalization of vertex normals, or FALSE to disable it. The default
			value is FALSE. Enabling this feature causes the system to normalize the vertex normals for 
			vertices after transforming them to camera space, which can be computationally time-consuming. 
		*/
		BD3DRS_NORMALIZENORMALS = 143,

		/*! BD3DRS_DIFFUSEMATERIALSOURCE 
			Diffuse color source for lighting calculations. Valid values are members of the 
			D3DMATERIALCOLORSOURCE enumerated type. The default value is D3DMCS_COLOR1. The value for this
			render state is used only if the D3DRS_COLORVERTEX render state is set to TRUE. 
		*/
		BD3DRS_DIFFUSEMATERIALSOURCE = 145,

		/*! BD3DRS_SPECULARMATERIALSOURCE 
			Specular color source for lighting calculations. Valid values are members of the 
			D3DMATERIALCOLORSOURCE enumerated type. The default value is D3DMCS_COLOR2. 
		*/
		BD3DRS_SPECULARMATERIALSOURCE = 146,

		/*! D3DRS_AMBIENTMATERIALSOURCE 
			Ambient color source for lighting calculations. Valid values are members of the
			D3DMATERIALCOLORSOURCE enumerated type. The default value is D3DMCS_MATERIAL. 
		*/
		BD3DRS_AMBIENTMATERIALSOURCE = 147,

		/*! D3DRS_EMISSIVEMATERIALSOURCE 
			Emissive color source for lighting calculations. Valid values are members of the 
			D3DMATERIALCOLORSOURCE enumerated type. The default value is D3DMCS_MATERIAL. 
		*/
		BD3DRS_EMISSIVEMATERIALSOURCE = 148,

		/*! D3DRS_VERTEXBLEND 
			Number of matrices to use to perform geometry blending, if any. Valid values are members
			of the D3DVERTEXBLENDFLAGS enumerated type. The default value is D3DVBF_DISABLE. 
		*/
		BD3DRS_VERTEXBLEND = 151,

		/* D3DRS_CLIPPLANEENABLE 
			Enables or disables user-defined clipping planes. Valid values are any DWORD in which the
			status of each bit (set or not set) toggles the activation state of a corresponding user-defined
			clipping plane. The least significant bit (bit 0) controls the first clipping plane at index 0,
			and subsequent bits control the activation of clipping planes at higher indexes. If a bit is set,
			the system applies the appropriate clipping plane during scene rendering. The default value is 0.
			The D3DCLIPPLANEn macros are defined to provide a convenient way to enable clipping planes.
		*/
		BD3DRS_CLIPPLANEENABLE = 152,
		BD3DRS_POINTSIZE = 154,
		BD3DRS_POINTSIZE_MIN = 155,
		BD3DRS_POINTSPRITEENABLE = 156,
		BD3DRS_POINTSCALEENABLE = 157,
		BD3DRS_POINTSCALE_A = 158,
		BD3DRS_POINTSCALE_B = 159,
		BD3DRS_POINTSCALE_C = 160,
		BD3DRS_MULTISAMPLEANTIALIAS = 161,
		BD3DRS_MULTISAMPLEMASK = 162,
		BD3DRS_PATCHEDGESTYLE = 163,
		BD3DRS_DEBUGMONITORTOKEN = 165,
		BD3DRS_POINTSIZE_MAX = 166,
		BD3DRS_INDEXEDVERTEXBLENDENABLE = 167,
		BD3DRS_COLORWRITEENABLE = 168,
		BD3DRS_TWEENFACTOR = 170,
		BD3DRS_BLENDOP = 171,
		BD3DRS_POSITIONDEGREE = 172,
		BD3DRS_NORMALDEGREE = 173,
		BD3DRS_SCISSORTESTENABLE = 174,
		BD3DRS_SLOPESCALEDEPTHBIAS = 175,
		BD3DRS_ANTIALIASEDLINEENABLE = 176,
		BD3DRS_MINTESSELLATIONLEVEL = 178,
		BD3DRS_MAXTESSELLATIONLEVEL = 179,
		BD3DRS_ADAPTIVETESS_X = 180,
		BD3DRS_ADAPTIVETESS_Y = 181,
		BD3DRS_ADAPTIVETESS_Z = 182,
		BD3DRS_ADAPTIVETESS_W = 183,
		BD3DRS_ENABLEADAPTIVETESSELLATION = 184,
		BD3DRS_TWOSIDEDSTENCILMODE = 185,
		BD3DRS_CCW_STENCILFAIL = 186,
		BD3DRS_CCW_STENCILZFAIL = 187,
		BD3DRS_CCW_STENCILPASS = 188,
		BD3DRS_CCW_STENCILFUNC = 189,
		BD3DRS_COLORWRITEENABLE1 = 190,
		BD3DRS_COLORWRITEENABLE2 = 191,
		BD3DRS_COLORWRITEENABLE3 = 192,
		BD3DRS_BLENDFACTOR = 193,
		BD3DRS_SRGBWRITEENABLE = 194,
		BD3DRS_DEPTHBIAS = 195,
		BD3DRS_WRAP8 = 198,
		BD3DRS_WRAP9 = 199,
		BD3DRS_WRAP10 = 200,
		BD3DRS_WRAP11 = 201,
		BD3DRS_WRAP12 = 202,
		BD3DRS_WRAP13 = 203,
		BD3DRS_WRAP14 = 204,
		BD3DRS_WRAP15 = 205,
		BD3DRS_SEPARATEALPHABLENDENABLE = 206,
		BD3DRS_SRCBLENDALPHA = 207,
		BD3DRS_DESTBLENDALPHA = 208,
		BD3DRS_BLENDOPALPHA = 209,

		BD3DRS_MAX_TYPE
	};



	/*! Defines constants that describe depth-buffer formats
		Members of this enumerated type are used with the D3DRS_ZENABLE render state.
	*/
	enum BD3DZBUFFERTYPE
	{
		BD3DZB_FALSE                 = 0,	// Disable depth buffering
		BD3DZB_TRUE                  = 1,	// Enable z-buffering
		BD3DZB_USEW                  = 2	//Enable w-buffering.
	};

	//! Defines the supported compare functions.
	enum BD3DCMPFUNC
	{
		BD3DCMP_NEVER	= 1,// Always fail the test. 
		BD3DCMP_LESS,		// Accept the new pixel if its value is less than the value of the current pixel. 
		BD3DCMP_EQUAL,		// Accept the new pixel if its value equals the value of the current pixel. 
		BD3DCMP_LESSEQUAL,	// Accept the new pixel if its value is less than or equal to the value of the current pixel. 
		BD3DCMP_GREATER,		// Accept the new pixel if its value is greater than the value of the current pixel. 
		BD3DCMP_NOTEQUAL,	// Accept the new pixel if its value does not equal the value of the current pixel. 
		BD3DCMP_GREATEREQUAL,// Accept the new pixel if its value is greater than or equal to the value of the current pixel. 
		BD3DCMP_ALWAYS		// Always pass the test. 
	};

	enum BD3DMATERIALCOLORSOURCE
	{
		BD3DMCS_MATERIAL = 0,	// Use the color from the current material. 
		BD3DMCS_COLOR1 = 1,		// Use the diffuse vertex color. 
		BD3DMCS_COLOR2 = 2		// Use the specular vertex color. 
	};


	//! Defines constants that describe the supported shading modes.
	enum BD3DSHADEMODE
	{
		/*!	BD3DSHADE_FLAT
			Flat shading mode. The color and specular component of the first vertex in the triangle
			are used to determine the color and specular component of the face. These colors remain
			constant across the triangle; that is, they are not interpolated. The specular alpha is
			interpolated.
		*/
		BD3DSHADE_FLAT = 1,

		/*!	BD3DSHADE_GOURAUD
			Gouraud shading mode. The color and specular components of the face are determined by a
			linear interpolation between all three of the triangle's vertices.
		*/
		BD3DSHADE_GOURAUD = 2,

		/*!	BD3DSHADE_PHONG
			Not supported. 
		*/
		BD3DSHADE_PHONG = 3
	};

	/*!	Defines constants describing the fill mode
		The values in this enumerated type are used by the BD3DRS_FILLMODE render state
	*/
	enum BD3DFILLMODE
	{
		BD3DFILL_POINT = 1,		// Fill points.
		BD3DFILL_WIREFRAME = 2,	// Fill wireframes.
		BD3DFILL_SOLID = 3		// Fill solids.
	};



	/*! Defines the supported culling modes.
		The values in this enumerated type are used by the B3DRS_CULLMODE render state.
		The culling modes define how back faces are culled when rendering a geometry.
	*/
	enum BD3DCULL
	{
		BD3DCULL_NONE = 1,	// Do not cull back faces. 
		BD3DCULL_CW = 2,	// Cull back faces with clockwise vertices. 
		BD3DCULL_CCW = 3	// Cull back faces with counterclockwise vertices. 
	};

	struct SShaderParam
	{
		u32 ColorUnits;
		u32 TextureUnits;

		u32 RenderState [ BD3DRS_MAX_TYPE ];
		void SetRenderState ( BD3DRENDERSTATETYPE state, u32 value );
	};

	void SShaderParam::SetRenderState ( BD3DRENDERSTATETYPE state, u32 value )
	{
		RenderState [ state ] = value;
	}



class CBurningShader_Raster_Reference : public IBurningShader
{
public:

	//! constructor
	CBurningShader_Raster_Reference(CBurningVideoDriver* driver);

	//! draws an indexed triangle list
	virtual void drawTriangle ( const s4DVertex *a,const s4DVertex *b,const s4DVertex *c );

	virtual void setMaterial ( const SBurningShaderMaterial &material );


private:
	void scanline ();
	void scanline2 ();

	sScanLineData line;
	sPixelShaderData pShader;

	void pShader_1 ();
	void pShader_EMT_LIGHTMAP_M4 ();

	SShaderParam ShaderParam;

	REALINLINE u32 depthFunc ();
	REALINLINE void depthWrite ();


};

//! constructor
CBurningShader_Raster_Reference::CBurningShader_Raster_Reference(CBurningVideoDriver* driver)
: IBurningShader(driver)
{
	#ifdef _DEBUG
	setDebugName("CBurningShader_Raster_Reference");
	#endif
}


/*!
*/
void CBurningShader_Raster_Reference::pShader_EMT_LIGHTMAP_M4 ()
{
	tFixPoint r0, g0, b0;
	tFixPoint r1, g1, b1;

	f32 inversew = fix_inverse32 ( line.w[0] );

	getSample_texture ( r0, g0, b0, &IT[0], tofix ( line.t[0][0].x,inversew), tofix ( line.t[0][0].y,inversew) );
	getSample_texture ( r1, g1, b1, &IT[1], tofix ( line.t[1][0].x,inversew), tofix ( line.t[1][0].y,inversew) );


	pShader.dst[pShader.i] = fix_to_color ( clampfix_maxcolor ( imulFix_tex2 ( r0, r1 ) ),
							clampfix_maxcolor ( imulFix_tex2 ( g0, g1 ) ),
							clampfix_maxcolor ( imulFix_tex2 ( b0, b1 ) )
						);

}

/*!
*/
void CBurningShader_Raster_Reference::pShader_1 ()
{
	tFixPoint r0, g0, b0;
	tFixPoint tx0, ty0;

	const f32 inversew = fix_inverse32 ( line.w[0] );

	tx0 = tofix ( line.t[0][0].x, inversew );
	ty0 = tofix ( line.t[0][0].y, inversew );

	getSample_texture ( r0, g0, b0, &IT[0], tx0, ty0 );
	pShader.dst[pShader.i] = fix_to_color ( r0, g0, b0 );

}


/*!
*/
void CBurningShader_Raster_Reference::setMaterial ( const SBurningShaderMaterial &material )
{
	const video::SMaterial &m = material.org;

	u32 i;
	u32 enable;

	ShaderParam.ColorUnits = 0;
	ShaderParam.TextureUnits = 0;
	for ( i = 0; i != BURNING_MATERIAL_MAX_TEXTURES; ++i )
	{
		if ( m.getTexture( i ) )
			ShaderParam.TextureUnits = i;
	}

	// shademode
	ShaderParam.SetRenderState( BD3DRS_SHADEMODE,
		m.GouraudShading ? BD3DSHADE_GOURAUD : BD3DSHADE_FLAT
	);

	// fillmode
	ShaderParam.SetRenderState( BD3DRS_FILLMODE,
		m.Wireframe ? BD3DFILL_WIREFRAME : m.PointCloud ? BD3DFILL_POINT : BD3DFILL_SOLID
	);

	// back face culling
	ShaderParam.SetRenderState( BD3DRS_CULLMODE, 
		m.BackfaceCulling ? BD3DCULL_CCW : BD3DCULL_NONE
	);

	// lighting
	ShaderParam.SetRenderState( BD3DRS_LIGHTING, m.Lighting );

	// specular highlights
	enable = F32_LOWER_EQUAL_0 ( m.Shininess );
	ShaderParam.SetRenderState( BD3DRS_SPECULARENABLE, enable);
	ShaderParam.SetRenderState( BD3DRS_NORMALIZENORMALS, enable);
	ShaderParam.SetRenderState( BD3DRS_SPECULARMATERIALSOURCE, (m.ColorMaterial==ECM_SPECULAR)?BD3DMCS_COLOR1:BD3DMCS_MATERIAL);

	// depth buffer enable and compare
	ShaderParam.SetRenderState( BD3DRS_ZENABLE, (material.org.ZBuffer==video::ECFN_NEVER) ? BD3DZB_FALSE : BD3DZB_USEW);
	switch (material.org.ZBuffer)
	{
	case ECFN_NEVER:
		ShaderParam.SetRenderState(BD3DRS_ZFUNC, BD3DCMP_NEVER);
		break;
	case ECFN_LESSEQUAL:
		ShaderParam.SetRenderState(BD3DRS_ZFUNC, BD3DCMP_LESSEQUAL);
		break;
	case ECFN_EQUAL:
		ShaderParam.SetRenderState(BD3DRS_ZFUNC, BD3DCMP_EQUAL);
		break;
	case ECFN_LESS:
		ShaderParam.SetRenderState(BD3DRS_ZFUNC, BD3DCMP_LESSEQUAL);
		break;
	case ECFN_NOTEQUAL:
		ShaderParam.SetRenderState(BD3DRS_ZFUNC, BD3DCMP_NOTEQUAL);
		break;
	case ECFN_GREATEREQUAL:
		ShaderParam.SetRenderState(BD3DRS_ZFUNC, BD3DCMP_GREATEREQUAL);
		break;
	case ECFN_GREATER:
		ShaderParam.SetRenderState(BD3DRS_ZFUNC, BD3DCMP_GREATER);
		break;
	case ECFN_ALWAYS:
		ShaderParam.SetRenderState(BD3DRS_ZFUNC, BD3DCMP_ALWAYS);
		break;
	}

	// depth buffer write
	ShaderParam.SetRenderState( BD3DRS_ZWRITEENABLE, m.ZWriteEnable );
}

/*!
*/
REALINLINE u32 CBurningShader_Raster_Reference::depthFunc ()
{
	if ( ShaderParam.RenderState [ BD3DRS_ZENABLE ] )
	{
		switch ( ShaderParam.RenderState [ BD3DRS_ZFUNC ] )
		{
			case BD3DCMP_LESSEQUAL:
				return line.w[0] >= pShader.z[ pShader.i];
			case BD3DCMP_EQUAL:
				return line.w[0] == pShader.z[ pShader.i];
		}
	}
	return 1;
}

/*!
*/
REALINLINE void CBurningShader_Raster_Reference::depthWrite ()
{
	if ( ShaderParam.RenderState [ BD3DRS_ZWRITEENABLE ] )
	{
		pShader.z[pShader.i] = line.w[0];
	}
}

/*!
*/
REALINLINE void CBurningShader_Raster_Reference::scanline2()
{
	// apply top-left fill-convention, left
	pShader.xStart = core::ceil32( line.x[0] );
	pShader.xEnd = core::ceil32( line.x[1] ) - 1;

	pShader.dx = pShader.xEnd - pShader.xStart;
	if ( pShader.dx < 0 )
		return;

	// slopes
	const f32 invDeltaX = core::reciprocal ( line.x[1] - line.x[0] );
	const f32 subPixel = ( (f32) pShader.xStart ) - line.x[0];

	// store slopes in endpoint, and correct first pixel

	line.w[0] += (line.w[1] = (line.w[1] - line.w[0]) * invDeltaX) * subPixel;

	u32 i;

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	for ( i = 0; i != ShaderParam.ColorUnits; ++i )
	{
		line.c[i][1] = (line.c[i][1] - line.c[i][0]) * invDeltaX;
		line.c[i][0] += line.c[i][1] * subPixel;
	}
#endif

	for ( i = 0; i != ShaderParam.TextureUnits; ++i )
	{
		line.t[i][1] = (line.t[i][1] - line.t[i][0]) * invDeltaX;
		line.t[i][0] += line.t[i][1] * subPixel;
	}

	pShader.dst = (tVideoSample*) ( (u8*) RenderTarget->lock() + ( line.y * RenderTarget->getPitch() ) + ( pShader.xStart << VIDEO_SAMPLE_GRANULARITY ) );
	pShader.z = (fp24*) ( (u8*) DepthBuffer->lock() + ( line.y * DepthBuffer->getPitch() ) + ( pShader.xStart << VIDEO_SAMPLE_GRANULARITY ) );

	for ( pShader.i = 0; pShader.i <= pShader.dx; ++pShader.i )
	{
		if ( depthFunc() )
		{
			depthWrite ();
		}

		// advance next pixel
		line.w[0] += line.w[1];

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
		for ( i = 0; i != ShaderParam.ColorUnits; ++i )
		{
			line.c[i][0] += line.c[i][1];
		}
#endif
		for ( i = 0; i != ShaderParam.TextureUnits; ++i )
		{
			line.t[i][0] += line.t[i][1];
		}
	}
}


/*!
*/
REALINLINE void CBurningShader_Raster_Reference::scanline ()
{
	u32 i;

	// apply top-left fill-convention, left
	pShader.xStart = core::ceil32( line.x[0] );
	pShader.xEnd = core::ceil32( line.x[1] ) - 1;

	pShader.dx = pShader.xEnd - pShader.xStart;
	if ( pShader.dx < 0 )
		return;

	// slopes
	const f32 invDeltaX = core::reciprocal ( line.x[1] - line.x[0] );

	// search z-buffer for first not occulled pixel
	pShader.z = (fp24*) ( (u8*) DepthBuffer->lock() + ( line.y * DepthBuffer->getPitch() ) + ( pShader.xStart << VIDEO_SAMPLE_GRANULARITY ) );

	// subTexel
	const f32 subPixel = ( (f32) pShader.xStart ) - line.x[0];

	const f32 b = (line.w[1] - line.w[0]) * invDeltaX;
	f32 a = line.w[0] + ( b * subPixel );

	pShader.i = 0;

	if ( ShaderParam.RenderState [ BD3DRS_ZENABLE ] )
	{
		u32 condition;
		switch ( ShaderParam.RenderState [ BD3DRS_ZFUNC ] )
		{
			case BD3DCMP_LESSEQUAL:
				condition = a < pShader.z[pShader.i];
				break;
			case BD3DCMP_EQUAL:
				condition = a != pShader.z[pShader.i];
				break;
		}
		while ( a < pShader.z[pShader.i] )
		{
			a += b;

			pShader.i += 1;
			if ( pShader.i > pShader.dx )
				return;
			
		}
	}

	// lazy setup rest of scanline

	line.w[0] = a;
	line.w[1] = b;

	pShader.dst = (tVideoSample*) ( (u8*) RenderTarget->lock() + ( line.y * RenderTarget->getPitch() ) + ( pShader.xStart << VIDEO_SAMPLE_GRANULARITY ) );

	a = (f32) pShader.i + subPixel;

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	for ( i = 0; i != ShaderParam.ColorUnits; ++i )
	{
		line.c[i][1] = (line.c[i][1] - line.c[i][0]) * invDeltaX;
		line.c[i][0] += line.c[i][1] * a;
	}
#endif

	for ( i = 0; i != ShaderParam.TextureUnits; ++i )
	{
		line.t[i][1] = (line.t[i][1] - line.t[i][0]) * invDeltaX;
		line.t[i][0] += line.t[i][1] * a;
	}

	for ( ; pShader.i <= pShader.dx; ++pShader.i )
	{
		if ( line.w[0] >= pShader.z[pShader.i] )
		{
			pShader.z[pShader.i] = line.w[0];

			pShader_EMT_LIGHTMAP_M4 ();
		}

		line.w[0] += line.w[1];

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
		for ( i = 0; i != ShaderParam.ColorUnits; ++i )
		{
			line.c[i][0] += line.c[i][1];
		}
#endif
		for ( i = 0; i != ShaderParam.TextureUnits; ++i )
		{
			line.t[i][0] += line.t[i][1];
		}
	}

}
	


void CBurningShader_Raster_Reference::drawTriangle ( const s4DVertex *a,const s4DVertex *b,const s4DVertex *c )
{
	sScanConvertData scan;
	u32 i;

	// sort on height, y
	if ( F32_A_GREATER_B ( a->Pos.y , b->Pos.y ) ) swapVertexPointer(&a, &b);
	if ( F32_A_GREATER_B ( b->Pos.y , c->Pos.y ) ) swapVertexPointer(&b, &c);
	if ( F32_A_GREATER_B ( a->Pos.y , b->Pos.y ) ) swapVertexPointer(&a, &b);


	// calculate delta y of the edges
	scan.invDeltaY[0] = core::reciprocal ( c->Pos.y - a->Pos.y );
	scan.invDeltaY[1] = core::reciprocal ( b->Pos.y - a->Pos.y );
	scan.invDeltaY[2] = core::reciprocal ( c->Pos.y - b->Pos.y );

	if ( F32_LOWER_EQUAL_0 ( scan.invDeltaY[0] )  )
		return;


	// find if the major edge is left or right aligned
	f32 temp[4];

	temp[0] = a->Pos.x - c->Pos.x;
	temp[1] = a->Pos.y - c->Pos.y;
	temp[2] = b->Pos.x - a->Pos.x;
	temp[3] = b->Pos.y - a->Pos.y;

	scan.left = ( temp[0] * temp[3] - temp[1] * temp[2] ) > (f32) 0.0 ? 0 : 1;
	scan.right = 1 - scan.left;

	// calculate slopes for the major edge
	scan.slopeX[0] = (c->Pos.x - a->Pos.x) * scan.invDeltaY[0];
	scan.x[0] = a->Pos.x;

	scan.slopeW[0] = (c->Pos.w - a->Pos.w) * scan.invDeltaY[0];
	scan.w[0] = a->Pos.w;

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	for ( i = 0; i != ShaderParam.ColorUnits; ++i )
	{
		scan.c[i][0] = a->Color[i];
		scan.slopeC[i][0] = (c->Color[i] - a->Color[i]) * scan.invDeltaY[0];
	}
#endif

	for ( i = 0; i != ShaderParam.TextureUnits; ++i )
	{
		scan.t[i][0] = a->Tex[i];
		scan.slopeT[i][0] = (c->Tex[i] - a->Tex[i]) * scan.invDeltaY[0];
	}

	// top left fill convention y run
	s32 yStart;
	s32 yEnd;

	f32 subPixel;

	// rasterize upper sub-triangle
	if ( F32_GREATER_0 ( scan.invDeltaY[1] ) )
	{
		// calculate slopes for top edge
		scan.slopeX[1] = (b->Pos.x - a->Pos.x) * scan.invDeltaY[1];
		scan.x[1] = a->Pos.x;

		scan.slopeW[1] = (b->Pos.w - a->Pos.w) * scan.invDeltaY[1];
		scan.w[1] = a->Pos.w;

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
		for ( i = 0; i != ShaderParam.ColorUnits; ++i )
		{
			scan.c[i][1] = a->Color[i];
			scan.slopeC[i][1] = (b->Color[i] - a->Color[i]) * scan.invDeltaY[1];
		}
#endif
		for ( i = 0; i != ShaderParam.TextureUnits; ++i )
		{
			scan.t[i][1] = a->Tex[i];
			scan.slopeT[i][1] = (b->Tex[i] - a->Tex[i]) * scan.invDeltaY[1];
		}

		// apply top-left fill convention, top part
		yStart = core::ceil32( a->Pos.y );
		yEnd = core::ceil32( b->Pos.y ) - 1;

		subPixel = ( (f32) yStart ) - a->Pos.y;

		// correct to pixel center
		scan.x[0] += scan.slopeX[0] * subPixel;
		scan.x[1] += scan.slopeX[1] * subPixel;		

		scan.w[0] += scan.slopeW[0] * subPixel;
		scan.w[1] += scan.slopeW[1] * subPixel;		

		for ( i = 0; i != ShaderParam.ColorUnits; ++i )
		{
			scan.c[i][0] += scan.slopeC[i][0] * subPixel;
			scan.c[i][1] += scan.slopeC[i][1] * subPixel;		
		}

		for ( i = 0; i != ShaderParam.TextureUnits; ++i )
		{
			scan.t[i][0] += scan.slopeT[i][0] * subPixel;
			scan.t[i][1] += scan.slopeT[i][1] * subPixel;		
		}

		// rasterize the edge scanlines
		for( line.y = yStart; line.y <= yEnd; ++line.y)
		{
			line.x[scan.left] = scan.x[0];
			line.w[scan.left] = scan.w[0];

			line.x[scan.right] = scan.x[1];
			line.w[scan.right] = scan.w[1];

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
			for ( i = 0; i != ShaderParam.ColorUnits; ++i )
			{
				line.c[i][scan.left] = scan.c[i][0];
				line.c[i][scan.right] = scan.c[i][1];
			}
#endif
			for ( i = 0; i != ShaderParam.TextureUnits; ++i )
			{
				line.t[i][scan.left] = scan.t[i][0];
				line.t[i][scan.right] = scan.t[i][1];
			}

			// render a scanline
			scanline ();

			scan.x[0] += scan.slopeX[0];
			scan.x[1] += scan.slopeX[1];

			scan.w[0] += scan.slopeW[0];
			scan.w[1] += scan.slopeW[1];

			for ( i = 0; i != ShaderParam.ColorUnits; ++i )
			{
				scan.c[i][0] += scan.slopeC[i][0];
				scan.c[i][1] += scan.slopeC[i][1];
			}

			for ( i = 0; i != ShaderParam.TextureUnits; ++i )
			{
				scan.t[i][0] += scan.slopeT[i][0];
				scan.t[i][1] += scan.slopeT[i][1];
			}

		}
	}

	// rasterize lower sub-triangle
	if ( F32_GREATER_0 ( scan.invDeltaY[2] ) )
	{
		// advance to middle point
		if ( F32_GREATER_0 ( scan.invDeltaY[1] )  )
		{
			temp[0] = b->Pos.y - a->Pos.y;	// dy

			scan.x[0] = a->Pos.x + scan.slopeX[0] * temp[0];
			scan.w[0] = a->Pos.w + scan.slopeW[0] * temp[0];

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
			for ( i = 0; i != ShaderParam.ColorUnits; ++i )
			{
				scan.c[i][0] = a->Color[i] + scan.slopeC[i][0] * temp[0];
			}
#endif
			for ( i = 0; i != ShaderParam.TextureUnits; ++i )
			{
				scan.t[i][0] = a->Tex[i] + scan.slopeT[i][0] * temp[0];
			}
		}

		// calculate slopes for bottom edge
		scan.slopeX[1] = (c->Pos.x - b->Pos.x) * scan.invDeltaY[2];
		scan.x[1] = b->Pos.x;

		scan.slopeW[1] = (c->Pos.w - b->Pos.w) * scan.invDeltaY[2];
		scan.w[1] = b->Pos.w;

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
		for ( i = 0; i != ShaderParam.ColorUnits; ++i )
		{
			scan.c[i][1] = b->Color[i];
			scan.slopeC[i][1] = (c->Color[i] - b->Color[i]) * scan.invDeltaY[2];
		}
#endif
		for ( i = 0; i != ShaderParam.TextureUnits; ++i )
		{
			scan.t[i][1] = b->Tex[i];
			scan.slopeT[i][1] = (c->Tex[i] - b->Tex[i]) * scan.invDeltaY[2];
		}

		// apply top-left fill convention, top part
		yStart = core::ceil32( b->Pos.y );
		yEnd = core::ceil32( c->Pos.y ) - 1;


		subPixel = ( (f32) yStart ) - b->Pos.y;

		// correct to pixel center
		scan.x[0] += scan.slopeX[0] * subPixel;
		scan.x[1] += scan.slopeX[1] * subPixel;		

		scan.w[0] += scan.slopeW[0] * subPixel;
		scan.w[1] += scan.slopeW[1] * subPixel;		

		for ( i = 0; i != ShaderParam.ColorUnits; ++i )
		{
			scan.c[i][0] += scan.slopeC[i][0] * subPixel;
			scan.c[i][1] += scan.slopeC[i][1] * subPixel;		
		}

		for ( i = 0; i != ShaderParam.TextureUnits; ++i )
		{
			scan.t[i][0] += scan.slopeT[i][0] * subPixel;
			scan.t[i][1] += scan.slopeT[i][1] * subPixel;		
		}

		// rasterize the edge scanlines
		for( line.y = yStart; line.y <= yEnd; ++line.y)
		{
			line.x[scan.left] = scan.x[0];
			line.w[scan.left] = scan.w[0];

			line.x[scan.right] = scan.x[1];
			line.w[scan.right] = scan.w[1];

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
			for ( i = 0; i != ShaderParam.ColorUnits; ++i )
			{
				line.c[i][scan.left] = scan.c[i][0];
				line.c[i][scan.right] = scan.c[i][1];
			}
#endif
			for ( i = 0; i != ShaderParam.TextureUnits; ++i )
			{
				line.t[i][scan.left] = scan.t[i][0];
				line.t[i][scan.right] = scan.t[i][1];
			}

			// render a scanline
			scanline ();

			scan.x[0] += scan.slopeX[0];
			scan.x[1] += scan.slopeX[1];

			scan.w[0] += scan.slopeW[0];
			scan.w[1] += scan.slopeW[1];

			for ( i = 0; i != ShaderParam.TextureUnits; ++i )
			{
				scan.c[i][0] += scan.slopeC[i][0];
				scan.c[i][1] += scan.slopeC[i][1];
			}

			for ( i = 0; i != ShaderParam.TextureUnits; ++i )
			{
				scan.t[i][0] += scan.slopeT[i][0];
				scan.t[i][1] += scan.slopeT[i][1];
			}
		}
	}
}


} // end namespace video
} // end namespace irr


namespace irr
{
namespace video
{



//! creates a flat triangle renderer
IBurningShader* createTriangleRendererReference(CBurningVideoDriver* driver)
{
	return new CBurningShader_Raster_Reference(driver);
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_


