//
//  QuadTree.h
//
//

#pragma once

#include "allocator.h"
#include "bbox.h"
#include "config.h"
#include "indexable.h"
#include "predicates.h"
#include "quad_tree_detail.h"
#include <vector>

namespace spatial {
	/**
	 \class QuadTree
	 \brief Implementation of a Quadtree for 2D space.

	 It has the following properties:
	 - hierarchical, you can add values to the internal nodes
	 - custom indexable getter similar to boost's
	 - custom allocation for internal nodes

	 @tparam T                  type of the space(eg. int, float, etc.)
	 @tparam ValueType		    type of value stored in the tree's nodes
	 @tparam max_child_items    maximum number of items per node
	 @tparam indexable_getter   the indexable getter, i.e. the getter for the
	 bounding box of a value
	 @tparam custom_allocator   the allocator class items exceeds the given factor
	 then it'll use only the parent value

	 @note It's recommended that ValueType should be a fast to copy object, eg: int,
	 id, obj*, etc.
	*/
	template <class T, class ValueType, int max_child_items = 16,
		typename indexable_getter = Indexable<T, ValueType>,
		typename custom_allocator = spatial::allocator<
		detail::QuadTreeNode<T, ValueType, max_child_items>>>
		class QuadTree {
		public:
			typedef BoundingBox<T, 2> bbox_type;
			typedef custom_allocator allocator_type;

			static const size_t max_items = max_child_items;

		private:
			typedef detail::QuadTreeNode<T, ValueType, max_child_items> node_type;
			typedef detail::QuadTreeObject<ValueType, bbox_type, node_type> object_type;
			typedef node_type *node_ptr_type;

		public:
			class base_iterator : public detail::QuadTreeStack<node_type> {
			public:
				/// Returns the depth level of the current branch.
				int level() const;

				/// Is iterator still valid.
				bool valid() const;
				/// Access the current data element. Caller must be sure iterator is not
				/// NULL first.
				ValueType &operator*();
				/// Access the current data element. Caller must be sure iterator is not
				/// NULL first.
				const ValueType &operator*() const;
				const bbox_type &bbox() const;

			protected:
				base_iterator(const int maxLevel);

				const int m_maxLevel;

				typedef detail::QuadTreeStack<node_type> base_type;
				using base_type::m_stack;
				using base_type::m_tos;
			};

			/**
			 @brief Iterator to traverese the items of a node of the tree.
			 */
			struct node_iterator {
				node_iterator();

				/// Returns the depth level of the current branch.
				int level() const;
				/// Is iterator still valid.
				bool valid() const;
				/// Access the current data element. Caller must be sure iterator is not
				/// NULL first.
				ValueType &operator*();
				/// Access the current data element. Caller must be sure iterator is not
				/// NULL first.
				const ValueType &operator*() const;
				const bbox_type &bbox() const;
				/// Advances to the next item.
				void next();

			private:
				node_iterator(node_ptr_type node, int maxLevel);
				node_iterator(node_ptr_type node, size_t childIndex, int maxLevel);

				const int m_maxLevel;
				size_t m_childIndex;
				int m_objectIndex;
				node_ptr_type m_node;

				friend class QuadTree;
			};

			/**
			@brief Iterator to traverse the whole tree similar to depth first search. It
			reaches the lowest level
			(i.e. leaves) and afterwards visits the higher levels until it reaches the
			root(highest) level.
			*/
			class depth_iterator : public base_iterator {
				typedef base_iterator base_type;
				using base_type::m_stack;
				using base_type::m_tos;
				using base_type::push;
				using base_type::pop;

			public:
				// Returns a lower level, ie. child, node of the current branch.
				node_iterator child();
				node_iterator current();

				/// Access the current data element. Caller must be sure iterator is not
				/// NULL first.
				ValueType &operator*();
				/// Access the current data element. Caller must be sure iterator is not
				/// NULL first.
				const ValueType &operator*() const;
				/// Advances to the next item.
				void next();

			private:
				depth_iterator(const int maxLevel);
				friend class QuadTree;
			};

