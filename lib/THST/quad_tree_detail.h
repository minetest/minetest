//
//  quad_tree_detail.h
//
//

#pragma once

#include "bbox.h"

#include <algorithm>
#include <vector>

namespace spatial {
	namespace detail {
		template <class NodeClass> class QuadTreeStack {
		protected:
			struct StackElement {
				NodeClass *node;
				size_t childIndex;
				int objectIndex;
			};

		protected:
			QuadTreeStack() : m_tos(0) {}

			void push(NodeClass *node, size_t childIndex, int objectIndex) {
				assert(node);

				StackElement &el = m_stack[m_tos++];
				el.node = node;
				el.childIndex = childIndex;
				el.objectIndex = objectIndex;

				assert(m_tos <= kMaxStackchildIndexze);
			}

			void push(NodeClass *node, int objectIndex) {
				assert(node);

				StackElement &el = m_stack[m_tos++];
				el.node = node;
				el.objectIndex = objectIndex;

				assert(m_tos <= kMaxStackchildIndexze);
			}

			StackElement &pop() {
				assert(m_tos > 0);

				StackElement &el = m_stack[--m_tos];
				return el;
			}

		protected:
			//  Max stack size. Allows almost n^16 where n is number of branches in node
			static const int kMaxStackchildIndexze = 16;
			StackElement m_stack[kMaxStackchildIndexze]; ///< Stack as we are doing
			/// iteration instead of recursion
			int m_tos; ///< Top Of Stack index
		};

		template <typename ValueType, class BBoxClass, class NodeClass>
		struct QuadTreeObject {
			ValueType value;
			BBoxClass box;

			QuadTreeObject() {}

			template <typename indexable_getter>
			QuadTreeObject(ValueType value, const indexable_getter &indexable)
				: value(value), box(indexable.min(value), indexable.max(value)) {}
		};

		template <class T, class ValueType, int max_child_items> struct QuadTreeNode {
			typedef BoundingBox<T, 2> bbox_type;
			typedef QuadTreeObject<ValueType, bbox_type, QuadTreeNode> object_type;
			typedef std::vector<object_type> ObjectList;

			const int level;
			ValueType value;
			ObjectList objects;
			bbox_type box;
			QuadTreeNode *children[4];

			QuadTreeNode(int level);

			template <typename custom_allocator>
			void copy(const QuadTreeNode &src, custom_allocator &allocator);
			template <typename custom_allocator>
			bool insert(const object_type &obj, int &levels, custom_allocator &allocator);
			template <typename Predicate, typename OutIter>
			size_t query(const Predicate &predicate, float factor, OutIter out_it) const;
			template <typename Predicate, typename OutIter>
			size_t queryHierachical(const Predicate &predicate, float factor,
				OutIter out_it) const;
			template <typename custom_allocator> void clear(custom_allocator &allocator);
			void translate(const T point[2]);
			size_t count() const;
			bool isEmpty() const;

			bool isBranch() const { return !isLeaf(); }

			bool isLeaf() const { return children[0] == NULL; }

			size_t objectCount() const { return objects.size(); }

			ValueType &objectValue(size_t objectIndex) {
				return objects[objectIndex].value;
			}

			const ValueType &objectValue(size_t objectIndex) const {
				return objects[objectIndex].value;
			}

			ValueType &objectValue(int objectIndex) {
				assert(objectIndex >= 0);
				return objects[(size_t)objectIndex].value;
			}

			const ValueType &objectValue(int objectIndex) const {
				assert(objectIndex >= 0);
				return objects[(size_t)objectIndex].value;
			}

			void updateCount() {
				m_count = count();
				m_invCount = 1.f / m_count;
			}

		private:
			size_t m_count;
			float m_invCount;

			template <typename custom_allocator>
			void subdivide(custom_allocator &allocator);
			void addObject(const object_type &obj);
			template <typename custom_allocator>
			bool addObjectsToChildren(custom_allocator &allocator);

			template <typename OutIter>
			void insertAll(size_t &foundCount, OutIter out_it) const;
		};

#define TREE_TEMPLATE template <class T, class ValueType, int max_child_items>
#define TREE_QUAL QuadTreeNode<T, ValueType, max_child_items>

