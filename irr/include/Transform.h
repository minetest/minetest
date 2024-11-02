#pragma once

#include "irrTypes.h"
#include "matrix4.h"
#include "vector3d.h"
#include "quaternion.h"

namespace irr {
namespace core {

struct Transform {
	core::vector3df translation;
	core::quaternion rotation;
	core::vector3df scale = {1, 1, 1};

	//! Lerps from this to other: progress = 0 is all this, progress = 1 is all other.
	Transform lerp(const Transform &other, f32 progress) {
		core::quaternion interpolated_rotation;
		interpolated_rotation.slerp(rotation, other.rotation, progress);
		return {
			translation.getInterpolated(other.translation, progress),
			interpolated_rotation,
			scale.getInterpolated(other.scale, progress),
		};
	}

	core::matrix4 toMatrix() const {
		core::matrix4 T, R, S;
		T.setTranslation(translation);
		rotation.getMatrix_transposed(R);
		S.setScale(scale);
		return T * R * S;
	}

	// There deliberately is *no* way to turn a matrix into a transform here.
	// There is pretty much only one reason for this matrix decomposition
	// (file formats which store transforms as matrices);
	// otherwise you should just have kept the transform around to begin with.
};

} // namespace core
} // namespace irr