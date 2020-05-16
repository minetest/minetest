// Copyright (C) 2002-2012 Thomas Alten / Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_BSP_LOADER_

#include "CQuake3ShaderSceneNode.h"
#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "ICameraSceneNode.h"
#include "SViewFrustum.h"
#include "IMeshManipulator.h"
#include "SMesh.h"
#include "IMaterialRenderer.h"
#include "CShadowVolumeSceneNode.h"

namespace irr
{
namespace scene
{

// who, if not you..
using namespace quake3;

/*!
*/
CQuake3ShaderSceneNode::CQuake3ShaderSceneNode(
			scene::ISceneNode* parent, scene::ISceneManager* mgr,s32 id,
			io::IFileSystem *fileSystem, const scene::IMeshBuffer *original,
			const IShader * shader)
: scene::IMeshSceneNode(parent, mgr, id, 
		core::vector3df(0.f, 0.f, 0.f),
		core::vector3df(0.f, 0.f, 0.f),
		core::vector3df(1.f, 1.f, 1.f)),
	Shader(shader), Mesh(0), Shadow(0), Original(0), MeshBuffer(0), TimeAbs(0.f)
{
	#ifdef _DEBUG
		core::stringc dName = "CQuake3ShaderSceneNode ";
		dName += Shader->name;

		setDebugName( dName.c_str() );
	#endif

	// name the Scene Node
	this->Name = Shader->name;

	// take lightmap vertex type
	MeshBuffer = new SMeshBuffer();

	Mesh = new SMesh ();
	Mesh->addMeshBuffer ( MeshBuffer );
	MeshBuffer->drop ();

	//Original = new SMeshBufferLightMap();
	Original = (const scene::SMeshBufferLightMap*) original;
	Original->grab();

	// clone meshbuffer to modifiable buffer
	cloneBuffer(MeshBuffer, Original, 
			Original->getMaterial().ColorMask != 0);

	// load all Textures in all stages
	loadTextures( fileSystem );

	setAutomaticCulling( scene::EAC_OFF );
}


/*!
*/
CQuake3ShaderSceneNode::~CQuake3ShaderSceneNode()
{
	if (Shadow)
		Shadow->drop();

	if (Mesh)
		Mesh->drop();

	if (Original)
		Original->drop();
}



/*
	create single copies
*/
void CQuake3ShaderSceneNode::cloneBuffer( scene::SMeshBuffer *dest, const scene::SMeshBufferLightMap * buffer, bool translateCenter )
{
	dest->Material = buffer->Material;
	dest->Indices = buffer->Indices;

	const u32 vsize = buffer->Vertices.size();

	dest->Vertices.set_used( vsize );
	for ( u32 i = 0; i!= vsize; ++i )
	{
		const video::S3DVertex2TCoords& src = buffer->Vertices[i];
		video::S3DVertex &dst = dest->Vertices[i];

		dst.Pos = src.Pos;
		dst.Normal = src.Normal;
		dst.Color = 0xFFFFFFFF;
		dst.TCoords = src.TCoords;

		if ( i == 0 )
			dest->BoundingBox.reset ( src.Pos );
		else
			dest->BoundingBox.addInternalPoint ( src.Pos );
	}

	// move the (temp) Mesh to a ScenePosititon
	// set Scene Node Position

	if ( translateCenter )
	{
		MeshOffset = dest->BoundingBox.getCenter();
		setPosition( MeshOffset );

		core::matrix4 m;
		m.setTranslation( -MeshOffset );
		SceneManager->getMeshManipulator()->transform( dest, m );
	}

	// No Texture!. Use Shader-Pointer for sorting
	dest->Material.setTexture(0, (video::ITexture*) Shader);
}


/*
	load the textures for all stages
*/
void CQuake3ShaderSceneNode::loadTextures( io::IFileSystem * fileSystem )
{
	const SVarGroup *group;
	u32 i;

	video::IVideoDriver *driver = SceneManager->getVideoDriver();

	// generic stage
	u32 mipmap = 0;
	group = Shader->getGroup( 1 );
	if ( group->isDefined ( "nomipmaps" ) )
	{
		mipmap = 2 | (driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS)? 1: 0 );
		driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
	}

	// clear all stages and prefill empty
	Q3Texture.setAllocStrategy ( core::ALLOC_STRATEGY_SAFE );
	Q3Texture.clear();
	for ( i = 0; i != Shader->VarGroup->VariableGroup.size(); ++i )
	{
		Q3Texture.push_back( SQ3Texture() );
	}

	u32 pos;

	// get texture map
	for ( i = 0; i < Shader->VarGroup->VariableGroup.size(); ++i )
	{
		group = Shader->getGroup( i );

		const core::stringc &mapname = group->get( "map" );
		if ( 0 == mapname.size() )
			continue;

		// our lightmap is passed in material.Texture[2]
		if ( mapname == "$lightmap" )
		{
			Q3Texture [i].Texture.push_back( Original->getMaterial().getTexture(1) );
		}
		else
		{
			pos = 0;
			getTextures( Q3Texture [i].Texture, mapname, pos, fileSystem, driver );
		}
	}

	// get anim map
	for ( i = 0; i < Shader->VarGroup->VariableGroup.size(); ++i )
	{
		if ( Q3Texture [i].Texture.size() )
			continue;

		group = Shader->getGroup( i );

		const core::stringc &animmap = group->get( "animmap" );
		if ( 0 == animmap.size() )
			continue;

		// first parameter is frequency
		pos = 0;
		Q3Texture [i].TextureFrequency =  core::max_( 0.0001f, getAsFloat( animmap, pos ) );

		getTextures( Q3Texture [i].Texture, animmap, pos,fileSystem, driver );
	}