		TREE_TEMPLATE
			TREE_QUAL::QuadTreeNode(int level)
			: level(level), children()
#ifndef NDEBUG
			,
			m_count(0)
#endif
		{
		}

		TREE_TEMPLATE
			template <typename custom_allocator>
		void TREE_QUAL::copy(const QuadTreeNode &src, custom_allocator &allocator) {
			assert(m_count == 0);
			value = src.value;
			objects = src.objects;
			box = src.box;
			m_count = src.m_count;
			m_invCount = src.m_invCount;

			if (src.isBranch()) {
				for (int i = 0; i < 4; ++i) {
					const QuadTreeNode *srcCurrent = src.children[i];
					QuadTreeNode *dstCurrent = children[i] =
						detail::allocate(allocator, srcCurrent->level);
					dstCurrent->copy(*srcCurrent, allocator);
				}
			}
		}

		TREE_TEMPLATE
			template <typename custom_allocator>
		bool TREE_QUAL::insert(const object_type &obj, int &levels,
			custom_allocator &allocator) {
			if (!this->box.contains(obj.box))
				// this object doesn't fit in this quadtree
				return false;

			if (isLeaf()) // No subdivision yet
			{
				if (objects.size() < max_child_items + 1) {
					addObject(obj);

					updateCount();
					return true;
				}

				// subdivide node
				subdivide(allocator);
				if (addObjectsToChildren(allocator)) {
					levels = std::max(levels, level + 1);
				}
				else {
					// could not insert anything in any of the sub-trees
					for (int i = 0; i < 4; ++i) {
						detail::deallocate(allocator, children[i]);
						children[i] = NULL;
					}

					addObject(obj);
					updateCount();
					return true;
				}
			}

			// try to add to children
			for (int i = 0; i < 4; ++i) {
				assert(children[i]);
				if (children[i]->insert(obj, levels, allocator)) {
					updateCount();
					return true;
				}
			}
			addObject(obj);
			updateCount();
			return true;
		}

		TREE_TEMPLATE
			template <typename Predicate, typename OutIter>
		size_t TREE_QUAL::query(const Predicate &predicate, float containmentFactor,
			OutIter out_it) const {
			assert(m_count == count());

			size_t foundCount = 0;

			// go further into the tree
			for (typename ObjectList::const_iterator it = objects.begin();
				it != objects.end(); ++it) {
				if (predicate(it->box)) {
					*out_it = it->value;
					++out_it;
					++foundCount;
				}
			}

			if (isLeaf()) {
				// reached leaves
				return foundCount;
			}

			for (int i = 0; i < 4; i++) {
				assert(children[i]);

				QuadTreeNode &node = *children[i];
				// Break if we know that the zone is fully contained by a region
				if (predicate.bbox.overlaps(node.box)) {
					foundCount += node.query(predicate, containmentFactor, out_it);
					if (node.box.contains(predicate.bbox)) {
						break;
					}
				}
			}

			return foundCount;
		}

		TREE_TEMPLATE
			template <typename Predicate, typename OutIter>
		size_t TREE_QUAL::queryHierachical(const Predicate &predicate,
			float containmentFactor,
			OutIter out_it) const {
			assert(m_count == count());

			size_t foundCount = 0;
			if (predicate.bbox.contains(this->box) && !isEmpty()) {
				// node is fully contained by the query
				*out_it = value;
				++out_it;
				foundCount += m_count;

				return foundCount;
			}

			const OutIter start = out_it;
			// go further into the tree
			for (typename ObjectList::const_iterator it = objects.begin();
				it != objects.end(); ++it) {
				if (predicate(it->box)) {
					*out_it = it->value;
					++out_it;
					++foundCount;
				}
			}

			if (isLeaf()) {
				if (foundCount) {
					const float factor = foundCount * m_invCount;
					if (factor > containmentFactor) {
						out_it = start;
						// node is fully contained by the query
						*out_it = value;
						++out_it;
						foundCount = m_count;
					}
				}

				// reached leaves
				return foundCount;
			}

			for (int i = 0; i < 4; i++) {
				assert(children[i]);

				QuadTreeNode &node = *children[i];
				// Break if we know that the zone is fully contained by a region
				if (predicate.bbox.overlaps(node.box)) {
					foundCount += node.query(predicate, containmentFactor, out_it);
					if (node.box.contains(predicate.bbox)) {
						break;
					}
				}
			}

			if (foundCount) {
				const float factor = foundCount * m_invCount;
				if (factor > containmentFactor) {
					out_it = start;
					// node is fully contained by the query
					*out_it = value;
					++out_it;
					foundCount = m_count;
				}
			}

			return foundCount;
		}

