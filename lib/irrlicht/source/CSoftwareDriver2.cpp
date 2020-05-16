// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "CSoftwareDriver2.h"

#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

#include "SoftwareDriver2_helper.h"
#include "CSoftwareTexture2.h"
#include "CSoftware2MaterialRenderer.h"
#include "S3DVertex.h"
#include "S4DVertex.h"
#include "CBlit.h"


#define MAT_TEXTURE(tex) ( (video::CSoftwareTexture2*) Material.org.getTexture ( tex ) )


namespace irr
{
namespace video
{

namespace glsl
{

typedef sVec4 vec4;
typedef sVec3 vec3;
typedef sVec2 vec2;

#define in
#define uniform
#define attribute
#define varying

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

struct mat4{
   float m[4][4];

   vec4 operator* ( const vec4 &in ) const
   {
	   vec4 out;
	   return out;
   }

};

struct mat3{
   float m[3][3];

   vec3 operator* ( const vec3 &in ) const
   {
	   vec3 out;
	   return out;
   }
};

const int gl_MaxLights = 8;


inline float dot (float x, float y) { return x * y; }
inline float dot ( const vec2 &x, const vec2 &y) { return x.x * y.x + x.y * y.y; }
inline float dot ( const vec3 &x, const vec3 &y) { return x.x * y.x + x.y * y.y + x.z * y.z; }
inline float dot ( const vec4 &x, const vec4 &y) { return x.x * y.x + x.y * y.y + x.z * y.z + x.w * y.w; }

inline float reflect (float I, float N)				{ return I - 2.0 * dot (N, I) * N; }
inline vec2 reflect (const vec2 &I, const vec2 &N)	{ return I - N * 2.0 * dot (N, I); }
inline vec3 reflect (const vec3 &I, const vec3 &N)	{ return I - N * 2.0 * dot (N, I); }
inline vec4 reflect (const vec4 &I, const vec4 &N)	{ return I - N * 2.0 * dot (N, I); }


inline float refract (float I, float N, float eta){
    const float k = 1.0 - eta * eta * (1.0 - dot (N, I) * dot (N, I));
    if (k < 0.0)
        return 0.0;
    return eta * I - (eta * dot (N, I) + sqrt (k)) * N;
}

inline vec2 refract (const vec2 &I, const vec2 &N, float eta){
    const float k = 1.0 - eta * eta * (1.0 - dot (N, I) * dot (N, I));
    if (k < 0.0)
        return vec2 (0.0);
    return I * eta - N * (eta * dot (N, I) + sqrt (k));
}

inline vec3 refract (const vec3 &I, const vec3 &N, float eta) {
    const float k = 1.0 - eta * eta * (1.0 - dot (N, I) * dot (N, I));
    if (k < 0.0)
        return vec3 (0.0);
    return I * eta - N * (eta * dot (N, I) + sqrt (k));
}

inline vec4 refract (const vec4 &I, const vec4 &N, float eta) {
    const float k = 1.0 - eta * eta * (1.0 - dot (N, I) * dot (N, I));
    if (k < 0.0)
        return vec4 (0.0);
    return I * eta - N * (eta * dot (N, I) + sqrt (k));
}


inline float length ( const vec3 &v ) { return sqrtf ( v.x * v.x + v.y * v.y + v.z * v.z ); }
vec3 normalize ( const vec3 &v ) { 	float l = 1.f / length ( v ); return vec3 ( v.x * l, v.y * l, v.z * l ); }
float max ( float a, float b ) { return a > b ? a : b; }
float min ( float a, float b ) { return a < b ? a : b; }
vec4 clamp ( const vec4 &a, f32 low, f32 high ) { return vec4 ( min (max(a.x,low), high), min (max(a.y,low), high), min (max(a.z,low), high), min (max(a.w,low), high) ); }



typedef int sampler2D;
sampler2D texUnit0;

vec4 texture2D (sampler2D sampler, const vec2 &coord) { return vec4 (0.0); }

struct gl_LightSourceParameters {
	vec4 ambient;              // Acli
	vec4 diffuse;              // Dcli
	vec4 specular;             // Scli
	vec4 position;             // Ppli
	vec4 halfVector;           // Derived: Hi
	vec3 spotDirection;        // Sdli
	float spotExponent;        // Srli
	float spotCutoff;          // Crli
							// (range: [0.0,90.0], 180.0)
	float spotCosCutoff;       // Derived: cos(Crli)
							// (range: [1.0,0.0],-1.0)
	float constantAttenuation; // K0
	float linearAttenuation;   // K1
	float quadraticAttenuation;// K2
};

uniform gl_LightSourceParameters gl_LightSource[gl_MaxLights];

struct gl_LightModelParameters {
    vec4 ambient;
};
uniform gl_LightModelParameters gl_LightModel;

struct gl_LightModelProducts {
    vec4 sceneColor;
};

uniform gl_LightModelProducts gl_FrontLightModelProduct;
uniform gl_LightModelProducts gl_BackLightModelProduct;

struct gl_LightProducts {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

uniform gl_LightProducts gl_FrontLightProduct[gl_MaxLights];
uniform gl_LightProducts gl_BackLightProduct[gl_MaxLights];

struct gl_MaterialParameters
{
	vec4 emission;    // Ecm
	vec4 ambient;     // Acm
	vec4 diffuse;     // Dcm
	vec4 specular;    // Scm
	float shininess;  // Srm
};
uniform gl_MaterialParameters gl_FrontMaterial;
uniform gl_MaterialParameters gl_BackMaterial;

// GLSL has some built-in attributes in a vertex shader:
attribute vec4 gl_Vertex;			// 4D vector representing the vertex position
attribute vec3 gl_Normal;			// 3D vector representing the vertex normal
attribute vec4 gl_Color;			// 4D vector representing the vertex color
attribute vec4 gl_MultiTexCoord0;	// 4D vector representing the texture coordinate of texture unit X
attribute vec4 gl_MultiTexCoord1;	// 4D vector representing the texture coordinate of texture unit X

uniform mat4 gl_ModelViewMatrix;			//4x4 Matrix representing the model-view matrix.
uniform mat4 gl_ModelViewProjectionMatrix;	//4x4 Matrix representing the model-view-projection matrix.
uniform mat3 gl_NormalMatrix;				//3x3 Matrix representing the inverse transpose model-view matrix. This matrix is used for normal transformation.


varying vec4 gl_FrontColor;				// 4D vector representing the primitives front color
varying vec4 gl_FrontSecondaryColor;	// 4D vector representing the primitives second front color
varying vec4 gl_BackColor;				// 4D vector representing the primitives back color
varying vec4 gl_TexCoord[4];			// 4D vector representing the Xth texture coordinate

// shader output
varying vec4 gl_Position;				// 4D vector representing the final processed vertex position. Only  available in vertex shader.
varying vec4 gl_FragColor;				// 4D vector representing the final color which is written in the frame buffer. Only available in fragment shader.
varying float gl_FragDepth;				// float representing the depth which is written in the depth buffer. Only available in fragment shader.

varying vec4 gl_SecondaryColor;
varying float gl_FogFragCoord;


vec4 ftransform(void)
{
	return gl_ModelViewProjectionMatrix * gl_Vertex;
}

vec3 fnormal(void)
{
    //Compute the normal
    vec3 normal = gl_NormalMatrix * gl_Normal;
    normal = normalize(normal);
    return normal;
}


struct program1
{
	vec4 Ambient;
	vec4 Diffuse;
	vec4 Specular;

	void pointLight(in int i, in vec3 normal, in vec3 eye, in vec3 ecPosition3)
	{
	   float nDotVP;       // normal . light direction
	   float nDotHV;       // normal . light half vector
	   float pf;           // power factor
	   float attenuation;  // computed attenuation factor
	   float d;            // distance from surface to light source
	   vec3  VP;           // direction from surface to light position
	   vec3  halfVector;   // direction of maximum highlights

	   // Compute vector from surface to light position
	   VP = vec3 (gl_LightSource[i].position) - ecPosition3;

	   // Compute distance between surface and light position
	   d = length(VP);

	   // Normalize the vector from surface to light position
	   VP = normalize(VP);

	   // Compute attenuation
	   attenuation = 1.0 / (gl_LightSource[i].constantAttenuation +
		   gl_LightSource[i].linearAttenuation * d +
		   gl_LightSource[i].quadraticAttenuation * d * d);

	   halfVector = normalize(VP + eye);

	   nDotVP = max(0.0, dot(normal, VP));
	   nDotHV = max(0.0, dot(normal, halfVector));

	   if (nDotVP == 0.0)
	   {
		   pf = 0.0;
	   }
	   else
	   {
		   pf = pow(nDotHV, gl_FrontMaterial.shininess);

	   }
	   Ambient  += gl_LightSource[i].ambient * attenuation;
	   Diffuse  += gl_LightSource[i].diffuse * nDotVP * attenuation;
	   Specular += gl_LightSource[i].specular * pf * attenuation;
	}

	vec3 fnormal(void)
	{
		//Compute the normal
		vec3 normal = gl_NormalMatrix * gl_Normal;
		normal = normalize(normal);
		return normal;
	}

	void ftexgen(in vec3 normal, in vec4 ecPosition)
	{

		gl_TexCoord[0] = gl_MultiTexCoord0;
	}

	void flight(in vec3 normal, in vec4 ecPosition, float alphaFade)
	{
		vec4 color;
		vec3 ecPosition3;
		vec3 eye;

		ecPosition3 = (vec3 (ecPosition)) / ecPosition.w;
		eye = vec3 (0.0, 0.0, 1.0);

		// Clear the light intensity accumulators
		Ambient  = vec4 (0.0);
		Diffuse  = vec4 (0.0);
		Specular = vec4 (0.0);

		pointLight(0, normal, eye, ecPosition3);

		pointLight(1, normal, eye, ecPosition3);

		color = gl_FrontLightModelProduct.sceneColor +
		  Ambient  * gl_FrontMaterial.ambient +
		  Diffuse  * gl_FrontMaterial.diffuse;
		gl_FrontSecondaryColor = Specular * gl_FrontMaterial.specular;
		color = clamp( color, 0.0, 1.0 );
		gl_FrontColor = color;

		gl_FrontColor.a *= alphaFade;
	}


	void vertexshader_main (void)
	{
		vec3  transformedNormal;
		float alphaFade = 1.0;

		// Eye-coordinate position of vertex, needed in various calculations
		vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;

		// Do fixed functionality vertex transform
		gl_Position = ftransform();
		transformedNormal = fnormal();
		flight(transformedNormal, ecPosition, alphaFade);
		ftexgen(transformedNormal, ecPosition);
	}

	void fragmentshader_main (void)
	{
		vec4 color;

		color = gl_Color;

		color *= texture2D(texUnit0, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y) );

		color += gl_SecondaryColor;
		color = clamp(color, 0.0, 1.0);

