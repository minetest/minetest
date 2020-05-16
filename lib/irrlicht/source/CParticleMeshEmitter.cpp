// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "CParticleMeshEmitter.h"
#include "os.h"

namespace irr
{
namespace scene
{

//! constructor
CParticleMeshEmitter::CParticleMeshEmitter(
	IMesh* mesh, bool useNormalDirection,
	const core::vector3df& direction, f32 normalDirectionModifier,
	s32 mbNumber, bool everyMeshVertex,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
	: Mesh(0), TotalVertices(0), MBCount(0), MBNumber(mbNumber),
	NormalDirectionModifier(normalDirectionModifier), Direction(direction),
	MaxStartSize(maxStartSize), MinStartSize(minStartSize),
	MinParticlesPerSecond(minParticlesPerSecond), MaxParticlesPerSecond(maxParticlesPerSecond),
	MinStartColor(minStartColor), MaxStartColor(maxStartColor),
	MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax),
	Time(0), Emitted(0), MaxAngleDegrees(maxAngleDegrees),
	EveryMeshVertex(everyMeshVertex), UseNormalDirection(useNormalDirection)
{
	#ifdef _DEBUG
	setDebugName("CParticleMeshEmitter");
	#endif
	setMesh(mesh);
}


//! Prepares an array with new particles to emitt into the system
//! and returns how much new particles there are.
s32 CParticleMeshEmitter::emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray)
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

		for(u32 i=0; i<amount; ++i)
		{
			if( EveryMeshVertex )
			{
				for( u32 j=0; j<Mesh->getMeshBufferCount(); ++j )
				{
					for( u32 k=0; k<Mesh->getMeshBuffer(j)->getVertexCount(); ++k )
					{
						p.pos = Mesh->getMeshBuffer(j)->getPosition(k);
						if( UseNormalDirection )
							p.vector = Mesh->getMeshBuffer(j)->getNormal(k) /
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
				const s32 randomMB = (MBNumber < 0) ? (os::Randomizer::rand() % MBCount) : MBNumber;

				u32 vertexNumber = Mesh->getMeshBuffer(randomMB)->getVertexCount();
				if (!vertexNumber)
					continue;
				vertexNumber = os::Randomizer::rand() % vertexNumber;

				p.pos = Mesh->getMeshBuffer(randomMB)->getPosition(vertexNumber);
				if( UseNormalDirection )
					p.vector = Mesh->getMeshBuffer(randomMB)->getNormal(vertexNumber) /
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
void CParticleMeshEmitter::setMesh(IMesh* mesh)
{
	Mesh = mesh;

	TotalVertices = 0;
	MBCount = 0;
	VertexPerMeshBufferList.clear();

	if ( !Mesh )
		return;

	MBCount = Mesh->getMeshBufferCount();
	VertexPerMeshBufferList.reallocate(MBCount);
	for( u32 i = 0; i < MBCount; ++i )
	{
		VertexPerMeshBufferList.push_back( Mesh->getMeshBuffer(i)->getVertexCount() );
		TotalVertices += Mesh->getMeshBuffer(i)->getVertexCount();
	}
}


} // end namespace scene
} // end namespace irr