		TREE_TEMPLATE
			void TREE_QUAL::translate(const T point[2]) {
			for (typename ObjectList::iterator it = objects.begin(); it != objects.end();
				++it) {
				it->box.translate(point);
			}
			box.translate(point);

			if (isBranch()) {
				for (int i = 0; i < 4; ++i) {
					if (children[i]) {
						children[i]->translate(point);
					}
				}
			}
		}

		TREE_TEMPLATE
			size_t TREE_QUAL::count() const {
			size_t count = objectCount();
			if (isBranch()) {
				for (int i = 0; i < 4; ++i) {
					count += children[i]->count();
				}
			}
			return count;
		}

		TREE_TEMPLATE
			bool TREE_QUAL::isEmpty() const {
			if (!objects.empty())
				return false;

			if (isBranch()) {
				for (int i = 0; i < 4; ++i) {
					if (!children[i]->isEmpty())
						return false;
				}
			}
			return true;
		}

		TREE_TEMPLATE
			template <typename custom_allocator>
		void TREE_QUAL::clear(custom_allocator &allocator) {
			if (isLeaf())
				return;

			for (int i = 0; i < 4; ++i) {
				children[i]->clear(allocator);
				detail::deallocate(allocator, children[i]);
			}
		}

		TREE_TEMPLATE
			template <typename custom_allocator>
		void TREE_QUAL::subdivide(custom_allocator &allocator) {
			for (int i = 0; i < 4; ++i) {
				assert(children[i] == NULL);
				children[i] = detail::allocate(allocator, level + 1);
				QuadTreeNode &node = *children[i];
				node.box = box.quad2d(static_cast<box::RegionType>(i));
			}
		}

		TREE_TEMPLATE
			void TREE_QUAL::addObject(const object_type &obj) { objects.push_back(obj); }

		TREE_TEMPLATE
			template <typename custom_allocator>
		bool TREE_QUAL::addObjectsToChildren(custom_allocator &allocator) {
			int dummy = 0;
			const size_t prevSize = objects.size();
			for (typename ObjectList::iterator it = objects.begin(); it != objects.end();
				++it) {
				for (int i = 0; i < 4; ++i) {
					if (children[i]->insert(*it, dummy, allocator)) {
						it = objects.erase(it);
						if (it == objects.end()) {
							objects.shrink_to_fit();
							for (int i = 0; i < 4; ++i) {
								children[i]->updateCount();
								assert(children[i]->m_count == children[i]->count());
							}

							return true;
						}
						break;
					}
				}
			}

			objects.shrink_to_fit();
			for (int i = 0; i < 4; ++i) {
				children[i]->updateCount();
				assert(children[i]->m_count == children[i]->count());
			}

			// return if we could insert anything in any of the sub-trees
			return prevSize != objects.size();
		}

		TREE_TEMPLATE
			template <typename OutIter>
		void TREE_QUAL::insertAll(size_t &foundCount, OutIter out_it) const {
			for (typename ObjectList::const_iterator it = objects.begin();
				it != objects.end(); ++it) {
				// add crossing/overlapping results
				*out_it = it->value;
				++out_it;
			}
			foundCount += objects.size();

			if (isBranch()) {
				for (int i = 0; i < 4; ++i) {
					children[i]->insertAll(foundCount, out_it);
				}
			}
		}
	} // namespace detail

#undef TREE_TEMPLATE
#undef TREE_QUAL
} // namespace spatial