	// get clamp map
	for ( i = 0; i < Shader->VarGroup->VariableGroup.size(); ++i )
	{
		if ( Q3Texture [i].Texture.size() )
			continue;

		group = Shader->getGroup( i );

		const core::stringc &clampmap = group->get( "clampmap" );
		if ( 0 == clampmap.size() )
			continue;

		Q3Texture [i].TextureAddressMode = video::ETC_CLAMP_TO_EDGE;
		pos = 0;
		getTextures( Q3Texture [i].Texture, clampmap, pos,fileSystem, driver );
	}

	if ( mipmap & 2 )
		driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, mipmap & 1);
}

/*
	Register each texture stage, if first is visible
*/
void CQuake3ShaderSceneNode::OnRegisterSceneNode()
{
	if ( isVisible() )
	{
		SceneManager->registerNodeForRendering(this, getRenderStage() );
	}
	ISceneNode::OnRegisterSceneNode();
}

/*
	is this a transparent node ?
*/
E_SCENE_NODE_RENDER_PASS CQuake3ShaderSceneNode::getRenderStage() const
{
	E_SCENE_NODE_RENDER_PASS ret = ESNRP_SOLID;

	// generic stage
	const SVarGroup *group;

	group = Shader->getGroup( 1 );
/*
	else
	if ( group->getIndex( "portal" ) >= 0 )
	{
		ret = ESNRP_TRANSPARENT_EFFECT;
	}
	else
*/	
	if ( group->isDefined( "sort", "opaque" ) )
	{
		ret = ESNRP_SOLID;
	}
	else
	if ( group->isDefined( "sort", "additive" ) )
	{
		ret = ESNRP_TRANSPARENT;
	}
	else
	if (	strstr ( Shader->name.c_str(), "flame" ) ||
			group->isDefined( "surfaceparm", "water" ) ||
			group->isDefined( "sort", "underwater" ) ||
			group->isDefined( "sort", "underwater" )
		)
	{
		ret = ESNRP_TRANSPARENT_EFFECT;
	}
	else
	{
		// Look if first drawing stage needs graphical underlay
		for ( u32 stage = 2; stage < Shader->VarGroup->VariableGroup.size(); ++stage )
		{
			if ( 0 == Q3Texture [ stage ].Texture.size() )
				continue;

			group = Shader->getGroup( stage );

			SBlendFunc blendfunc ( video::EMFN_MODULATE_1X );
			getBlendFunc( group->get( "blendfunc" ), blendfunc );
			getBlendFunc( group->get( "alphafunc" ), blendfunc );

			//ret = blendfunc.isTransparent ? ESNRP_TRANSPARENT : ESNRP_SOLID;
			if ( blendfunc.isTransparent )
			{
				ret = ESNRP_TRANSPARENT;
			}
			break;
		}
	}

	return ret;
}


