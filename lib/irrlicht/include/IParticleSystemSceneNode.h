// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_PARTICLE_SYSTEM_SCENE_NODE_H_INCLUDED__
#define __I_PARTICLE_SYSTEM_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"
#include "IParticleAnimatedMeshSceneNodeEmitter.h"
#include "IParticleBoxEmitter.h"
#include "IParticleCylinderEmitter.h"
#include "IParticleMeshEmitter.h"
#include "IParticleRingEmitter.h"
#include "IParticleSphereEmitter.h"
#include "IParticleAttractionAffector.h"
#include "IParticleFadeOutAffector.h"
#include "IParticleGravityAffector.h"
#include "IParticleRotationAffector.h"
#include "dimension2d.h"

namespace irr
{
namespace scene
{

//! A particle system scene node for creating snow, fire, exlosions, smoke...
/** A scene node controlling a particle System. The behavior of the particles
can be controlled by setting the right particle emitters and affectors.
You can for example easily create a campfire by doing this:

\code
	scene::IParticleSystemSceneNode* p = scenemgr->addParticleSystemSceneNode();
	p->setParticleSize(core::dimension2d<f32>(20.0f, 10.0f));
	scene::IParticleEmitter* em = p->createBoxEmitter(
		core::aabbox3d<f32>(-5,0,-5,5,1,5),
		core::vector3df(0.0f,0.03f,0.0f),
		40,80, video::SColor(0,255,255,255),video::SColor(0,255,255,255), 1100,2000);
	p->setEmitter(em);
	em->drop();
	scene::IParticleAffector* paf = p->createFadeOutParticleAffector();
	p->addAffector(paf);
	paf->drop();
\endcode

*/
class IParticleSystemSceneNode : public ISceneNode
{
public:

	//! Constructor
	IParticleSystemSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
		const core::vector3df& position = core::vector3df(0,0,0),
		const core::vector3df& rotation = core::vector3df(0,0,0),
		const core::vector3df& scale = core::vector3df(1.0f, 1.0f, 1.0f))
			: ISceneNode(parent, mgr, id, position, rotation, scale) {}

	//! Sets the size of all particles.
	virtual void setParticleSize(
		const core::dimension2d<f32> &size = core::dimension2d<f32>(5.0f, 5.0f)) = 0;

	//! Sets if the particles should be global.
	/** If they are, the particles are affected by the movement of the
	particle system scene node too, otherwise they completely ignore it.
	Default is true. */
	virtual void setParticlesAreGlobal(bool global=true) = 0;

	//! Remove all currently visible particles
	virtual void clearParticles() = 0;

	//! Do manually update the particles.
 	//! This should only be called when you want to render the node outside the scenegraph,
 	//! as the node will care about this otherwise automatically.
	virtual void doParticleSystem(u32 time) = 0;

	//! Gets the particle emitter, which creates the particles.
	/** \return The particle emitter. Can be 0 if none is set. */
	virtual IParticleEmitter* getEmitter() =0;

	//! Sets the particle emitter, which creates the particles.
	/** A particle emitter can be created using one of the createEmitter
	methods. For example to create and use a simple PointEmitter, call
	IParticleEmitter* p = createPointEmitter(); setEmitter(p); p->drop();
	\param emitter: Sets the particle emitter. You can set this to 0 for
	removing the current emitter and stopping the particle system emitting
	new particles. */
	virtual void setEmitter(IParticleEmitter* emitter) = 0;

	//! Adds new particle effector to the particle system.
	/** A particle affector modifies the particles. For example, the FadeOut
	affector lets all particles fade out after some time. It is created and
	used in this way:
	\code
	IParticleAffector* p = createFadeOutParticleAffector();
	addAffector(p);
	p->drop();
	\endcode
	Please note that an affector is not necessary for the particle system to
	work.
	\param affector: New affector. */
	virtual void addAffector(IParticleAffector* affector) = 0;

	//! Get a list of all particle affectors.
	/** \return The list of particle affectors attached to this node. */
	virtual const core::list<IParticleAffector*>& getAffectors() const = 0;

	//! Removes all particle affectors in the particle system.
	virtual void removeAllAffectors() = 0;