		gl_FragColor = color;
	}
};

}

//! constructor
CBurningVideoDriver::CBurningVideoDriver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, video::IImagePresenter* presenter)
: CNullDriver(io, params.WindowSize), BackBuffer(0), Presenter(presenter),
	WindowId(0), SceneSourceRect(0),
	RenderTargetTexture(0), RenderTargetSurface(0), CurrentShader(0),
	 DepthBuffer(0), StencilBuffer ( 0 ),
	 CurrentOut ( 12 * 2, 128 ), Temp ( 12 * 2, 128 )
{
	#ifdef _DEBUG
	setDebugName("CBurningVideoDriver");
	#endif

	// create backbuffer
	BackBuffer = new CImage(BURNINGSHADER_COLOR_FORMAT, params.WindowSize);
	if (BackBuffer)
	{
		BackBuffer->fill(SColor(0));

		// create z buffer
		if ( params.ZBufferBits )
			DepthBuffer = video::createDepthBuffer(BackBuffer->getDimension());

		// create stencil buffer
		if ( params.Stencilbuffer )
			StencilBuffer = video::createStencilBuffer(BackBuffer->getDimension());
	}

	DriverAttributes->setAttribute("MaxTextures", 2);
	DriverAttributes->setAttribute("MaxIndices", 1<<16);
	DriverAttributes->setAttribute("MaxTextureSize", 1024);
	DriverAttributes->setAttribute("MaxLights", glsl::gl_MaxLights);
	DriverAttributes->setAttribute("MaxTextureLODBias", 16.f);
	DriverAttributes->setAttribute("Version", 47);

	// create triangle renderers

	irr::memset32 ( BurningShader, 0, sizeof ( BurningShader ) );
	//BurningShader[ETR_FLAT] = createTRFlat2(DepthBuffer);
	//BurningShader[ETR_FLAT_WIRE] = createTRFlatWire2(DepthBuffer);
	BurningShader[ETR_GOURAUD] = createTriangleRendererGouraud2(this);
	BurningShader[ETR_GOURAUD_ALPHA] = createTriangleRendererGouraudAlpha2(this );
	BurningShader[ETR_GOURAUD_ALPHA_NOZ] = createTRGouraudAlphaNoZ2(this );
	//BurningShader[ETR_GOURAUD_WIRE] = createTriangleRendererGouraudWire2(DepthBuffer);
	//BurningShader[ETR_TEXTURE_FLAT] = createTriangleRendererTextureFlat2(DepthBuffer);
	//BurningShader[ETR_TEXTURE_FLAT_WIRE] = createTriangleRendererTextureFlatWire2(DepthBuffer);
	BurningShader[ETR_TEXTURE_GOURAUD] = createTriangleRendererTextureGouraud2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_LIGHTMAP_M1] = createTriangleRendererTextureLightMap2_M1(this);
	BurningShader[ETR_TEXTURE_GOURAUD_LIGHTMAP_M2] = createTriangleRendererTextureLightMap2_M2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_LIGHTMAP_M4] = createTriangleRendererGTextureLightMap2_M4(this);
	BurningShader[ETR_TEXTURE_LIGHTMAP_M4] = createTriangleRendererTextureLightMap2_M4(this);
	BurningShader[ETR_TEXTURE_GOURAUD_LIGHTMAP_ADD] = createTriangleRendererTextureLightMap2_Add(this);
	BurningShader[ETR_TEXTURE_GOURAUD_DETAIL_MAP] = createTriangleRendererTextureDetailMap2(this);

	BurningShader[ETR_TEXTURE_GOURAUD_WIRE] = createTriangleRendererTextureGouraudWire2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_NOZ] = createTRTextureGouraudNoZ2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_ADD] = createTRTextureGouraudAdd2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_ADD_NO_Z] = createTRTextureGouraudAddNoZ2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_VERTEX_ALPHA] = createTriangleRendererTextureVertexAlpha2 ( this );

	BurningShader[ETR_TEXTURE_GOURAUD_ALPHA] = createTRTextureGouraudAlpha(this );
	BurningShader[ETR_TEXTURE_GOURAUD_ALPHA_NOZ] = createTRTextureGouraudAlphaNoZ( this );

	BurningShader[ETR_NORMAL_MAP_SOLID] = createTRNormalMap ( this );
	BurningShader[ETR_STENCIL_SHADOW] = createTRStencilShadow ( this );
	BurningShader[ETR_TEXTURE_BLEND] = createTRTextureBlend( this );

	BurningShader[ETR_REFERENCE] = createTriangleRendererReference ( this );


	// add the same renderer for all solid types
	CSoftware2MaterialRenderer_SOLID* smr = new CSoftware2MaterialRenderer_SOLID( this);
	CSoftware2MaterialRenderer_TRANSPARENT_ADD_COLOR* tmr = new CSoftware2MaterialRenderer_TRANSPARENT_ADD_COLOR( this);
	CSoftware2MaterialRenderer_UNSUPPORTED * umr = new CSoftware2MaterialRenderer_UNSUPPORTED ( this );

	//!TODO: addMaterialRenderer depends on pushing order....
	addMaterialRenderer ( smr ); // EMT_SOLID
	addMaterialRenderer ( smr ); // EMT_SOLID_2_LAYER,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP,
	addMaterialRenderer ( tmr ); // EMT_LIGHTMAP_ADD,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_M2,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_M4,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_LIGHTING,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_LIGHTING_M2,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_LIGHTING_M4,
	addMaterialRenderer ( smr ); // EMT_DETAIL_MAP,
	addMaterialRenderer ( umr ); // EMT_SPHERE_MAP,
	addMaterialRenderer ( smr ); // EMT_REFLECTION_2_LAYER,
	addMaterialRenderer ( tmr ); // EMT_TRANSPARENT_ADD_COLOR,
	addMaterialRenderer ( tmr ); // EMT_TRANSPARENT_ALPHA_CHANNEL,
	addMaterialRenderer ( tmr ); // EMT_TRANSPARENT_ALPHA_CHANNEL_REF,
	addMaterialRenderer ( tmr ); // EMT_TRANSPARENT_VERTEX_ALPHA,
	addMaterialRenderer ( smr ); // EMT_TRANSPARENT_REFLECTION_2_LAYER,
	addMaterialRenderer ( smr ); // EMT_NORMAL_MAP_SOLID,
	addMaterialRenderer ( umr ); // EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR,
	addMaterialRenderer ( tmr ); // EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA,
	addMaterialRenderer ( smr ); // EMT_PARALLAX_MAP_SOLID,
	addMaterialRenderer ( tmr ); // EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR,
	addMaterialRenderer ( tmr ); // EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA,
	addMaterialRenderer ( tmr ); // EMT_ONETEXTURE_BLEND

	smr->drop ();
	tmr->drop ();
	umr->drop ();

	// select render target
	setRenderTarget(BackBuffer);

	//reset Lightspace
	LightSpace.reset ();

	// select the right renderer
	setCurrentShader();
}


//! destructor
CBurningVideoDriver::~CBurningVideoDriver()
{
	// delete Backbuffer
	if (BackBuffer)
		BackBuffer->drop();

	// delete triangle renderers

	for (s32 i=0; i<ETR2_COUNT; ++i)
	{
		if (BurningShader[i])
			BurningShader[i]->drop();
	}

	// delete Additional buffer
	if (StencilBuffer)
		StencilBuffer->drop();

	if (DepthBuffer)
		DepthBuffer->drop();

	if (RenderTargetTexture)
		RenderTargetTexture->drop();

	if (RenderTargetSurface)
		RenderTargetSurface->drop();
}


/*!
	selects the right triangle renderer based on the render states.
*/
void CBurningVideoDriver::setCurrentShader()
{
	ITexture *texture0 = Material.org.getTexture(0);
	ITexture *texture1 = Material.org.getTexture(1);

	bool zMaterialTest =	Material.org.ZBuffer != ECFN_NEVER &&
							Material.org.ZWriteEnable &&
							( AllowZWriteOnTransparent || !Material.org.isTransparent() );

	EBurningFFShader shader = zMaterialTest ? ETR_TEXTURE_GOURAUD : ETR_TEXTURE_GOURAUD_NOZ;

	TransformationFlag[ ETS_TEXTURE_0] &= ~(ETF_TEXGEN_CAMERA_NORMAL|ETF_TEXGEN_CAMERA_REFLECTION);
	LightSpace.Flags &= ~VERTEXTRANSFORM;

	switch ( Material.org.MaterialType )
	{
		case EMT_ONETEXTURE_BLEND:
			shader = ETR_TEXTURE_BLEND;
			break;

		case EMT_TRANSPARENT_ALPHA_CHANNEL_REF:
			Material.org.MaterialTypeParam = 0.5f;
			// fall through
		case EMT_TRANSPARENT_ALPHA_CHANNEL:
			if ( texture0 && texture0->hasAlpha () )
			{
				shader = zMaterialTest ? ETR_TEXTURE_GOURAUD_ALPHA : ETR_TEXTURE_GOURAUD_ALPHA_NOZ;
				break;
			}
			// fall through

		case EMT_TRANSPARENT_ADD_COLOR:
			shader = zMaterialTest ? ETR_TEXTURE_GOURAUD_ADD : ETR_TEXTURE_GOURAUD_ADD_NO_Z;
			break;

		case EMT_TRANSPARENT_VERTEX_ALPHA:
			shader = ETR_TEXTURE_GOURAUD_VERTEX_ALPHA;
			break;

		case EMT_LIGHTMAP:
		case EMT_LIGHTMAP_LIGHTING:
			shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_M1;
			break;

		case EMT_LIGHTMAP_M2:
		case EMT_LIGHTMAP_LIGHTING_M2:
			shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_M2;
			break;

		case EMT_LIGHTMAP_LIGHTING_M4:
			if ( texture1 )
				shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_M4;
			break;
		case EMT_LIGHTMAP_M4:
			if ( texture1 )
				shader = ETR_TEXTURE_LIGHTMAP_M4;
			break;

		case EMT_LIGHTMAP_ADD:
			if ( texture1 )
				shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_ADD;
			break;

		case EMT_DETAIL_MAP:
			if ( texture1 )
				shader = ETR_TEXTURE_GOURAUD_DETAIL_MAP;
			break;

		case EMT_SPHERE_MAP:
			TransformationFlag[ ETS_TEXTURE_0] |= ETF_TEXGEN_CAMERA_REFLECTION; // ETF_TEXGEN_CAMERA_NORMAL;
			LightSpace.Flags |= VERTEXTRANSFORM;
			break;
		case EMT_REFLECTION_2_LAYER:
			shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_M1;
			TransformationFlag[ ETS_TEXTURE_1] |= ETF_TEXGEN_CAMERA_REFLECTION;
			LightSpace.Flags |= VERTEXTRANSFORM;
			break;

		case EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA:
		case EMT_NORMAL_MAP_SOLID:
		case EMT_PARALLAX_MAP_SOLID:
		case EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA:
			shader = ETR_NORMAL_MAP_SOLID;
			LightSpace.Flags |= VERTEXTRANSFORM;
			break;

		default:
			break;

	}

	if ( !texture0 )
	{
		shader = ETR_GOURAUD;
	}

	if ( Material.org.Wireframe )
	{
		shader = ETR_TEXTURE_GOURAUD_WIRE;
	}

	//shader = ETR_REFERENCE;

	// switchToTriangleRenderer
	CurrentShader = BurningShader[shader];
	if ( CurrentShader )
	{
		CurrentShader->setZCompareFunc ( Material.org.ZBuffer );
		CurrentShader->setRenderTarget(RenderTargetSurface, ViewPort);
		CurrentShader->setMaterial ( Material );

		switch ( shader )
		{
			case ETR_TEXTURE_GOURAUD_ALPHA:
			case ETR_TEXTURE_GOURAUD_ALPHA_NOZ:
			case ETR_TEXTURE_BLEND:
				CurrentShader->setParam ( 0, Material.org.MaterialTypeParam );
				break;
			default:
			break;
		}
	}

}



//! queries the features of the driver, returns true if feature is available
bool CBurningVideoDriver::queryFeature(E_VIDEO_DRIVER_FEATURE feature) const
{
	if (!FeatureEnabled[feature])
		return false;

	switch (feature)
	{
#ifdef SOFTWARE_DRIVER_2_BILINEAR
	case EVDF_BILINEAR_FILTER:
		return true;
#endif
#ifdef SOFTWARE_DRIVER_2_MIPMAPPING
	case EVDF_MIP_MAP:
		return true;
#endif
	case EVDF_STENCIL_BUFFER:
	case EVDF_RENDER_TO_TARGET:
	case EVDF_MULTITEXTURE:
	case EVDF_HARDWARE_TL:
	case EVDF_TEXTURE_NSQUARE:
		return true;

	default:
		return false;
	}
}



//! sets transformation
void CBurningVideoDriver::setTransform(E_TRANSFORMATION_STATE state, const core::matrix4& mat)
{
	Transformation[state] = mat;
	core::setbit_cond ( TransformationFlag[state], mat.isIdentity(), ETF_IDENTITY );

	switch ( state )
	{
		case ETS_VIEW:
			Transformation[ETS_VIEW_PROJECTION].setbyproduct_nocheck (
				Transformation[ETS_PROJECTION],
				Transformation[ETS_VIEW]
			);
			getCameraPosWorldSpace ();
			break;

		case ETS_WORLD:
			if ( TransformationFlag[state] & ETF_IDENTITY )
			{
				Transformation[ETS_WORLD_INVERSE] = Transformation[ETS_WORLD];
				TransformationFlag[ETS_WORLD_INVERSE] |= ETF_IDENTITY;
				Transformation[ETS_CURRENT] = Transformation[ETS_VIEW_PROJECTION];
			}
			else
			{
				//Transformation[ETS_WORLD].getInversePrimitive ( Transformation[ETS_WORLD_INVERSE] );
				Transformation[ETS_CURRENT].setbyproduct_nocheck (
					Transformation[ETS_VIEW_PROJECTION],
					Transformation[ETS_WORLD]
				);
			}
			TransformationFlag[ETS_CURRENT] = 0;
			//getLightPosObjectSpace ();
			break;
		case ETS_TEXTURE_0:
		case ETS_TEXTURE_1:
		case ETS_TEXTURE_2:
		case ETS_TEXTURE_3:
			if ( 0 == (TransformationFlag[state] & ETF_IDENTITY ) )
				LightSpace.Flags |= VERTEXTRANSFORM;
		default:
			break;
	}
}