/*
	render in multipass technique
*/
void CQuake3ShaderSceneNode::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	E_SCENE_NODE_RENDER_PASS pass = SceneManager->getSceneNodeRenderPass();

	video::SMaterial material;
	const SVarGroup *group;

	material.Lighting = false;
	material.setTexture(1, 0);
	material.NormalizeNormals = false;

	// generic stage
	group = Shader->getGroup( 1 );
	material.BackfaceCulling = getCullingFunction( group->get( "cull" ) );

	u32 pushProjection = 0;
	core::matrix4 projection ( core::matrix4::EM4CONST_NOTHING );

	// decal ( solve z-fighting )
	if ( group->isDefined( "polygonoffset" ) )
	{
		projection = driver->getTransform( video::ETS_PROJECTION );

		core::matrix4 decalProjection ( projection );

/*
		f32 n = SceneManager->getActiveCamera()->getNearValue();
		f32 f = SceneManager->getActiveCamera()->getFarValue ();

		f32 delta = 0.01f;
		f32 pz = 0.2f;
		f32 epsilon = -2.f * f * n * delta / ( ( f + n ) * pz * ( pz + delta ) );
		decalProjection[10] *= 1.f + epsilon;
*/
		// TODO: involve camera
		decalProjection[10] -= 0.0002f;
		driver->setTransform( video::ETS_PROJECTION, decalProjection );
		pushProjection |= 1;
	}

	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation );
	if (Shadow)
		Shadow->updateShadowVolumes();

	//! render all stages
	u32 drawCount = (pass == ESNRP_TRANSPARENT_EFFECT) ? 1 : 0;
	core::matrix4 textureMatrix ( core::matrix4::EM4CONST_NOTHING );
	for ( u32 stage = 1; stage < Shader->VarGroup->VariableGroup.size(); ++stage )
	{
		SQ3Texture &q = Q3Texture[stage];

		// advance current stage
		textureMatrix.makeIdentity();
		animate( stage, textureMatrix );

		// stage finished, no drawing stage ( vertex transform only )
		video::ITexture * tex = q.Texture.size() ? q.Texture [ q.TextureIndex ] : 0;
		if ( 0 == tex )
			continue;

		// current stage
		group = Shader->getGroup( stage );

		material.setTexture(0, tex );
		material.ZBuffer = getDepthFunction( group->get( "depthfunc" ) );

		if ( group->isDefined( "depthwrite" ) )
		{
			material.ZWriteEnable = true;
		}
		else
		{
			material.ZWriteEnable = drawCount == 0;
		}

		//resolve quake3 blendfunction to irrlicht Material Type
		SBlendFunc blendfunc ( video::EMFN_MODULATE_1X );
		getBlendFunc( group->get( "blendfunc" ), blendfunc );
		getBlendFunc( group->get( "alphafunc" ), blendfunc );

		material.MaterialType = blendfunc.type;
		material.MaterialTypeParam = blendfunc.param0;

		material.TextureLayer[0].TextureWrapU = q.TextureAddressMode;
		material.TextureLayer[0].TextureWrapV = q.TextureAddressMode;
		//material.TextureLayer[0].TrilinearFilter = 1;
		//material.TextureLayer[0].AnisotropicFilter = 0xFF;
		material.setTextureMatrix( 0, textureMatrix );

		driver->setMaterial( material );
		driver->drawMeshBuffer( MeshBuffer );
		drawCount += 1;

	}

	if ( DebugDataVisible & scene::EDS_MESH_WIRE_OVERLAY )
	{
		video::SMaterial deb_m;
		deb_m.Wireframe = true;
		deb_m.Lighting = false;
		deb_m.BackfaceCulling = material.BackfaceCulling;
		driver->setMaterial( deb_m );

		driver->drawMeshBuffer( MeshBuffer );
	}

	// show normals
	if ( DebugDataVisible & scene::EDS_NORMALS )
	{
		video::SMaterial deb_m;

		IAnimatedMesh * arrow = SceneManager->addArrowMesh (
				"__debugnormalq3", 
				0xFFECEC00,0xFF999900, 
				4, 8,
				8.f, 6.f,
				0.5f,1.f
			);
		if ( 0 == arrow )
		{
			arrow = SceneManager->getMesh ( "__debugnormalq3" );
		}
		const IMesh *mesh = arrow->getMesh ( 0 );

		// find a good scaling factor

		core::matrix4 m2;

		// draw normals
		const scene::IMeshBuffer* mb = MeshBuffer;
		const u32 vSize = video::getVertexPitchFromType(mb->getVertexType());
		const video::S3DVertex* v = ( const video::S3DVertex*)mb->getVertices();

		//f32 colCycle = 270.f / (f32) core::s32_max ( mb->getVertexCount() - 1, 1 );

		for ( u32 i=0; i != mb->getVertexCount(); ++i )
		{
			// Align to v->normal
			m2.buildRotateFromTo ( core::vector3df ( 0.f, 1.f, 0 ), v->Normal );
			m2.setTranslation ( v->Pos + AbsoluteTransformation.getTranslation () );
/*
			core::quaternion quatRot( v->Normal.Z, 0.f, -v->Normal.X, 1 + v->Normal.Y );
			quatRot.normalize();
			quatRot.getMatrix ( m2, v->Pos );

			m2 [ 12 ] += AbsoluteTransformation [ 12 ];
			m2 [ 13 ] += AbsoluteTransformation [ 13 ];
			m2 [ 14 ] += AbsoluteTransformation [ 14 ];
*/
			driver->setTransform(video::ETS_WORLD, m2 );

			deb_m.Lighting = true;
/*
			irr::video::SColorHSL color;
			irr::video::SColor rgb(0);
			color.Hue = i * colCycle * core::DEGTORAD;
			color.Saturation = 1.f;
			color.Luminance = 0.5f;
			color.toRGB(  deb_m.EmissiveColor );
*/
			switch ( i )
			{
				case 0: deb_m.EmissiveColor.set(0xFFFFFFFF); break;
				case 1: deb_m.EmissiveColor.set(0xFFFF0000); break;
				case 2: deb_m.EmissiveColor.set(0xFF00FF00); break;
				case 3: deb_m.EmissiveColor.set(0xFF0000FF); break;
				default:
					deb_m.EmissiveColor = v->Color; break;
			}
			driver->setMaterial( deb_m );

			for ( u32 a = 0; a != mesh->getMeshBufferCount(); ++a )
				driver->drawMeshBuffer ( mesh->getMeshBuffer ( a ) );

			v = (const video::S3DVertex*) ( (u8*) v + vSize );
		}
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
	}


	if ( pushProjection & 1 )
	{
		driver->setTransform( video::ETS_PROJECTION, projection );
	}

	if ( DebugDataVisible & scene::EDS_BBOX )
	{
		video::SMaterial deb_m;
		deb_m.Lighting = false;
		driver->setMaterial(deb_m);
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		driver->draw3DBox( getBoundingBox(), video::SColor(255,255,0,0));
	}

}


//! Removes a child from this scene node.
//! Implemented here, to be able to remove the shadow properly, if there is one,
//! or to remove attached childs.
bool CQuake3ShaderSceneNode::removeChild(ISceneNode* child)
{
	if (child && Shadow == child)
	{
		Shadow->drop();
		Shadow = 0;
	}

	return ISceneNode::removeChild(child);
}


//! Creates shadow volume scene node as child of this node
//! and returns a pointer to it.
IShadowVolumeSceneNode* CQuake3ShaderSceneNode::addShadowVolumeSceneNode(
		const IMesh* shadowMesh, s32 id, bool zfailmethod, f32 infinity)
{
	if (!SceneManager->getVideoDriver()->queryFeature(video::EVDF_STENCIL_BUFFER))
		return 0;

	if (!shadowMesh)
		shadowMesh = Mesh; // if null is given, use the mesh of node

	if (Shadow)
		Shadow->drop();

	Shadow = new CShadowVolumeSceneNode(shadowMesh, this, SceneManager, id,  zfailmethod, infinity);
	return Shadow;
}


