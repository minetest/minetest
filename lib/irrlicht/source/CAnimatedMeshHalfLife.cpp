// Copyright (C) 2002-2012 Nikolaus Gebhardt / Fabio Concas / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_HALFLIFE_LOADER_

#include "CAnimatedMeshHalfLife.h"
#include "os.h"
#include "CColorConverter.h"
#include "CImage.h"
#include "coreutil.h"
#include "SMeshBuffer.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"

namespace irr
{
namespace scene
{

	using namespace video;

	void AngleQuaternion(const core::vector3df& angles, vec4_hl quaternion)
	{
		f32		angle;
		f32		sr, sp, sy, cr, cp, cy;

		// FIXME: rescale the inputs to 1/2 angle
		angle = angles.Z * 0.5f;
		sy = sin(angle);
		cy = cos(angle);
		angle = angles.Y * 0.5f;
		sp = sin(angle);
		cp = cos(angle);
		angle = angles.X * 0.5f;
		sr = sin(angle);
		cr = cos(angle);

		quaternion[0] = sr*cp*cy-cr*sp*sy; // X
		quaternion[1] = cr*sp*cy+sr*cp*sy; // Y
		quaternion[2] = cr*cp*sy-sr*sp*cy; // Z
		quaternion[3] = cr*cp*cy+sr*sp*sy; // W
	}

	void QuaternionMatrix( const vec4_hl quaternion, f32 (*matrix)[4] )
	{
		matrix[0][0] = 1.f - 2.f * quaternion[1] * quaternion[1] - 2.f * quaternion[2] * quaternion[2];
		matrix[1][0] = 2.f * quaternion[0] * quaternion[1] + 2.f * quaternion[3] * quaternion[2];
		matrix[2][0] = 2.f * quaternion[0] * quaternion[2] - 2.f * quaternion[3] * quaternion[1];

		matrix[0][1] = 2.f * quaternion[0] * quaternion[1] - 2.f * quaternion[3] * quaternion[2];
		matrix[1][1] = 1.f - 2.f * quaternion[0] * quaternion[0] - 2.f * quaternion[2] * quaternion[2];
		matrix[2][1] = 2.f * quaternion[1] * quaternion[2] + 2.f * quaternion[3] * quaternion[0];

		matrix[0][2] = 2.f * quaternion[0] * quaternion[2] + 2.f * quaternion[3] * quaternion[1];
		matrix[1][2] = 2.f * quaternion[1] * quaternion[2] - 2.f * quaternion[3] * quaternion[0];
		matrix[2][2] = 1.f - 2.f * quaternion[0] * quaternion[0] - 2.f * quaternion[1] * quaternion[1];
	}

	void QuaternionSlerp( const vec4_hl p, vec4_hl q, f32 t, vec4_hl qt )
	{
		s32 i;
		f32 omega, cosom, sinom, sclp, sclq;

		// decide if one of the quaternions is backwards
		f32 a = 0;
		f32 b = 0;
		for (i = 0; i < 4; i++) {
			a += (p[i]-q[i])*(p[i]-q[i]);
			b += (p[i]+q[i])*(p[i]+q[i]);
		}
		if (a > b) {
			for (i = 0; i < 4; i++) {
				q[i] = -q[i];
			}
		}

		cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

		if ((1.f + cosom) > 0.00000001) {
			if ((1.f - cosom) > 0.00000001) {
				omega = acos( cosom );
				sinom = sin( omega );
				sclp = sin( (1.f - t)*omega) / sinom;
				sclq = sin( t*omega ) / sinom;
			}
			else {
				sclp = 1.f - t;
				sclq = t;
			}
			for (i = 0; i < 4; i++) {
				qt[i] = sclp * p[i] + sclq * q[i];
			}
		}
		else {
			qt[0] = -p[1];
			qt[1] = p[0];
			qt[2] = -p[3];
			qt[3] = p[2];
			sclp = sin( (1.f - t) * 0.5f * core::PI);
			sclq = sin( t * 0.5f * core::PI);
			for (i = 0; i < 3; i++) {
				qt[i] = sclp * p[i] + sclq * qt[i];
			}
		}
	}

	void R_ConcatTransforms (const f32 in1[3][4], const f32 in2[3][4], f32 out[3][4])
	{
		out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
		out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
		out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
		out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];
		out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
		out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
		out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
		out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];
		out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
		out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
		out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
		out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
	}

	#define DotProduct(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])

	inline void VectorTransform(const vec3_hl in1, const f32 in2[3][4], core::vector3df& out)
	{
		out.X = DotProduct(in1, in2[0]) + in2[0][3];
		out.Z = DotProduct(in1, in2[1]) + in2[1][3];
		out.Y = DotProduct(in1, in2[2]) + in2[2][3];
	}

	static f32 BoneTransform[MAXSTUDIOBONES][3][4];	// bone transformation matrix

	void getBoneVector ( core::vector3df &out, u32 index )
	{
		out.X = BoneTransform[index][0][3];
		out.Z = BoneTransform[index][1][3];
		out.Y = BoneTransform[index][2][3];
	}

	void getBoneBox ( core::aabbox3df &box, u32 index, f32 size = 0.5f )
	{
		box.MinEdge.X = BoneTransform[index][0][3] - size;
		box.MinEdge.Z = BoneTransform[index][1][3] - size;
		box.MinEdge.Y = BoneTransform[index][2][3] - size;

		size *= 2.f;
		box.MaxEdge.X = box.MinEdge.X + size;
		box.MaxEdge.Y = box.MinEdge.Y + size;
		box.MaxEdge.Z = box.MinEdge.Z + size;
	}

	void getTransformedBoneVector ( core::vector3df &out, u32 index, const vec3_hl in)
	{
		out.X = DotProduct(in, BoneTransform[index][0]) + BoneTransform[index][0][3];
		out.Z = DotProduct(in, BoneTransform[index][1]) + BoneTransform[index][1][3];
		out.Y = DotProduct(in, BoneTransform[index][2]) + BoneTransform[index][2][3];

	}


