//
//  bbox.h
//
//

#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>
#include <stdint.h>

namespace spatial {
	namespace box {
		enum VolumeMode {
			eNormalVolume = 0, // Faster but can cause poor merges
			eSphericalVolume   // Slower but helps certain merge cases
		};

		enum RegionType {
			eNW = 0, ///< North-West (Top left)
			eNE,     ///< North-East (Top right)
			eSW,     ///< South-West (Bottom left)
			eSE      ///< South-East (Bottom right)
		};

		/// Initialize a bounding box with an empty box.
		struct empty_init {};
	} // namespace spatial

	/// Minimal bounding bbox (n-dimensional)
	template <typename T, int Dimension> struct BoundingBox {

		T min[Dimension];
		T max[Dimension];

		BoundingBox();
		BoundingBox(box::empty_init);
		BoundingBox(const T min[Dimension], const T max[Dimension]);

		void extend(const T point[Dimension]);
		void extend(const BoundingBox &bbox);
		BoundingBox extended(const BoundingBox &bbox) const;
		void set(const T min[Dimension], const T max[Dimension]);
		void translate(const T point[Dimension]);

		bool overlaps(const BoundingBox &bbox) const;
		bool overlaps(const T point[Dimension], T radius) const;
		bool contains(const BoundingBox &bbox) const;
		bool contains(const T point[Dimension]) const;

		float distance(const T point[Dimension]) const;
		T distanceSquare(const T point[Dimension]) const;

		void center(T center[Dimension]) const;

		template <int VolumeMode, typename RealType> RealType volume() const;
		BoundingBox quad2d(box::RegionType type) const;

	private:
		void checkValid() const;

		template <typename RealType> RealType normalVolume() const;
		// The exact volume of the bounding sphere for the given bbox.
		template <typename RealType> RealType sphericalVolume() const;
		/// Unit sphere constant for required number of dimensions
		template <typename RealType> static RealType unitSphereVolume();
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	namespace detail {
		/// C++03 backward compatibility
		/// http://en.cppreference.com/w/cpp/types/numeric_limits/lowest
		template <typename T, bool is_integer> struct NumericLimitsImpl;

		template <typename T> struct NumericLimitsImpl<T, false> {
			static const T highest() { return std::numeric_limits<T>::max(); }

			static const T lowest() { return -std::numeric_limits<T>::max(); }
		};

		template <typename T> struct NumericLimitsImpl<T, true> {
			static const T highest() { return std::numeric_limits<T>::max(); }

			static const T lowest() { return std::numeric_limits<T>::min(); }
		};

		template <typename T>
		struct NumericLimits
			: public NumericLimitsImpl<T, std::numeric_limits<T>::is_integer> {};
	} // namespace detail

#define BBOX_TEMPLATE template <typename T, int Dimension>
#define BBOX_QUAL BoundingBox<T, Dimension>

	BBOX_TEMPLATE
		BBOX_QUAL::BoundingBox() {}

	BBOX_TEMPLATE
		BBOX_QUAL::BoundingBox(const T min[Dimension], const T max[Dimension]) {
		set(min, max);
	}

	BBOX_TEMPLATE
		BBOX_QUAL::BoundingBox(box::empty_init) {
		for (size_t index = 0; index < (size_t)Dimension; ++index) {
			min[index] = spatial::detail::NumericLimits<T>::highest();
			max[index] = spatial::detail::NumericLimits<T>::lowest();
		}
	}

	BBOX_TEMPLATE
		void BBOX_QUAL::set(const T min[Dimension], const T max[Dimension]) {
		// loop will get unrolled
		for (int axis = 0; axis < Dimension; ++axis) {
			this->min[axis] = min[axis];
			this->max[axis] = max[axis];
		}
		checkValid();
	}

	BBOX_TEMPLATE
		void BBOX_QUAL::translate(const T point[Dimension]) {
		// loop will get unrolled
		for (int axis = 0; axis < Dimension; ++axis) {
			this->min[axis] += point[axis];
			this->max[axis] += point[axis];
		}
		checkValid();
	}

	BBOX_TEMPLATE
		void BBOX_QUAL::extend(const T point[Dimension]) {
		// loop will get unrolled
		for (int index = 0; index < Dimension; ++index) {
			min[index] = std::min(min[index], point[index]);
			max[index] = std::max(max[index], point[index]);
		}
		checkValid();
	}

	BBOX_TEMPLATE
		void BBOX_QUAL::extend(const BoundingBox &bbox) {
		bbox.checkValid();
		// loop will get unrolled
		for (int index = 0; index < Dimension; ++index) {
			min[index] = std::min(min[index], bbox.min[index]);
			max[index] = std::max(max[index], bbox.max[index]);
		}
		checkValid();
	}

	BBOX_TEMPLATE
		BBOX_QUAL
		BBOX_QUAL::extended(const BoundingBox &obbox) const {
		checkValid();

		BoundingBox res;
		// loop will get unrolled
		for (int index = 0; index < Dimension; ++index) {
			res.min[index] = std::min(min[index], obbox.min[index]);
			res.max[index] = std::max(max[index], obbox.max[index]);
		}
		return res;
	}

	BBOX_TEMPLATE
		bool BBOX_QUAL::overlaps(const BoundingBox &bbox) const {
		// loop will get unrolled
		for (int index = 0; index < Dimension; ++index) {
			if (min[index] > bbox.max[index] || bbox.min[index] > max[index]) {
				return false;
			}
		}
		return true;
	}

	BBOX_TEMPLATE
		bool BBOX_QUAL::overlaps(const T center[Dimension], T radius) const {
		T point[Dimension];
		for (int index = 0; index < Dimension; ++index)
			point[index] = center[index];

		// find the closest point near the circle
		for (int index = 0; index < Dimension; ++index) {
			if (point[index] > max[index])
				point[index] = max[index];
			if (point[index] < min[index])
				point[index] = min[index];
		}

		// compute euclidean distance between points
		double distance = 0;
		for (int index = 0; index < Dimension; ++index) {
			const T d = point[index] - center[index];
			distance += (double)d * d;
		}
		return distance < (radius * radius); // avoid square root
	}

	BBOX_TEMPLATE
		bool BBOX_QUAL::contains(const T point[Dimension]) const {
		// loop will get unrolled
		for (int index = 0; index < Dimension; ++index) {
			if (min[index] > point[index] || point[index] > max[index]) {
				return false;
			}
		}
		return true;
	}

	BBOX_TEMPLATE
		bool BBOX_QUAL::contains(const BoundingBox &bbox) const {
		// loop will get unrolled
		for (int index = 0; index < Dimension; ++index) {
			if (min[index] > bbox.min[index] || bbox.max[index] > max[index]) {
				return false;
			}
		}
		return true;
	}

	BBOX_TEMPLATE
		void BBOX_QUAL::center(T center[Dimension]) const {
		for (int i = 0; i < Dimension; ++i) {
			center[i] = min[i] + (max[i] - min[i]) / 2;
		}
	}

	BBOX_TEMPLATE
		T BBOX_QUAL::distanceSquare(const T point[Dimension]) const {

		T d = std::max(std::max(min[0] - point[0], (T)0), point[0] - max[0]);
		d *= d;
		for (int i = 1; i < Dimension; i++)
		{
			T temp = std::max(std::max(min[i] - point[i], (T)0), point[i] - max[i]);
			d += temp * temp;
		}

		return d;
	}

	BBOX_TEMPLATE
		float BBOX_QUAL::distance(const T point[Dimension]) const {

		return std::sqrt(float(distanceSquare(point)));
	}

	BBOX_TEMPLATE
		template <int VolumeMode, typename RealType>
	RealType BBOX_QUAL::volume() const {
		if (VolumeMode == spatial::box::eSphericalVolume)
			return sphericalVolume<RealType>();

		return normalVolume<RealType>();
	}

	BBOX_TEMPLATE
		BBOX_QUAL BBOX_QUAL::quad2d(box::RegionType type) const {
		const T halfW = (max[0] - min[0]) / 2;
		const T halfH = (max[1] - min[1]) / 2;
		switch (type) {
		case box::eNW: {
			const T hmin[Dimension] = { min[0], min[1] + halfH };
			const T hmax[Dimension] = { min[0] + halfW, max[1] };
			return BoundingBox(hmin, hmax);
		}
		case box::eNE: {
			const T hmin[Dimension] = { min[0] + halfW, min[1] + halfH };
			return BoundingBox(hmin, max);
		}
		case box::eSW: {
			const T hmax[Dimension] = { min[0] + halfW, min[1] + halfH };
			return BoundingBox(min, hmax);
		}
		case box::eSE: {
			const T hmin[Dimension] = { min[0] + halfW, min[1] };
			const T hmax[Dimension] = { max[0], min[1] + halfH };
			return BoundingBox(hmin, hmax);
		}
		default:
			assert(false);
			return *this;
		}
	}

	BBOX_TEMPLATE
		void BBOX_QUAL::checkValid() const {
#ifndef NDEBUG
		for (int index = 0; index < Dimension; ++index) {
			assert(min[index] <= max[index]);
		}
#endif
	}

	BBOX_TEMPLATE
		template <typename RealType> RealType BBOX_QUAL::normalVolume() const {
		RealType volume = (RealType)1;

		for (int index = 0; index < Dimension; ++index) {
			volume *= max[index] - min[index];
		}

		assert(volume >= (RealType)0);
		return volume;
	}

	// The exact volume of the bounding sphere for the given bbox
	BBOX_TEMPLATE
		template <typename RealType> RealType BBOX_QUAL::sphericalVolume() const {
		RealType sumOfSquares = (RealType)0;

		for (int index = 0; index < Dimension; ++index) {
			RealType halfExtent = (max[index] - min[index]) * (RealType)0.5;
			sumOfSquares += halfExtent * halfExtent;
		}

		RealType radius = (RealType)std::sqrt(sumOfSquares);

		// Pow maybe slow, so test for common dims like 2,3 and just use x*x, x*x*x.
		if (Dimension == 3) {
			return (radius * radius * radius * unitSphereVolume<RealType>());
		}
		else if (Dimension == 2) {
			return (radius * radius * unitSphereVolume<RealType>());
		}
		else {
			return (RealType)(pow(radius, Dimension) * unitSphereVolume<RealType>());
		}
	}

	BBOX_TEMPLATE
		template <typename RealType> RealType BBOX_QUAL::unitSphereVolume() {
		// Precomputed volumes of the unit spheres for the first few dimensions
		static const float kVolumes[] = {
			0.000000f, 2.000000f, 3.141593f, // Dimension  0,1,2
			4.188790f, 4.934802f, 5.263789f, // Dimension  3,4,5
			5.167713f, 4.724766f, 4.058712f, // Dimension  6,7,8
			3.298509f, 2.550164f, 1.884104f, // Dimension  9,10,11
			1.335263f, 0.910629f, 0.599265f, // Dimension  12,13,14
			0.381443f, 0.235331f, 0.140981f, // Dimension  15,16,17
			0.082146f, 0.046622f, 0.025807f, // Dimension  18,19,20
		};
		static const RealType val = (RealType)kVolumes[Dimension];
		return val;
	} // namespace box

	template <typename T>
	std::ostream &operator<<(std::ostream &stream, const BoundingBox<T, 2> &bbox) {
		stream << "min: " << bbox.min[0] << " " << bbox.min[1] << " ";
		stream << "max: " << bbox.max[0] << " " << bbox.max[1];
		return stream;
	}

	template <typename T>
	std::ostream &operator<<(std::ostream &stream, const BoundingBox<T, 3> &bbox) {
		stream << "min: " << bbox.min[0] << " " << bbox.min[1] << " " << bbox.min[2]
			<< " ";
		stream << "max: " << bbox.max[0] << " " << bbox.max[1] << " " << bbox.max[2]
			<< " ";
		return stream;
	}

#undef BBOX_TEMPLATE
#undef BBOX_QUAL
} // namespace spatial
