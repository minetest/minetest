//
//  rtree_detail.h
//
//

#pragma once

//#define TREE_DEBUG_TAG

namespace spatial {
	namespace detail {

		template <class NodeClass, typename CountType> class Stack {
		public:
			enum StatusType { eNormal = 0, eBranchTraversed, eNextBranch };

		protected:
			struct StackElement {
				NodeClass *node;
				CountType branchIndex;
				StatusType status;

				StackElement() : status(eNormal) {}
			};

		protected:
			Stack() : m_tos(0) {}

			void push(NodeClass *node, CountType branchIndex,
				StatusType status = eNormal) {
				assert(node);

				StackElement &el = m_stack[m_tos++];
				el.node = node;
				el.branchIndex = branchIndex;
				el.status = status;

				assert(m_tos <= kMaxStackSize);
			}
			StackElement &pop() {
				assert(m_tos > 0);

				StackElement &el = m_stack[--m_tos];
				return el;
			}

		protected:
			//  Max stack size. Allows almost n^16 where n is number of branches in node
			static const int kMaxStackSize = 16;
			StackElement m_stack[kMaxStackSize]; ///< Stack as we are doing iteration
												 /// instead of recursion
			int m_tos;                           ///< Top Of Stack index
		};                                     // class stack_iterator

		/// May be data or may be another subtree
		/// The parents level determines this.
		/// If the parents level is 0, then this is data
		template <typename ValueType, class BBoxClass, class NodeClass> struct Branch {
			ValueType value;
			BBoxClass bbox;
			NodeClass *child;

#ifndef NDEBUG
			Branch() : child(NULL) {}
#endif
		}; // Branch

		template <typename ValueType, class BBoxClass, int max_child_items>
		struct Node {
			typedef Branch<ValueType, BBoxClass, Node> branch_type;
			typedef uint32_t count_type;
			typedef BBoxClass box_type;

			count_type count; ///< Number of branches in the node
			int32_t level;    ///< Leaf is zero, others positive
			ValueType values[max_child_items];
			BBoxClass bboxes[max_child_items];
			Node *children[max_child_items];

#ifdef TREE_DEBUG_TAG
			tn::string debugTags[max_child_items];
#endif

			Node() : level(0) {}

			Node(int level) : count(0), level(level) {}

			// Not a leaf, but a internal/branch node
			bool isBranch() const { return (level > 0); }

			bool isLeaf() const { return (level == 0); }

			// Find the smallest rectangle that includes all rectangles in branches of a
			// node.
			BBoxClass cover() const {
				BBoxClass bbox = bboxes[0];
				for (count_type index = 1; index < count; ++index) {
					bbox.extend(bboxes[index]);
				}

				return bbox;
			}
			bool addBranch(const branch_type &branch) {
				if (count >= max_child_items) // Split is necessary
					return false;

				values[count] = branch.value;
				children[count] = branch.child;
				bboxes[count++] = branch.bbox;
				return true;
			}

			// Disconnect a dependent node.
			// Caller must return (or stop using iteration index) after this as count has
			// changed
			void disconnectBranch(count_type index) {
				assert(index >= 0 && index < max_child_items);
				assert(count > 0);

				// Remove element by swapping with the last element to prevent gaps in array
				values[index] = values[--count];
				children[index] = children[count];
				bboxes[index] = bboxes[count];
			}
		}; // Node

		struct DummyInsertPredicate {};

		template <typename Predicate, class NodeClass>
		struct CheckInsertPredicateHelper {

			inline bool operator()(const Predicate &predicate,
				const NodeClass &node) const {
				for (typename NodeClass::count_type index = 0; index < node.count;
					++index) {
					if (!predicate(node.bboxes[index]))
						return false;
				}
				return true;
			}
		};

		template <class NodeClass>
		struct CheckInsertPredicateHelper<DummyInsertPredicate, NodeClass> {
			inline bool operator()(const DummyInsertPredicate & /*predicate*/,
				const NodeClass & /*node*/) const {
				return true;
			};
		};

		template <typename Predicate, class NodeClass>
		inline bool checkInsertPredicate(const Predicate &predicate,
			const NodeClass &node) {
			return CheckInsertPredicateHelper<Predicate, NodeClass>()(predicate, node);
		}

		template <class RTreeClass>
		typename RTreeClass::node_ptr_type &getRootNode(RTreeClass &tree) {
			return tree.m_root;
		}
	} // namespace detail
} // namespace spatial