//! Constructor
CHalflifeMDLMeshFileLoader::CHalflifeMDLMeshFileLoader(
		scene::ISceneManager* smgr) : SceneManager(smgr)
{
#ifdef _DEBUG
	setDebugName("CHalflifeMDLMeshFileLoader");
#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CHalflifeMDLMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension(filename, "mdl");
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CHalflifeMDLMeshFileLoader::createMesh(io::IReadFile* file)
{
	CAnimatedMeshHalfLife* msh = new CAnimatedMeshHalfLife();
	if (msh)
	{
		if (msh->loadModelFile(file, SceneManager))
			return msh;
		msh->drop();
	}

	return 0;
}


//! Constructor
CAnimatedMeshHalfLife::CAnimatedMeshHalfLife()
	: FrameCount(0), MeshIPol(0), SceneManager(0), Header(0), TextureHeader(0),
	OwnTexModel(false), SequenceIndex(0), CurrentFrame(0), FramesPerSecond(25.f),
	SkinGroupSelection(0)
#ifdef HL_TEXTURE_ATLAS
	, TextureMaster(0)
#endif
{
#ifdef _DEBUG
	setDebugName("CAnimatedMeshHalfLife");
#endif
	initData();
}

/*!
	loads a complete model
*/
bool CAnimatedMeshHalfLife::loadModelFile(io::IReadFile* file,
		ISceneManager* smgr)
{
	if (!file)
		return false;

	SceneManager = smgr;

	if ( loadModel(file, file->getFileName()) )
	{
		if ( postLoadModel ( file->getFileName() ) )
		{
			initModel ();
			//dumpModelInfo ( 1 );
			return true;
		}
	}
	return false;
}


//! Destructor
CAnimatedMeshHalfLife::~CAnimatedMeshHalfLife()
{
	delete [] (u8*) Header;
	if (OwnTexModel)
		delete [] (u8*) TextureHeader;

	for (u32 i = 0; i < 32; ++i)
		delete [] (u8*) AnimationHeader[i];

	if (MeshIPol)
		MeshIPol->drop();
}


//! Returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
u32 CAnimatedMeshHalfLife::getFrameCount() const
{
	return FrameCount;
}


//! set the hardware mapping hint, for driver
void CAnimatedMeshHalfLife::setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint,E_BUFFER_TYPE buffer)
{
}


//! flags the meshbuffer as changed, reloads hardware buffers
void CAnimatedMeshHalfLife::setDirty(E_BUFFER_TYPE buffer)
{
}


static core::vector3df TransformedVerts[MAXSTUDIOVERTS];	// transformed vertices
//static core::vector3df TransformedNormals[MAXSTUDIOVERTS];	// light surface normals


/*!
*/
void CAnimatedMeshHalfLife::initModel()
{
	// init Sequences to Animation
	KeyFrameInterpolation ipol;
	ipol.Name.reserve ( 64 );
	u32 i;

	AnimList.clear();
	FrameCount = 0;

	SHalflifeSequence *seq = (SHalflifeSequence*) ((u8*) Header + Header->seqindex);
	for ( i = 0; i < Header->numseq; i++)
	{
		ipol.Name = seq[i].label;
		ipol.StartFrame = FrameCount;
		ipol.Frames = core::max_ ( 1, seq[i].numframes - 1 );
		ipol.EndFrame = ipol.StartFrame + ipol.Frames - 1;
		ipol.FramesPerSecond = seq[i].fps;
		ipol.AnimationType = seq[i].flags & STUDIO_LOOPING ? EAMT_LOOPING : EAMT_WAYPOINT;
		AnimList.push_back ( ipol );

		FrameCount += ipol.Frames;
	}

	// initBoneControllers
/*
	SHalflifeBoneController *bonecontroller = (SHalflifeBoneController *)((u8*) Header + Header->bonecontrollerindex);
	for ( i = 0; i < Header->numbonecontrollers; i++)
	{
		printf ( "BoneController%d index:%d%s range:%f - %f\n",
			i,
			bonecontroller[i].index, bonecontroller[i].index == MOUTH_CONTROLLER ? " (Mouth)": "",
			bonecontroller[i].start,bonecontroller[i].end
			);
	}

	// initSkins
	for (i = 0; i < TextureHeader->numskinfamilies; i++)
	{
		printf ( "Skin%d\n", i + 1);
	}
*/

	// initBodyparts
	u32 meshBuffer = 0;
	BodyList.clear();
	SHalflifeBody *body = (SHalflifeBody *) ((u8*) Header + Header->bodypartindex);
	for (i=0; i < Header->numbodyparts; ++i)
	{
		BodyPart part;
		part.name = body[i].name;
		part.defaultModel = core::max_ ( 0, (s32) body[i].base - 1 );

		SHalflifeModel * model = (SHalflifeModel *)((u8*) Header + body[i].modelindex);
		for ( u32 g = 0; g < body[i].nummodels; ++g)
		{
			SubModel sub;
			sub.name = model[g].name;
			sub.startBuffer = meshBuffer;
			sub.endBuffer = sub.startBuffer + model[g].nummesh;
			sub.state = g == part.defaultModel;
			part.model.push_back ( sub );
			meshBuffer += model[g].nummesh;
		}
		BodyList.push_back ( part );
	}

	SequenceIndex = 0;
	CurrentFrame = 0.f;

	SetController(0, 0.f);
	SetController(1, 0.f);
	SetController(2, 0.f);
	SetController(3, 0.f);
	SetController(MOUTH_CONTROLLER, 0.f);

	SetSkin (0);

	// init Meshbuffers
	const SHalflifeTexture *tex = (SHalflifeTexture *) ((u8*) TextureHeader + TextureHeader->textureindex);
	const u16 *skinref = (u16 *)((u8*)TextureHeader + TextureHeader->skinindex);
	if ((SkinGroupSelection != 0) && (SkinGroupSelection < TextureHeader->numskinfamilies))
		skinref += (SkinGroupSelection * TextureHeader->numskinref);

	core::vector2df tex_scale;
	core::vector2di tex_trans ( 0, 0 );

#ifdef HL_TEXTURE_ATLAS
	TextureAtlas.getScale(tex_scale);
#endif

	for (u32 bodypart=0 ; bodypart < Header->numbodyparts ; ++bodypart)
	{
		const SHalflifeBody *body = (SHalflifeBody *)((u8*) Header + Header->bodypartindex) + bodypart;

		for (u32 modelnr = 0; modelnr < body->nummodels; ++modelnr)
		{
			const SHalflifeModel *model = (SHalflifeModel *)((u8*) Header + body->modelindex) + modelnr;
#if 0
			const vec3_hl *studioverts = (vec3_hl *)((u8*)Header + model->vertindex);
			const vec3_hl *studionorms = (vec3_hl *)((u8*)Header + model->normindex);
#endif
			for (i = 0; i < model->nummesh; ++i)
			{
				const SHalflifeMesh *mesh = (SHalflifeMesh *)((u8*)Header + model->meshindex) + i;
				const SHalflifeTexture *currentex = &tex[skinref[mesh->skinref]];

#ifdef HL_TEXTURE_ATLAS
				TextureAtlas.getTranslation ( currentex->name, tex_trans );
#else
				tex_scale.X = 1.f/(f32)currentex->width;
				tex_scale.Y = 1.f/(f32)currentex->height;
#endif

				SMeshBuffer * buffer = new SMeshBuffer();

				// count index vertex size	indexcount = mesh->numtris * 3
				u32 indexCount = 0;
				u32 vertexCount = 0;

				const s16 *tricmd = (s16*)((u8*)Header + mesh->triindex);
				s32 c;
				while ( (c = *(tricmd++)) )
				{
					if (c < 0)
						c = -c;

					indexCount += ( c - 2 ) * 3;
					vertexCount += c;
					tricmd += ( 4 * c );
				}

				// indices
				buffer->Indices.set_used ( indexCount );
				buffer->Vertices.set_used ( vertexCount );

				// fill in static indices and vertex
				u16 *index = buffer->Indices.pointer();
				video::S3DVertex * v = buffer->Vertices.pointer();

				// blow up gl_triangle_fan/gl_triangle_strip to indexed triangle list
				E_PRIMITIVE_TYPE type;
				vertexCount = 0;
				indexCount = 0;
				tricmd = (s16*)((u8*)Header + mesh->triindex);
				while ( (c = *(tricmd++)) )
				{
					if (c < 0)
					{
						// triangle fan
						c = -c;
						type = EPT_TRIANGLE_FAN;
					}
					else
					{
						type = EPT_TRIANGLE_STRIP;
					}

					for ( s32 g = 0; g < c; ++g, v += 1, tricmd += 4 )
					{
						// fill vertex
	#if 0
						const f32 *av = studioverts[tricmd[0]];
						v->Pos.X = av[0];
						v->Pos.Z = av[1];
						v->Pos.Y = av[2];

						av = studionorms[tricmd[1]];
						v->Normal.X = av[0];
						v->Normal.Z = av[1];
						v->Normal.Y = av[2];

	#endif
						v->Normal.X = 0.f;
						v->Normal.Z = 0.f;
						v->Normal.Y = 1.f;

						v->TCoords.X = (tex_trans.X + tricmd[2])*tex_scale.X;
						v->TCoords.Y = (tex_trans.Y + tricmd[3])*tex_scale.Y;

						v->Color.color = 0xFFFFFFFF;

						// fill index
						if ( g < c - 2 )
						{
							if ( type == EPT_TRIANGLE_FAN )
							{
								index[indexCount+0] = vertexCount;
								index[indexCount+1] = vertexCount+g+1;
								index[indexCount+2] = vertexCount+g+2;
							}
							else
							{
								if ( g & 1 )
								{
									index[indexCount+0] = vertexCount+g+1;
									index[indexCount+1] = vertexCount+g+0;
									index[indexCount+2] = vertexCount+g+2;
								}
								else
								{
									index[indexCount+0] = vertexCount+g+0;
									index[indexCount+1] = vertexCount+g+1;
									index[indexCount+2] = vertexCount+g+2;
								}
							}

							indexCount += 3;
						}
					}

					vertexCount += c;
				}

				// material
				video::SMaterial &m = buffer->getMaterial();

				m.MaterialType = video::EMT_SOLID;
				m.BackfaceCulling = true;

				if ( currentex->flags & STUDIO_NF_CHROME )
				{
					// don't know what to do with chrome here
				}

#ifdef HL_TEXTURE_ATLAS
				io::path store = TextureBaseName + "atlas";
#else
				io::path fname;
				io::path ext;
				core::splitFilename ( currentex->name, 0, &fname, &ext );
				io::path store = TextureBaseName + fname;
#endif
				m.TextureLayer[0].Texture = SceneManager->getVideoDriver()->getTexture ( store );
				m.Lighting = false;

				MeshIPol->addMeshBuffer(buffer);
				buffer->recalculateBoundingBox();
				buffer->drop();
			} // mesh
			MeshIPol->recalculateBoundingBox();
		} // model
	} // body part
}


