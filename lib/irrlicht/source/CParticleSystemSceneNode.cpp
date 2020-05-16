// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticleSystemSceneNode.h"
#include "os.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "IVideoDriver.h"

#include "CParticleAnimatedMeshSceneNodeEmitter.h"
#include "CParticleBoxEmitter.h"
#include "CParticleCylinderEmitter.h"
#include "CParticleMeshEmitter.h"
#include "CParticlePointEmitter.h"
#include "CParticleRingEmitter.h"
#include "CParticleSphereEmitter.h"
#include "CParticleAttractionAffector.h"
#include "CParticleFadeOutAffector.h"
#include "CParticleGravityAffector.h"
#include "CParticleRotationAffector.h"
#include "CParticleScaleAffector.h"
#include "SViewFrustum.h"

namespace irr
{
namespace scene
{

//! constructor
CParticleSystemSceneNode::CParticleSystemSceneNode(bool createDefaultEmitter,
	ISceneNode* parent, ISceneManager* mgr, s32 id,
	const core::vector3df& position, const core::vector3df& rotation,
	const core::vector3df& scale)
	: IParticleSystemSceneNode(parent, mgr, id, position, rotation, scale),
	Emitter(0), ParticleSize(core::dimension2d<f32>(5.0f, 5.0f)), LastEmitTime(0),
	MaxParticles(0xffff), Buffer(0), ParticlesAreGlobal(true)
{
	#ifdef _DEBUG
	setDebugName("CParticleSystemSceneNode");
	#endif

	Buffer = new SMeshBuffer();
	if (createDefaultEmitter)
	{
		IParticleEmitter* e = createBoxEmitter();
		setEmitter(e);
		e->drop();
	}
}


//! destructor
CParticleSystemSceneNode::~CParticleSystemSceneNode()
{
	if (Emitter)
		Emitter->drop();
	if (Buffer)
		Buffer->drop();

	removeAllAffectors();
}


//! Gets the particle emitter, which creates the particles.
IParticleEmitter* CParticleSystemSceneNode::getEmitter()
{
	return Emitter;
}


//! Sets the particle emitter, which creates the particles.
void CParticleSystemSceneNode::setEmitter(IParticleEmitter* emitter)
{
	if (emitter == Emitter)
		return;
	if (Emitter)
		Emitter->drop();

	Emitter = emitter;

	if (Emitter)
		Emitter->grab();
}


//! Adds new particle effector to the particle system.
void CParticleSystemSceneNode::addAffector(IParticleAffector* affector)
{
	affector->grab();
	AffectorList.push_back(affector);
}

//! Get a list of all particle affectors.
const core::list<IParticleAffector*>& CParticleSystemSceneNode::getAffectors() const
{
	return AffectorList;
}

//! Removes all particle affectors in the particle system.
void CParticleSystemSceneNode::removeAllAffectors()
{
	core::list<IParticleAffector*>::Iterator it = AffectorList.begin();
	while (it != AffectorList.end())
	{
		(*it)->drop();
		it = AffectorList.erase(it);
	}
}


//! Returns the material based on the zero based index i.
video::SMaterial& CParticleSystemSceneNode::getMaterial(u32 i)
{
	return Buffer->Material;
}


//! Returns amount of materials used by this scene node.
u32 CParticleSystemSceneNode::getMaterialCount() const
{
	return 1;
}


//! Creates a particle emitter for an animated mesh scene node
IParticleAnimatedMeshSceneNodeEmitter*
CParticleSystemSceneNode::createAnimatedMeshSceneNodeEmitter(
	scene::IAnimatedMeshSceneNode* node, bool useNormalDirection,
	const core::vector3df& direction, f32 normalDirectionModifier,
	s32 mbNumber, bool everyMeshVertex,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticleAnimatedMeshSceneNodeEmitter( node,
			useNormalDirection, direction, normalDirectionModifier,
			mbNumber, everyMeshVertex,
			minParticlesPerSecond, maxParticlesPerSecond,
			minStartColor, maxStartColor,
			lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a box particle emitter.
IParticleBoxEmitter* CParticleSystemSceneNode::createBoxEmitter(
	const core::aabbox3df& box, const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees, const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticleBoxEmitter(box, direction, minParticlesPerSecond,
		maxParticlesPerSecond, minStartColor, maxStartColor,
		lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a particle emitter for emitting from a cylinder
IParticleCylinderEmitter* CParticleSystemSceneNode::createCylinderEmitter(
	const core::vector3df& center, f32 radius,
	const core::vector3df& normal, f32 length,
	bool outlineOnly, const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticleCylinderEmitter( center, radius, normal, length,
			outlineOnly, direction,
			minParticlesPerSecond, maxParticlesPerSecond,
			minStartColor, maxStartColor,
			lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a mesh particle emitter.
IParticleMeshEmitter* CParticleSystemSceneNode::createMeshEmitter(
	scene::IMesh* mesh, bool useNormalDirection,
	const core::vector3df& direction, f32 normalDirectionModifier,
	s32 mbNumber, bool everyMeshVertex,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize)
{
	return new CParticleMeshEmitter( mesh, useNormalDirection, direction,
			normalDirectionModifier, mbNumber, everyMeshVertex,
			minParticlesPerSecond, maxParticlesPerSecond,
			minStartColor, maxStartColor,
			lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a point particle emitter.
IParticlePointEmitter* CParticleSystemSceneNode::createPointEmitter(
	const core::vector3df& direction, u32 minParticlesPerSecond,
	u32 maxParticlesPerSecond, const video::SColor& minStartColor,
	const video::SColor& maxStartColor, u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees, const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticlePointEmitter(direction, minParticlesPerSecond,
		maxParticlesPerSecond, minStartColor, maxStartColor,
		lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a ring particle emitter.
IParticleRingEmitter* CParticleSystemSceneNode::createRingEmitter(
	const core::vector3df& center, f32 radius, f32 ringThickness,
	const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize, const core::dimension2df& maxStartSize )
{
	return new CParticleRingEmitter( center, radius, ringThickness, direction,
		minParticlesPerSecond, maxParticlesPerSecond, minStartColor,
		maxStartColor, lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a sphere particle emitter.
IParticleSphereEmitter* CParticleSystemSceneNode::createSphereEmitter(
	const core::vector3df& center, f32 radius, const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees, const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticleSphereEmitter(center, radius, direction,
			minParticlesPerSecond, maxParticlesPerSecond,
			minStartColor, maxStartColor,
			lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a point attraction affector. This affector modifies the positions of the
//! particles and attracts them to a specified point at a specified speed per second.
IParticleAttractionAffector* CParticleSystemSceneNode::createAttractionAffector(
	const core::vector3df& point, f32 speed, bool attract,
	bool affectX, bool affectY, bool affectZ )
{
	return new CParticleAttractionAffector( point, speed, attract, affectX, affectY, affectZ );
}

//! Creates a scale particle affector.
IParticleAffector* CParticleSystemSceneNode::createScaleParticleAffector(const core::dimension2df& scaleTo)
{
	return new CParticleScaleAffector(scaleTo);
}


//! Creates a fade out particle affector.
IParticleFadeOutAffector* CParticleSystemSceneNode::createFadeOutParticleAffector(
		const video::SColor& targetColor, u32 timeNeededToFadeOut)
{
	return new CParticleFadeOutAffector(targetColor, timeNeededToFadeOut);
}


//! Creates a gravity affector.
IParticleGravityAffector* CParticleSystemSceneNode::createGravityAffector(
		const core::vector3df& gravity, u32 timeForceLost)
{
	return new CParticleGravityAffector(gravity, timeForceLost);
}


//! Creates a rotation affector. This affector rotates the particles around a specified pivot
//! point.  The speed represents Degrees of rotation per second.
IParticleRotationAffector* CParticleSystemSceneNode::createRotationAffector(
	const core::vector3df& speed, const core::vector3df& pivotPoint )
{
	return new CParticleRotationAffector( speed, pivotPoint );
}


//! pre render event
void CParticleSystemSceneNode::OnRegisterSceneNode()
{
	doParticleSystem(os::Timer::getTime());

	if (IsVisible && (Particles.size() != 0))
	{
		SceneManager->registerNodeForRendering(this);
		ISceneNode::OnRegisterSceneNode();
	}
}


//! render
void CParticleSystemSceneNode::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;


#if 0
	// calculate vectors for letting particles look to camera
	core::vector3df view(camera->getTarget() - camera->getAbsolutePosition());
	view.normalize();

	view *= -1.0f;

#else

	const core::matrix4 &m = camera->getViewFrustum()->getTransform( video::ETS_VIEW );

	const core::vector3df view ( -m[2], -m[6] , -m[10] );

#endif

	// reallocate arrays, if they are too small
	reallocateBuffers();

	// create particle vertex data
	s32 idx = 0;
	for (u32 i=0; i<Particles.size(); ++i)
	{
		const SParticle& particle = Particles[i];

		#if 0
			core::vector3df horizontal = camera->getUpVector().crossProduct(view);
			horizontal.normalize();
			horizontal *= 0.5f * particle.size.Width;

			core::vector3df vertical = horizontal.crossProduct(view);
			vertical.normalize();
			vertical *= 0.5f * particle.size.Height;

		#else
			f32 f;

			f = 0.5f * particle.size.Width;
			const core::vector3df horizontal ( m[0] * f, m[4] * f, m[8] * f );

			f = -0.5f * particle.size.Height;
			const core::vector3df vertical ( m[1] * f, m[5] * f, m[9] * f );
		#endif

		Buffer->Vertices[0+idx].Pos = particle.pos + horizontal + vertical;
		Buffer->Vertices[0+idx].Color = particle.color;
		Buffer->Vertices[0+idx].Normal = view;

		Buffer->Vertices[1+idx].Pos = particle.pos + horizontal - vertical;
		Buffer->Vertices[1+idx].Color = particle.color;
		Buffer->Vertices[1+idx].Normal = view;

		Buffer->Vertices[2+idx].Pos = particle.pos - horizontal - vertical;
		Buffer->Vertices[2+idx].Color = particle.color;
		Buffer->Vertices[2+idx].Normal = view;

		Buffer->Vertices[3+idx].Pos = particle.pos - horizontal + vertical;
		Buffer->Vertices[3+idx].Color = particle.color;
		Buffer->Vertices[3+idx].Normal = view;

		idx +=4;
	}

	// render all
	core::matrix4 mat;
	if (!ParticlesAreGlobal)
		mat.setTranslation(AbsoluteTransformation.getTranslation());
	driver->setTransform(video::ETS_WORLD, mat);

	driver->setMaterial(Buffer->Material);

	driver->drawVertexPrimitiveList(Buffer->getVertices(), Particles.size()*4,
		Buffer->getIndices(), Particles.size()*2, video::EVT_STANDARD, EPT_TRIANGLES,Buffer->getIndexType());

	// for debug purposes only:
	if ( DebugDataVisible & scene::EDS_BBOX )
	{
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		video::SMaterial deb_m;
		deb_m.Lighting = false;
		driver->setMaterial(deb_m);
		driver->draw3DBox(Buffer->BoundingBox, video::SColor(0,255,255,255));
	}
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CParticleSystemSceneNode::getBoundingBox() const
{
	return Buffer->getBoundingBox();
}


void CParticleSystemSceneNode::doParticleSystem(u32 time)
{
	if (LastEmitTime==0)
	{
		LastEmitTime = time;
		return;
	}

	u32 now = time;
	u32 timediff = time - LastEmitTime;
	LastEmitTime = time;

	// run emitter

	if (Emitter && IsVisible)
	{
		SParticle* array = 0;
		s32 newParticles = Emitter->emitt(now, timediff, array);

		if (newParticles && array)
		{
			s32 j=Particles.size();
			if (newParticles > 16250-j)
				newParticles=16250-j;
			Particles.set_used(j+newParticles);
			for (s32 i=j; i<j+newParticles; ++i)
			{
				Particles[i]=array[i-j];
				AbsoluteTransformation.rotateVect(Particles[i].startVector);
				if (ParticlesAreGlobal)
					AbsoluteTransformation.transformVect(Particles[i].pos);
			}
		}
	}

	// run affectors
	core::list<IParticleAffector*>::Iterator ait = AffectorList.begin();
	for (; ait != AffectorList.end(); ++ait)
		(*ait)->affect(now, Particles.pointer(), Particles.size());

	if (ParticlesAreGlobal)
		Buffer->BoundingBox.reset(AbsoluteTransformation.getTranslation());
	else
		Buffer->BoundingBox.reset(core::vector3df(0,0,0));

	// animate all particles
	f32 scale = (f32)timediff;

	for (u32 i=0; i<Particles.size();)
	{
		// erase is pretty expensive!
		if (now > Particles[i].endTime)
		{
			// Particle order does not seem to matter.
			// So we can delete by switching with last particle and deleting that one.
			// This is a lot faster and speed is very important here as the erase otherwise
			// can cause noticable freezes.
			Particles[i] = Particles[Particles.size()-1];
			Particles.erase( Particles.size()-1 );
		}
		else
		{
			Particles[i].pos += (Particles[i].vector * scale);
			Buffer->BoundingBox.addInternalPoint(Particles[i].pos);
			++i;
		}
	}

	const f32 m = (ParticleSize.Width > ParticleSize.Height ? ParticleSize.Width : ParticleSize.Height) * 0.5f;
	Buffer->BoundingBox.MaxEdge.X += m;
	Buffer->BoundingBox.MaxEdge.Y += m;
	Buffer->BoundingBox.MaxEdge.Z += m;

	Buffer->BoundingBox.MinEdge.X -= m;
	Buffer->BoundingBox.MinEdge.Y -= m;
	Buffer->BoundingBox.MinEdge.Z -= m;

	if (ParticlesAreGlobal)
	{
		core::matrix4 absinv( AbsoluteTransformation, core::matrix4::EM4CONST_INVERSE );
		absinv.transformBoxEx(Buffer->BoundingBox);
	}
}


//! Sets if the particles should be global. If it is, the particles are affected by
//! the movement of the particle system scene node too, otherwise they completely
//! ignore it. Default is true.
void CParticleSystemSceneNode::setParticlesAreGlobal(bool global)
{
	ParticlesAreGlobal = global;
}

//! Remove all currently visible particles
void CParticleSystemSceneNode::clearParticles()
{
	Particles.set_used(0);
}

//! Sets the size of all particles.
void CParticleSystemSceneNode::setParticleSize(const core::dimension2d<f32> &size)
{
	os::Printer::log("setParticleSize is deprecated, use setMinStartSize/setMaxStartSize in emitter.", irr::ELL_WARNING);
	//A bit of a hack, but better here than in the particle code
	if (Emitter)
	{
		Emitter->setMinStartSize(size);
		Emitter->setMaxStartSize(size);
	}
	ParticleSize = size;
}


void CParticleSystemSceneNode::reallocateBuffers()
{
	if (Particles.size() * 4 > Buffer->getVertexCount() ||
			Particles.size() * 6 > Buffer->getIndexCount())
	{
		u32 oldSize = Buffer->getVertexCount();
		Buffer->Vertices.set_used(Particles.size() * 4);

		u32 i;

		// fill remaining vertices
		for (i=oldSize; i<Buffer->Vertices.size(); i+=4)
		{
			Buffer->Vertices[0+i].TCoords.set(0.0f, 0.0f);
			Buffer->Vertices[1+i].TCoords.set(0.0f, 1.0f);
			Buffer->Vertices[2+i].TCoords.set(1.0f, 1.0f);
			Buffer->Vertices[3+i].TCoords.set(1.0f, 0.0f);
		}

		// fill remaining indices
		u32 oldIdxSize = Buffer->getIndexCount();
		u32 oldvertices = oldSize;
		Buffer->Indices.set_used(Particles.size() * 6);

		for (i=oldIdxSize; i<Buffer->Indices.size(); i+=6)
		{
			Buffer->Indices[0+i] = (u16)0+oldvertices;
			Buffer->Indices[1+i] = (u16)2+oldvertices;
			Buffer->Indices[2+i] = (u16)1+oldvertices;
			Buffer->Indices[3+i] = (u16)0+oldvertices;
			Buffer->Indices[4+i] = (u16)3+oldvertices;
			Buffer->Indices[5+i] = (u16)2+oldvertices;
			oldvertices += 4;
		}
	}
}


//! Writes attributes of the scene node.
void CParticleSystemSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	IParticleSystemSceneNode::serializeAttributes(out, options);

	out->addBool("GlobalParticles", ParticlesAreGlobal);
	out->addFloat("ParticleWidth", ParticleSize.Width);
	out->addFloat("ParticleHeight", ParticleSize.Height);

	// write emitter

	E_PARTICLE_EMITTER_TYPE type = EPET_COUNT;
	if (Emitter)
		type = Emitter->getType();

	out->addEnum("Emitter", (s32)type, ParticleEmitterTypeNames);

	if (Emitter)
		Emitter->serializeAttributes(out, options);

	// write affectors

	E_PARTICLE_AFFECTOR_TYPE atype = EPAT_NONE;

	for (core::list<IParticleAffector*>::ConstIterator it = AffectorList.begin();
		it != AffectorList.end(); ++it)
	{
		atype = (*it)->getType();

		out->addEnum("Affector", (s32)atype, ParticleAffectorTypeNames);

		(*it)->serializeAttributes(out);
	}

	// add empty affector to make it possible to add further affectors

	if (options && options->Flags & io::EARWF_FOR_EDITOR)
		out->addEnum("Affector", EPAT_NONE, ParticleAffectorTypeNames);
}


//! Reads attributes of the scene node.
void CParticleSystemSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	IParticleSystemSceneNode::deserializeAttributes(in, options);

	ParticlesAreGlobal = in->getAttributeAsBool("GlobalParticles");
	ParticleSize.Width = in->getAttributeAsFloat("ParticleWidth");
	ParticleSize.Height = in->getAttributeAsFloat("ParticleHeight");

	// read emitter

	int emitterIdx = in->findAttribute("Emitter");
	if (emitterIdx == -1)
		return;

	if (Emitter)
		Emitter->drop();
	Emitter = 0;

	E_PARTICLE_EMITTER_TYPE type = (E_PARTICLE_EMITTER_TYPE)
		in->getAttributeAsEnumeration("Emitter", ParticleEmitterTypeNames);

	switch(type)
	{
	case EPET_POINT:
		Emitter = createPointEmitter();
		break;
	case EPET_ANIMATED_MESH:
		Emitter = createAnimatedMeshSceneNodeEmitter(NULL); // we can't set the node - the user will have to do this
		break;
	case EPET_BOX:
		Emitter = createBoxEmitter();
		break;
	case EPET_CYLINDER:
		Emitter = createCylinderEmitter(core::vector3df(0,0,0), 10.f, core::vector3df(0,1,0), 10.f);	// (values here don't matter)
		break;
	case EPET_MESH:
		Emitter = createMeshEmitter(NULL);	// we can't set the mesh - the user will have to do this
		break;
	case EPET_RING:
		Emitter = createRingEmitter(core::vector3df(0,0,0), 10.f, 10.f);	// (values here don't matter)
		break;
	case EPET_SPHERE:
		Emitter = createSphereEmitter(core::vector3df(0,0,0), 10.f);	// (values here don't matter)
		break;
	default:
		break;
	}

	u32 idx = 0;

#if 0
	if (Emitter)
		idx = Emitter->deserializeAttributes(idx, in);

	++idx;
#else
	if (Emitter)
		Emitter->deserializeAttributes(in);
#endif

	// read affectors

	removeAllAffectors();
	u32 cnt = in->getAttributeCount();

	while(idx < cnt)
	{
		const char* name = in->getAttributeName(idx);

		if (!name || strcmp("Affector", name))
			return;

		E_PARTICLE_AFFECTOR_TYPE atype =
			(E_PARTICLE_AFFECTOR_TYPE)in->getAttributeAsEnumeration(idx, ParticleAffectorTypeNames);

		IParticleAffector* aff = 0;

		switch(atype)
		{
		case EPAT_ATTRACT:
			aff = createAttractionAffector(core::vector3df(0,0,0));
			break;
		case EPAT_FADE_OUT:
			aff = createFadeOutParticleAffector();
			break;
		case EPAT_GRAVITY:
			aff = createGravityAffector();
			break;
		case EPAT_ROTATE:
			aff = createRotationAffector();
			break;
		case EPAT_SCALE:
			aff = createScaleParticleAffector();
			break;
		case EPAT_NONE:
		default:
			break;
		}

		++idx;

		if (aff)
		{
#if 0
			idx = aff->deserializeAttributes(idx, in, options);
			++idx;
#else
			aff->deserializeAttributes(in, options);
#endif

			addAffector(aff);
			aff->drop();
		}
	}
}


} // end namespace scene
} // end namespace irr


