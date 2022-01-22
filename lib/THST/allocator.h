//
//  allocator.h
//
//

#pragma once

#include "config.h"

#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
#include <assert.h>
#include <stdint.h>
#elif SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_STD_ALLOCATOR
#include <memory>
#endif

namespace spatial {

#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR

	/// A simplified heap allocator.
	template <class NodeClass> struct allocator {
		typedef NodeClass value_type;

		/// @note If true then overflow checks will be performed.
		enum { is_overflowable = 0 };

		value_type *allocate(int level) { return new NodeClass(level); }

		void deallocate(const value_type *node) {
			assert(node);
			delete node;
		}

		/// If true then an overflowed has occurred.
		/// @note Only used if is_overflowable is true.
		bool overflowed() const { return false; }
	};

#elif SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_STD_ALLOCATOR

	using std::allocator;
#else
#error "Unknown allocator type!"
#endif
} // namespace spatial