/*!
*/
void CAnimatedMeshHalfLife::buildVertices()
{
/*
	const u16 *skinref = (u16 *)((u8*)TextureHeader + TextureHeader->skinindex);
	if (SkinGroupSelection != 0 && SkinGroupSelection < TextureHeader->numskinfamilies)
		skinref += (SkinGroupSelection * TextureHeader->numskinref);
*/
	u32 i;
	s32 c,g;
	const s16 *tricmd;

	u32 meshBufferNr = 0;
	for ( u32 bodypart = 0 ; bodypart < Header->numbodyparts; ++bodypart)
	{
		const SHalflifeBody *body = (SHalflifeBody *)((u8*) Header + Header->bodypartindex) + bodypart;

		for ( u32 modelnr = 0; modelnr < body->nummodels; ++modelnr )
		{
			const SHalflifeModel *model = (SHalflifeModel *)((u8*) Header + body->modelindex) + modelnr;

			const u8 *vertbone = ((u8*)Header + model->vertinfoindex);

			const vec3_hl *studioverts = (vec3_hl *)((u8*)Header + model->vertindex);

			for ( i = 0; i < model->numverts; i++)
			{
				VectorTransform ( studioverts[i],  BoneTransform[vertbone[i]], TransformedVerts[i]  );
			}
	/*
			const u8 *normbone = ((u8*)Header + model->norminfoindex);
			const vec3_hl *studionorms = (vec3_hl *)((u8*)Header + model->normindex);
			for ( i = 0; i < model->numnorms; i++)
			{
				VectorTransform ( studionorms[i],  BoneTransform[normbone[i]], TransformedNormals[i]  );
			}
	*/
			for (i = 0; i < model->nummesh; i++)
			{
				const SHalflifeMesh *mesh = (SHalflifeMesh *)((u8*)Header + model->meshindex) + i;

				IMeshBuffer * buffer = MeshIPol->getMeshBuffer ( meshBufferNr++ );
				video::S3DVertex* v = (video::S3DVertex* ) buffer->getVertices();

				tricmd = (s16*)((u8*)Header + mesh->triindex);
				while ( (c = *(tricmd++)) )
				{
					if (c < 0)
						c = -c;

					for ( g = 0; g < c; ++g, v += 1, tricmd += 4 )
					{
						// fill vertex
						const core::vector3df& av = TransformedVerts[tricmd[0]];
						v->Pos = av;
	/*
						const core::vector3df& an = TransformedNormals[tricmd[1]];
						v->Normal = an;
						//v->Normal.normalize();
	*/
					}
				} // tricmd
			} // nummesh
		} // model
	} // bodypart
}