/*!
3.3.1 deformVertexes wave <div> <func> <base> <amplitude> <phase> <freq>
	Designed for water surfaces, modifying the values differently at each point.
	It accepts the standard wave functions of the type sin, triangle, square, sawtooth
	or inversesawtooth. The "div" parameter is used to control the wave "spread" 
	- a value equal to the tessSize of the surface is a good default value
	(tessSize is subdivision size, in game units, used for the shader when seen in the game world) .
*/
void CQuake3ShaderSceneNode::deformvertexes_wave( f32 dt, SModifierFunction &function )
{
	function.wave = core::reciprocal( function.wave );

	const f32 phase = function.phase;

	const u32 vsize = Original->Vertices.size();
	for ( u32 i = 0; i != vsize; ++i )
	{
		const video::S3DVertex2TCoords &src = Original->Vertices[i];
		video::S3DVertex &dst = MeshBuffer->Vertices[i];

		if ( 0 == function.count )
			dst.Pos = src.Pos - MeshOffset;

		const f32 wavephase = (dst.Pos.X + dst.Pos.Y + dst.Pos.Z) * function.wave;
		function.phase = phase + wavephase;

		const f32 f = function.evaluate( dt );

		dst.Pos.X += f * src.Normal.X;
		dst.Pos.Y += f * src.Normal.Y;
		dst.Pos.Z += f * src.Normal.Z;

		if ( i == 0 )
			MeshBuffer->BoundingBox.reset ( dst.Pos );
		else
			MeshBuffer->BoundingBox.addInternalPoint ( dst.Pos );
	}
	function.count = 1;
}

/*!
	deformVertexes move x y z func base amplitude phase freq
	The move parameter is used to make a brush, curve patch or model 
	appear to move together as a unit. The x y z values are the distance
	and direction in game units the object appears to move relative to 
	it's point of origin in the map. The func base amplitude phase freq values are
	the same as found in other waveform manipulations.

	The product of the function modifies the values x, y, and z.
	Therefore, if you have an amplitude of 5 and an x value of 2, 
	the object will travel 10 units from its point of origin along the x axis. 
	This results in a total of 20 units of motion along the x axis, since the
	amplitude is the variation both above and below the base.

	It must be noted that an object made with this shader does not actually
	change position, it only appears to.

	Design Notes:
	If an object is made up of surfaces with different shaders, all must have 
	matching deformVertexes move values or the object will appear to tear itself apart.
*/
void CQuake3ShaderSceneNode::deformvertexes_move( f32 dt, SModifierFunction &function )
{
	function.wave = core::reciprocal( function.wave );
	const f32 f = function.evaluate( dt );

	const u32 vsize = Original->Vertices.size();
	for ( u32 i = 0; i != vsize; ++i )
	{
		const video::S3DVertex2TCoords &src = Original->Vertices[i];
		video::S3DVertex &dst = MeshBuffer->Vertices[i];

		if ( 0 == function.count )
			dst.Pos = src.Pos - MeshOffset;

		dst.Pos.X += f * function.x;
		dst.Pos.Y += f * function.y;
		dst.Pos.Z += f * function.z;

		if ( i == 0 )
			MeshBuffer->BoundingBox.reset ( dst.Pos );
		else
			MeshBuffer->BoundingBox.addInternalPoint ( dst.Pos );
	}
	function.count = 1;

}

/*!
	3.3.2 deformVertexes normal <div> <func> <base> <amplitude ~0.1-~0.5> <frequency ~1.0-~4.0>
		This deformation affects the normals of a vertex without actually moving it,
		which will effect later shader options like lighting and especially environment mapping.
		If the shader stages don't use normals in any of their calculations, there will
		be no visible effect.

		Design Notes: Putting values of 0.1 t o 0.5 in Amplitude and 1.0 to 4.0 in the
		Frequency can produce some satisfying results. Some things that have been 
		done with it: A small fluttering bat, falling leaves, rain, flags.
*/
void CQuake3ShaderSceneNode::deformvertexes_normal( f32 dt, SModifierFunction &function )
{
	function.func = SINUS;
	const u32 vsize = Original->Vertices.size();
	for ( u32 i = 0; i != vsize; ++i )
	{
		const video::S3DVertex2TCoords &src = Original->Vertices[i];
		video::S3DVertex &dst = MeshBuffer->Vertices[i];

		function.base = atan2f ( src.Pos.X, src.Pos.Y );
		function.phase = src.Pos.X + src.Pos.Z;

		const f32 lat = function.evaluate( dt );

		function.base = src.Normal.Y;
		function.phase = src.Normal.Z + src.Normal.X;

		const f32 lng = function.evaluate( dt );

		dst.Normal.X = cosf ( lat ) * sinf ( lng );
		dst.Normal.Y = sinf ( lat ) * sinf ( lng );
		dst.Normal.Z = cosf ( lng );

	}
}