//! clears the zbuffer
bool CBurningVideoDriver::beginScene(bool backBuffer, bool zBuffer,
		SColor color, const SExposedVideoData& videoData,
		core::rect<s32>* sourceRect)
{
	CNullDriver::beginScene(backBuffer, zBuffer, color, videoData, sourceRect);
	WindowId = videoData.D3D9.HWnd;
	SceneSourceRect = sourceRect;

	if (backBuffer && BackBuffer)
		BackBuffer->fill(color);

	if (zBuffer && DepthBuffer)
		DepthBuffer->clear();

	memset ( TransformationFlag, 0, sizeof ( TransformationFlag ) );
	return true;
}


//! presents the rendered scene on the screen, returns false if failed
bool CBurningVideoDriver::endScene()
{
	CNullDriver::endScene();

	return Presenter->present(BackBuffer, WindowId, SceneSourceRect);
}


//! sets a render target
bool CBurningVideoDriver::setRenderTarget(video::ITexture* texture, bool clearBackBuffer,
								 bool clearZBuffer, SColor color)
{
	if (texture && texture->getDriverType() != EDT_BURNINGSVIDEO)
	{
		os::Printer::log("Fatal Error: Tried to set a texture not owned by this driver.", ELL_ERROR);
		return false;
	}

	if (RenderTargetTexture)
		RenderTargetTexture->drop();

	RenderTargetTexture = texture;

	if (RenderTargetTexture)
	{
		RenderTargetTexture->grab();
		setRenderTarget(((CSoftwareTexture2*)RenderTargetTexture)->getTexture());
	}
	else
	{
		setRenderTarget(BackBuffer);
	}

	if (RenderTargetSurface && (clearBackBuffer || clearZBuffer))
	{
		if (clearZBuffer)
			DepthBuffer->clear();

		if (clearBackBuffer)
			RenderTargetSurface->fill( color );
	}

	return true;
}


//! sets a render target
void CBurningVideoDriver::setRenderTarget(video::CImage* image)
{
	if (RenderTargetSurface)
		RenderTargetSurface->drop();

	RenderTargetSurface = image;
	RenderTargetSize.Width = 0;
	RenderTargetSize.Height = 0;

	if (RenderTargetSurface)
	{
		RenderTargetSurface->grab();
		RenderTargetSize = RenderTargetSurface->getDimension();
	}

	setViewPort(core::rect<s32>(0,0,RenderTargetSize.Width,RenderTargetSize.Height));

	if (DepthBuffer)
		DepthBuffer->setSize(RenderTargetSize);

	if (StencilBuffer)
		StencilBuffer->setSize(RenderTargetSize);
}



//! sets a viewport
void CBurningVideoDriver::setViewPort(const core::rect<s32>& area)
{
	ViewPort = area;

	core::rect<s32> rendert(0,0,RenderTargetSize.Width,RenderTargetSize.Height);
	ViewPort.clipAgainst(rendert);

	Transformation [ ETS_CLIPSCALE ].buildNDCToDCMatrix ( ViewPort, 1 );

	if (CurrentShader)
		CurrentShader->setRenderTarget(RenderTargetSurface, ViewPort);
}

/*
	generic plane clipping in homogenous coordinates
	special case ndc frustum <-w,w>,<-w,w>,<-w,w>
	can be rewritten with compares e.q near plane, a.z < -a.w and b.z < -b.w
*/

const sVec4 CBurningVideoDriver::NDCPlane[6] =
{
	sVec4(  0.f,  0.f, -1.f, -1.f ),	// near
	sVec4(  0.f,  0.f,  1.f, -1.f ),	// far
	sVec4(  1.f,  0.f,  0.f, -1.f ),	// left
	sVec4( -1.f,  0.f,  0.f, -1.f ),	// right
	sVec4(  0.f,  1.f,  0.f, -1.f ),	// bottom
	sVec4(  0.f, -1.f,  0.f, -1.f )		// top
};



/*
	test a vertex if it's inside the standard frustum

	this is the generic one..

	f32 dotPlane;
	for ( u32 i = 0; i!= 6; ++i )
	{
		dotPlane = v->Pos.dotProduct ( NDCPlane[i] );
		core::setbit_cond( flag, dotPlane <= 0.f, 1 << i );
	}

	// this is the base for ndc frustum <-w,w>,<-w,w>,<-w,w>
	core::setbit_cond( flag, ( v->Pos.z - v->Pos.w ) <= 0.f, 1 );
	core::setbit_cond( flag, (-v->Pos.z - v->Pos.w ) <= 0.f, 2 );
	core::setbit_cond( flag, ( v->Pos.x - v->Pos.w ) <= 0.f, 4 );
	core::setbit_cond( flag, (-v->Pos.x - v->Pos.w ) <= 0.f, 8 );
	core::setbit_cond( flag, ( v->Pos.y - v->Pos.w ) <= 0.f, 16 );
	core::setbit_cond( flag, (-v->Pos.y - v->Pos.w ) <= 0.f, 32 );

*/
#ifdef IRRLICHT_FAST_MATH

REALINLINE u32 CBurningVideoDriver::clipToFrustumTest ( const s4DVertex * v  ) const
{
	f32 test[6];
	u32 flag;
	const f32 w = - v->Pos.w;

	// a conditional move is needed....FCOMI ( but we don't have it )
	// so let the fpu calculate and write it back.
	// cpu makes the compare, interleaving

	test[0] =  v->Pos.z + w;
	test[1] = -v->Pos.z + w;
	test[2] =  v->Pos.x + w;
	test[3] = -v->Pos.x + w;
	test[4] =  v->Pos.y + w;
	test[5] = -v->Pos.y + w;

	flag  = (IR ( test[0] )              ) >> 31;
	flag |= (IR ( test[1] ) & 0x80000000 ) >> 30;
	flag |= (IR ( test[2] ) & 0x80000000 ) >> 29;
	flag |= (IR ( test[3] ) & 0x80000000 ) >> 28;
	flag |= (IR ( test[4] ) & 0x80000000 ) >> 27;
	flag |= (IR ( test[5] ) & 0x80000000 ) >> 26;

/*
	flag  = F32_LOWER_EQUAL_0 ( test[0] );
	flag |= F32_LOWER_EQUAL_0 ( test[1] ) << 1;
	flag |= F32_LOWER_EQUAL_0 ( test[2] ) << 2;
	flag |= F32_LOWER_EQUAL_0 ( test[3] ) << 3;
	flag |= F32_LOWER_EQUAL_0 ( test[4] ) << 4;
	flag |= F32_LOWER_EQUAL_0 ( test[5] ) << 5;
*/
	return flag;
}

#else


REALINLINE u32 CBurningVideoDriver::clipToFrustumTest ( const s4DVertex * v  ) const
{
	u32 flag = 0;

	if ( v->Pos.z <= v->Pos.w ) flag |= 1;
	if (-v->Pos.z <= v->Pos.w ) flag |= 2;

	if ( v->Pos.x <= v->Pos.w ) flag |= 4;
	if (-v->Pos.x <= v->Pos.w ) flag |= 8;

	if ( v->Pos.y <= v->Pos.w ) flag |= 16;
	if (-v->Pos.y <= v->Pos.w ) flag |= 32;

/*
	for ( u32 i = 0; i!= 6; ++i )
	{
		core::setbit_cond( flag, v->Pos.dotProduct ( NDCPlane[i] ) <= 0.f, 1 << i );
	}
*/
	return flag;
}

#endif // _MSC_VER

u32 CBurningVideoDriver::clipToHyperPlane ( s4DVertex * dest, const s4DVertex * source, u32 inCount, const sVec4 &plane )
{
	u32 outCount = 0;
	s4DVertex * out = dest;

	const s4DVertex * a;
	const s4DVertex * b = source;

	f32 bDotPlane;

	bDotPlane = b->Pos.dotProduct ( plane );

	for( u32 i = 1; i < inCount + 1; ++i)
	{
		const s32 condition = i - inCount;
		const s32 index = (( ( condition >> 31 ) & ( i ^ condition ) ) ^ condition ) << 1;

		a = &source[ index ];

		// current point inside
		if ( a->Pos.dotProduct ( plane ) <= 0.f )
		{
			// last point outside
			if ( F32_GREATER_0 ( bDotPlane ) )
			{
				// intersect line segment with plane
				out->interpolate ( *b, *a, bDotPlane / (b->Pos - a->Pos).dotProduct ( plane ) );
				out += 2;
				outCount += 1;
			}

			// copy current to out
			//*out = *a;
			irr::memcpy32_small ( out, a, SIZEOF_SVERTEX * 2 );
			b = out;

			out += 2;
			outCount += 1;
		}
		else
		{
			// current point outside

			if ( F32_LOWER_EQUAL_0 (  bDotPlane ) )
			{
				// previous was inside
				// intersect line segment with plane
				out->interpolate ( *b, *a, bDotPlane / (b->Pos - a->Pos).dotProduct ( plane ) );
				out += 2;
				outCount += 1;
			}
			// pointer
			b = a;
		}

		bDotPlane = b->Pos.dotProduct ( plane );

	}

	return outCount;
}


u32 CBurningVideoDriver::clipToFrustum ( s4DVertex *v0, s4DVertex * v1, const u32 vIn )
{
	u32 vOut = vIn;

	vOut = clipToHyperPlane ( v1, v0, vOut, NDCPlane[0] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v0, v1, vOut, NDCPlane[1] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v1, v0, vOut, NDCPlane[2] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v0, v1, vOut, NDCPlane[3] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v1, v0, vOut, NDCPlane[4] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v0, v1, vOut, NDCPlane[5] );
	return vOut;
}

/*!
 Part I:
	apply Clip Scale matrix
	From Normalized Device Coordiante ( NDC ) Space to Device Coordinate Space ( DC )

 Part II:
	Project homogeneous vector
	homogeneous to non-homogenous coordinates ( dividebyW )

	Incoming: ( xw, yw, zw, w, u, v, 1, R, G, B, A )
	Outgoing: ( xw/w, yw/w, zw/w, w/w, u/w, v/w, 1/w, R/w, G/w, B/w, A/w )


	replace w/w by 1/w
*/
inline void CBurningVideoDriver::ndc_2_dc_and_project ( s4DVertex *dest,s4DVertex *source, u32 vIn ) const
{
	u32 g;

	for ( g = 0; g != vIn; g += 2 )
	{
		if ( (dest[g].flag & VERTEX4D_PROJECTED ) == VERTEX4D_PROJECTED )
			continue;

		dest[g].flag = source[g].flag | VERTEX4D_PROJECTED;

		const f32 w = source[g].Pos.w;
		const f32 iw = core::reciprocal ( w );

		// to device coordinates
		dest[g].Pos.x = iw * ( source[g].Pos.x * Transformation [ ETS_CLIPSCALE ][ 0] + w * Transformation [ ETS_CLIPSCALE ][12] );
		dest[g].Pos.y = iw * ( source[g].Pos.y * Transformation [ ETS_CLIPSCALE ][ 5] + w * Transformation [ ETS_CLIPSCALE ][13] );

#ifndef SOFTWARE_DRIVER_2_USE_WBUFFER
		dest[g].Pos.z = iw * source[g].Pos.z;
#endif

	#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
		#ifdef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
			dest[g].Color[0] = source[g].Color[0] * iw;
		#else
			dest[g].Color[0] = source[g].Color[0];
		#endif

	#endif
		dest[g].LightTangent[0] = source[g].LightTangent[0] * iw;
		dest[g].Pos.w = iw;
	}
}