/*!
	render Bones
*/
void CAnimatedMeshHalfLife::renderModel(u32 param, IVideoDriver * driver, const core::matrix4 &absoluteTransformation)
{
	SHalflifeBone *bone = (SHalflifeBone *) ((u8 *) Header + Header->boneindex);

	video::SColor blue(0xFF000080);
	video::SColor red(0xFF800000);
	video::SColor yellow(0xFF808000);
	video::SColor cyan(0xFF008080);

	core::aabbox3df box;

	u32 i;
	for ( i = 0; i < Header->numbones; i++)
	{
		if (bone[i].parent >= 0)
		{
			getBoneVector ( box.MinEdge, bone[i].parent );
			getBoneVector ( box.MaxEdge, i );
			driver->draw3DLine ( box.MinEdge, box.MaxEdge, blue );

			// draw parent bone node
			if (bone[bone[i].parent].parent >=0 )
			{
				getBoneBox ( box, bone[i].parent );
				driver->draw3DBox ( box, blue );
			}
			getBoneBox ( box, i );
			driver->draw3DBox ( box, blue );
		}
		else
		{
			// draw parent bone node
			getBoneBox ( box, i, 1.f );
			driver->draw3DBox ( box , red );
		}
	}

	// attachements
	SHalflifeAttachment *attach = (SHalflifeAttachment *) ((u8*) Header + Header->attachmentindex);
	core::vector3df v[8];
	for ( i = 0; i < Header->numattachments; i++)
	{
		getTransformedBoneVector ( v[0],attach[i].bone,attach[i].org );
		getTransformedBoneVector ( v[1],attach[i].bone,attach[i].vectors[0] );
		getTransformedBoneVector ( v[2],attach[i].bone,attach[i].vectors[1] );
		getTransformedBoneVector ( v[3],attach[i].bone,attach[i].vectors[2] );
		driver->draw3DLine ( v[0], v[1], cyan );
		driver->draw3DLine ( v[0], v[2], cyan );
		driver->draw3DLine ( v[0], v[3], cyan );
	}

	// hit boxes
	SHalflifeBBox *hitbox = (SHalflifeBBox *) ((u8*) Header + Header->hitboxindex);
	f32 *bbmin,*bbmax;
	vec3_hl v2[8];
	for (i = 0; i < Header->numhitboxes; i++)
	{
		bbmin = hitbox[i].bbmin;
		bbmax = hitbox[i].bbmax;

		v2[0][0] = bbmin[0];
		v2[0][1] = bbmax[1];
		v2[0][2] = bbmin[2];

		v2[1][0] = bbmin[0];
		v2[1][1] = bbmin[1];
		v2[1][2] = bbmin[2];

		v2[2][0] = bbmax[0];
		v2[2][1] = bbmax[1];
		v2[2][2] = bbmin[2];

		v2[3][0] = bbmax[0];
		v2[3][1] = bbmin[1];
		v2[3][2] = bbmin[2];

		v2[4][0] = bbmax[0];
		v2[4][1] = bbmax[1];
		v2[4][2] = bbmax[2];

		v2[5][0] = bbmax[0];
		v2[5][1] = bbmin[1];
		v2[5][2] = bbmax[2];

		v2[6][0] = bbmin[0];
		v2[6][1] = bbmax[1];
		v2[6][2] = bbmax[2];

		v2[7][0] = bbmin[0];
		v2[7][1] = bbmin[1];
		v2[7][2] = bbmax[2];

		for ( u32 g = 0; g < 8; ++g )
			getTransformedBoneVector ( v[g],hitbox[i].bone,v2[g] );

		driver->draw3DLine(v[0], v[1], yellow);
		driver->draw3DLine(v[1], v[3], yellow);
		driver->draw3DLine(v[3], v[2], yellow);
		driver->draw3DLine(v[2], v[0], yellow);

		driver->draw3DLine(v[4], v[5], yellow);
		driver->draw3DLine(v[5], v[7], yellow);
		driver->draw3DLine(v[7], v[6], yellow);
		driver->draw3DLine(v[6], v[4], yellow);

		driver->draw3DLine(v[0], v[6], yellow);
		driver->draw3DLine(v[1], v[7], yellow);
		driver->draw3DLine(v[3], v[5], yellow);
		driver->draw3DLine(v[2], v[4], yellow);
	}
}


//! Returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail.
IMesh* CAnimatedMeshHalfLife::getMesh(s32 frameInt, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop)
{
	f32 frame = frameInt + (detailLevel * 0.001f);
	u32 frameA = core::floor32 ( frame );
//	f32 blend = core::fract ( frame );

	SHalflifeSequence *seq = (SHalflifeSequence*) ((u8*) Header + Header->seqindex);

	// find SequenceIndex from summed list
	u32 frameCount = 0;
	for (u32 i = 0; i < Header->numseq; ++i)
	{
		u32 val = core::max_ ( 1, seq[i].numframes - 1 );
		if ( frameCount + val > frameA  )
		{
			SequenceIndex = i;
			CurrentFrame = frame - frameCount;
			break;
		}
		frameCount += val;
	}

	seq += SequenceIndex;

	//SetBodyPart ( 1, 1 );
	setUpBones ();
	buildVertices();

	MeshIPol->BoundingBox.MinEdge.X = seq->bbmin[0];
	MeshIPol->BoundingBox.MinEdge.Z = seq->bbmin[1];
	MeshIPol->BoundingBox.MinEdge.Y = seq->bbmin[2];

	MeshIPol->BoundingBox.MaxEdge.X = seq->bbmax[0];
	MeshIPol->BoundingBox.MaxEdge.Z = seq->bbmax[1];
	MeshIPol->BoundingBox.MaxEdge.Y = seq->bbmax[2];

	return MeshIPol;
}


/*!
*/
void CAnimatedMeshHalfLife::initData ()
{
	u32 i;

	Header = 0;
	TextureHeader = 0;
	OwnTexModel = false;

	for ( i = 0; i < 32; ++i )
		AnimationHeader[i] = 0;

	SequenceIndex = 0;
	CurrentFrame = 0.f;

	for ( i = 0; i < 5; ++i )
		BoneController[i] = 0;

	for ( i = 0; i < 2; ++i )
		Blending[i] = 0;

	SkinGroupSelection = 0;

	AnimList.clear();
	FrameCount = 0;

	if (!MeshIPol)
		MeshIPol = new SMesh();
	MeshIPol->clear();

#ifdef HL_TEXTURE_ATLAS
	TextureAtlas.release();
#endif
}


/*!
*/
void STextureAtlas::release()
{
	for (u32 i = 0; i < atlas.size(); i++)
	{
		if ( atlas[i].image )
		{
			atlas[i].image->drop();
			atlas[i].image = 0;
		}
	}
	Master = 0;
}


/*!
*/
void STextureAtlas::addSource ( const c8 * name, video::IImage * image )
{
	TextureAtlasEntry entry;
	entry.name = name;
	entry.image = image;
	entry.width = image->getDimension().Width;
	entry.height = image->getDimension().Height;
	entry.pos.X = 0;
	entry.pos.Y = 0;
	atlas.push_back ( entry );
}


/*!
*/
void STextureAtlas::getScale(core::vector2df& scale)
{
	for (s32 i = static_cast<s32>(atlas.size()) - 1; i >= 0; --i)
	{
		if ( atlas[i].name == "_merged_" )
		{
			scale.X = 1.f / atlas[i].width;
			scale.Y = 1.f / atlas[i].height;
			return;
		}
	}
	scale.X = 1.f;
	scale.Y = 1.f;
}


/*!
*/
void STextureAtlas::getTranslation(const c8* name, core::vector2di& pos)
{
	for ( u32 i = 0; i < atlas.size(); ++i)
	{
		if ( atlas[i].name == name )
		{
			pos = atlas[i].pos;
			return;
		}
	}
}