			/**
			 @brief Iterator to traverse only the leaves of the tree similar, left to
			 right order.
			 */
			class leaf_iterator : public base_iterator {
				typedef base_iterator base_type;
				using base_type::m_stack;
				using base_type::m_tos;
				using base_type::push;
				using base_type::pop;

			public:
				/// Advances to the next item.
				void next();

			private:
				leaf_iterator(const int maxLevel);
				friend class QuadTree;
			};

			QuadTree(const T min[2], const T max[2],                  //
				indexable_getter indexable = indexable_getter(), //
				const allocator_type &allocator = allocator_type());
			template <typename Iter>
			QuadTree(const T min[2], const T max[2],                  //
				Iter first, Iter last,                           //
				indexable_getter indexable = indexable_getter(), //
				const allocator_type &allocator = allocator_type());
			QuadTree(const QuadTree &src);
#ifdef SPATIAL_TREE_USE_CPP11
			QuadTree(QuadTree &&src);
#endif
			~QuadTree();

			QuadTree &operator=(const QuadTree &rhs);
#ifdef SPATIAL_TREE_USE_CPP11
			QuadTree &operator=(QuadTree &&rhs);
#endif

			void swap(QuadTree &other);

			///@note The tree must be empty before doing this.
			void setBox(const T min[2], const T max[2]);
			template <typename Iter> void insert(Iter first, Iter last);
			void insert(const ValueType &value);

			/// Translates the internal boxes with the given offset point.
			void translate(const T point[2]);

			/// Special query to find all within search rectangle using the hierarchical
			/// order.
			/// @see spatial::SpatialPredicate for available predicates.
			/// \return Returns the number of entries found.
			template <typename Predicate, typename OutIter>
			size_t hierachical_query(const Predicate &predicate, OutIter out_it) const;
			///@param containment_factor the containment factor in percentange used for
			/// query, if the total number of visible.
			///@note Only used for hierarchical query.
			void setContainmentFactor(int factor);

			/// @see spatial::SpatialPredicate for available predicates.
			template <typename Predicate> bool query(const Predicate &predicate) const;
			template <typename Predicate, typename OutIter>
			size_t query(const Predicate &predicate, OutIter out_it) const;

			/// Remove all entries from tree
			void clear(bool recursiveCleanup = true);
			/// Count the data elements in this container.
			size_t count() const;
			/// Returns the number of levels(height) of the tree.
			int levels() const;
			/// Returns the bbox of the root node.
			bbox_type bbox() const;
			/// Returns the custom allocator
			allocator_type &allocator();
			const allocator_type &allocator() const;

			node_iterator root();
			depth_iterator dbegin();
			leaf_iterator lbegin();

		private:
			indexable_getter m_indexable;
			mutable allocator_type m_allocator;

			size_t m_count;
			int m_levels;
			float m_factor;
			node_ptr_type m_root;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TREE_TEMPLATE                                                          \
  template <class T, class ValueType, int max_child_items,                     \
            typename indexable_getter, typename custom_allocator>

#define TREE_QUAL                                                              \
  QuadTree<T, ValueType, max_child_items, indexable_getter, custom_allocator>

	TREE_TEMPLATE
		TREE_QUAL::QuadTree(const T min[2], const T max[2], //
			indexable_getter indexable /*= indexable_getter()*/,
			const allocator_type &allocator /*= allocator_type()*/)
		: m_indexable(indexable), m_allocator(allocator), m_count(0), m_levels(0),
		m_factor(60), m_root(NULL) {
		SPATIAL_TREE_STATIC_ASSERT((max_child_items > 1), "Invalid child size!");

		m_root = detail::allocate(m_allocator, 0);
		m_root->box.set(min, max);
	}

	TREE_TEMPLATE
		template <typename Iter>
	TREE_QUAL::QuadTree(const T min[2], const T max[2], //
		Iter first, Iter last,          //
		indexable_getter indexable /*= indexable_getter()*/,
		const allocator_type &allocator /*= allocator_type()*/)
		: m_indexable(indexable), m_allocator(allocator), m_count(0), m_levels(0),
		m_factor(60) {
		SPATIAL_TREE_STATIC_ASSERT((max_child_items > 1), "Invalid child size!");

		m_root = detail::allocate(m_allocator, 0);
		m_root->box.set(min, max);

		insert(first, last);
	}

