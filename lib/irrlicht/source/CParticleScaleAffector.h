// Copyright (C) 2010-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef C_PARTICLE_SCALE_AFFECTOR_H
#define C_PARTICLE_SCALE_AFFECTOR_H

#include <IParticleAffector.h>

namespace irr
{
	namespace scene
	{
		class CParticleScaleAffector : public IParticleAffector
		{
		public:
			CParticleScaleAffector(const core::dimension2df& scaleTo = core::dimension2df(1.0f, 1.0f));

			virtual void affect(u32 now, SParticle *particlearray, u32 count);

			//! Writes attributes of the object.
			//! Implement this to expose the attributes of your scene node animator for
			//! scripting languages, editors, debuggers or xml serialization purposes.
			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

			//! Reads attributes of the object.
			//! Implement this to set the attributes of your scene node animator for
			//! scripting languages, editors, debuggers or xml deserialization purposes.
			//! \param startIndex: start index where to start reading attributes.
			//! \return: returns last index of an attribute read by this affector
			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

			//! Get emitter type
			virtual E_PARTICLE_AFFECTOR_TYPE getType() const;

		protected:
			core::dimension2df ScaleTo;
		};
	}
}


#endif // C_PARTICLE_SCALE_AFFECTOR_H