/*!
*/
void STextureAtlas::create(u32 border, E_TEXTURE_CLAMP texmode)
{
	u32 i = 0;
	u32 w = 0;
	u32 w2;
	u32 h2;
	u32 h;
	u32 wsum;
	u32 hsum = 0;
	ECOLOR_FORMAT format = ECF_R8G8B8;

	const s32 frame = core::s32_max ( 0, (border - 1 ) / 2 );

	// sort for biggest coming first
	atlas.sort();

	// split size
	wsum = frame;
	for (i = 0; i < atlas.size(); i++)
	{
		// make space
		w2 = atlas[i].width + border;

		// align
		w2 = (w2 + 1) & ~1;
		wsum += w2;
	}
	u32 splitsize = 256;
	if ( wsum > 512 )
		splitsize = 512;

	wsum = frame;
	hsum = frame;
	w = frame;
	h = 0;
	for (i = 0; i < atlas.size(); i++)
	{
		if ( atlas[i].image->getColorFormat() == ECF_A8R8G8B8 )
		{
			format = ECF_A8R8G8B8;
		}

		// make space
		w2 = atlas[i].width + border;
		h2 = atlas[i].height + border;

		// align
		w2 = (w2 + 1) & ~1;
		h2 = (h2 + 1) & ~1;

		h = core::s32_max ( h, h2 );

		if ( w + w2 >= splitsize )
		{
			hsum += h;
			wsum = core::s32_max ( wsum, w );
			h = h2;
			w = frame;
		}

		atlas[i].pos.X = w;
		atlas[i].pos.Y = hsum;

		w += w2;

	}
	hsum += h;
	wsum = core::s32_max ( wsum, w );

	// build image
	core::dimension2d<u32> dim = core::dimension2d<u32>( wsum, hsum ).getOptimalSize();
	IImage* master = new CImage(format, dim);
	master->fill(0);

	video::SColor col[2];

	static const u8 wrap[][4] =
	{
		{1, 0},	// ETC_REPEAT
		{0, 1},	// ETC_CLAMP
		{0, 1},	// ETC_CLAMP_TO_EDGE
		{0, 1}	// ETC_MIRROR
	};

	s32 a,b;
	for (i = 0; i < atlas.size(); i++)
	{
		atlas[i].image->copyTo ( master, atlas[i].pos );

		// clamp/wrap ( copy edges, filtering needs it )

		for ( b = 0; b < frame; ++b )
		{
			for ( a = 0 - b; a <= (s32) atlas[i].width + b; ++a )
			{
				col[0] = atlas[i].image->getPixel ( core::s32_clamp ( a, 0, atlas[i].width - 1 ), 0 );
				col[1] = atlas[i].image->getPixel ( core::s32_clamp ( a, 0, atlas[i].width - 1 ), atlas[i].height - 1 );

				master->setPixel ( atlas[i].pos.X + a, atlas[i].pos.Y + ( b + 1 ) * -1, col[wrap[texmode][0]] );
				master->setPixel ( atlas[i].pos.X + a, atlas[i].pos.Y + atlas[i].height - 1 + ( b + 1 ) * 1, col[wrap[texmode][1]] );
			}

			for ( a = -1 - b; a <= (s32) atlas[i].height + b; ++a )
			{
				col[0] = atlas[i].image->getPixel ( 0, core::s32_clamp ( a, 0, atlas[i].height - 1 ) );
				col[1] = atlas[i].image->getPixel ( atlas[i].width - 1, core::s32_clamp ( a, 0, atlas[i].height - 1 ) );

				master->setPixel ( atlas[i].pos.X + ( b + 1 ) * -1, atlas[i].pos.Y + a, col[wrap[texmode][0]] );
				master->setPixel ( atlas[i].pos.X + atlas[i].width + b, atlas[i].pos.Y + a, col[wrap[texmode][1]] );
			}
		}
	}

	addSource ( "_merged_", master );
	Master = master;
}


/*!
*/
SHalflifeHeader* CAnimatedMeshHalfLife::loadModel(io::IReadFile* file, const io::path& filename)
{
	bool closefile = false;

	// if secondary files are needed, open here and mark for closing
	if ( 0 == file )
	{
		file = SceneManager->getFileSystem()->createAndOpenFile(filename);
		closefile = true;
	}

	if ( 0 == file )
		return 0;

	// read into memory
	u8* pin = new u8[file->getSize()];
	file->read(pin, file->getSize());

	SHalflifeHeader* header = (SHalflifeHeader*) pin;

	const bool idst = (0 == strncmp(header->id, "IDST", 4));
	const bool idsq = (0 == strncmp(header->id, "IDSQ", 4));

	if ( (!idst && !idsq) || (idsq && !Header) )
	{
		os::Printer::log("MDL Halflife Loader: Wrong file header", file->getFileName(), ELL_WARNING);
		if ( closefile )
		{
			file->drop();
			file = 0;
		}
		delete [] pin;
		return 0;
	}

	// don't know the real header.. idsg might be different
	if (header->textureindex && idst )
	{
		io::path path;
		io::path fname;
		io::path ext;

		core::splitFilename(file->getFileName(), &path, &fname, &ext);
		TextureBaseName = path + fname + "_";

		SHalflifeTexture *tex = (SHalflifeTexture *)(pin + header->textureindex);
		u32 *palette = new u32[256];
		for (u32 i = 0; i < header->numtextures; ++i)
		{
			const u8 *src = pin + tex[i].index;

			// convert rgb to argb palette
			{
				const u8 *pal = src + tex[i].width * tex[i].height;
				for( u32 g=0; g<256; ++g )
				{
					palette[g] = 0xFF000000 | pal[0] << 16 | pal[1] << 8 | pal[2];
					pal += 3;
				}
			}

			IImage* image = SceneManager->getVideoDriver()->createImage(ECF_R8G8B8, core::dimension2d<u32>(tex[i].width, tex[i].height));

			CColorConverter::convert8BitTo24Bit(src, (u8*)image->lock(), tex[i].width, tex[i].height, (u8*) palette, 0, false);
			image->unlock();

#ifdef HL_TEXTURE_ATLAS
			TextureAtlas.addSource ( tex[i].name, image );
#else
			core::splitFilename ( tex[i].name, 0, &fname, &ext );
			SceneManager->getVideoDriver()->addTexture ( TextureBaseName + fname, image );
			image->drop();
#endif
		}
		delete [] palette;

#ifdef HL_TEXTURE_ATLAS
		TextureAtlas.create ( 2 * 2 + 1, ETC_CLAMP );
		SceneManager->getVideoDriver()->addTexture ( TextureBaseName + "atlas", TextureAtlas.Master );
		TextureAtlas.release();
#endif
	}

	if (!Header)
		Header = header;

	if ( closefile )
	{
		file->drop();
		file = 0;
	}

	return header;
}