/*!
	3.3.3 deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
	This forces a bulge to move along the given s and t directions. Designed for use
	on curved pipes.

	Specific parameter definitions for deform keywords:
		<div>	This is roughly defined as the size of the waves that occur.
				It is measured in game units. Smaller values create a greater 
				density of smaller wave forms occurring in a given area. 
				Larger values create a lesser density of waves, or otherwise put,
				the appearance of larger waves. To look correct this value should 
				closely correspond to the value (in pixels) set for tessSize (tessellation size)
				of the texture. A value of 100.0 is a good default value 
				(which means your tessSize should be close to that for things to look "wavelike").

		<func>	This is the type of wave form being created. Sin stands for sine wave, 
				a regular smoothly flowing wave. Triangle is a wave with a sharp ascent
				and a sharp decay. It will make a choppy looking wave forms.
				A square wave is simply on or off for the period of the 
				frequency with no in between. The sawtooth wave has the ascent of a
				triangle wave, but has the decay cut off sharply like a square wave. 
				An inversesawtooth wave reverses this.

		<base>	This is the distance, in game units that the apparent surface of the 
				texture is displaced from the actual surface of the brush as placed 
				in the editor. A positive value appears above the brush surface. 
				A negative value appears below the brush surface. 
				An example of this is the Quad effect, which essentially is a 
				shell with a positive base value to stand it away from the model 
				surface and a 0 (zero) value for amplitude.

		<amplitude> The distance that the deformation moves away from the base value. 
					See Wave Forms in the introduction for a description of amplitude.

		<phase> See Wave Forms in the introduction for a description of phase)

		<frequency> See Wave Forms in the introduction for a description of frequency)

		Design Note: The div and amplitude parameters, when used in conjunction with
		liquid volumes like water should take into consideration how much the water
		will be moving. A large ocean area would have have massive swells (big div values) 
		that rose and fell dramatically (big amplitude values). While a small, quiet pool
		may move very little.
*/
void CQuake3ShaderSceneNode::deformvertexes_bulge( f32 dt, SModifierFunction &function )
{
	function.func = SINUS;
	function.wave = core::reciprocal( function.bulgewidth );

	dt *= function.bulgespeed * 0.1f;
	const f32 phase = function.phase;

	const u32 vsize = Original->Vertices.size();
	for ( u32 i = 0; i != vsize; ++i )
	{
		const video::S3DVertex2TCoords &src = Original->Vertices[i];
		video::S3DVertex &dst = MeshBuffer->Vertices[i];

		const f32 wavephase = (Original->Vertices[i].TCoords.X ) * function.wave;
		function.phase = phase + wavephase;

		const f32 f = function.evaluate( dt );

		if ( 0 == function.count )
			dst.Pos = src.Pos - MeshOffset;

		dst.Pos.X += f * src.Normal.X;
		dst.Pos.Y += f * src.Normal.Y;
		dst.Pos.Z += f * src.Normal.Z;

		if ( i == 0 )
			MeshBuffer->BoundingBox.reset ( dst.Pos );
		else
			MeshBuffer->BoundingBox.addInternalPoint ( dst.Pos );
	}

	function.count = 1;
}

						
/*!
	deformVertexes autosprite

	This function can be used to make any given triangle quad 
	(pair of triangles that form a square rectangle) automatically behave
	like a sprite without having to make it a separate entity. This means 
	that the "sprite" on which the texture is placed will rotate to always 
	appear at right angles to the player's view as a sprite would. Any four-sided
	brush side, flat patch, or pair of triangles in a model can have the autosprite
	effect on it. The brush face containing a texture with this shader keyword must
	be square.
*/
void CQuake3ShaderSceneNode::deformvertexes_autosprite( f32 dt, SModifierFunction &function )
{
	u32 vsize = Original->Vertices.size();
	u32 g;
	u32 i;

	const core::vector3df& camPos = SceneManager->getActiveCamera()->getPosition();

	video::S3DVertex * dv = MeshBuffer->Vertices.pointer();
	const video::S3DVertex2TCoords * vin = Original->Vertices.const_pointer();

	core::matrix4 lookat ( core::matrix4::EM4CONST_NOTHING );
	core::quaternion q;
	for ( i = 0; i < vsize; i += 4 )
	{
		// quad-plane
		core::vector3df center = 0.25f * ( vin[i+0].Pos + vin[i+1].Pos + vin[i+2].Pos + vin[i+3].Pos );
		core::vector3df forward = camPos - center;

		q.rotationFromTo ( vin[i].Normal, forward );
		q.getMatrixCenter ( lookat, center, MeshOffset );

		for ( g = 0; g < 4; ++g )
		{
			lookat.transformVect ( dv[i+g].Pos, vin[i+g].Pos );
			lookat.rotateVect ( dv[i+g].Normal, vin[i+g].Normal );
		}

	}
	function.count = 1;
}


/*!
	deformVertexes autosprite2
	Is a slightly modified "sprite" that only rotates around the middle of its longest axis.
	This allows you to make a pillar of fire that you can walk around, or an energy beam 
	stretched across the room.
*/

