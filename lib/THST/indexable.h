//
//  indexable.h
//
//

#pragma once

namespace spatial {
	// Indexable getter for bbox limits for a given type
	template <typename T, typename ValueType> struct Indexable {
		const T *min(const ValueType &value) const { return value.min; }
		const T *max(const ValueType &value) const { return value.max; }
	};
} // namespace spatial