	//! Creates a particle emitter for an animated mesh scene node
	/** \param node: Pointer to the animated mesh scene node to emit
	particles from
	\param useNormalDirection: If true, the direction of each particle
	created will be the normal of the vertex that it's emitting from. The
	normal is divided by the normalDirectionModifier parameter, which
	defaults to 100.0f.
	\param direction: Direction and speed of particle emission.
	\param normalDirectionModifier: If the emitter is using the normal
	direction then the normal of the vertex that is being emitted from is
	divided by this number.
	\param mbNumber: This allows you to specify a specific meshBuffer for
	the IMesh* to emit particles from. The default value is -1, which
	means a random meshBuffer picked from all of the meshes meshBuffers
	will be selected to pick a random vertex from. If the value is 0 or
	greater, it will only pick random vertices from the meshBuffer
	specified by this value.
	\param everyMeshVertex: If true, the emitter will emit between min/max
	particles every second, for every vertex in the mesh, if false, it will
	emit between min/max particles from random vertices in the mesh.
	\param minParticlesPerSecond: Minimal amount of particles emitted per
	second.
	\param maxParticlesPerSecond: Maximal amount of particles emitted per
	second.
	\param minStartColor: Minimal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param maxStartColor: Maximal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param lifeTimeMin: Minimal lifetime of a particle, in milliseconds.
	\param lifeTimeMax: Maximal lifetime of a particle, in milliseconds.
	\param maxAngleDegrees: Maximal angle in degrees, the emitting
	direction of the particle will differ from the original direction.
	\param minStartSize: Minimal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\param maxStartSize: Maximal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\return Pointer to the created particle emitter. To set this emitter
	as new emitter of this particle system, just call setEmitter(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleAnimatedMeshSceneNodeEmitter* createAnimatedMeshSceneNodeEmitter(
		scene::IAnimatedMeshSceneNode* node, bool useNormalDirection = true,
		const core::vector3df& direction = core::vector3df(0.0f,0.03f,0.0f),
		f32 normalDirectionModifier = 100.0f, s32 mbNumber = -1,
		bool everyMeshVertex = false,
		u32 minParticlesPerSecond = 5, u32 maxParticlesPerSecond = 10,
		const video::SColor& minStartColor = video::SColor(255,0,0,0),
		const video::SColor& maxStartColor = video::SColor(255,255,255,255),
		u32 lifeTimeMin = 2000, u32 lifeTimeMax = 4000,
		s32 maxAngleDegrees = 0,
		const core::dimension2df& minStartSize = core::dimension2df(5.0f,5.0f),
		const core::dimension2df& maxStartSize = core::dimension2df(5.0f,5.0f) ) = 0;

	//! Creates a box particle emitter.
	/** \param box: The box for the emitter.
	\param direction: Direction and speed of particle emission.
	\param minParticlesPerSecond: Minimal amount of particles emitted per
	second.
	\param maxParticlesPerSecond: Maximal amount of particles emitted per
	second.
	\param minStartColor: Minimal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param maxStartColor: Maximal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param lifeTimeMin: Minimal lifetime of a particle, in milliseconds.
	\param lifeTimeMax: Maximal lifetime of a particle, in milliseconds.
	\param maxAngleDegrees: Maximal angle in degrees, the emitting
	direction of the particle will differ from the original direction.
	\param minStartSize: Minimal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\param maxStartSize: Maximal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\return Pointer to the created particle emitter. To set this emitter
	as new emitter of this particle system, just call setEmitter(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleBoxEmitter* createBoxEmitter(
		const core::aabbox3df& box = core::aabbox3df(-10,28,-10,10,30,10),
		const core::vector3df& direction = core::vector3df(0.0f,0.03f,0.0f),
		u32 minParticlesPerSecond = 5,
		u32 maxParticlesPerSecond = 10,
		const video::SColor& minStartColor = video::SColor(255,0,0,0),
		const video::SColor& maxStartColor = video::SColor(255,255,255,255),
		u32 lifeTimeMin=2000, u32 lifeTimeMax=4000,
		s32 maxAngleDegrees=0,
		const core::dimension2df& minStartSize = core::dimension2df(5.0f,5.0f),
		const core::dimension2df& maxStartSize = core::dimension2df(5.0f,5.0f) ) = 0;

	//! Creates a particle emitter for emitting from a cylinder
	/** \param center: The center of the circle at the base of the cylinder
	\param radius: The thickness of the cylinder
	\param normal: Direction of the length of the cylinder
	\param length: The length of the the cylinder
	\param outlineOnly: Whether or not to put points inside the cylinder or
	on the outline only
	\param direction: Direction and speed of particle emission.
	\param minParticlesPerSecond: Minimal amount of particles emitted per
	second.
	\param maxParticlesPerSecond: Maximal amount of particles emitted per
	second.
	\param minStartColor: Minimal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param maxStartColor: Maximal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param lifeTimeMin: Minimal lifetime of a particle, in milliseconds.
	\param lifeTimeMax: Maximal lifetime of a particle, in milliseconds.
	\param maxAngleDegrees: Maximal angle in degrees, the emitting
	direction of the particle will differ from the original direction.
	\param minStartSize: Minimal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\param maxStartSize: Maximal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\return Pointer to the created particle emitter. To set this emitter
	as new emitter of this particle system, just call setEmitter(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleCylinderEmitter* createCylinderEmitter(
		const core::vector3df& center, f32 radius,
		const core::vector3df& normal, f32 length,
		bool outlineOnly = false,
		const core::vector3df& direction = core::vector3df(0.0f,0.03f,0.0f),
		u32 minParticlesPerSecond = 5, u32 maxParticlesPerSecond = 10,
		const video::SColor& minStartColor = video::SColor(255,0,0,0),
		const video::SColor& maxStartColor = video::SColor(255,255,255,255),
		u32 lifeTimeMin = 2000, u32 lifeTimeMax = 4000,
		s32 maxAngleDegrees = 0,
		const core::dimension2df& minStartSize = core::dimension2df(5.0f,5.0f),
		const core::dimension2df& maxStartSize = core::dimension2df(5.0f,5.0f) ) = 0;

	//! Creates a mesh particle emitter.
	/** \param mesh: Pointer to mesh to emit particles from
	\param useNormalDirection: If true, the direction of each particle
	created will be the normal of the vertex that it's emitting from. The
	normal is divided by the normalDirectionModifier parameter, which
	defaults to 100.0f.
	\param direction: Direction and speed of particle emission.
	\param normalDirectionModifier: If the emitter is using the normal
	direction then the normal of the vertex that is being emitted from is
	divided by this number.
	\param mbNumber: This allows you to specify a specific meshBuffer for
	the IMesh* to emit particles from. The default value is -1, which
	means a random meshBuffer picked from all of the meshes meshBuffers
	will be selected to pick a random vertex from. If the value is 0 or
	greater, it will only pick random vertices from the meshBuffer
	specified by this value.
	\param everyMeshVertex: If true, the emitter will emit between min/max
	particles every second, for every vertex in the mesh, if false, it will
	emit between min/max particles from random vertices in the mesh.
	\param minParticlesPerSecond: Minimal amount of particles emitted per
	second.
	\param maxParticlesPerSecond: Maximal amount of particles emitted per
	second.
	\param minStartColor: Minimal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param maxStartColor: Maximal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param lifeTimeMin: Minimal lifetime of a particle, in milliseconds.
	\param lifeTimeMax: Maximal lifetime of a particle, in milliseconds.
	\param maxAngleDegrees: Maximal angle in degrees, the emitting
	direction of the particle will differ from the original direction.
	\param minStartSize: Minimal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\param maxStartSize: Maximal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\return Pointer to the created particle emitter. To set this emitter
	as new emitter of this particle system, just call setEmitter(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleMeshEmitter* createMeshEmitter(
		scene::IMesh* mesh, bool useNormalDirection = true,
		const core::vector3df& direction = core::vector3df(0.0f,0.03f,0.0f),
		f32 normalDirectionModifier = 100.0f, s32 mbNumber = -1,
		bool everyMeshVertex = false,
		u32 minParticlesPerSecond = 5, u32 maxParticlesPerSecond = 10,
		const video::SColor& minStartColor = video::SColor(255,0,0,0),
		const video::SColor& maxStartColor = video::SColor(255,255,255,255),
		u32 lifeTimeMin = 2000, u32 lifeTimeMax = 4000,
		s32 maxAngleDegrees = 0,
		const core::dimension2df& minStartSize = core::dimension2df(5.0f,5.0f),
		const core::dimension2df& maxStartSize = core::dimension2df(5.0f,5.0f) ) = 0;

	//! Creates a point particle emitter.
	/** \param direction: Direction and speed of particle emission.
	\param minParticlesPerSecond: Minimal amount of particles emitted per
	second.
	\param maxParticlesPerSecond: Maximal amount of particles emitted per
	second.
	\param minStartColor: Minimal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param maxStartColor: Maximal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param lifeTimeMin: Minimal lifetime of a particle, in milliseconds.
	\param lifeTimeMax: Maximal lifetime of a particle, in milliseconds.
	\param maxAngleDegrees: Maximal angle in degrees, the emitting
	direction of the particle will differ from the original direction.
	\param minStartSize: Minimal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\param maxStartSize: Maximal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\return Pointer to the created particle emitter. To set this emitter
	as new emitter of this particle system, just call setEmitter(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticlePointEmitter* createPointEmitter(
		const core::vector3df& direction = core::vector3df(0.0f,0.03f,0.0f),
		u32 minParticlesPerSecond = 5,
		u32 maxParticlesPerSecond = 10,
		const video::SColor& minStartColor = video::SColor(255,0,0,0),
		const video::SColor& maxStartColor = video::SColor(255,255,255,255),
		u32 lifeTimeMin=2000, u32 lifeTimeMax=4000,
		s32 maxAngleDegrees=0,
		const core::dimension2df& minStartSize = core::dimension2df(5.0f,5.0f),
		const core::dimension2df& maxStartSize = core::dimension2df(5.0f,5.0f) ) = 0;

	//! Creates a ring particle emitter.
	/** \param center: Center of ring
	\param radius: Distance of points from center, points will be rotated
	around the Y axis at a random 360 degrees and will then be shifted by
	the provided ringThickness values in each axis.
	\param ringThickness : thickness of the ring or how wide the ring is
	\param direction: Direction and speed of particle emission.
	\param minParticlesPerSecond: Minimal amount of particles emitted per
	second.
	\param maxParticlesPerSecond: Maximal amount of particles emitted per
	second.
	\param minStartColor: Minimal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param maxStartColor: Maximal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param lifeTimeMin: Minimal lifetime of a particle, in milliseconds.
	\param lifeTimeMax: Maximal lifetime of a particle, in milliseconds.
	\param maxAngleDegrees: Maximal angle in degrees, the emitting
	direction of the particle will differ from the original direction.
	\param minStartSize: Minimal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\param maxStartSize: Maximal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\return Pointer to the created particle emitter. To set this emitter
	as new emitter of this particle system, just call setEmitter(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleRingEmitter* createRingEmitter(
		const core::vector3df& center, f32 radius, f32 ringThickness,
		const core::vector3df& direction = core::vector3df(0.0f,0.03f,0.0f),
		u32 minParticlesPerSecond = 5,
		u32 maxParticlesPerSecond = 10,
		const video::SColor& minStartColor = video::SColor(255,0,0,0),
		const video::SColor& maxStartColor = video::SColor(255,255,255,255),
		u32 lifeTimeMin=2000, u32 lifeTimeMax=4000,
		s32 maxAngleDegrees=0,
		const core::dimension2df& minStartSize = core::dimension2df(5.0f,5.0f),
		const core::dimension2df& maxStartSize = core::dimension2df(5.0f,5.0f) ) = 0;

	//! Creates a sphere particle emitter.
	/** \param center: Center of sphere
	\param radius: Radius of sphere
	\param direction: Direction and speed of particle emission.
	\param minParticlesPerSecond: Minimal amount of particles emitted per
	second.
	\param maxParticlesPerSecond: Maximal amount of particles emitted per
	second.
	\param minStartColor: Minimal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param maxStartColor: Maximal initial start color of a particle. The
	real color of every particle is calculated as random interpolation
	between minStartColor and maxStartColor.
	\param lifeTimeMin: Minimal lifetime of a particle, in milliseconds.
	\param lifeTimeMax: Maximal lifetime of a particle, in milliseconds.
	\param maxAngleDegrees: Maximal angle in degrees, the emitting
	direction of the particle will differ from the original direction.
	\param minStartSize: Minimal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\param maxStartSize: Maximal initial start size of a particle. The
	real size of every particle is calculated as random interpolation
	between minStartSize and maxStartSize.
	\return Pointer to the created particle emitter. To set this emitter
	as new emitter of this particle system, just call setEmitter(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleSphereEmitter* createSphereEmitter(
		const core::vector3df& center, f32 radius,
		const core::vector3df& direction = core::vector3df(0.0f,0.03f,0.0f),
		u32 minParticlesPerSecond = 5,
		u32 maxParticlesPerSecond = 10,
		const video::SColor& minStartColor = video::SColor(255,0,0,0),
		const video::SColor& maxStartColor = video::SColor(255,255,255,255),
		u32 lifeTimeMin=2000, u32 lifeTimeMax=4000,
		s32 maxAngleDegrees=0,
		const core::dimension2df& minStartSize = core::dimension2df(5.0f,5.0f),
		const core::dimension2df& maxStartSize = core::dimension2df(5.0f,5.0f) ) = 0;

	//! Creates a point attraction affector.
	/** This affector modifies the positions of the particles and attracts
	them to a specified point at a specified speed per second.
	\param point: Point to attract particles to.
	\param speed: Speed in units per second, to attract to the specified
	point.
	\param attract: Whether the particles attract or detract from this
	point.
	\param affectX: Whether or not this will affect the X position of the
	particle.
	\param affectY: Whether or not this will affect the Y position of the
	particle.
	\param affectZ: Whether or not this will affect the Z position of the
	particle.
	\return Pointer to the created particle affector. To add this affector
	as new affector of this particle system, just call addAffector(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleAttractionAffector* createAttractionAffector(
		const core::vector3df& point, f32 speed = 1.0f, bool attract = true,
		bool affectX = true, bool affectY = true, bool affectZ = true) = 0;

	//! Creates a scale particle affector.
	/** This affector scales the particle to the a multiple of its size defined
	by the scaleTo variable.
	\param scaleTo: multiple of the size which the particle will be scaled to until deletion
	\return Pointer to the created particle affector.
	To add this affector as new affector of this particle system,
	just call addAffector(). Note that you'll have to drop() the
	returned pointer, after you don't need it any more, see
	IReferenceCounted::drop() for more information. */
	virtual IParticleAffector* createScaleParticleAffector(const core::dimension2df& scaleTo = core::dimension2df(1.0f, 1.0f)) = 0;