struct sortaxis
{
	core::vector3df v;
	bool operator < ( const sortaxis &other ) const
	{
		return v.getLengthSQ () < other.v.getLengthSQ ();
	}
};
/*!
*/
void CQuake3ShaderSceneNode::deformvertexes_autosprite2( f32 dt, SModifierFunction &function )
{
	u32 vsize = Original->Vertices.size();
	u32 g;
	u32 i;

	const core::vector3df camPos = SceneManager->getActiveCamera()->getAbsolutePosition();

	video::S3DVertex * dv = MeshBuffer->Vertices.pointer();
	const video::S3DVertex2TCoords * vin = Original->Vertices.const_pointer();

	core::matrix4 lookat ( core::matrix4::EM4CONST_NOTHING );

	core::array < sortaxis > axis;
	axis.set_used ( 3 );

	for ( i = 0; i < vsize; i += 4 )
	{
		// quad-plane
		core::vector3df center = 0.25f * ( vin[i+0].Pos + vin[i+1].Pos + vin[i+2].Pos + vin[i+3].Pos );

		// longes axe
		axis[0].v = vin[i+1].Pos - vin[i+0].Pos;
		axis[1].v = vin[i+2].Pos - vin[i+0].Pos;
		axis[2].v = vin[i+3].Pos - vin[i+0].Pos;
		axis.set_sorted ( false );
		axis.sort ();

		lookat.buildAxisAlignedBillboard ( camPos, center, MeshOffset, axis[1].v, vin[i+0].Normal );

		for ( g = 0; g < 4; ++g )
		{
			lookat.transformVect ( dv[i+g].Pos, vin[i+g].Pos );
			lookat.rotateVect ( dv[i+g].Normal, vin[i+g].Normal );
		}
	}
	function.count = 1;
}

/*
	Generate Vertex Color
*/
void CQuake3ShaderSceneNode::vertextransform_rgbgen( f32 dt, SModifierFunction &function )
{
	u32 i;
	const u32 vsize = Original->Vertices.size();

	switch ( function.rgbgen )
	{
		case IDENTITY:
			//rgbgen identity
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color.set(0xFFFFFFFF);
			break;

		case IDENTITYLIGHTING:
			// rgbgen identitylighting TODO: overbright
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color.set(0xFF7F7F7F);
			break;

		case EXACTVERTEX:		
			// alphagen exactvertex TODO lighting
		case VERTEX:
			// rgbgen vertex
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color=Original->Vertices[i].Color;
			break;
		case WAVE:
		{
			// rgbGen wave <func> <base> <amp> <phase> <freq>
			f32 f = function.evaluate( dt ) * 255.f;
			s32 value = core::clamp( core::floor32(f), 0, 255 );
			value = 0xFF000000 | value << 16 | value << 8 | value;

			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color.set(value);
		} break;
		case CONSTANT:
		{
			//rgbgen const ( x y z )
			video::SColorf cf( function.x, function.y, function.z );
			video::SColor col = cf.toSColor();
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color=col;
		} break;
		default:
			break;
	}
}

/*
	Generate Vertex Color, Alpha
*/
void CQuake3ShaderSceneNode::vertextransform_alphagen( f32 dt, SModifierFunction &function )
{
	u32 i;
	const u32 vsize = Original->Vertices.size();

	switch ( function.alphagen )
	{
		case IDENTITY:
			//alphagen identity
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color.setAlpha ( 0xFF );
			break;

		case EXACTVERTEX:	
			// alphagen exactvertex TODO lighting
		case VERTEX:
			// alphagen vertex
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color.setAlpha ( Original->Vertices[i].Color.getAlpha() );
			break;
		case CONSTANT:
		{
			// alphagen const
			u32 a = (u32) ( function.x * 255.f );
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color.setAlpha ( a );
		} break;

		case LIGHTINGSPECULAR:
		{
			// alphagen lightingspecular TODO!!!
			const SViewFrustum *frustum = SceneManager->getActiveCamera()->getViewFrustum();
			const core::matrix4 &view = frustum->getTransform ( video::ETS_VIEW );

			const f32 *m = view.pointer();

			for ( i = 0; i != vsize; ++i )
			{
				const core::vector3df &n = Original->Vertices[i].Normal;
				MeshBuffer->Vertices[i].Color.setAlpha ((u32)( 128.f *(1.f+(n.X*m[0]+n.Y*m[1]+n.Z*m[2]))));
			}

		} break;


		case WAVE:
		{
			// alphagen wave
			f32 f = function.evaluate( dt ) * 255.f;
			s32 value = core::clamp( core::floor32(f), 0, 255 );

			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].Color.setAlpha ( value );
		} break;
		default:
			break;
	}
}



/*
	Generate Texture Coordinates
*/
void CQuake3ShaderSceneNode::vertextransform_tcgen( f32 dt, SModifierFunction &function )
{
	u32 i;
	const u32 vsize = Original->Vertices.size();

	switch ( function.tcgen )
	{
		case TURBULENCE:
			//tcgen turb
		{
			function.wave = core::reciprocal( function.phase );

			const f32 phase = function.phase;

			for ( i = 0; i != vsize; ++i )
			{
				const video::S3DVertex2TCoords &src = Original->Vertices[i];
				video::S3DVertex &dst = MeshBuffer->Vertices[i];

				const f32 wavephase = (src.Pos.X + src.Pos.Y + src.Pos.Z) * function.wave;
				function.phase = phase + wavephase;

				const f32 f = function.evaluate( dt );

				dst.TCoords.X = src.TCoords.X + f * src.Normal.X;
				dst.TCoords.Y = src.TCoords.Y + f * src.Normal.Y;
			}
		}
		break;

		case TEXTURE:
			// tcgen texture
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].TCoords = Original->Vertices[i].TCoords;
			break;
		case LIGHTMAP:
			// tcgen lightmap
			for ( i = 0; i != vsize; ++i )
				MeshBuffer->Vertices[i].TCoords = Original->Vertices[i].TCoords2;
			break;
		case ENVIRONMENT:
		{
			// tcgen environment
			const SViewFrustum *frustum = SceneManager->getActiveCamera()->getViewFrustum();
			const core::matrix4 &view = frustum->getTransform ( video::ETS_VIEW );

			const f32 *m = view.pointer();

			core::vector3df n;
			for ( i = 0; i != vsize; ++i )
			{
				//const core::vector3df &n = Original->Vertices[i].Normal;

				n = frustum->cameraPosition - Original->Vertices[i].Pos;
				n.normalize();
				n += Original->Vertices[i].Normal;
				n.normalize();

				MeshBuffer->Vertices[i].TCoords.X = 0.5f*(1.f+(n.X*m[0]+n.Y*m[1]+n.Z*m[2])); 
				MeshBuffer->Vertices[i].TCoords.Y = 0.5f*(1.f+(n.X*m[4]+n.Y*m[5]+n.Z*m[6])); 
			}

		} break;
		default:
			break;
	}
}


