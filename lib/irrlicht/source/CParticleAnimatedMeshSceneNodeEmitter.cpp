// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticleAnimatedMeshSceneNodeEmitter.h"
#include "IAnimatedMeshSceneNode.h"
#include "IMesh.h"
#include "os.h"

namespace irr
{
namespace scene
{

//! constructor
CParticleAnimatedMeshSceneNodeEmitter::CParticleAnimatedMeshSceneNodeEmitter(
		IAnimatedMeshSceneNode* node, bool useNormalDirection,
		const core::vector3df& direction, f32 normalDirectionModifier,
		s32 mbNumber, bool everyMeshVertex,
		u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
		const video::SColor& minStartColor, const video::SColor& maxStartColor,
		u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
		const core::dimension2df& minStartSize, const core::dimension2df& maxStartSize )
	: Node(0), AnimatedMesh(0), BaseMesh(0), TotalVertices(0), MBCount(0), MBNumber(mbNumber),
	Direction(direction), NormalDirectionModifier(normalDirectionModifier),
	MinParticlesPerSecond(minParticlesPerSecond), MaxParticlesPerSecond(maxParticlesPerSecond),
	MinStartColor(minStartColor), MaxStartColor(maxStartColor),
	MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax),
	MaxStartSize(maxStartSize), MinStartSize(minStartSize),
	Time(0), Emitted(0), MaxAngleDegrees(maxAngleDegrees),
	EveryMeshVertex(everyMeshVertex), UseNormalDirection(useNormalDirection)
{
	#ifdef _DEBUG
	setDebugName("CParticleAnimatedMeshSceneNodeEmitter");
	#endif
	setAnimatedMeshSceneNode(node);
}


//! Prepares an array with new particles to emitt into the system
//! and returns how much new particles there are.
s32 CParticleAnimatedMeshSceneNodeEmitter::emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray)
{
	Time += timeSinceLastCall;

	const u32 pps = (MaxParticlesPerSecond - MinParticlesPerSecond);
	const f32 perSecond = pps ? ((f32)MinParticlesPerSecond + os::Randomizer::frand() * pps) : MinParticlesPerSecond;
	const f32 everyWhatMillisecond = 1000.0f / perSecond;

	if(Time > everyWhatMillisecond)
	{
		Particles.set_used(0);
		u32 amount = (u32)((Time / everyWhatMillisecond) + 0.5f);
		Time = 0;
		SParticle p;

		if(amount > MaxParticlesPerSecond * 2)
			amount = MaxParticlesPerSecond * 2;

		// Get Mesh for this frame
		IMesh* frameMesh = AnimatedMesh->getMesh( core::floor32(Node->getFrameNr()),
				255, Node->getStartFrame(), Node->getEndFrame() );
		for(u32 i=0; i<amount; ++i)
		{
			if( EveryMeshVertex )
			{
				for( u32 j=0; j<frameMesh->getMeshBufferCount(); ++j )
				{
					for( u32 k=0; k<frameMesh->getMeshBuffer(j)->getVertexCount(); ++k )
					{
						p.pos = frameMesh->getMeshBuffer(j)->getPosition(k);
						if( UseNormalDirection )
							p.vector = frameMesh->getMeshBuffer(j)->getNormal(k) /
								NormalDirectionModifier;
						else
							p.vector = Direction;

						p.startTime = now;

						if( MaxAngleDegrees )
						{
							core::vector3df tgt = p.vector;
							tgt.rotateXYBy(os::Randomizer::frand() * MaxAngleDegrees);
							tgt.rotateYZBy(os::Randomizer::frand() * MaxAngleDegrees);
							tgt.rotateXZBy(os::Randomizer::frand() * MaxAngleDegrees);
							p.vector = tgt;
						}

						p.endTime = now + MinLifeTime;
						if (MaxLifeTime != MinLifeTime)
							p.endTime += os::Randomizer::rand() % (MaxLifeTime - MinLifeTime);

						if (MinStartColor==MaxStartColor)
							p.color=MinStartColor;
						else
							p.color = MinStartColor.getInterpolated(MaxStartColor, os::Randomizer::frand());

						p.startColor = p.color;
						p.startVector = p.vector;

						if (MinStartSize==MaxStartSize)
							p.startSize = MinStartSize;
						else
							p.startSize = MinStartSize.getInterpolated(MaxStartSize, os::Randomizer::frand());
						p.size = p.startSize;

						Particles.push_back(p);
					}
				}
			}
			else
			{
				s32 randomMB = 0;
				if( MBNumber < 0 )
					randomMB = os::Randomizer::rand() % MBCount;
				else
					randomMB = MBNumber;

				u32 vertexNumber = frameMesh->getMeshBuffer(randomMB)->getVertexCount();
				if (!vertexNumber)
					continue;
				vertexNumber = os::Randomizer::rand() % vertexNumber;

				p.pos = frameMesh->getMeshBuffer(randomMB)->getPosition(vertexNumber);
				if( UseNormalDirection )
					p.vector = frameMesh->getMeshBuffer(randomMB)->getNormal(vertexNumber) /
						NormalDirectionModifier;
				else
					p.vector = Direction;

				p.startTime = now;

				if( MaxAngleDegrees )
				{
					core::vector3df tgt = Direction;
					tgt.rotateXYBy(os::Randomizer::frand() * MaxAngleDegrees);
					tgt.rotateYZBy(os::Randomizer::frand() * MaxAngleDegrees);
					tgt.rotateXZBy(os::Randomizer::frand() * MaxAngleDegrees);
					p.vector = tgt;
				}

				p.endTime = now + MinLifeTime;
				if (MaxLifeTime != MinLifeTime)
					p.endTime += os::Randomizer::rand() % (MaxLifeTime - MinLifeTime);

				if (MinStartColor==MaxStartColor)
					p.color=MinStartColor;
				else
					p.color = MinStartColor.getInterpolated(MaxStartColor, os::Randomizer::frand());

				p.startColor = p.color;
				p.startVector = p.vector;

				if (MinStartSize==MaxStartSize)
					p.startSize = MinStartSize;
				else
					p.startSize = MinStartSize.getInterpolated(MaxStartSize, os::Randomizer::frand());
				p.size = p.startSize;

				Particles.push_back(p);
			}
		}

		outArray = Particles.pointer();

		return Particles.size();
	}

	return 0;
}


//! Set Mesh to emit particles from
void CParticleAnimatedMeshSceneNodeEmitter::setAnimatedMeshSceneNode( IAnimatedMeshSceneNode* node )
{
	Node = node;
	AnimatedMesh = 0;
	BaseMesh = 0;
	TotalVertices = 0;
	VertexPerMeshBufferList.clear();
	if ( !node )
	{
		return;
	}

	AnimatedMesh = node->getMesh();
	BaseMesh = AnimatedMesh->getMesh(0);

	MBCount = BaseMesh->getMeshBufferCount();
	VertexPerMeshBufferList.reallocate(MBCount);
	for( u32 i = 0; i < MBCount; ++i )
	{
		VertexPerMeshBufferList.push_back( BaseMesh->getMeshBuffer(i)->getVertexCount() );
		TotalVertices += BaseMesh->getMeshBuffer(i)->getVertexCount();
	}
}

} // end namespace scene
} // end namespace irr