	TREE_TEMPLATE
		TREE_QUAL::QuadTree(const QuadTree &src)
		: m_indexable(src.m_indexable), m_allocator(src.m_allocator),
		m_count(src.m_count), m_levels(src.m_levels), m_factor(src.m_factor),
		m_root(detail::allocate(m_allocator, 0)) {
		m_root->copy(*src.m_root, m_allocator);
	}

#ifdef SPATIAL_TREE_USE_CPP11
	TREE_TEMPLATE
		TREE_QUAL::QuadTree(QuadTree &&src) : m_root(NULL) { swap(src); }
#endif

	TREE_TEMPLATE
		TREE_QUAL::~QuadTree() {
		if (m_root)
			m_root->clear(m_allocator);
		detail::deallocate(m_allocator, m_root);
	}

	TREE_TEMPLATE
		TREE_QUAL &TREE_QUAL::operator=(const QuadTree &rhs) {
		if (&rhs != this) {
			if (m_count > 0)
				clear(true);

			m_count = rhs.m_count;
			m_levels = rhs.m_levels;
			m_factor = rhs.m_factor;
			m_allocator = rhs.m_allocator;
			m_indexable = rhs.m_indexable;
			m_root->copy(*rhs.m_root, m_allocator);
		}
		return *this;
	}

#ifdef SPATIAL_TREE_USE_CPP11
	TREE_TEMPLATE
		TREE_QUAL &TREE_QUAL::operator=(QuadTree &&rhs) {
		assert(this != &rhs);

		if (m_count > 0)
			clear(true);
		swap(rhs);

		return *this;
	}
#endif

	TREE_TEMPLATE
		void TREE_QUAL::swap(QuadTree &other) {
		std::swap(m_root, other.m_root);
		std::swap(m_count, other.m_count);
		std::swap(m_factor, other.m_factor);
		std::swap(m_levels, other.m_levels);
		std::swap(m_allocator, other.m_allocator);
		std::swap(m_indexable, other.m_indexable);
	}

	TREE_TEMPLATE
		void TREE_QUAL::setBox(const T min[2], const T max[2]) {
		assert(m_root);
		assert(m_count == 0);
		m_root->box.set(min, max);
	}

	TREE_TEMPLATE
		template <typename Iter> void TREE_QUAL::insert(Iter first, Iter last) {
		assert(m_root);

		bool success = true;
		for (Iter it = first; it != last; ++it) {
			const object_type obj(*it, m_indexable);
			assert(m_root->box.contains(obj.box));
			success &= m_root->insert(obj, m_levels, m_allocator);
			++m_count;
		}
		(void)(success);
		assert(success);
	}

	TREE_TEMPLATE
		void TREE_QUAL::insert(const ValueType &value) {
		assert(m_root);

		const object_type obj(value, m_indexable);
		assert(m_root->box.contains(obj.box));

		bool success = m_root->insert(obj, m_levels, m_allocator);
		++m_count;

		(void)(success);
		assert(success);
	}

	TREE_TEMPLATE
		void TREE_QUAL::translate(const T point[2]) { m_root->translate(point); }

	TREE_TEMPLATE
		template <typename Predicate, typename OutIter>
	size_t TREE_QUAL::hierachical_query(const Predicate &predicate,
		OutIter out_it) const {
		assert(m_root);
		return m_root->queryHierachical(predicate, m_factor, out_it);
	}

	TREE_TEMPLATE
		void TREE_QUAL::setContainmentFactor(int factor) { m_factor = factor / 100.f; }

	TREE_TEMPLATE
		template <typename Predicate>
	bool TREE_QUAL::query(const Predicate &predicate) const {
		return query(predicate, spatial::detail::dummy_iterator()) > 0;
	}