#if 0
/*
	Transform Texture Coordinates
*/
void CQuake3ShaderSceneNode::transformtex( const core::matrix4 &m, const u32 addressMode )
{
	u32 i;
	const u32 vsize = MeshBuffer->Vertices.size();

	f32 tx1;
	f32 ty1;

	if ( addressMode )
	{
		for ( i = 0; i != vsize; ++i )
		{
			core::vector2df &tx = MeshBuffer->Vertices[i].TCoords;

			tx1 = m[0] * tx.X + m[4] * tx.Y + m[8];
			ty1 = m[1] * tx.X + m[5] * tx.Y + m[9];

			tx.X = tx1;
			tx.Y = ty1;
		}
	}
	else
	{

		for ( i = 0; i != vsize; ++i )
		{
			core::vector2df &tx = MeshBuffer->Vertices[i].TCoords;

			tx1 = m[0] * tx.X + m[4] * tx.Y + m[8];
			ty1 = m[1] * tx.X + m[5] * tx.Y + m[9];

			tx.X = tx1 <= 0.f ? 0.f : tx1 >= 1.f ? 1.f : tx1;
			tx.Y = ty1 <= 0.f ? 0.f : ty1 >= 1.f ? 1.f : ty1;

			//tx.X = core::clamp( tx1, 0.f, 1.f );
			//tx.Y = core::clamp( ty1, 0.f, 1.f );
		}
	}
}

#endif