/*!
*/
f32 CAnimatedMeshHalfLife::SetController( s32 controllerIndex, f32 value )
{
	if (!Header)
		return 0.f;

	SHalflifeBoneController *bonecontroller = (SHalflifeBoneController *)((u8*) Header + Header->bonecontrollerindex);

	// find first controller that matches the index
	u32 i;
	for (i = 0; i < Header->numbonecontrollers; i++, bonecontroller++)
	{
		if (bonecontroller->index == controllerIndex)
			break;
	}
	if (i >= Header->numbonecontrollers)
		return value;

	// wrap 0..360 if it's a rotational controller
	if (bonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (bonecontroller->end < bonecontroller->start)
			value = -value;

		// does the controller not wrap?
		if (bonecontroller->start + 359.f >= bonecontroller->end)
		{
			if (value > ((bonecontroller->start + bonecontroller->end) / 2.f) + 180.f)
				value = value - 360.f;
			if (value < ((bonecontroller->start + bonecontroller->end) / 2.f) - 180.f)
				value = value + 360.f;
		}
		else
		{
			if (value > 360.f)
				value = value - (s32)(value / 360.f) * 360.f;
			else if (value < 0.f)
				value = value + (s32)((value / -360.f) + 1) * 360.f;
		}
	}

	s32 range = controllerIndex == MOUTH_CONTROLLER ? 64 : 255;

	s32 setting = (s32) ( (f32) range * (value - bonecontroller->start) / (bonecontroller->end - bonecontroller->start));

	if (setting < 0) setting = 0;
	if (setting > range) setting = range;

	BoneController[controllerIndex] = setting;

	return setting * (1.f / (f32) range ) * (bonecontroller->end - bonecontroller->start) + bonecontroller->start;
}


/*!
*/
u32 CAnimatedMeshHalfLife::SetSkin( u32 value )
{
	if (value < Header->numskinfamilies)
		SkinGroupSelection = value;

	return SkinGroupSelection;
}


/*!
*/
bool CAnimatedMeshHalfLife::postLoadModel( const io::path &filename )
{
	io::path path;
	io::path texname;
	io::path submodel;

	core::splitFilename ( filename ,&path, &texname, 0 );

	// preload textures
	// if no textures are stored in main file, use texfile
	if (Header->numtextures == 0)
	{
		submodel = path + texname + "T.mdl";
		TextureHeader = loadModel(0, submodel);
		if (!TextureHeader)
			return false;
		OwnTexModel = true;
	}
	else
	{
		TextureHeader = Header;
		OwnTexModel = false;
	}

	// preload animations
	if (Header->numseqgroups > 1)
	{
		c8 seq[8];
		for (u32 i = 1; i < Header->numseqgroups; i++)
		{
			snprintf( seq, 8, "%02d.mdl", i );
			submodel = path + texname + seq;

			AnimationHeader[i] = loadModel(0, submodel);
			if (!AnimationHeader[i])
				return false;
		}
	}

	return true;
}


/*!
*/
void CAnimatedMeshHalfLife::dumpModelInfo(u32 level) const
{
	const u8 *phdr = (const u8*) Header;
	const SHalflifeHeader * hdr = Header;
	u32 i;

	if (level == 0)
	{
		printf (
			"Bones: %d\n"
			"Bone Controllers: %d\n"
			"Hit Boxes: %d\n"
			"Sequences: %d\n"
			"Sequence Groups: %d\n",
			hdr->numbones,
			hdr->numbonecontrollers,
			hdr->numhitboxes,
			hdr->numseq,
			hdr->numseqgroups
			);
		printf (
			"Textures: %d\n"
			"Skin Families: %d\n"
			"Bodyparts: %d\n"
			"Attachments: %d\n"
			"Transitions: %d\n",
			hdr->numtextures,
			hdr->numskinfamilies,
			hdr->numbodyparts,
			hdr->numattachments,
			hdr->numtransitions);
		return;
	}

	printf("id: %c%c%c%c\n", phdr[0], phdr[1], phdr[2], phdr[3]);
	printf("version: %d\n", hdr->version);
	printf("name: \"%s\"\n", hdr->name);
	printf("length: %d\n\n", hdr->length);

	printf("eyeposition: %f %f %f\n", hdr->eyeposition[0], hdr->eyeposition[1], hdr->eyeposition[2]);
	printf("min: %f %f %f\n", hdr->min[0], hdr->min[1], hdr->min[2]);
	printf("max: %f %f %f\n", hdr->max[0], hdr->max[1], hdr->max[2]);
	printf("bbmin: %f %f %f\n", hdr->bbmin[0], hdr->bbmin[1], hdr->bbmin[2]);
	printf("bbmax: %f %f %f\n", hdr->bbmax[0], hdr->bbmax[1], hdr->bbmax[2]);

	printf("flags: %d\n\n", hdr->flags);

	printf("numbones: %d\n", hdr->numbones);
	for (i = 0; i < hdr->numbones; i++)
	{
		const SHalflifeBone *bone = (const SHalflifeBone *) (phdr + hdr->boneindex);
		printf("bone %d.name: \"%s\"\n", i + 1, bone[i].name);
		printf("bone %d.parent: %d\n", i + 1, bone[i].parent);
		printf("bone %d.flags: %d\n", i + 1, bone[i].flags);
		printf("bone %d.bonecontroller: %d %d %d %d %d %d\n", i + 1, bone[i].bonecontroller[0], bone[i].bonecontroller[1], bone[i].bonecontroller[2], bone[i].bonecontroller[3], bone[i].bonecontroller[4], bone[i].bonecontroller[5]);
		printf("bone %d.value: %f %f %f %f %f %f\n", i + 1, bone[i].value[0], bone[i].value[1], bone[i].value[2], bone[i].value[3], bone[i].value[4], bone[i].value[5]);
		printf("bone %d.scale: %f %f %f %f %f %f\n", i + 1, bone[i].scale[0], bone[i].scale[1], bone[i].scale[2], bone[i].scale[3], bone[i].scale[4], bone[i].scale[5]);
	}

	printf("\nnumbonecontrollers: %d\n", hdr->numbonecontrollers);
	const SHalflifeBoneController *bonecontrollers = (const SHalflifeBoneController *) (phdr + hdr->bonecontrollerindex);
	for (i = 0; i < hdr->numbonecontrollers; i++)
	{
		printf("bonecontroller %d.bone: %d\n", i + 1, bonecontrollers[i].bone);
		printf("bonecontroller %d.type: %d\n", i + 1, bonecontrollers[i].type);
		printf("bonecontroller %d.start: %f\n", i + 1, bonecontrollers[i].start);
		printf("bonecontroller %d.end: %f\n", i + 1, bonecontrollers[i].end);
		printf("bonecontroller %d.rest: %d\n", i + 1, bonecontrollers[i].rest);
		printf("bonecontroller %d.index: %d\n", i + 1, bonecontrollers[i].index);
	}

	printf("\nnumhitboxes: %d\n", hdr->numhitboxes);
	const SHalflifeBBox *box = (const SHalflifeBBox *) (phdr + hdr->hitboxindex);
	for (i = 0; i < hdr->numhitboxes; i++)
	{
		printf("hitbox %d.bone: %d\n", i + 1, box[i].bone);
		printf("hitbox %d.group: %d\n", i + 1, box[i].group);
		printf("hitbox %d.bbmin: %f %f %f\n", i + 1, box[i].bbmin[0], box[i].bbmin[1], box[i].bbmin[2]);
		printf("hitbox %d.bbmax: %f %f %f\n", i + 1, box[i].bbmax[0], box[i].bbmax[1], box[i].bbmax[2]);
	}

	printf("\nnumseq: %d\n", hdr->numseq);
	const SHalflifeSequence *seq = (const SHalflifeSequence *) (phdr + hdr->seqindex);
	for (i = 0; i < hdr->numseq; i++)
	{
		printf("seqdesc %d.label: \"%s\"\n", i + 1, seq[i].label);
		printf("seqdesc %d.fps: %f\n", i + 1, seq[i].fps);
		printf("seqdesc %d.flags: %d\n", i + 1, seq[i].flags);
		printf("<...>\n");
	}

	printf("\nnumseqgroups: %d\n", hdr->numseqgroups);
	for (i = 0; i < hdr->numseqgroups; i++)
	{
		const SHalflifeSequenceGroup *group = (const SHalflifeSequenceGroup *) (phdr + hdr->seqgroupindex);
		printf("\nseqgroup %d.label: \"%s\"\n", i + 1, group[i].label);
		printf("\nseqgroup %d.namel: \"%s\"\n", i + 1, group[i].name);
		printf("\nseqgroup %d.data: %d\n", i + 1, group[i].data);
	}

	printf("\nnumskinref: %d\n", hdr->numskinref);
	printf("numskinfamilies: %d\n", hdr->numskinfamilies);

	printf("\nnumbodyparts: %d\n", hdr->numbodyparts);
	const SHalflifeBody *pbodyparts = (const SHalflifeBody*) ((const u8*) hdr + hdr->bodypartindex);
	for (i = 0; i < hdr->numbodyparts; i++)
	{
		printf("bodypart %d.name: \"%s\"\n", i + 1, pbodyparts[i].name);
		printf("bodypart %d.nummodels: %d\n", i + 1, pbodyparts[i].nummodels);
		printf("bodypart %d.base: %d\n", i + 1, pbodyparts[i].base);
		printf("bodypart %d.modelindex: %d\n", i + 1, pbodyparts[i].modelindex);
	}

	printf("\nnumattachments: %d\n", hdr->numattachments);
	for (i = 0; i < hdr->numattachments; i++)
	{
		const SHalflifeAttachment *attach = (const SHalflifeAttachment *) ((const u8*) hdr + hdr->attachmentindex);
		printf("attachment %d.name: \"%s\"\n", i + 1, attach[i].name);
	}

	hdr = TextureHeader;
	printf("\nnumtextures: %d\n", hdr->numtextures);
	printf("textureindex: %d\n", hdr->textureindex);
	printf("texturedataindex: %d\n", hdr->texturedataindex);
	const SHalflifeTexture *ptextures = (const SHalflifeTexture *) ((const u8*) hdr + hdr->textureindex);
	for (i = 0; i < hdr->numtextures; i++)
	{
		printf("texture %d.name: \"%s\"\n", i + 1, ptextures[i].name);
		printf("texture %d.flags: %d\n", i + 1, ptextures[i].flags);
		printf("texture %d.width: %d\n", i + 1, ptextures[i].width);
		printf("texture %d.height: %d\n", i + 1, ptextures[i].height);
		printf("texture %d.index: %d\n", i + 1, ptextures[i].index);
	}
}