	TREE_TEMPLATE
		template <typename Predicate, typename OutIter>
	size_t TREE_QUAL::query(const Predicate &predicate, OutIter out_it) const {
		assert(m_root);
		return m_root->query(predicate, m_factor, out_it);
	}

	TREE_TEMPLATE
		void TREE_QUAL::clear(bool recursiveCleanup /*= true*/) {

#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
		if (allocator_type::is_overflowable)
			recursiveCleanup &= !m_allocator.overflowed();
#endif

		// Delete all existing nodes
		if (recursiveCleanup) {
			if (m_root)
				m_root->clear(m_allocator);
		}
		const bbox_type box = m_root->box;
		detail::deallocate(m_allocator, m_root);

		m_root = detail::allocate(m_allocator, 0);
		m_root->box = box;
		m_levels = m_count = 0;
	}

	TREE_TEMPLATE
		size_t TREE_QUAL::count() const {
		assert(m_root);
		assert(m_root->count() == m_count);
		return m_count;
	}

	TREE_TEMPLATE
		int TREE_QUAL::levels() const {
		assert(m_root);
		return m_levels;
	}

	TREE_TEMPLATE
		typename TREE_QUAL::bbox_type TREE_QUAL::bbox() const {
		assert(m_root);
		return m_root->box;
	}

	TREE_TEMPLATE
		typename TREE_QUAL::allocator_type &TREE_QUAL::allocator() {
		return m_allocator;
	}

	TREE_TEMPLATE
		const typename TREE_QUAL::allocator_type &TREE_QUAL::allocator() const {
		return m_allocator;
	}

	TREE_TEMPLATE
		typename TREE_QUAL::node_iterator TREE_QUAL::root() {
		assert(m_root);
		return node_iterator(m_root, m_levels);
	}

	TREE_TEMPLATE
		typename TREE_QUAL::depth_iterator TREE_QUAL::dbegin() {
		assert(m_root);
		depth_iterator it(m_levels);

		it.push(m_root, 0, -1);
		for (;;) {
			if (it.m_tos <= 0)
				break;

			// Copy stack top cause it may change as we use it
			const typename depth_iterator::StackElement curTos = it.pop();

			if (curTos.node->isLeaf()) {
				if (curTos.node->objectCount()) {
					// Visit leaf once
					it.push(curTos.node, 0);
					break;
				}
				// fallback to previous
			}
			else {
				// Keep walking through children while we can
				if (curTos.childIndex + 1 < 4) {
					// Push sibling on for future tree walk
					// This is the 'fall back' node when we finish with the current level
					it.push(curTos.node, curTos.childIndex + 1, 0);
				}
				else {
					// Push current node on for future objects walk
					it.push(curTos.node, 4, 0);
				}

				// Since cur node is not a leaf, push first of next level to get deeper
				// into the tree
				node_ptr_type nextLevelnode = curTos.node->children[curTos.childIndex];
				if (nextLevelnode) {
					it.push(nextLevelnode, 0, 0);
				}
			}
		}
		return it;
	}