/*
	Texture & Vertex Transform Animator

	Return a Texture Transformation for this stage
	Vertex transformation are called if found

*/
void CQuake3ShaderSceneNode::animate( u32 stage,core::matrix4 &texture )
{
	const SVarGroup *group = Shader->getGroup( stage );

	// select current texture
	SQ3Texture &q3Tex = Q3Texture [ stage ];
	if ( q3Tex.TextureFrequency != 0.f )
	{
		s32 v = core::floor32( TimeAbs * q3Tex.TextureFrequency );
		q3Tex.TextureIndex = v % q3Tex.Texture.size();
	}

	core::matrix4 m2;
	SModifierFunction function;

	f32 f[16];

	// walk group for all modifiers
	for ( u32 g = 0; g != group->Variable.size(); ++g )
	{
		const SVariable &v = group->Variable[g];

		// get the modifier 
		static const c8 * modifierList[] =
		{ 
			"tcmod","deformvertexes","rgbgen","tcgen","map","alphagen"
		};

		u32 pos = 0;
		function.masterfunc0 = (eQ3ModifierFunction) isEqual( v.name, pos, modifierList, 6 );

		if ( UNKNOWN == function.masterfunc0 )
			continue;

		switch ( function.masterfunc0 )
		{
			//tcmod
			case TCMOD:
				m2.makeIdentity();
				break;
			default:
				break;
		}

		// get the modifier function
		static const c8 * funclist[] =
		{ 
			"scroll","scale","rotate","stretch","turb",
			"wave","identity","vertex",
			"texture","lightmap","environment","$lightmap",
			"bulge","autosprite","autosprite2","transform",
			"exactvertex","const","lightingspecular","move","normal",
			"identitylighting"
		};
		static const c8 * groupToken[] = { "(", ")" };

		pos = 0;
		function.masterfunc1 = (eQ3ModifierFunction) isEqual( v.content, pos, funclist, 22 );
		if ( function.masterfunc1 != UNKNOWN )
			function.masterfunc1 = (eQ3ModifierFunction) ((u32) function.masterfunc1 + FUNCTION2 + 1);

		switch ( function.masterfunc1 )
		{
			case SCROLL:
				// tcMod scroll <sSpeed> <tSpeed>
				f[0] = getAsFloat( v.content, pos ) * TimeAbs;
				f[1] = getAsFloat( v.content, pos ) * TimeAbs;
				m2.setTextureTranslate( f[0], f[1] );
				break;
			case SCALE:
				// tcmod scale <sScale> <tScale>
				f[0] = getAsFloat( v.content, pos );
				f[1] = getAsFloat( v.content, pos );
				m2.setTextureScale( f[0], f[1] );
				break;
			case ROTATE:
				// tcmod rotate <degress per second>
				m2.setTextureRotationCenter(	getAsFloat( v.content, pos ) * 
												core::DEGTORAD *
												TimeAbs
											);
				break;
			case TRANSFORM:
				// tcMod <transform> <m00> <m01> <m10> <m11> <t0> <t1>
				memset(f, 0, sizeof ( f ));
				f[10] = f[15] = 1.f;

				f[0] = getAsFloat( v.content, pos );
				f[1] = getAsFloat( v.content, pos );
				f[4] = getAsFloat( v.content, pos );
				f[5] = getAsFloat( v.content, pos );
				f[8] = getAsFloat( v.content, pos );
				f[9] = getAsFloat( v.content, pos );
				m2.setM ( f );
				break;

			case STRETCH:	// stretch
			case TURBULENCE: // turb
			case WAVE: // wave
			case IDENTITY: // identity
			case IDENTITYLIGHTING:
			case VERTEX: // vertex
			case MOVE:
			case CONSTANT:
			{
				// turb == sin, default == sin
				function.func = SINUS;

				if ( function.masterfunc0 == DEFORMVERTEXES )
				{
					switch ( function.masterfunc1 )
					{
						case WAVE:
							// deformvertexes wave
							function.wave = getAsFloat( v.content, pos );
							break;
						case MOVE:
							//deformvertexes move
							function.x = getAsFloat( v.content, pos );
							function.z = getAsFloat( v.content, pos );
							function.y = getAsFloat( v.content, pos );
							break;
						default:
							break;
					}
				}

				switch ( function.masterfunc1 )
				{
					case STRETCH:
					case TURBULENCE:
					case WAVE:
					case MOVE:
						getModifierFunc( function, v.content, pos );
						break;
					default:
						break;
				}

				switch ( function.masterfunc1 )
				{
					case STRETCH:
						//tcMod stretch <func> <base> <amplitude> <phase> <frequency>
						f[0] = core::reciprocal( function.evaluate(TimeAbs) );
						m2.setTextureScaleCenter( f[0], f[0] );
						break;
					case TURBULENCE:
						//tcMod turb <base> <amplitude> <phase> <freq>
						//function.tcgen = TURBULENCE;
						m2.setTextureRotationCenter(	function.frequency * 
														core::DEGTORAD * 
														TimeAbs
													);
						break;
					case WAVE:
					case IDENTITY:
					case IDENTITYLIGHTING:
					case VERTEX:
					case EXACTVERTEX:
					case CONSTANT:
					case LIGHTINGSPECULAR:
					case MOVE:
						switch ( function.masterfunc0 )
						{
							case DEFORMVERTEXES:
								switch ( function.masterfunc1 )
								{
									case WAVE:
										deformvertexes_wave( TimeAbs, function );
										break;
									case MOVE:
										deformvertexes_move( TimeAbs, function );
										break;
									default:
										break;
								}
								break;
							case RGBGEN:
								function.rgbgen = function.masterfunc1;
								if ( function.rgbgen == CONSTANT )
								{
									isEqual ( v.content, pos, groupToken, 2 );
									function.x = getAsFloat( v.content, pos );
									function.y = getAsFloat( v.content, pos );
									function.z = getAsFloat( v.content, pos );
								}
								//vertextransform_rgbgen( TimeAbs, function );
								break;
							case ALPHAGEN:
								function.alphagen = function.masterfunc1;
								if ( function.alphagen == CONSTANT )
								{
									function.x = getAsFloat( v.content, pos );
								}

								//vertextransform_alphagen( TimeAbs, function );
								break;
							default:
								break;
						}
						break;
					default:
						break;
				}
				
			} break;
			case TEXTURE:
			case LIGHTMAP:
			case ENVIRONMENT:
				// "texture","lightmap","environment"
				function.tcgen = function.masterfunc1;
				break;
			case DOLLAR_LIGHTMAP:
				// map == lightmap, tcgen == lightmap
				function.tcgen = LIGHTMAP;
				break;
			case BULGE:
				// deformvertexes bulge
				function.bulgewidth = getAsFloat( v.content, pos );
				function.bulgeheight = getAsFloat( v.content, pos );
				function.bulgespeed = getAsFloat( v.content, pos );

				deformvertexes_bulge(TimeAbs, function);
				break;

			case NORMAL:
				// deformvertexes normal
				function.amp = getAsFloat( v.content, pos );
				function.frequency = getAsFloat( v.content, pos );

				deformvertexes_normal(TimeAbs, function);
				break;

			case AUTOSPRITE:
				// deformvertexes autosprite
				deformvertexes_autosprite(TimeAbs, function);
				break;

			case AUTOSPRITE2:
				// deformvertexes autosprite2
				deformvertexes_autosprite2(TimeAbs, function);
				break;
			default:
				break;
		} // func

		switch ( function.masterfunc0 )
		{
			case TCMOD:
				texture *= m2;
				break;
			default:
				break;
		}

	} // group

	vertextransform_rgbgen( TimeAbs, function );
	vertextransform_alphagen( TimeAbs, function );
	vertextransform_tcgen( TimeAbs, function );
}


void CQuake3ShaderSceneNode::OnAnimate(u32 timeMs)
{
	TimeAbs = f32( timeMs ) * (1.f/1000.f);
	ISceneNode::OnAnimate( timeMs );
}

const core::aabbox3d<f32>& CQuake3ShaderSceneNode::getBoundingBox() const
{
	return MeshBuffer->getBoundingBox();
}


u32 CQuake3ShaderSceneNode::getMaterialCount() const
{
	return Q3Texture.size();
}

video::SMaterial& CQuake3ShaderSceneNode::getMaterial(u32 i)
{
	video::SMaterial& m = MeshBuffer->Material;
	m.setTexture(0, 0);
	if ( Q3Texture [ i ].TextureIndex )
		m.setTexture(0, Q3Texture [ i ].Texture [ Q3Texture [ i ].TextureIndex ]);
	return m;
}	


} // end namespace scene
} // end namespace irr

#endif

