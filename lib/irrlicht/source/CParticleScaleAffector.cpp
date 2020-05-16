// Copyright (C) 2010-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticleScaleAffector.h"
#include "IAttributes.h"

namespace irr
{
	namespace scene
	{
		CParticleScaleAffector::CParticleScaleAffector(const core::dimension2df& scaleTo)
			: ScaleTo(scaleTo)
		{
			#ifdef _DEBUG
			setDebugName("CParticleScaleAffector");
			#endif
		}


		void CParticleScaleAffector::affect (u32 now, SParticle *particlearray, u32 count)
		{
			for(u32 i=0;i<count;i++)
			{
				const u32 maxdiff = particlearray[i].endTime - particlearray[i].startTime;
				const u32 curdiff = now - particlearray[i].startTime;
				const f32 newscale = (f32)curdiff/maxdiff;
				particlearray[i].size = particlearray[i].startSize+ScaleTo*newscale;
			}
		}


		void CParticleScaleAffector::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
			out->addFloat("ScaleToWidth", ScaleTo.Width);
			out->addFloat("ScaleToHeight", ScaleTo.Height);
		}


		void CParticleScaleAffector::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
			ScaleTo.Width = in->getAttributeAsFloat("ScaleToWidth");
			ScaleTo.Height = in->getAttributeAsFloat("ScaleToHeight");
		}


		E_PARTICLE_AFFECTOR_TYPE CParticleScaleAffector::getType() const
		{
			return scene::EPAT_SCALE;
		}
	}
}