/*!
*/
void CAnimatedMeshHalfLife::ExtractBbox(s32 sequence, core::aabbox3df &box) const
{
	const SHalflifeSequence *seq = (const SHalflifeSequence *)((const u8*)Header + Header->seqindex) + sequence;

	box.MinEdge.X = seq[0].bbmin[0];
	box.MinEdge.Y = seq[0].bbmin[1];
	box.MinEdge.Z = seq[0].bbmin[2];

	box.MaxEdge.X = seq[0].bbmax[0];
	box.MaxEdge.Y = seq[0].bbmax[1];
	box.MaxEdge.Z = seq[0].bbmax[2];
}


/*!
*/
void CAnimatedMeshHalfLife::calcBoneAdj()
{
	const SHalflifeBoneController *bonecontroller =
		(const SHalflifeBoneController *)((const u8*) Header + Header->bonecontrollerindex);

	for (u32 j = 0; j < Header->numbonecontrollers; j++)
	{
		const s32 i = bonecontroller[j].index;
		// check for 360% wrapping
		f32 value;
		if (bonecontroller[j].type & STUDIO_RLOOP)
		{
			value = BoneController[i] * (360.f/256.f) + bonecontroller[j].start;
		}
		else
		{
			const f32 range = i <= 3 ? 255.f : 64.f;
			value = core::clamp(BoneController[i] / range,0.f,1.f);
			value = (1.f - value) * bonecontroller[j].start + value * bonecontroller[j].end;
		}

		switch(bonecontroller[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			BoneAdj[j] = value * core::DEGTORAD;
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			BoneAdj[j] = value;
			break;
		}
	}
}


/*!
*/
void CAnimatedMeshHalfLife::calcBoneQuaternion(const s32 frame, const SHalflifeBone * const bone,
		SHalflifeAnimOffset *anim, const u32 j, f32& angle1, f32& angle2) const
{
	// three vector components
	if (anim->offset[j+3] == 0)
	{
		angle2 = angle1 = bone->value[j+3]; // default
	}
	else
	{
		SHalflifeAnimationFrame *animvalue = (SHalflifeAnimationFrame *)((u8*)anim + anim->offset[j+3]);
		s32 k = frame;
		while (animvalue->num.total <= k)
		{
			k -= animvalue->num.total;
			animvalue += animvalue->num.valid + 1;
		}
		// Bah, missing blend!
		if (animvalue->num.valid > k)
		{
			angle1 = animvalue[k+1].value;

			if (animvalue->num.valid > k + 1)
			{
				angle2 = animvalue[k+2].value;
			}
			else
			{
				if (animvalue->num.total > k + 1)
					angle2 = angle1;
				else
					angle2 = animvalue[animvalue->num.valid+2].value;
			}
		}
		else
		{
			angle1 = animvalue[animvalue->num.valid].value;
			if (animvalue->num.total > k + 1)
			{
				angle2 = angle1;
			}
			else
			{
				angle2 = animvalue[animvalue->num.valid + 2].value;
			}
		}
		angle1 = bone->value[j+3] + angle1 * bone->scale[j+3];
		angle2 = bone->value[j+3] + angle2 * bone->scale[j+3];
	}

	if (bone->bonecontroller[j+3] != -1)
	{
		angle1 += BoneAdj[bone->bonecontroller[j+3]];
		angle2 += BoneAdj[bone->bonecontroller[j+3]];
	}
}


/*!
*/
void CAnimatedMeshHalfLife::calcBonePosition(const s32 frame, f32 s,
		const SHalflifeBone * const bone, SHalflifeAnimOffset *anim, f32 *pos) const
{
	for (s32 j = 0; j < 3; ++j)
	{
		pos[j] = bone->value[j]; // default;
		if (anim->offset[j] != 0)
		{
			SHalflifeAnimationFrame	*animvalue = (SHalflifeAnimationFrame *)((u8*)anim + anim->offset[j]);

			s32 k = frame;
			// find span of values that includes the frame we want
			while (animvalue->num.total <= k)
			{
				k -= animvalue->num.total;
				animvalue += animvalue->num.valid + 1;
			}
			// if we're inside the span
			if (animvalue->num.valid > k)
			{
				// and there's more data in the span
				if (animvalue->num.valid > k + 1)
				{
					pos[j] += (animvalue[k+1].value * (1.f - s) + s * animvalue[k+2].value) * bone->scale[j];
				}
				else
				{
					pos[j] += animvalue[k+1].value * bone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (animvalue->num.total <= k + 1)
				{
					pos[j] += (animvalue[animvalue->num.valid].value * (1.f - s) + s * animvalue[animvalue->num.valid + 2].value) * bone->scale[j];
				}
				else
				{
					pos[j] += animvalue[animvalue->num.valid].value * bone->scale[j];
				}
			}
		}
		if (bone->bonecontroller[j] != -1)
		{
			pos[j] += BoneAdj[bone->bonecontroller[j]];
		}
	}
}


/*!
*/
void CAnimatedMeshHalfLife::calcRotations(vec3_hl *pos, vec4_hl *q,
		SHalflifeSequence *seq, SHalflifeAnimOffset *anim, f32 f)
{
	s32 frame = (s32)f;
	f32 s = (f - frame);

	// add in programatic controllers
	calcBoneAdj();

	SHalflifeBone *bone = (SHalflifeBone *)((u8 *)Header + Header->boneindex);
	for ( u32 i = 0; i < Header->numbones; i++, bone++, anim++)
	{
		core::vector3df angle1, angle2;
		calcBoneQuaternion(frame, bone, anim, 0, angle1.X, angle2.X);
		calcBoneQuaternion(frame, bone, anim, 1, angle1.Y, angle2.Y);
		calcBoneQuaternion(frame, bone, anim, 2, angle1.Z, angle2.Z);

		if (!angle1.equals(angle2))
		{
			vec4_hl q1, q2;
			AngleQuaternion( angle1, q1 );
			AngleQuaternion( angle2, q2 );
			QuaternionSlerp( q1, q2, s, q[i] );
		}
		else
		{
			AngleQuaternion( angle1, q[i] );
		}

		calcBonePosition(frame, s, bone, anim, pos[i]);
	}

	if (seq->motiontype & STUDIO_X)
		pos[seq->motionbone][0] = 0.f;
	if (seq->motiontype & STUDIO_Y)
		pos[seq->motionbone][1] = 0.f;
	if (seq->motiontype & STUDIO_Z)
		pos[seq->motionbone][2] = 0.f;
}


/*!
*/
SHalflifeAnimOffset * CAnimatedMeshHalfLife::getAnim( SHalflifeSequence *seq )
{
	SHalflifeSequenceGroup *seqgroup = (SHalflifeSequenceGroup *)((u8*)Header + Header->seqgroupindex) + seq->seqgroup;

	if (seq->seqgroup == 0)
	{
		return (SHalflifeAnimOffset *)((u8*)Header + seqgroup->data + seq->animindex);
	}

	return (SHalflifeAnimOffset *)((u8*)AnimationHeader[seq->seqgroup] + seq->animindex);
}


/*!
*/
void CAnimatedMeshHalfLife::slerpBones(vec4_hl q1[], vec3_hl pos1[], vec4_hl q2[], vec3_hl pos2[], f32 s)
{
	if (s < 0)
		s = 0;
	else if (s > 1.f)
		s = 1.f;

	f32 s1 = 1.f - s;

	for ( u32 i = 0; i < Header->numbones; i++)
	{
		vec4_hl q3;
		QuaternionSlerp( q1[i], q2[i], s, q3 );
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}


/*!
*/
void CAnimatedMeshHalfLife::setUpBones()
{
	static vec3_hl pos[MAXSTUDIOBONES];
	f32 bonematrix[3][4];
	static vec4_hl q[MAXSTUDIOBONES];

	static vec3_hl pos2[MAXSTUDIOBONES];
	static vec4_hl q2[MAXSTUDIOBONES];
	static vec3_hl pos3[MAXSTUDIOBONES];
	static vec4_hl q3[MAXSTUDIOBONES];
	static vec3_hl pos4[MAXSTUDIOBONES];
	static vec4_hl q4[MAXSTUDIOBONES];

	if (SequenceIndex >= Header->numseq)
		SequenceIndex = 0;

	SHalflifeSequence *seq = (SHalflifeSequence *)((u8*) Header + Header->seqindex) + SequenceIndex;

	SHalflifeAnimOffset *anim = getAnim(seq);
	calcRotations(pos, q, seq, anim, CurrentFrame);

	if (seq->numblends > 1)
	{
		anim += Header->numbones;
		calcRotations( pos2, q2, seq, anim, CurrentFrame );
		f32 s = Blending[0] / 255.f;

		slerpBones( q, pos, q2, pos2, s );

		if (seq->numblends == 4)
		{
			anim += Header->numbones;
			calcRotations( pos3, q3, seq, anim, CurrentFrame );

			anim += Header->numbones;
			calcRotations( pos4, q4, seq, anim, CurrentFrame );

			s = Blending[0] / 255.f;
			slerpBones( q3, pos3, q4, pos4, s );

			s = Blending[1] / 255.f;
			slerpBones( q, pos, q3, pos3, s );
		}
	}

	SHalflifeBone *bone = (SHalflifeBone *)((u8*) Header + Header->boneindex);

	for (u32 i = 0; i < Header->numbones; i++)
	{
		QuaternionMatrix( q[i], bonematrix );

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (bone[i].parent == -1) {
			memcpy(BoneTransform[i], bonematrix, sizeof(f32) * 12);
		}
		else {
			R_ConcatTransforms (BoneTransform[bone[i].parent], bonematrix, BoneTransform[i]);
		}
	}
}


//! Returns an axis aligned bounding box
const core::aabbox3d<f32>& CAnimatedMeshHalfLife::getBoundingBox() const
{
	return MeshIPol->BoundingBox;
}


//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CAnimatedMeshHalfLife::getMeshType() const
{
	return EAMT_MDL_HALFLIFE;
}


//! returns amount of mesh buffers.
u32 CAnimatedMeshHalfLife::getMeshBufferCount() const
{
	return MeshIPol->getMeshBufferCount();
}


//! returns pointer to a mesh buffer
IMeshBuffer* CAnimatedMeshHalfLife::getMeshBuffer(u32 nr) const
{
	return MeshIPol->getMeshBuffer(nr);
}


//! Returns pointer to a mesh buffer which fits a material
/** \param material: material to search for
\return Returns the pointer to the mesh buffer or
NULL if there is no such mesh buffer. */
IMeshBuffer* CAnimatedMeshHalfLife::getMeshBuffer(const video::SMaterial &material) const
{
	return MeshIPol->getMeshBuffer(material);
}


void CAnimatedMeshHalfLife::setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue)
{
	MeshIPol->setMaterialFlag ( flag, newvalue );
}


//! set user axis aligned bounding box
void CAnimatedMeshHalfLife::setBoundingBox(const core::aabbox3df& box)
{
	MeshIPol->setBoundingBox(box);
}


} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_MD3_LOADER_