	TREE_TEMPLATE
		typename TREE_QUAL::leaf_iterator TREE_QUAL::lbegin() {
		assert(m_root);
		leaf_iterator it(m_levels);

		it.push(m_root, 0, -1);
		for (;;) {
			if (it.m_tos <= 0)
				break;

			// Copy stack top cause it may change as we use it
			const typename leaf_iterator::StackElement curTos = it.pop();

			if (curTos.node->isLeaf()) {
				if (curTos.node->objectCount()) {
					it.push(curTos.node, 0);
					break;
				}
				// Empty node, fallback to previous
			}
			else {
				// Keep walking through children while we can
				if (curTos.childIndex + 1 < 4) {
					// Push sibling on for future tree walk
					// This is the 'fall back' node when we finish with the current level
					it.push(curTos.node, curTos.childIndex + 1, -1);
				}
				else {
					// Push current node on for future objects walk
					it.push(curTos.node, 4, -1);
				}

				// Since cur node is not a leaf, push first of next level to get deeper
				// into the tree
				node_ptr_type nextLevelnode = curTos.node->children[curTos.childIndex];
				if (nextLevelnode) {
					it.push(nextLevelnode, 0, -1);
				}
			}
		}
		return it;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TREE_TEMPLATE
		TREE_QUAL::base_iterator::base_iterator(const int maxLevel)
		: m_maxLevel(maxLevel) {}

	TREE_TEMPLATE
		int TREE_QUAL::base_iterator::level() const {
		assert(m_tos > 0);
		const typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return m_maxLevel - curTos.node->level;
	}

	TREE_TEMPLATE
		bool TREE_QUAL::base_iterator::valid() const { return (m_tos > 0); }

	TREE_TEMPLATE
		ValueType &TREE_QUAL::base_iterator::operator*() {
		assert(valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->objectValue(curTos.objectIndex);
	}

	TREE_TEMPLATE
		const ValueType &TREE_QUAL::base_iterator::operator*() const {
		assert(valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->objectValue(curTos.objectIndex);
	}

	TREE_TEMPLATE
		const typename TREE_QUAL::bbox_type &TREE_QUAL::base_iterator::bbox() const {
		assert(valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->children[curTos.childIndex]->bbox;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TREE_TEMPLATE
		TREE_QUAL::node_iterator::node_iterator()
		: m_maxLevel(0), m_childIndex(0), m_objectIndex(0), m_node(NULL) {}

	TREE_TEMPLATE
		TREE_QUAL::node_iterator::node_iterator(node_ptr_type node, size_t childIndex,
			int maxLevel)
		: m_maxLevel(maxLevel), m_childIndex(childIndex),
		m_objectIndex(node->isLeaf() ? 0 : -1), m_node(node) {
		assert(node);

		while (m_childIndex < 4 && m_node->children[m_childIndex]->isEmpty())
			++m_childIndex;
		if (m_childIndex >= 4)
			m_objectIndex = 0;
	}

	TREE_TEMPLATE
		TREE_QUAL::node_iterator::node_iterator(node_ptr_type node, int maxLevel)
		: m_maxLevel(maxLevel), m_childIndex(node->isLeaf() ? 4 : 0),
		m_objectIndex(node->isLeaf() ? 0 : -1), m_node(node) {
		assert(node);

		while (m_childIndex < 4 && m_node->children[m_childIndex]->isEmpty())
			++m_childIndex;
		if (m_childIndex >= 4)
			m_objectIndex = 0;
	}

	TREE_TEMPLATE
		int TREE_QUAL::node_iterator::level() const {
		assert(m_node);
		return m_maxLevel - m_node->level;
	}

	TREE_TEMPLATE
		bool TREE_QUAL::node_iterator::valid() const {
		assert(m_node);
		return (m_objectIndex < (int)m_node->objectCount());
	}

	TREE_TEMPLATE
		ValueType &TREE_QUAL::node_iterator::operator*() {
		assert(valid());
		if (m_childIndex < 4) {
			assert(m_node->isBranch());
			return m_node->children[m_childIndex]->value;
		}
		else
			return m_node->objectValue(m_objectIndex);
	}

	TREE_TEMPLATE
		const ValueType &TREE_QUAL::node_iterator::operator*() const {
		assert(valid());
		if (m_childIndex < 4) {
			assert(m_node->isBranch());
			return m_node->children[m_childIndex]->value;
		}
		else
			return m_node->objectValue(m_objectIndex);
	}

	TREE_TEMPLATE
		const typename TREE_QUAL::bbox_type &TREE_QUAL::node_iterator::bbox() const {
		return m_node->bbox;
	}

	TREE_TEMPLATE
		void TREE_QUAL::node_iterator::next() {
		assert(valid());
		if (m_childIndex < 4 && m_node->isBranch()) {
			++m_childIndex;
			while (m_childIndex < 4 && m_node->children[m_childIndex]->isEmpty())
				++m_childIndex;

			if (m_childIndex < 4)
				// found a children which is not empty
				return;
		}
		++m_objectIndex;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TREE_TEMPLATE
		TREE_QUAL::depth_iterator::depth_iterator(const int maxLevel)
		: base_type(maxLevel) {}

	TREE_TEMPLATE
		ValueType &TREE_QUAL::depth_iterator::operator*() {
		assert(!base_type::valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->value;
	}

	TREE_TEMPLATE
		const ValueType &TREE_QUAL::depth_iterator::operator*() const {
		assert(!base_type::valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->value;
	}

	TREE_TEMPLATE
		void TREE_QUAL::depth_iterator::next() {
		for (;;) {
			if (m_tos <= 0)
				break;

			// Copy stack top cause it may change as we use it
			const typename base_type::StackElement curTos = pop();

			if (curTos.node->isLeaf()) {
				// Reached leaves, fall back to previous level
			}
			else {
				// Keep walking through children while we can
				if (curTos.childIndex + 1 < 4) {
					// Push sibling on for future tree walk
					// This is the 'fall back' node when we finish with the current level
					push(curTos.node, curTos.childIndex + 1, 0);
				}
				else {
					if (curTos.childIndex == 3) {
						// Visit node after visiting the last child
						push(curTos.node, 4, 0);
					}
					else if (curTos.childIndex == 4) {
						assert(!curTos.node->isEmpty());
						push(curTos.node, 5, 0);
						break;
					}
					else if (curTos.childIndex > 4)
						// Node visited, fallback to previous
						continue;
				}

				// Since cur node is not a leaf, push first of next level to get deeper
				// into the tree
				node_ptr_type nextLevelnode = curTos.node->children[curTos.childIndex];
				if (nextLevelnode->isLeaf()) {
					push(nextLevelnode, 0, 0);
					if (nextLevelnode->objectCount()) {
						// If we pushed on a new leaf, exit as the data is ready at TOS
						break;
					}
				}
				else {
					push(nextLevelnode, 0, 0);
				}
			}
		}
	}

	TREE_TEMPLATE
		typename TREE_QUAL::node_iterator TREE_QUAL::depth_iterator::child() {
		assert(m_tos > 0);
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return node_iterator(curTos.node, base_type::m_maxLevel);
	}

	TREE_TEMPLATE
		typename TREE_QUAL::node_iterator TREE_QUAL::depth_iterator::current() {
		assert(m_tos > 0);
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return node_iterator(curTos.node, 4, base_type::m_maxLevel);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TREE_TEMPLATE
		TREE_QUAL::leaf_iterator::leaf_iterator(const int maxLevel)
		: base_type(maxLevel) {}

	TREE_TEMPLATE
		void TREE_QUAL::leaf_iterator::next() {
		for (;;) {
			if (m_tos <= 0)
				break;

			// Copy stack top cause it may change as we use it
			const typename base_type::StackElement curTos = pop();

			assert(curTos.objectIndex >= -1);
			if (size_t(curTos.objectIndex + 1) < curTos.node->objectCount()) {
				// Keep walking through data while we can
				push(curTos.node, curTos.objectIndex + 1);
				break;
			}

			if (curTos.node->isLeaf()) {
				// reached leaves, fall back to previous level
			}
			else {
				// Keep walking through children while we can
				if (curTos.childIndex + 1 < 4) {
					// Push sibling on for future tree walk
					// This is the 'fall back' node when we finish with the current level
					push(curTos.node, curTos.childIndex + 1, curTos.objectIndex);
				}
				else if (curTos.childIndex >= 4)
					// Node visited, fallback to previous
					continue;

				// Since cur node is not a leaf, push first of next level to get deeper
				// into the tree
				node_ptr_type nextLevelnode = curTos.node->children[curTos.childIndex];
				if (nextLevelnode->isLeaf()) {
					push(nextLevelnode, 0, 0);
					if (nextLevelnode->objectCount()) {
						// If we pushed on a new leaf, exit as the data is ready at TOS
						break;
					}
				}
				else {
					push(nextLevelnode, 0, -1);
				}
			}
		}
	}

#undef TREE_TEMPLATE
#undef TREE_QUAL

} // namespace spatial
