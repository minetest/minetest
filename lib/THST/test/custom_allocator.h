
#pragma once

#include <cassert>
#include <iostream>
#include <vector>

namespace test {

	// example of a cache friendly allocator with contiguous memory
	template <class NodeClass, bool IsDebugMode = true,
		size_t DefaultNodeCount = 100>
		struct tree_allocator {

		typedef NodeClass value_type;
		typedef NodeClass *ptr_type;

		enum { is_overflowable = 0 };

		tree_allocator() : buffer(DefaultNodeCount), count(0), index(0) {}

		ptr_type allocate(int level) {
			if (IsDebugMode)
				std::cout << "Allocate node: " << level << " current count: " << count
				<< "\n";
			++count;
			assert(index + 1 <= buffer.size());
			ptr_type node = &buffer[index++];
			*node = NodeClass(level);
			return node;
		}

		void deallocate(const ptr_type node) {
			if (IsDebugMode)
				std::cout << "Deallocate node: " << node->level << " - " << node->count
				<< " current count: " << count << "\n";

			assert(count > 0);
			if (count > 1)
				--count;
			else {
				count = 0;
				index = 0;
			}
		}

		void clear() {
			buffer.clear();
			count = index = 0;
		}
		void resize(size_t count) { buffer.resize(count); }

		bool overflowed() const { return false; }

		std::vector<NodeClass> buffer;
		size_t count;
		size_t index;
	};

	template <class NodeClass> struct heap_allocator {
		typedef NodeClass value_type;
		typedef NodeClass *ptr_type;

		enum { is_overflowable = 0 };

		heap_allocator() : count(0) {}

		ptr_type allocate(int level) {
			++count;
			return new NodeClass(level);
		}

		void deallocate(const ptr_type node) {
			assert(node);
			delete node;
		}

		bool overflowed() const { return false; }

		size_t count;
	};
}