	//! Creates a fade out particle affector.
	/** This affector modifies the color of every particle and and reaches
	the final color when the particle dies. This affector looks really
	good, if the EMT_TRANSPARENT_ADD_COLOR material is used and the
	targetColor is video::SColor(0,0,0,0): Particles are fading out into
	void with this setting.
	\param targetColor: Color whereto the color of the particle is changed.
	\param timeNeededToFadeOut: How much time in milli seconds should the
	affector need to change the color to the targetColor.
	\return Pointer to the created particle affector. To add this affector
	as new affector of this particle system, just call addAffector(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleFadeOutAffector* createFadeOutParticleAffector(
		const video::SColor& targetColor = video::SColor(0,0,0,0),
		u32 timeNeededToFadeOut = 1000) = 0;

	//! Creates a gravity affector.
	/** This affector modifies the direction of the particle. It assumes
	that the particle is fired out of the emitter with huge force, but is
	loosing this after some time and is catched by the gravity then. This
	affector is ideal for creating things like fountains.
	\param gravity: Direction and force of gravity.
	\param timeForceLost: Time in milli seconds when the force of the
	emitter is totally lost and the particle does not move any more. This
	is the time where gravity fully affects the particle.
	\return Pointer to the created particle affector. To add this affector
	as new affector of this particle system, just call addAffector(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleGravityAffector* createGravityAffector(
		const core::vector3df& gravity = core::vector3df(0.0f,-0.03f,0.0f),
		u32 timeForceLost = 1000) = 0;

	//! Creates a rotation affector.
	/** This affector modifies the positions of the particles and attracts
	them to a specified point at a specified speed per second.
	\param speed: Rotation in degrees per second
	\param pivotPoint: Point to rotate the particles around
	\return Pointer to the created particle affector. To add this affector
	as new affector of this particle system, just call addAffector(). Note
	that you'll have to drop() the returned pointer, after you don't need
	it any more, see IReferenceCounted::drop() for more informations. */
	virtual IParticleRotationAffector* createRotationAffector(
		const core::vector3df& speed = core::vector3df(5.0f,5.0f,5.0f),
		const core::vector3df& pivotPoint = core::vector3df(0.0f,0.0f,0.0f) ) = 0;
};

} // end namespace scene
} // end namespace irr


#endif