inline void CBurningVideoDriver::ndc_2_dc_and_project2 ( const s4DVertex **v, const u32 size ) const
{
	u32 g;

	for ( g = 0; g != size; g += 1 )
	{
		s4DVertex * a = (s4DVertex*) v[g];

		if ( (a[1].flag & VERTEX4D_PROJECTED ) == VERTEX4D_PROJECTED )
			continue;

		a[1].flag = a->flag | VERTEX4D_PROJECTED;

		// project homogenous vertex, store 1/w
		const f32 w = a->Pos.w;
		const f32 iw = core::reciprocal ( w );

		// to device coordinates
		const f32 * p = Transformation [ ETS_CLIPSCALE ].pointer();
		a[1].Pos.x = iw * ( a->Pos.x * p[ 0] + w * p[12] );
		a[1].Pos.y = iw * ( a->Pos.y * p[ 5] + w * p[13] );

#ifndef SOFTWARE_DRIVER_2_USE_WBUFFER
		a[1].Pos.z = a->Pos.z * iw;
#endif

	#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
		#ifdef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
			a[1].Color[0] = a->Color[0] * iw;
		#else
			a[1].Color[0] = a->Color[0];
		#endif
	#endif

		a[1].LightTangent[0] = a[0].LightTangent[0] * iw;
		a[1].Pos.w = iw;

	}

}


/*!
	crossproduct in projected 2D -> screen area triangle
*/
inline f32 CBurningVideoDriver::screenarea ( const s4DVertex *v ) const
{
	return	( ( v[3].Pos.x - v[1].Pos.x ) * ( v[5].Pos.y - v[1].Pos.y ) ) -
			( ( v[3].Pos.y - v[1].Pos.y ) * ( v[5].Pos.x - v[1].Pos.x ) );
}


/*!
*/
inline f32 CBurningVideoDriver::texelarea ( const s4DVertex *v, int tex ) const
{
	f32 z;

	z =		( (v[2].Tex[tex].x - v[0].Tex[tex].x ) * (v[4].Tex[tex].y - v[0].Tex[tex].y ) )
		 -	( (v[4].Tex[tex].x - v[0].Tex[tex].x ) * (v[2].Tex[tex].y - v[0].Tex[tex].y ) );

	return MAT_TEXTURE ( tex )->getLODFactor ( z );
}

/*!
	crossproduct in projected 2D
*/
inline f32 CBurningVideoDriver::screenarea2 ( const s4DVertex **v ) const
{
	return	( (( v[1] + 1 )->Pos.x - (v[0] + 1 )->Pos.x ) * ( (v[2] + 1 )->Pos.y - (v[0] + 1 )->Pos.y ) ) -
			( (( v[1] + 1 )->Pos.y - (v[0] + 1 )->Pos.y ) * ( (v[2] + 1 )->Pos.x - (v[0] + 1 )->Pos.x ) );
}

/*!
*/
inline f32 CBurningVideoDriver::texelarea2 ( const s4DVertex **v, s32 tex ) const
{
	f32 z;
	z =		( (v[1]->Tex[tex].x - v[0]->Tex[tex].x ) * (v[2]->Tex[tex].y - v[0]->Tex[tex].y ) )
		 -	( (v[2]->Tex[tex].x - v[0]->Tex[tex].x ) * (v[1]->Tex[tex].y - v[0]->Tex[tex].y ) );

	return MAT_TEXTURE ( tex )->getLODFactor ( z );
}


/*!
*/
inline void CBurningVideoDriver::select_polygon_mipmap ( s4DVertex *v, u32 vIn, u32 tex, const core::dimension2du& texSize ) const
{
	f32 f[2];

	f[0] = (f32) texSize.Width - 0.25f;
	f[1] = (f32) texSize.Height - 0.25f;

#ifdef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
	for ( u32 g = 0; g != vIn; g += 2 )
	{
		(v + g + 1 )->Tex[tex].x	= (v + g + 0)->Tex[tex].x * ( v + g + 1 )->Pos.w * f[0];
		(v + g + 1 )->Tex[tex].y	= (v + g + 0)->Tex[tex].y * ( v + g + 1 )->Pos.w * f[1];
	}
#else
	for ( u32 g = 0; g != vIn; g += 2 )
	{
		(v + g + 1 )->Tex[tex].x	= (v + g + 0)->Tex[tex].x * f[0];
		(v + g + 1 )->Tex[tex].y	= (v + g + 0)->Tex[tex].y * f[1];
	}
#endif
}

inline void CBurningVideoDriver::select_polygon_mipmap2 ( s4DVertex **v, u32 tex, const core::dimension2du& texSize ) const
{
	f32 f[2];

	f[0] = (f32) texSize.Width - 0.25f;
	f[1] = (f32) texSize.Height - 0.25f;

#ifdef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
	(v[0] + 1 )->Tex[tex].x	= v[0]->Tex[tex].x * ( v[0] + 1 )->Pos.w * f[0];
	(v[0] + 1 )->Tex[tex].y	= v[0]->Tex[tex].y * ( v[0] + 1 )->Pos.w * f[1];

	(v[1] + 1 )->Tex[tex].x	= v[1]->Tex[tex].x * ( v[1] + 1 )->Pos.w * f[0];
	(v[1] + 1 )->Tex[tex].y	= v[1]->Tex[tex].y * ( v[1] + 1 )->Pos.w * f[1];

	(v[2] + 1 )->Tex[tex].x	= v[2]->Tex[tex].x * ( v[2] + 1 )->Pos.w * f[0];
	(v[2] + 1 )->Tex[tex].y	= v[2]->Tex[tex].y * ( v[2] + 1 )->Pos.w * f[1];

#else
	(v[0] + 1 )->Tex[tex].x	= v[0]->Tex[tex].x * f[0];
	(v[0] + 1 )->Tex[tex].y	= v[0]->Tex[tex].y * f[1];

	(v[1] + 1 )->Tex[tex].x	= v[1]->Tex[tex].x * f[0];
	(v[1] + 1 )->Tex[tex].y	= v[1]->Tex[tex].y * f[1];

	(v[2] + 1 )->Tex[tex].x	= v[2]->Tex[tex].x * f[0];
	(v[2] + 1 )->Tex[tex].y	= v[2]->Tex[tex].y * f[1];
#endif
}

// Vertex Cache
const SVSize CBurningVideoDriver::vSize[] =
{
	{ VERTEX4D_FORMAT_TEXTURE_1 | VERTEX4D_FORMAT_COLOR_1, sizeof(S3DVertex), 1 },
	{ VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1, sizeof(S3DVertex2TCoords),2 },
	{ VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1 | VERTEX4D_FORMAT_BUMP_DOT3, sizeof(S3DVertexTangents),2 },
	{ VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1, sizeof(S3DVertex), 2 },	// reflection map
	{ 0, sizeof(f32) * 3, 0 },	// core::vector3df*
};



/*!
	fill a cache line with transformed, light and clipp test triangles
*/
void CBurningVideoDriver::VertexCache_fill(const u32 sourceIndex, const u32 destIndex)
{
	u8 * source;
	s4DVertex *dest;

	source = (u8*) VertexCache.vertices + ( sourceIndex * vSize[VertexCache.vType].Pitch );

	// it's a look ahead so we never hit it..
	// but give priority...
	//VertexCache.info[ destIndex ].hit = hitCount;

	// store info
	VertexCache.info[ destIndex ].index = sourceIndex;
	VertexCache.info[ destIndex ].hit = 0;

	// destination Vertex
	dest = (s4DVertex *) ( (u8*) VertexCache.mem.data + ( destIndex << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );

	// transform Model * World * Camera * Projection * NDCSpace matrix
	const S3DVertex *base = ((S3DVertex*) source );
	Transformation [ ETS_CURRENT].transformVect ( &dest->Pos.x, base->Pos );

	//mhm ;-) maybe no goto
	if ( VertexCache.vType == 4 ) goto clipandproject;


#if defined (SOFTWARE_DRIVER_2_LIGHTING) || defined ( SOFTWARE_DRIVER_2_TEXTURE_TRANSFORM )

	// vertex normal in light space
	if ( Material.org.Lighting || (LightSpace.Flags & VERTEXTRANSFORM) )
	{
		if ( TransformationFlag[ETS_WORLD] & ETF_IDENTITY )
		{
			LightSpace.normal.set ( base->Normal.X, base->Normal.Y, base->Normal.Z, 1.f );
			LightSpace.vertex.set ( base->Pos.X, base->Pos.Y, base->Pos.Z, 1.f );
		}
		else
		{
			Transformation[ETS_WORLD].rotateVect ( &LightSpace.normal.x, base->Normal );

			// vertex in light space
			if ( LightSpace.Flags & ( POINTLIGHT | FOG | SPECULAR | VERTEXTRANSFORM) )
				Transformation[ETS_WORLD].transformVect ( &LightSpace.vertex.x, base->Pos );
		}

		if ( LightSpace.Flags & NORMALIZE )
			LightSpace.normal.normalize_xyz();

	}

#endif

#if defined ( SOFTWARE_DRIVER_2_USE_VERTEX_COLOR )
	// apply lighting model
	#if defined (SOFTWARE_DRIVER_2_LIGHTING)
		if ( Material.org.Lighting )
		{
			lightVertex ( dest, base->Color.color );
		}
		else
		{
			dest->Color[0].setA8R8G8B8 ( base->Color.color );
		}
	#else
		dest->Color[0].setA8R8G8B8 ( base->Color.color );
	#endif
#endif

	// Texture Transform
#if !defined ( SOFTWARE_DRIVER_2_TEXTURE_TRANSFORM )
	irr::memcpy32_small ( &dest->Tex[0],&base->TCoords,
					vSize[VertexCache.vType].TexSize << 3 //  * ( sizeof ( f32 ) * 2 )
				);
#else

	if ( 0 == (LightSpace.Flags & VERTEXTRANSFORM) )
	{
		irr::memcpy32_small ( &dest->Tex[0],&base->TCoords,
						vSize[VertexCache.vType].TexSize << 3 //  * ( sizeof ( f32 ) * 2 )
					);
	}
	else
	{
	/*
			Generate texture coordinates as linear functions so that:
				u = Ux*x + Uy*y + Uz*z + Uw
				v = Vx*x + Vy*y + Vz*z + Vw
			The matrix M for this case is:
				Ux  Vx  0  0
				Uy  Vy  0  0
				Uz  Vz  0  0
				Uw  Vw  0  0
	*/

		u32 t;
		sVec4 n;
		sVec2 srcT;

		for ( t = 0; t != vSize[VertexCache.vType].TexSize; ++t )
		{
			const core::matrix4& M = Transformation [ ETS_TEXTURE_0 + t ];

			// texgen
			if ( TransformationFlag [ ETS_TEXTURE_0 + t ] & (ETF_TEXGEN_CAMERA_NORMAL|ETF_TEXGEN_CAMERA_REFLECTION) )
			{
				n.x = LightSpace.campos.x - LightSpace.vertex.x;
				n.y = LightSpace.campos.x - LightSpace.vertex.y;
				n.z = LightSpace.campos.x - LightSpace.vertex.z;
				n.normalize_xyz();
				n.x += LightSpace.normal.x;
				n.y += LightSpace.normal.y;
				n.z += LightSpace.normal.z;
				n.normalize_xyz();

				const f32 *view = Transformation[ETS_VIEW].pointer();

				if ( TransformationFlag [ ETS_TEXTURE_0 + t ] & ETF_TEXGEN_CAMERA_REFLECTION )
				{
					srcT.x = 0.5f * ( 1.f + (n.x * view[0] + n.y * view[4] + n.z * view[8] ));
					srcT.y = 0.5f * ( 1.f + (n.x * view[1] + n.y * view[5] + n.z * view[9] ));
				}
				else
				{
					srcT.x = 0.5f * ( 1.f + (n.x * view[0] + n.y * view[1] + n.z * view[2] ));
					srcT.y = 0.5f * ( 1.f + (n.x * view[4] + n.y * view[5] + n.z * view[6] ));
				}
			}
			else
			{
				irr::memcpy32_small ( &srcT,(&base->TCoords) + t,
					sizeof ( f32 ) * 2 );
			}

			switch ( Material.org.TextureLayer[t].TextureWrapU )
			{
				case ETC_CLAMP:
				case ETC_CLAMP_TO_EDGE:
				case ETC_CLAMP_TO_BORDER:
					dest->Tex[t].x = core::clamp ( (f32) ( M[0] * srcT.x + M[4] * srcT.y + M[8] ), 0.f, 1.f );
					break;
				case ETC_MIRROR:
					dest->Tex[t].x = M[0] * srcT.x + M[4] * srcT.y + M[8];
					if (core::fract(dest->Tex[t].x)>0.5f)
						dest->Tex[t].x=1.f-dest->Tex[t].x;
				break;
				case ETC_MIRROR_CLAMP:
				case ETC_MIRROR_CLAMP_TO_EDGE:
				case ETC_MIRROR_CLAMP_TO_BORDER:
					dest->Tex[t].x = core::clamp ( (f32) ( M[0] * srcT.x + M[4] * srcT.y + M[8] ), 0.f, 1.f );
					if (core::fract(dest->Tex[t].x)>0.5f)
						dest->Tex[t].x=1.f-dest->Tex[t].x;
				break;
				case ETC_REPEAT:
				default:
					dest->Tex[t].x = M[0] * srcT.x + M[4] * srcT.y + M[8];
					break;
			}
			switch ( Material.org.TextureLayer[t].TextureWrapV )
			{
				case ETC_CLAMP:
				case ETC_CLAMP_TO_EDGE:
				case ETC_CLAMP_TO_BORDER:
					dest->Tex[t].y = core::clamp ( (f32) ( M[1] * srcT.x + M[5] * srcT.y + M[9] ), 0.f, 1.f );
					break;
				case ETC_MIRROR:
					dest->Tex[t].y = M[1] * srcT.x + M[5] * srcT.y + M[9];
					if (core::fract(dest->Tex[t].y)>0.5f)
						dest->Tex[t].y=1.f-dest->Tex[t].y;
				break;
				case ETC_MIRROR_CLAMP:
				case ETC_MIRROR_CLAMP_TO_EDGE:
				case ETC_MIRROR_CLAMP_TO_BORDER:
					dest->Tex[t].y = core::clamp ( (f32) ( M[1] * srcT.x + M[5] * srcT.y + M[9] ), 0.f, 1.f );
					if (core::fract(dest->Tex[t].y)>0.5f)
						dest->Tex[t].y=1.f-dest->Tex[t].y;
				break;
				case ETC_REPEAT:
				default:
					dest->Tex[t].y = M[1] * srcT.x + M[5] * srcT.y + M[9];
					break;
			}
		}
	}

#if 0
	// tangent space light vector, emboss
	if ( Lights.size () && ( vSize[VertexCache.vType].Format & VERTEX4D_FORMAT_BUMP_DOT3 ) )
	{
		const S3DVertexTangents *tangent = ((S3DVertexTangents*) source );
		const SBurningShaderLight &light = LightSpace.Light[0];

		sVec4 vp;

		vp.x = light.pos.x - LightSpace.vertex.x;
		vp.y = light.pos.y - LightSpace.vertex.y;
		vp.z = light.pos.z - LightSpace.vertex.z;

		vp.normalize_xyz();

		LightSpace.tangent.x = vp.x * tangent->Tangent.X + vp.y * tangent->Tangent.Y + vp.z * tangent->Tangent.Z;
		LightSpace.tangent.y = vp.x * tangent->Binormal.X + vp.y * tangent->Binormal.Y + vp.z * tangent->Binormal.Z;
		//LightSpace.tangent.z = vp.x * tangent->Normal.X + vp.y * tangent->Normal.Y + vp.z * tangent->Normal.Z;
		LightSpace.tangent.z = 0.f;
		LightSpace.tangent.normalize_xyz();

		f32 scale = 1.f / 128.f;
		if ( Material.org.MaterialTypeParam > 0.f )
			scale = Material.org.MaterialTypeParam;

		// emboss, shift coordinates
		dest->Tex[1].x = dest->Tex[0].x + LightSpace.tangent.x * scale;
		dest->Tex[1].y = dest->Tex[0].y + LightSpace.tangent.y * scale;
		//dest->Tex[1].z = LightSpace.tangent.z * scale;
	}
#endif

	if ( LightSpace.Light.size () && ( vSize[VertexCache.vType].Format & VERTEX4D_FORMAT_BUMP_DOT3 ) )
	{
		const S3DVertexTangents *tangent = ((S3DVertexTangents*) source );

		sVec4 vp;

		dest->LightTangent[0].x = 0.f;
		dest->LightTangent[0].y = 0.f;
		dest->LightTangent[0].z = 0.f;
		for ( u32 i = 0; i < 2 && i < LightSpace.Light.size (); ++i )
		{
			const SBurningShaderLight &light = LightSpace.Light[i];

			if ( !light.LightIsOn )
				continue;

			vp.x = light.pos.x - LightSpace.vertex.x;
			vp.y = light.pos.y - LightSpace.vertex.y;
			vp.z = light.pos.z - LightSpace.vertex.z;

	/*
			vp.x = light.pos_objectspace.x - base->Pos.X;
			vp.y = light.pos_objectspace.y - base->Pos.Y;
			vp.z = light.pos_objectspace.z - base->Pos.Z;
	*/

			vp.normalize_xyz();


			// transform by tangent matrix
			sVec3 l;
	#if 1
			l.x = (vp.x * tangent->Tangent.X + vp.y * tangent->Tangent.Y + vp.z * tangent->Tangent.Z );
			l.y = (vp.x * tangent->Binormal.X + vp.y * tangent->Binormal.Y + vp.z * tangent->Binormal.Z );
			l.z = (vp.x * tangent->Normal.X + vp.y * tangent->Normal.Y + vp.z * tangent->Normal.Z );
	#else
			l.x = (vp.x * tangent->Tangent.X + vp.y * tangent->Binormal.X + vp.z * tangent->Normal.X );
			l.y = (vp.x * tangent->Tangent.Y + vp.y * tangent->Binormal.Y + vp.z * tangent->Normal.Y );
			l.z = (vp.x * tangent->Tangent.Z + vp.y * tangent->Binormal.Z + vp.z * tangent->Normal.Z );
	#endif


	/*
			f32 scale = 1.f / 128.f;
			scale /= dest->LightTangent[0].b;

			// emboss, shift coordinates
			dest->Tex[1].x = dest->Tex[0].x + l.r * scale;
			dest->Tex[1].y = dest->Tex[0].y + l.g * scale;
	*/
			dest->Tex[1].x = dest->Tex[0].x;
			dest->Tex[1].y = dest->Tex[0].y;

			// scale bias
			dest->LightTangent[0].x += l.x;
			dest->LightTangent[0].y += l.y;
			dest->LightTangent[0].z += l.z;
		}
		dest->LightTangent[0].setLength ( 0.5f );
		dest->LightTangent[0].x += 0.5f;
		dest->LightTangent[0].y += 0.5f;
		dest->LightTangent[0].z += 0.5f;
	}


#endif

clipandproject:
	dest[0].flag = dest[1].flag = vSize[VertexCache.vType].Format;

	// test vertex
	dest[0].flag |= clipToFrustumTest ( dest);

	// to DC Space, project homogenous vertex
	if ( (dest[0].flag & VERTEX4D_CLIPMASK ) == VERTEX4D_INSIDE )
	{
		ndc_2_dc_and_project2 ( (const s4DVertex**) &dest, 1 );
	}

	//return dest;
}

//

REALINLINE s4DVertex * CBurningVideoDriver::VertexCache_getVertex ( const u32 sourceIndex )
{
	for ( s32 i = 0; i < VERTEXCACHE_ELEMENT; ++i )
	{
		if ( VertexCache.info[ i ].index == sourceIndex )
		{
			return (s4DVertex *) ( (u8*) VertexCache.mem.data + ( i << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );
		}
	}
	return 0;
}


/*
	Cache based on linear walk indices
	fill blockwise on the next 16(Cache_Size) unique vertices in indexlist
	merge the next 16 vertices with the current
*/
REALINLINE void CBurningVideoDriver::VertexCache_get(const s4DVertex ** face)
{
	SCacheInfo info[VERTEXCACHE_ELEMENT];

	// next primitive must be complete in cache
	if (	VertexCache.indicesIndex - VertexCache.indicesRun < 3 &&
			VertexCache.indicesIndex < VertexCache.indexCount
		)
	{
		// rewind to start of primitive
		VertexCache.indicesIndex = VertexCache.indicesRun;

		irr::memset32 ( info, VERTEXCACHE_MISS, sizeof ( info ) );

		// get the next unique vertices cache line
		u32 fillIndex = 0;
		u32 dIndex;
		u32 i;
		u32 sourceIndex;

		while ( VertexCache.indicesIndex < VertexCache.indexCount &&
				fillIndex < VERTEXCACHE_ELEMENT
				)
		{
			switch ( VertexCache.iType )
			{
				case 1:
					sourceIndex =  ((u16*)VertexCache.indices) [ VertexCache.indicesIndex ];
					break;
				case 2:
					sourceIndex =  ((u32*)VertexCache.indices) [ VertexCache.indicesIndex ];
					break;
				case 4:
					sourceIndex = VertexCache.indicesIndex;
					break;
			}

			VertexCache.indicesIndex += 1;

			// if not exist, push back
			s32 exist = 0;
			for ( dIndex = 0;  dIndex < fillIndex; ++dIndex )
			{
				if ( info[ dIndex ].index == sourceIndex )
				{
					exist = 1;
					break;
				}
			}

			if ( 0 == exist )
			{
				info[fillIndex++].index = sourceIndex;
			}
		}

		// clear marks
		for ( i = 0; i!= VERTEXCACHE_ELEMENT; ++i )
		{
			VertexCache.info[i].hit = 0;
		}

		// mark all existing
		for ( i = 0; i!= fillIndex; ++i )
		{
			for ( dIndex = 0;  dIndex < VERTEXCACHE_ELEMENT; ++dIndex )
			{
				if ( VertexCache.info[ dIndex ].index == info[i].index )
				{
					info[i].hit = dIndex;
					VertexCache.info[ dIndex ].hit = 1;
					break;
				}
			}
		}

		// fill new
		for ( i = 0; i!= fillIndex; ++i )
		{
			if ( info[i].hit != VERTEXCACHE_MISS )
				continue;

			for ( dIndex = 0;  dIndex < VERTEXCACHE_ELEMENT; ++dIndex )
			{
				if ( 0 == VertexCache.info[dIndex].hit )
				{
					VertexCache_fill ( info[i].index, dIndex );
					VertexCache.info[dIndex].hit += 1;
					info[i].hit = dIndex;
					break;
				}
			}
		}
	}

	const u32 i0 = core::if_c_a_else_0 ( VertexCache.pType != scene::EPT_TRIANGLE_FAN, VertexCache.indicesRun );

	switch ( VertexCache.iType )
	{
		case 1:
		{
			const u16 *p = (const u16 *) VertexCache.indices;
			face[0] = VertexCache_getVertex ( p[ i0    ] );
			face[1] = VertexCache_getVertex ( p[ VertexCache.indicesRun + 1] );
			face[2] = VertexCache_getVertex ( p[ VertexCache.indicesRun + 2] );
		}
		break;

		case 2:
		{
			const u32 *p = (const u32 *) VertexCache.indices;
			face[0] = VertexCache_getVertex ( p[ i0    ] );
			face[1] = VertexCache_getVertex ( p[ VertexCache.indicesRun + 1] );
			face[2] = VertexCache_getVertex ( p[ VertexCache.indicesRun + 2] );
		}
		break;

		case 4:
			face[0] = VertexCache_getVertex ( VertexCache.indicesRun + 0 );
			face[1] = VertexCache_getVertex ( VertexCache.indicesRun + 1 );
			face[2] = VertexCache_getVertex ( VertexCache.indicesRun + 2 );
		break;
		default:
			face[0] = face[1] = face[2] = VertexCache_getVertex(VertexCache.indicesRun + 0);
		break;
	}

	VertexCache.indicesRun += VertexCache.primitivePitch;
}

/*!
*/
REALINLINE void CBurningVideoDriver::VertexCache_getbypass ( s4DVertex ** face )
{
	const u32 i0 = core::if_c_a_else_0 ( VertexCache.pType != scene::EPT_TRIANGLE_FAN, VertexCache.indicesRun );

	if ( VertexCache.iType == 1 )
	{
		const u16 *p = (const u16 *) VertexCache.indices;
		VertexCache_fill ( p[ i0    ], 0 );
		VertexCache_fill ( p[ VertexCache.indicesRun + 1], 1 );
		VertexCache_fill ( p[ VertexCache.indicesRun + 2], 2 );
	}
	else
	{
		const u32 *p = (const u32 *) VertexCache.indices;
		VertexCache_fill ( p[ i0    ], 0 );
		VertexCache_fill ( p[ VertexCache.indicesRun + 1], 1 );
		VertexCache_fill ( p[ VertexCache.indicesRun + 2], 2 );
	}

	VertexCache.indicesRun += VertexCache.primitivePitch;

	face[0] = (s4DVertex *) ( (u8*) VertexCache.mem.data + ( 0 << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );
	face[1] = (s4DVertex *) ( (u8*) VertexCache.mem.data + ( 1 << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );
	face[2] = (s4DVertex *) ( (u8*) VertexCache.mem.data + ( 2 << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );

}

/*!
*/
void CBurningVideoDriver::VertexCache_reset ( const void* vertices, u32 vertexCount,
											const void* indices, u32 primitiveCount,
											E_VERTEX_TYPE vType,
											scene::E_PRIMITIVE_TYPE pType,
											E_INDEX_TYPE iType)
{
	VertexCache.vertices = vertices;
	VertexCache.vertexCount = vertexCount;

	VertexCache.indices = indices;
	VertexCache.indicesIndex = 0;
	VertexCache.indicesRun = 0;

	if ( Material.org.MaterialType == video::EMT_REFLECTION_2_LAYER )
		VertexCache.vType = 3;
	else
		VertexCache.vType = vType;
	VertexCache.pType = pType;

	switch ( iType )
	{
		case EIT_16BIT: VertexCache.iType = 1; break;
		case EIT_32BIT: VertexCache.iType = 2; break;
		default:
			VertexCache.iType = iType; break;
	}

	switch ( VertexCache.pType )
	{
		// most types here will not work as expected, only triangles/triangle_fan
		// is known to work.
		case scene::EPT_POINTS:
			VertexCache.indexCount = primitiveCount;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_LINE_STRIP:
			VertexCache.indexCount = primitiveCount+1;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_LINE_LOOP:
			VertexCache.indexCount = primitiveCount+1;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_LINES:
			VertexCache.indexCount = 2*primitiveCount;
			VertexCache.primitivePitch = 2;
			break;
		case scene::EPT_TRIANGLE_STRIP:
			VertexCache.indexCount = primitiveCount+2;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_TRIANGLES:
			VertexCache.indexCount = primitiveCount + primitiveCount + primitiveCount;
			VertexCache.primitivePitch = 3;
			break;
		case scene::EPT_TRIANGLE_FAN:
			VertexCache.indexCount = primitiveCount + 2;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_QUAD_STRIP:
			VertexCache.indexCount = 2*primitiveCount + 2;
			VertexCache.primitivePitch = 2;
			break;
		case scene::EPT_QUADS:
			VertexCache.indexCount = 4*primitiveCount;
			VertexCache.primitivePitch = 4;
			break;
		case scene::EPT_POLYGON:
			VertexCache.indexCount = primitiveCount+1;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_POINT_SPRITES:
			VertexCache.indexCount = primitiveCount;
			VertexCache.primitivePitch = 1;
			break;
	}

	irr::memset32 ( VertexCache.info, VERTEXCACHE_MISS, sizeof ( VertexCache.info ) );
}


void CBurningVideoDriver::drawVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType)

{
	if (!checkPrimitiveCount(primitiveCount))
		return;

	CNullDriver::drawVertexPrimitiveList(vertices, vertexCount, indexList, primitiveCount, vType, pType, iType);

	// These calls would lead to crashes due to wrong index usage.
	// The vertex cache needs to be rewritten for these primitives.
	if (pType==scene::EPT_POINTS || pType==scene::EPT_LINE_STRIP ||
		pType==scene::EPT_LINE_LOOP || pType==scene::EPT_LINES || pType==scene::EPT_POLYGON ||
		pType==scene::EPT_POINT_SPRITES)
		return;

	if ( 0 == CurrentShader )
		return;

	VertexCache_reset ( vertices, vertexCount, indexList, primitiveCount, vType, pType, iType );

	const s4DVertex * face[3];

	f32 dc_area;
	s32 lodLevel;
	u32 i;
	u32 g;
	u32 m;
	video::CSoftwareTexture2* tex;

	for ( i = 0; i < (u32) primitiveCount; ++i )
	{
		VertexCache_get(face);

		// if fully outside or outside on same side
		if ( ( (face[0]->flag | face[1]->flag | face[2]->flag) & VERTEX4D_CLIPMASK )
				!= VERTEX4D_INSIDE
			)
			continue;

		// if fully inside
		if ( ( face[0]->flag & face[1]->flag & face[2]->flag & VERTEX4D_CLIPMASK ) == VERTEX4D_INSIDE )
		{
			dc_area = screenarea2 ( face );
			if ( Material.org.BackfaceCulling && F32_LOWER_EQUAL_0( dc_area ) )
				continue;
			else
			if ( Material.org.FrontfaceCulling && F32_GREATER_EQUAL_0( dc_area ) )
				continue;

			// select mipmap
			dc_area = core::reciprocal ( dc_area );
			for ( m = 0; m != vSize[VertexCache.vType].TexSize; ++m )
			{
				if ( 0 == (tex = MAT_TEXTURE ( m )) )
				{
					CurrentShader->setTextureParam(m, 0, 0);
					continue;
				}

				lodLevel = s32_log2_f32 ( texelarea2 ( face, m ) * dc_area  );
				CurrentShader->setTextureParam(m, tex, lodLevel );
				select_polygon_mipmap2 ( (s4DVertex**) face, m, tex->getSize() );
			}

			// rasterize
			CurrentShader->drawTriangle ( face[0] + 1, face[1] + 1, face[2] + 1 );
			continue;
		}

		// else if not complete inside clipping necessary
		irr::memcpy32_small ( ( (u8*) CurrentOut.data + ( 0 << ( SIZEOF_SVERTEX_LOG2 + 1 ) ) ), face[0], SIZEOF_SVERTEX * 2 );
		irr::memcpy32_small ( ( (u8*) CurrentOut.data + ( 1 << ( SIZEOF_SVERTEX_LOG2 + 1 ) ) ), face[1], SIZEOF_SVERTEX * 2 );
		irr::memcpy32_small ( ( (u8*) CurrentOut.data + ( 2 << ( SIZEOF_SVERTEX_LOG2 + 1 ) ) ), face[2], SIZEOF_SVERTEX * 2 );

		const u32 flag = CurrentOut.data->flag & VERTEX4D_FORMAT_MASK;

		for ( g = 0; g != CurrentOut.ElementSize; ++g )
		{
			CurrentOut.data[g].flag = flag;
			Temp.data[g].flag = flag;
		}

		u32 vOut;
		vOut = clipToFrustum ( CurrentOut.data, Temp.data, 3 );
		if ( vOut < 3 )
			continue;

		vOut <<= 1;

		// to DC Space, project homogenous vertex
		ndc_2_dc_and_project ( CurrentOut.data + 1, CurrentOut.data, vOut );

/*
		// TODO: don't stick on 32 Bit Pointer
		#define PointerAsValue(x) ( (u32) (u32*) (x) )

		// if not complete inside clipping necessary
		if ( ( test & VERTEX4D_INSIDE ) != VERTEX4D_INSIDE )
		{
			u32 v[2] = { PointerAsValue ( Temp ) , PointerAsValue ( CurrentOut ) };
			for ( g = 0; g != 6; ++g )
			{
				vOut = clipToHyperPlane ( (s4DVertex*) v[0], (s4DVertex*) v[1], vOut, NDCPlane[g] );
				if ( vOut < 3 )
					break;

				v[0] ^= v[1];
				v[1] ^= v[0];
				v[0] ^= v[1];
			}

			if ( vOut < 3 )
				continue;

		}
*/

		// check 2d backface culling on first
		dc_area = screenarea ( CurrentOut.data );
		if ( Material.org.BackfaceCulling && F32_LOWER_EQUAL_0 ( dc_area ) )
			continue;
		else if ( Material.org.FrontfaceCulling && F32_GREATER_EQUAL_0( dc_area ) )
			continue;

		// select mipmap
		dc_area = core::reciprocal ( dc_area );
		for ( m = 0; m != vSize[VertexCache.vType].TexSize; ++m )
		{
			if ( 0 == (tex = MAT_TEXTURE ( m )) )
			{
				CurrentShader->setTextureParam(m, 0, 0);
				continue;
			}

			lodLevel = s32_log2_f32 ( texelarea ( CurrentOut.data, m ) * dc_area );
			CurrentShader->setTextureParam(m, tex, lodLevel );
			select_polygon_mipmap ( CurrentOut.data, vOut, m, tex->getSize() );
		}


		// re-tesselate ( triangle-fan, 0-1-2,0-2-3.. )
		for ( g = 0; g <= vOut - 6; g += 2 )
		{
			// rasterize
			CurrentShader->drawTriangle ( CurrentOut.data + 0 + 1,
							CurrentOut.data + g + 3,
							CurrentOut.data + g + 5);
		}

	}

	// dump statistics
/*
	char buf [64];
	sprintf ( buf,"VCount:%d PCount:%d CacheMiss: %d",
					vertexCount, primitiveCount,
					VertexCache.CacheMiss
				);
	os::Printer::log( buf );
*/

}


//! Sets the dynamic ambient light color. The default color is
//! (0,0,0,0) which means it is dark.
//! \param color: New color of the ambient light.
void CBurningVideoDriver::setAmbientLight(const SColorf& color)
{
	LightSpace.Global_AmbientLight.setColorf ( color );
}


//! adds a dynamic light
s32 CBurningVideoDriver::addDynamicLight(const SLight& dl)
{
	(void) CNullDriver::addDynamicLight( dl );

	SBurningShaderLight l;
//	l.org = dl;
	l.Type = dl.Type;
	l.LightIsOn = true;

	l.AmbientColor.setColorf ( dl.AmbientColor );
	l.DiffuseColor.setColorf ( dl.DiffuseColor );
	l.SpecularColor.setColorf ( dl.SpecularColor );

	switch ( dl.Type )
	{
		case video::ELT_DIRECTIONAL:
			l.pos.x = -dl.Direction.X;
			l.pos.y = -dl.Direction.Y;
			l.pos.z = -dl.Direction.Z;
			l.pos.w = 1.f;
			break;
		case ELT_POINT:
		case ELT_SPOT:
			LightSpace.Flags |= POINTLIGHT;
			l.pos.x = dl.Position.X;
			l.pos.y = dl.Position.Y;
			l.pos.z = dl.Position.Z;
			l.pos.w = 1.f;
/*
			l.radius = (1.f / dl.Attenuation.Y) * (1.f / dl.Attenuation.Y);
			l.constantAttenuation = dl.Attenuation.X;
			l.linearAttenuation = dl.Attenuation.Y;
			l.quadraticAttenuation = dl.Attenuation.Z;
*/
			l.radius = dl.Radius * dl.Radius;
			l.constantAttenuation = dl.Attenuation.X;
			l.linearAttenuation = 1.f / dl.Radius;
			l.quadraticAttenuation = dl.Attenuation.Z;

			break;
		default:
			break;
	}

	LightSpace.Light.push_back ( l );
	return LightSpace.Light.size() - 1;
}

//! Turns a dynamic light on or off
void CBurningVideoDriver::turnLightOn(s32 lightIndex, bool turnOn)
{
	if(lightIndex > -1 && lightIndex < (s32)LightSpace.Light.size())
	{
		LightSpace.Light[lightIndex].LightIsOn = turnOn;
	}
}

//! deletes all dynamic lights there are
void CBurningVideoDriver::deleteAllDynamicLights()
{
	LightSpace.reset ();
	CNullDriver::deleteAllDynamicLights();

}

//! returns the maximal amount of dynamic lights the device can handle
u32 CBurningVideoDriver::getMaximalDynamicLightAmount() const
{
	return 8;
}


//! sets a material
void CBurningVideoDriver::setMaterial(const SMaterial& material)
{
	Material.org = material;

#ifdef SOFTWARE_DRIVER_2_TEXTURE_TRANSFORM
	for (u32 i = 0; i < 2; ++i)
	{
		setTransform((E_TRANSFORMATION_STATE) (ETS_TEXTURE_0 + i),
				material.getTextureMatrix(i));
	}
#endif

#ifdef SOFTWARE_DRIVER_2_LIGHTING
	Material.AmbientColor.setR8G8B8 ( Material.org.AmbientColor.color );
	Material.DiffuseColor.setR8G8B8 ( Material.org.DiffuseColor.color );
	Material.EmissiveColor.setR8G8B8 ( Material.org.EmissiveColor.color );
	Material.SpecularColor.setR8G8B8 ( Material.org.SpecularColor.color );

	core::setbit_cond ( LightSpace.Flags, Material.org.Shininess != 0.f, SPECULAR );
	core::setbit_cond ( LightSpace.Flags, Material.org.FogEnable, FOG );
	core::setbit_cond ( LightSpace.Flags, Material.org.NormalizeNormals, NORMALIZE );
#endif

	setCurrentShader();
}


/*!
	Camera Position in World Space
*/
void CBurningVideoDriver::getCameraPosWorldSpace ()
{
	Transformation[ETS_VIEW_INVERSE] = Transformation[ ETS_VIEW ];
	Transformation[ETS_VIEW_INVERSE].makeInverse ();
	TransformationFlag[ETS_VIEW_INVERSE] = 0;

	const f32 *M = Transformation[ETS_VIEW_INVERSE].pointer ();

	/*	The  viewpoint is at (0., 0., 0.) in eye space.
		Turning this into a vector [0 0 0 1] and multiply it by
		the inverse of the view matrix, the resulting vector is the
		object space location of the camera.
	*/

	LightSpace.campos.x = M[12];
	LightSpace.campos.y = M[13];
	LightSpace.campos.z = M[14];
	LightSpace.campos.w = 1.f;
}

void CBurningVideoDriver::getLightPosObjectSpace ()
{
	if ( TransformationFlag[ETS_WORLD] & ETF_IDENTITY )
	{
		Transformation[ETS_WORLD_INVERSE] = Transformation[ETS_WORLD];
		TransformationFlag[ETS_WORLD_INVERSE] |= ETF_IDENTITY;
	}
	else
	{
		Transformation[ETS_WORLD].getInverse ( Transformation[ETS_WORLD_INVERSE] );
		TransformationFlag[ETS_WORLD_INVERSE] &= ~ETF_IDENTITY;
	}

	for ( u32 i = 0; i < 1 && i < LightSpace.Light.size(); ++i )
	{
		SBurningShaderLight &l = LightSpace.Light[i];

		Transformation[ETS_WORLD_INVERSE].transformVec3 ( &l.pos_objectspace.x, &l.pos.x );
	}
}


#ifdef SOFTWARE_DRIVER_2_LIGHTING

//! Sets the fog mode.
void CBurningVideoDriver::setFog(SColor color, E_FOG_TYPE fogType, f32 start,
	f32 end, f32 density, bool pixelFog, bool rangeFog)
{
	CNullDriver::setFog(color, fogType, start, end, density, pixelFog, rangeFog);
	LightSpace.FogColor.setA8R8G8B8 ( color.color );
}

/*!
	applies lighting model
*/
void CBurningVideoDriver::lightVertex ( s4DVertex *dest, u32 vertexargb )
{
	sVec3 dColor;

	dColor = LightSpace.Global_AmbientLight;
	dColor.add ( Material.EmissiveColor );

	if ( Lights.size () == 0 )
	{
		dColor.saturate( dest->Color[0], vertexargb);
		return;
	}

	sVec3 ambient;
	sVec3 diffuse;
	sVec3 specular;


	// the universe started in darkness..
	ambient.set ( 0.f, 0.f, 0.f );
	diffuse.set ( 0.f, 0.f, 0.f );
	specular.set ( 0.f, 0.f, 0.f );


	u32 i;
	f32 dot;
	f32 len;
	f32 attenuation;
	sVec4 vp;			// unit vector vertex to light
	sVec4 lightHalf;	// blinn-phong reflection

	for ( i = 0; i!= LightSpace.Light.size (); ++i )
	{
		const SBurningShaderLight &light = LightSpace.Light[i];

		if ( !light.LightIsOn )
			continue;

		// accumulate ambient
		ambient.add ( light.AmbientColor );

		switch ( light.Type )
		{
			case video::ELT_SPOT:
			case video::ELT_POINT:
				// surface to light
				vp.x = light.pos.x - LightSpace.vertex.x;
				vp.y = light.pos.y - LightSpace.vertex.y;
				vp.z = light.pos.z - LightSpace.vertex.z;
				//vp.x = light.pos_objectspace.x - LightSpace.vertex.x;
				//vp.y = light.pos_objectspace.y - LightSpace.vertex.x;
				//vp.z = light.pos_objectspace.z - LightSpace.vertex.x;

				len = vp.get_length_xyz_square();
				if ( light.radius < len )
					continue;

				len = core::reciprocal_squareroot ( len );

				// build diffuse reflection

				//angle between normal and light vector
				vp.mul ( len );
				dot = LightSpace.normal.dot_xyz ( vp );
				if ( dot < 0.f )
					continue;

				attenuation = light.constantAttenuation + ( 1.f - ( len * light.linearAttenuation ) );

				// diffuse component
				diffuse.mulAdd ( light.DiffuseColor, 3.f * dot * attenuation );

				if ( !(LightSpace.Flags & SPECULAR) )
					continue;

				// build specular
				// surface to view
				lightHalf.x = LightSpace.campos.x - LightSpace.vertex.x;
				lightHalf.y = LightSpace.campos.y - LightSpace.vertex.y;
				lightHalf.z = LightSpace.campos.z - LightSpace.vertex.z;
				lightHalf.normalize_xyz();
				lightHalf += vp;
				lightHalf.normalize_xyz();

				// specular
				dot = LightSpace.normal.dot_xyz ( lightHalf );
				if ( dot < 0.f )
					continue;

				//specular += light.SpecularColor * ( powf ( Material.org.Shininess ,dot ) * attenuation );
				specular.mulAdd ( light.SpecularColor, dot * attenuation );
				break;

			case video::ELT_DIRECTIONAL:

				//angle between normal and light vector
				dot = LightSpace.normal.dot_xyz ( light.pos );
				if ( dot < 0.f )
					continue;

				// diffuse component
				diffuse.mulAdd ( light.DiffuseColor, dot );
				break;
			default:
				break;
		}

	}

	// sum up lights
	dColor.mulAdd (ambient, Material.AmbientColor );
	dColor.mulAdd (diffuse, Material.DiffuseColor);
	dColor.mulAdd (specular, Material.SpecularColor);

	dColor.saturate ( dest->Color[0], vertexargb );
}

#endif


//! draws an 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
void CBurningVideoDriver::draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos,
					 const core::rect<s32>& sourceRect,
					 const core::rect<s32>* clipRect, SColor color,
					 bool useAlphaChannelOfTexture)
{
	if (texture)
	{
		if (texture->getDriverType() != EDT_BURNINGSVIDEO)
		{
			os::Printer::log("Fatal Error: Tried to copy from a surface not owned by this driver.", ELL_ERROR);
			return;
		}

#if 0
		// 2d methods don't use viewPort
		core::position2di dest = destPos;
		core::recti clip=ViewPort;
		if (ViewPort.getSize().Width != ScreenSize.Width)
		{
			dest.X=ViewPort.UpperLeftCorner.X+core::round32(destPos.X*ViewPort.getWidth()/(f32)ScreenSize.Width);
			dest.Y=ViewPort.UpperLeftCorner.Y+core::round32(destPos.Y*ViewPort.getHeight()/(f32)ScreenSize.Height);
			if (clipRect)
			{
				clip.constrainTo(*clipRect);
			}
			clipRect = &clip;
		}
#endif
		if (useAlphaChannelOfTexture)
			((CSoftwareTexture2*)texture)->getImage()->copyToWithAlpha(
			RenderTargetSurface, destPos, sourceRect, color, clipRect);
		else
			((CSoftwareTexture2*)texture)->getImage()->copyTo(
				RenderTargetSurface, destPos, sourceRect, clipRect);
	}
}


//! Draws a part of the texture into the rectangle.
void CBurningVideoDriver::draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
		const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect,
		const video::SColor* const colors, bool useAlphaChannelOfTexture)
{
	if (texture)
	{
		if (texture->getDriverType() != EDT_BURNINGSVIDEO)
		{
			os::Printer::log("Fatal Error: Tried to copy from a surface not owned by this driver.", ELL_ERROR);
			return;
		}

	if (useAlphaChannelOfTexture)
		StretchBlit(BLITTER_TEXTURE_ALPHA_BLEND, RenderTargetSurface, &destRect, &sourceRect,
			    ((CSoftwareTexture2*)texture)->getImage(), (colors ? colors[0].color : 0));
	else
		StretchBlit(BLITTER_TEXTURE, RenderTargetSurface, &destRect, &sourceRect,
			    ((CSoftwareTexture2*)texture)->getImage(), (colors ? colors[0].color : 0));
	}
}

//! Draws a 2d line.
void CBurningVideoDriver::draw2DLine(const core::position2d<s32>& start,
					const core::position2d<s32>& end,
					SColor color)
{
	drawLine(BackBuffer, start, end, color );
}


//! Draws a pixel
void CBurningVideoDriver::drawPixel(u32 x, u32 y, const SColor & color)
{
	BackBuffer->setPixel(x, y, color, true);
}


//! draw an 2d rectangle
void CBurningVideoDriver::draw2DRectangle(SColor color, const core::rect<s32>& pos,
									 const core::rect<s32>* clip)
{
	if (clip)
	{
		core::rect<s32> p(pos);

		p.clipAgainst(*clip);

		if(!p.isValid())
			return;

		drawRectangle(BackBuffer, p, color);
	}
	else
	{
		if(!pos.isValid())
			return;

		drawRectangle(BackBuffer, pos, color);
	}
}


//! Only used by the internal engine. Used to notify the driver that
//! the window was resized.
void CBurningVideoDriver::OnResize(const core::dimension2d<u32>& size)
{
	// make sure width and height are multiples of 2
	core::dimension2d<u32> realSize(size);

	if (realSize.Width % 2)
		realSize.Width += 1;

	if (realSize.Height % 2)
		realSize.Height += 1;

	if (ScreenSize != realSize)
	{
		if (ViewPort.getWidth() == (s32)ScreenSize.Width &&
			ViewPort.getHeight() == (s32)ScreenSize.Height)
		{
			ViewPort.UpperLeftCorner.X = 0;
			ViewPort.UpperLeftCorner.Y = 0;
			ViewPort.LowerRightCorner.X = realSize.Width;
			ViewPort.LowerRightCorner.X = realSize.Height;
		}

		ScreenSize = realSize;

		bool resetRT = (RenderTargetSurface == BackBuffer);

		if (BackBuffer)
			BackBuffer->drop();
		BackBuffer = new CImage(BURNINGSHADER_COLOR_FORMAT, realSize);

		if (resetRT)
			setRenderTarget(BackBuffer);
	}
}


//! returns the current render target size
const core::dimension2d<u32>& CBurningVideoDriver::getCurrentRenderTargetSize() const
{
	return RenderTargetSize;
}


//!Draws an 2d rectangle with a gradient.
void CBurningVideoDriver::draw2DRectangle(const core::rect<s32>& position,
	SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
	const core::rect<s32>* clip)
{
#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR

	core::rect<s32> pos = position;

	if (clip)
		pos.clipAgainst(*clip);

	if (!pos.isValid())
		return;

	const core::dimension2d<s32> renderTargetSize ( ViewPort.getSize() );

	const s32 xPlus = -(renderTargetSize.Width>>1);
	const f32 xFact = 1.0f / (renderTargetSize.Width>>1);

	const s32 yPlus = renderTargetSize.Height-(renderTargetSize.Height>>1);
	const f32 yFact = 1.0f / (renderTargetSize.Height>>1);

	// fill VertexCache direct
	s4DVertex *v;

	VertexCache.vertexCount = 4;

	VertexCache.info[0].index = 0;
	VertexCache.info[1].index = 1;
	VertexCache.info[2].index = 2;
	VertexCache.info[3].index = 3;

	v = &VertexCache.mem.data [ 0 ];

	v[0].Pos.set ( (f32)(pos.UpperLeftCorner.X+xPlus) * xFact, (f32)(yPlus-pos.UpperLeftCorner.Y) * yFact, 0.f, 1.f );
	v[0].Color[0].setA8R8G8B8 ( colorLeftUp.color );

	v[2].Pos.set ( (f32)(pos.LowerRightCorner.X+xPlus) * xFact, (f32)(yPlus- pos.UpperLeftCorner.Y) * yFact, 0.f, 1.f );
	v[2].Color[0].setA8R8G8B8 ( colorRightUp.color );

	v[4].Pos.set ( (f32)(pos.LowerRightCorner.X+xPlus) * xFact, (f32)(yPlus-pos.LowerRightCorner.Y) * yFact, 0.f ,1.f );
	v[4].Color[0].setA8R8G8B8 ( colorRightDown.color );

	v[6].Pos.set ( (f32)(pos.UpperLeftCorner.X+xPlus) * xFact, (f32)(yPlus-pos.LowerRightCorner.Y) * yFact, 0.f, 1.f );
	v[6].Color[0].setA8R8G8B8 ( colorLeftDown.color );

	s32 i;
	u32 g;

	for ( i = 0; i!= 8; i += 2 )
	{
		v[i + 0].flag = clipToFrustumTest ( v + i );
		v[i + 1].flag = 0;
		if ( (v[i].flag & VERTEX4D_INSIDE ) == VERTEX4D_INSIDE )
		{
			ndc_2_dc_and_project ( v + i + 1, v + i, 2 );
		}
	}


	IBurningShader * render;

	render = BurningShader [ ETR_GOURAUD_ALPHA_NOZ ];
	render->setRenderTarget(RenderTargetSurface, ViewPort);

	static const s16 indexList[6] = {0,1,2,0,2,3};

	s4DVertex * face[3];

	for ( i = 0; i!= 6; i += 3 )
	{
		face[0] = VertexCache_getVertex ( indexList [ i + 0 ] );
		face[1] = VertexCache_getVertex ( indexList [ i + 1 ] );
		face[2] = VertexCache_getVertex ( indexList [ i + 2 ] );

		// test clipping
		u32 test = face[0]->flag & face[1]->flag & face[2]->flag & VERTEX4D_INSIDE;

		if ( test == VERTEX4D_INSIDE )
		{
			render->drawTriangle ( face[0] + 1, face[1] + 1, face[2] + 1 );
			continue;
		}
		// Todo: all vertices are clipped in 2d..
		// is this true ?
		u32 vOut = 6;
		memcpy ( CurrentOut.data + 0, face[0], sizeof ( s4DVertex ) * 2 );
		memcpy ( CurrentOut.data + 2, face[1], sizeof ( s4DVertex ) * 2 );
		memcpy ( CurrentOut.data + 4, face[2], sizeof ( s4DVertex ) * 2 );

		vOut = clipToFrustum ( CurrentOut.data, Temp.data, 3 );
		if ( vOut < 3 )
			continue;

		vOut <<= 1;
		// to DC Space, project homogenous vertex
		ndc_2_dc_and_project ( CurrentOut.data + 1, CurrentOut.data, vOut );

		// re-tesselate ( triangle-fan, 0-1-2,0-2-3.. )
		for ( g = 0; g <= vOut - 6; g += 2 )
		{
			// rasterize
			render->drawTriangle ( CurrentOut.data + 1, &CurrentOut.data[g + 3], &CurrentOut.data[g + 5] );
		}

	}
#else
	draw2DRectangle ( colorLeftUp, position, clip );
#endif
}


//! Draws a 3d line.
void CBurningVideoDriver::draw3DLine(const core::vector3df& start,
	const core::vector3df& end, SColor color)
{
	Transformation [ ETS_CURRENT].transformVect ( &CurrentOut.data[0].Pos.x, start );
	Transformation [ ETS_CURRENT].transformVect ( &CurrentOut.data[2].Pos.x, end );

	u32 g;
	u32 vOut;

	// no clipping flags
	for ( g = 0; g != CurrentOut.ElementSize; ++g )
	{
		CurrentOut.data[g].flag = 0;
		Temp.data[g].flag = 0;
	}

	// vertices count per line
	vOut = clipToFrustum ( CurrentOut.data, Temp.data, 2 );
	if ( vOut < 2 )
		return;

	vOut <<= 1;

	IBurningShader * line;
	line = BurningShader [ ETR_TEXTURE_GOURAUD_WIRE ];
	line->setRenderTarget(RenderTargetSurface, ViewPort);

	// to DC Space, project homogenous vertex
	ndc_2_dc_and_project ( CurrentOut.data + 1, CurrentOut.data, vOut );

	// unproject vertex color
#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	for ( g = 0; g != vOut; g+= 2 )
	{
		CurrentOut.data[ g + 1].Color[0].setA8R8G8B8 ( color.color );
	}
#endif


	for ( g = 0; g <= vOut - 4; g += 2 )
	{
		// rasterize
		line->drawLine ( CurrentOut.data + 1, CurrentOut.data + g + 3 );
	}
}


//! \return Returns the name of the video driver. Example: In case of the DirectX8
//! driver, it would return "Direct3D8.1".
const wchar_t* CBurningVideoDriver::getName() const
{
#ifdef BURNINGVIDEO_RENDERER_BEAUTIFUL
	return L"Burning's Video 0.47 beautiful";
#elif defined ( BURNINGVIDEO_RENDERER_ULTRA_FAST )
	return L"Burning's Video 0.47 ultra fast";
#elif defined ( BURNINGVIDEO_RENDERER_FAST )
	return L"Burning's Video 0.47 fast";
#else
	return L"Burning's Video 0.47";
#endif
}

//! Returns the graphics card vendor name.
core::stringc CBurningVideoDriver::getVendorInfo()
{
	return "Burning's Video: Ing. Thomas Alten (c) 2006-2012";
}


//! Returns type of video driver
E_DRIVER_TYPE CBurningVideoDriver::getDriverType() const
{
	return EDT_BURNINGSVIDEO;
}


//! returns color format
ECOLOR_FORMAT CBurningVideoDriver::getColorFormat() const
{
	return BURNINGSHADER_COLOR_FORMAT;
}


//! Returns the transformation set by setTransform
const core::matrix4& CBurningVideoDriver::getTransform(E_TRANSFORMATION_STATE state) const
{
	return Transformation[state];
}


//! Creates a render target texture.
ITexture* CBurningVideoDriver::addRenderTargetTexture(const core::dimension2d<u32>& size,
		const io::path& name, const ECOLOR_FORMAT format)
{
	IImage* img = createImage(BURNINGSHADER_COLOR_FORMAT, size);
	ITexture* tex = new CSoftwareTexture2(img, name, CSoftwareTexture2::IS_RENDERTARGET );
	img->drop();
	addTexture(tex);
	tex->drop();
	return tex;
}


//! Clears the DepthBuffer.
void CBurningVideoDriver::clearZBuffer()
{
	if (DepthBuffer)
		DepthBuffer->clear();
}


//! Returns an image created from the last rendered frame.
IImage* CBurningVideoDriver::createScreenShot(video::ECOLOR_FORMAT format, video::E_RENDER_TARGET target)
{
	if (target != video::ERT_FRAME_BUFFER)
		return 0;

	if (BackBuffer)
	{
		IImage* tmp = createImage(BackBuffer->getColorFormat(), BackBuffer->getDimension());
		BackBuffer->copyTo(tmp);
		return tmp;
	}
	else
		return 0;
}


//! returns a device dependent texture from a software surface (IImage)
//! THIS METHOD HAS TO BE OVERRIDDEN BY DERIVED DRIVERS WITH OWN TEXTURES
ITexture* CBurningVideoDriver::createDeviceDependentTexture(IImage* surface, const io::path& name, void* mipmapData)
{
	return new CSoftwareTexture2(
		surface, name,
		(getTextureCreationFlag(ETCF_CREATE_MIP_MAPS) ? CSoftwareTexture2::GEN_MIPMAP : 0 ) |
		(getTextureCreationFlag(ETCF_ALLOW_NON_POWER_2) ? 0 : CSoftwareTexture2::NP2_SIZE ), mipmapData);

}


//! Returns the maximum amount of primitives (mostly vertices) which
//! the device is able to render with one drawIndexedTriangleList
//! call.
u32 CBurningVideoDriver::getMaximalPrimitiveCount() const
{
	return 0xFFFFFFFF;
}


//! Draws a shadow volume into the stencil buffer. To draw a stencil shadow, do
//! this: First, draw all geometry. Then use this method, to draw the shadow
//! volume. Next use IVideoDriver::drawStencilShadow() to visualize the shadow.
void CBurningVideoDriver::drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail, u32 debugDataVisible)
{
	const u32 count = triangles.size();
	IBurningShader *shader = BurningShader [ ETR_STENCIL_SHADOW ];

	CurrentShader = shader;
	shader->setRenderTarget(RenderTargetSurface, ViewPort);

	Material.org.MaterialType = video::EMT_SOLID;
	Material.org.Lighting = false;
	Material.org.ZWriteEnable = false;
	Material.org.ZBuffer = ECFN_LESSEQUAL;
	LightSpace.Flags &= ~VERTEXTRANSFORM;

	//glStencilMask(~0);
	//glStencilFunc(GL_ALWAYS, 0, ~0);

	if (true)// zpass does not work yet
	{
		Material.org.BackfaceCulling = true;
		Material.org.FrontfaceCulling = false;
		shader->setParam ( 0, 0 );
		shader->setParam ( 1, 1 );
		shader->setParam ( 2, 0 );
		drawVertexPrimitiveList (triangles.const_pointer(), count, 0, count/3, (video::E_VERTEX_TYPE) 4, scene::EPT_TRIANGLES, (video::E_INDEX_TYPE) 4 );
		//glStencilOp(GL_KEEP, incr, GL_KEEP);
		//glDrawArrays(GL_TRIANGLES,0,count);

		Material.org.BackfaceCulling = false;
		Material.org.FrontfaceCulling = true;
		shader->setParam ( 0, 0 );
		shader->setParam ( 1, 2 );
		shader->setParam ( 2, 0 );
		drawVertexPrimitiveList (triangles.const_pointer(), count, 0, count/3, (video::E_VERTEX_TYPE) 4, scene::EPT_TRIANGLES, (video::E_INDEX_TYPE) 4 );
		//glStencilOp(GL_KEEP, decr, GL_KEEP);
		//glDrawArrays(GL_TRIANGLES,0,count);
	}
	else // zpass
	{
		Material.org.BackfaceCulling = true;
		Material.org.FrontfaceCulling = false;
		shader->setParam ( 0, 0 );
		shader->setParam ( 1, 0 );
		shader->setParam ( 2, 1 );
		//glStencilOp(GL_KEEP, GL_KEEP, incr);
		//glDrawArrays(GL_TRIANGLES,0,count);

		Material.org.BackfaceCulling = false;
		Material.org.FrontfaceCulling = true;
		shader->setParam ( 0, 0 );
		shader->setParam ( 1, 0 );
		shader->setParam ( 2, 2 );
		//glStencilOp(GL_KEEP, GL_KEEP, decr);
		//glDrawArrays(GL_TRIANGLES,0,count);
	}
}

//! Fills the stencil shadow with color. After the shadow volume has been drawn
//! into the stencil buffer using IVideoDriver::drawStencilShadowVolume(), use this
//! to draw the color of the shadow.
void CBurningVideoDriver::drawStencilShadow(bool clearStencilBuffer, video::SColor leftUpEdge,
	video::SColor rightUpEdge, video::SColor leftDownEdge, video::SColor rightDownEdge)
{
	if (!StencilBuffer)
		return;
	// draw a shadow rectangle covering the entire screen using stencil buffer
	const u32 h = RenderTargetSurface->getDimension().Height;
	const u32 w = RenderTargetSurface->getDimension().Width;
	tVideoSample *dst;
	u32 *stencil;
	u32* const stencilBase=(u32*) StencilBuffer->lock();

	for ( u32 y = 0; y < h; ++y )
	{
		dst = (tVideoSample*)RenderTargetSurface->lock() + ( y * w );
		stencil =  stencilBase + ( y * w );

		for ( u32 x = 0; x < w; ++x )
		{
			if ( stencil[x] > 1 )
			{
				dst[x] = PixelBlend32 ( dst[x], leftUpEdge.color );
			}
		}
	}

	StencilBuffer->clear();
}


core::dimension2du CBurningVideoDriver::getMaxTextureSize() const
{
	return core::dimension2du(SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE, SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE);
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_

namespace irr
{
namespace video
{

//! creates a video driver
IVideoDriver* createBurningVideoDriver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, video::IImagePresenter* presenter)
{
	#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
	return new CBurningVideoDriver(params, io, presenter);
	#else
	return 0;
	#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_
}



} // end namespace video
} // end namespace irr

