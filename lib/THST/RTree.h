//
//  RTree.h
//
//

#pragma once

#include "allocator.h"
#include "bbox.h"
#include "config.h"
#include "indexable.h"
#include "predicates.h"
#include "rtree_detail.h"

#include <vector>
#include <queue>

namespace spatial {
	namespace rtree {
		// Type of element that allows fractional and large values such as float or
		// double, for use in volume calculations.
		template <typename T> struct RealType { typedef float type; };

		template <> struct RealType<double> { typedef double type; };
	}

	/**
	 @class RTree
	 @brief Implementation of a custom RTree tree based on the version
	 by Greg Douglas at Auran and original algorithm was by Toni Gutman.
			R-Trees provide Log(n) speed rectangular indexing into multi-dimensional
	 data. Only support the quadratric split heuristic.

			It has the following properties:
			- hierarchical, you can add values to the internal branch nodes
			- custom indexable getter similar to boost's
			- configurable volume calculation
			- custom allocation for internal nodes

	 @tparam T                type of the space(eg. int, float, etc.)
	 @tparam ValueType        type of value stored in the tree's nodes
	 @tparam Dimension        number of dimensions for the spatial space of the
	 bounding boxes
	 @tparam min_child_items  m, in the range 2 <= m < M
	 @tparam max_child_items  M, in the range 2 <= m < M
	 @tparam indexable_getter the indexable getter, i.e. the getter for the bounding
	 box of a value
	 @tparam bbox_volume_mode The rectangle calculation of volume, spherical results
	 better split classification but can be slower. While the normal can be faster
	 but worse classification.
	 @tparam RealType type of element that allows fractional and large
	 values such as float or double, for use in volume calculations.
	 @tparam custom_allocator the allocator class

	 @note It's recommended that ValueType should be a fast to copy object, eg: int,
	 id, obj*, etc.
	 */
	template <typename T,                                            //
		typename ValueType,                                    //
		int Dimension,                                         //
		int max_child_items = 8,                               //
		int min_child_items = max_child_items / 2,             //
		typename indexable_getter = Indexable<T, ValueType>,   //
		int bbox_volume_mode = box::eNormalVolume,             //
		typename RealType = typename rtree::RealType<T>::type, //
		typename custom_allocator = spatial::allocator<detail::Node<
		ValueType, BoundingBox<T, Dimension>, max_child_items>>>
		class RTree {
		public:
			typedef RealType real_type;
			typedef BoundingBox<T, Dimension> bbox_type;
			typedef custom_allocator allocator_type;

			static const size_t max_items = max_child_items;
			static const size_t min_items = min_child_items;

		private:
			typedef detail::Node<ValueType, bbox_type, max_child_items> node_type;
			typedef node_type *node_ptr_type;
			typedef node_type **node_dptr_type;
			typedef typename node_type::branch_type branch_type;
			typedef typename node_type::count_type count_type;

		public:
			class base_iterator : public detail::Stack<node_type, count_type> {
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

#ifdef TREE_DEBUG_TAG
				std::string &tag();
#endif
			protected:
				typedef detail::Stack<node_type, count_type> base_type;
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

#ifdef TREE_DEBUG_TAG
				std::string &tag();
#endif

			private:
				node_iterator(node_ptr_type node);

				count_type m_index;
				node_ptr_type m_node;
				friend class RTree;
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

				/// Advances to the next item.
				void next();

			private:
				friend class RTree;
			};

			/**
			 @brief Iterator to traverse only the leaves(i.e. level 0) of the tree
			 similar, left to right order.
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
				friend class RTree;
			};

			RTree(indexable_getter indexable = indexable_getter(),
				const allocator_type &allocator = allocator_type(),
				bool allocateRoot = true);
			template <typename Iter>
			RTree(Iter first, Iter last,                           //
				indexable_getter indexable = indexable_getter(), //
				const allocator_type &allocator = allocator_type());
			RTree(const RTree &src);
#ifdef SPATIAL_TREE_USE_CPP11
			RTree(RTree &&src);
#endif
			~RTree();

			RTree &operator=(const RTree &rhs);
#ifdef SPATIAL_TREE_USE_CPP11
			RTree &operator=(RTree &&rhs);
#endif

			void swap(RTree &other);

			template <typename Iter> void insert(Iter first, Iter last);
			void insert(const ValueType &value);
			/// Insert the value if the predicate condition is true.
			template <typename Predicate>
			bool insert(const ValueType &value, const Predicate &predicate);
			bool remove(const ValueType &value);

			/// Translates the internal boxes with the given offset point.
			void translate(const T point[Dimension]);

			/// Special query to find all within search rectangle using the hierarchical
			/// order.
			/// @see spatial::SpatialPredicate for available predicates.
			template <typename Predicate>
			bool hierachical_query(const Predicate &predicate) const;
			/// \return Returns the number of entries found.
			template <typename Predicate, typename OutIter>
			size_t hierachical_query(const Predicate &predicate, OutIter out_it) const;
			/// Defines the traget query level, if 0 then leaf values are retrieved
			/// otherwise hierachical node values.
			/// @note Only used for hierachical_query.
			void setQueryTargetLevel(int level);

			/// @see spatial::SpatialPredicate for available predicates.
			template <typename Predicate> bool query(const Predicate &predicate) const;
			template <typename Predicate, typename OutIter>
			size_t query(const Predicate &predicate, OutIter out_it) const;

			/// Performs a nearest neighbour search.
			/// @note Uses the distance to the center of the bbox.
			template <typename OutIter>
			size_t nearest(const T point[2], T radius, OutIter out_it) const;

			/// Performs a knn-nearest search.
			/// /// @note Uses the minimum distance to the bbox.
			template <typename OutIter>
			size_t k_nearest(const T point[Dimension], uint32_t k, OutIter out_it) const;

			/// Remove all entries from tree
			void clear(bool recursiveCleanup = true);
			/// Count the data elements in this container.
			size_t count() const;
			/// Returns the estimated internal node count for the given number of elements
			static size_t nodeCount(size_t numberOfItems);
			/// Returns the estimated branch count for the given number of elements
			static size_t branchCount(size_t numberOfItems);
			/// Returns the number of levels(height) of the tree.
			size_t levels() const;
			/// Returns the estimated depth for the given number of elements
			static double levels(size_t numberOfItems);
			/// Returns the bbox of the root node.
			bbox_type bbox() const;

			/// Returns the custom allocator
			allocator_type &allocator();
			const allocator_type &allocator() const;

			node_iterator root();
			depth_iterator dbegin();
			leaf_iterator lbegin();

			/// Returns the size in bytes of the tree
			/// @param estimate if true then it estimates the size via using the item
			/// count for guessing the internal node
			/// count, otherwise it counts the nodes which is slower
			size_t bytes(bool estimate = true) const;

		private:
			/// Variables for finding a split partition
			struct BranchVars {
				branch_type branches[max_child_items + 1];
				bbox_type coverSplit;
				real_type coverSplitArea;
			};

			struct PartitionVars : BranchVars {
				enum { ePartitionUnused = -1 };

				int partitions[max_child_items + 1];
				count_type count[2];
				bbox_type cover[2];
				real_type area[2];

				PartitionVars()
					: m_maxFill(max_child_items + 1), m_minFill(min_child_items) {}

				void clear() {
					count[0] = count[1] = 0;
					area[0] = area[1] = (real_type)0;
					for (count_type index = 0; index < m_maxFill; ++index) {
						partitions[index] = PartitionVars::ePartitionUnused;
					}
				}

				count_type totalCount() const { return count[0] + count[1]; }

				count_type maxFill() const { return m_maxFill; }

				count_type minFill() const { return m_minFill; }

				count_type diffFill() const { return m_maxFill - m_minFill; }

			private:
				const count_type m_maxFill;
				const count_type m_minFill;
			};

			struct BranchDistance {
				RealType distance;
				count_type index;

				bool operator<(const BranchDistance &other) const {
					return distance < other.distance;
				}
			};


			struct NearestDistance {
				RealType distance;
				node_ptr_type node;
				count_type index;

				NearestDistance(node_ptr_type node, RealType distance, count_type index = 0xFFFFFFF) :
					node(node), distance(distance), index(index)
				{
				}

				bool operator<(const NearestDistance& other) const
				{
					return distance > other.distance;
				}

				bool isLeaf() const
				{
					return node->isLeaf();
				}

				bool isObject() const
				{
					return isLeaf() && index != 0xFFFFFFF;
				}

				const bbox_type& object_bbox() const
				{
					assert(isObject());
					return node->bboxes[index];
				}
			};

			template <typename Predicate>
			bool insertImpl(const branch_type &branch, const Predicate &predicate,
				int level);
			template <typename Predicate>
			bool insertRec(const branch_type &branch, const Predicate &predicate,
				node_type &node, node_ptr_type &newNode, bool &added,
				int level);
			void copyRec(const node_ptr_type src, node_ptr_type dst);

			count_type pickBranch(const bbox_type &bbox, const node_type &node) const;
			void getBranches(const node_type &node, const branch_type &branch,
				BranchVars &branchVars) const;
			bool addBranch(const branch_type &branch, node_type &node,
				node_dptr_type newNode) const;

			void splitNode(node_type &node, const branch_type &branch,
				node_dptr_type newNode) const;
			void loadNodes(node_type &nodeA, node_type &nodeB,
				const PartitionVars &partitionVars) const;
			void choosePartition(PartitionVars &partitionVars) const;
			void pickSeeds(PartitionVars &partitionVars) const;
			void classify(count_type index, int group,
				PartitionVars &partitionVars) const;

			bool removeImpl(const bbox_type &bbox, const ValueType &value);
			bool removeRec(const bbox_type &bbox, const ValueType &value,
				node_ptr_type node, std::vector<node_ptr_type> &reInsertList);
			void clearRec(const node_ptr_type node);

			template <typename Predicate, typename OutIter>
			void queryHierachicalRec(node_ptr_type node, const Predicate &predicate,
				size_t &foundCount, OutIter out_it) const;

			template <typename Predicate, typename OutIter>
			void queryRec(node_ptr_type node, const Predicate &predicate,
				size_t &foundCount, OutIter out_it) const;

			template <typename OutIter>
			void nearestRec(node_ptr_type node, const T point[2], T radius,
				size_t &foundCount, OutIter it) const;

			void translateRec(node_type &node, const T point[Dimension]);
			size_t countImpl(const node_type &node) const;
			void countRec(const node_type &node, size_t &count) const;

			inline static RealType distance(const T point0[Dimension], const T point1[Dimension]) {

				RealType d = point0[0] - point1[0];
				d *= d;
				for (int i = 1; i < Dimension; i++)
				{
					RealType temp = point0[i] - point1[i];
					d += temp * temp;
				}
				return d;
			}

			inline static RealType distance(const T point[Dimension], const bbox_type& bbox) {

				RealType d = std::max(std::max(bbox.min[0] - point[0], (T)0), point[0] - bbox.max[0]);
				d *= d;
				for (int i = 1; i < Dimension; i++)
				{
					RealType temp = std::max(std::max(bbox.min[i] - point[i], (T)0), point[i] - bbox.max[i]);
					d += temp * temp;
				}

				return d;
			}

		private:
			indexable_getter m_indexable;
			mutable allocator_type m_allocator;
			mutable PartitionVars m_parVars;
			size_t m_count;
			int m_queryTargetLevel;
			node_ptr_type m_root;

			template <class RTreeClass>
			friend typename RTreeClass::node_ptr_type &
				detail::getRootNode(RTreeClass &tree);
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TREE_TEMPLATE                                                          \
  template <typename T, typename ValueType, int Dimension,                     \
            int max_child_items, int min_child_items,                          \
            typename indexable_getter, int bbox_volume_mode,                   \
            typename RealType, typename custom_allocator>

#define TREE_QUAL                                                              \
  RTree<T, ValueType, Dimension, max_child_items, min_child_items,             \
        indexable_getter, bbox_volume_mode, RealType, custom_allocator>

	TREE_TEMPLATE
		TREE_QUAL::RTree(indexable_getter indexable /*= indexable_getter()*/,
			const allocator_type &allocator /*= allocator_type()*/,
			bool allocateRoot /*= true*/)
		: m_indexable(indexable), m_allocator(allocator), m_count(0),
		m_queryTargetLevel(0),
		m_root(allocateRoot ? detail::allocate(m_allocator, 0) : NULL) {
		SPATIAL_TREE_STATIC_ASSERT((max_child_items > min_child_items),
			"Invalid child size!");
		SPATIAL_TREE_STATIC_ASSERT((min_child_items > 0), "Invalid child size!");
	}

	TREE_TEMPLATE
		template <typename Iter>
	TREE_QUAL::RTree(Iter first, Iter last,
		indexable_getter indexable /*= indexable_getter()*/,
		const allocator_type &allocator /*= allocator_type()*/)
		: m_indexable(indexable), m_allocator(allocator), m_count(0),
		m_queryTargetLevel(0) {
		SPATIAL_TREE_STATIC_ASSERT((max_child_items > min_child_items),
			"Invalid child size!");
		SPATIAL_TREE_STATIC_ASSERT((min_child_items > 0), "Invalid child size!");

		m_root = detail::allocate(m_allocator, 0);
		// TODO: use packing algorithm
		insert(first, last);
	}

	TREE_TEMPLATE
		TREE_QUAL::RTree(const RTree &src)
		: m_indexable(src.m_indexable), m_allocator(src.m_allocator),
		m_count(src.m_count), m_queryTargetLevel(src.m_queryTargetLevel),
		m_root(detail::allocate(m_allocator, 0)) {
		copyRec(src.m_root, m_root);
	}

#ifdef SPATIAL_TREE_USE_CPP11
	TREE_TEMPLATE
		TREE_QUAL::RTree(RTree &&src) : m_root(NULL) { swap(src); }
#endif

	TREE_TEMPLATE
		TREE_QUAL::~RTree() {
#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
		if (m_root && !m_allocator.overflowed())
#else
		if (m_root)
#endif
			clearRec(m_root);
	}

	TREE_TEMPLATE
		TREE_QUAL &TREE_QUAL::operator=(const RTree &rhs) {
		if (&rhs != this) {
			if (m_count > 0)
				clear(true);

			m_count = rhs.m_count;
			m_queryTargetLevel = rhs.m_queryTargetLevel;
			m_allocator = rhs.m_allocator;
			m_indexable = rhs.m_indexable;
			copyRec(rhs.m_root, m_root);
		}
		return *this;
	}

#ifdef SPATIAL_TREE_USE_CPP11
	TREE_TEMPLATE
		TREE_QUAL &TREE_QUAL::operator=(RTree &&rhs) {
		assert(this != &rhs);

		if (m_count > 0)
			clear(true);
		swap(rhs);

		return *this;
	}
#endif

	TREE_TEMPLATE
		void TREE_QUAL::swap(RTree &other) {
		std::swap(m_root, other.m_root);
		std::swap(m_count, other.m_count);
		std::swap(m_queryTargetLevel, other.m_queryTargetLevel);
		std::swap(m_allocator, other.m_allocator);
		std::swap(m_indexable, other.m_indexable);
	}

	TREE_TEMPLATE
		template <typename Iter> void TREE_QUAL::insert(Iter first, Iter last) {
		assert(m_root);

		branch_type branch;
		branch.child = NULL;

		for (Iter it = first; it != last; ++it) {
			const ValueType &value = *it;
			branch.value = value;
			branch.bbox.set(m_indexable.min(value), m_indexable.max(value));
			insertImpl(branch, spatial::detail::DummyInsertPredicate(), 0);

			++m_count;
#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
			if (allocator_type::is_overflowable && m_allocator.overflowed())
				break;
#endif
		}
	}

	TREE_TEMPLATE
		void TREE_QUAL::insert(const ValueType &value) {
		assert(m_root);

		branch_type branch;
		branch.value = value;
		branch.child = NULL;
		branch.bbox.set(m_indexable.min(value), m_indexable.max(value));
		insertImpl(branch, spatial::detail::DummyInsertPredicate(), 0);

		++m_count;
	}

	TREE_TEMPLATE
		template <typename Predicate>
	bool TREE_QUAL::insert(const ValueType &value, const Predicate &predicate) {
		assert(m_root);

		branch_type branch;
		branch.value = value;
		branch.child = NULL;
		branch.bbox.set(m_indexable.min(value), m_indexable.max(value));
		bool success = insertImpl(branch, predicate, 0);
		if (success)
			++m_count;

		return success;
	}

	TREE_TEMPLATE
		void TREE_QUAL::translate(const T point[Dimension]) {
		assert(m_root);

		translateRec(*m_root, point);
	}

	TREE_TEMPLATE
		bool TREE_QUAL::remove(const ValueType &value) {
		assert(m_root);

		const bbox_type bbox(m_indexable.min(value), m_indexable.max(value));
		if (removeImpl(bbox, value)) {
			--m_count;
			return true;
		}
		return false;
	}

	TREE_TEMPLATE
		template <typename Predicate>
	bool TREE_QUAL::hierachical_query(const Predicate &predicate) const {
		return hierachical_query(predicate, spatial::detail::dummy_iterator()) > 0;
	}

	TREE_TEMPLATE
		template <typename Predicate, typename OutIter>
	size_t TREE_QUAL::hierachical_query(const Predicate &predicate,
		OutIter out_it) const {
		size_t foundCount = 0;
		queryHierachicalRec(m_root, predicate, foundCount, out_it);
		return foundCount;
	}

	TREE_TEMPLATE
		template <typename Predicate>
	bool TREE_QUAL::query(const Predicate &predicate) const {
		return query(predicate, spatial::detail::dummy_iterator()) > 0;
	}

	TREE_TEMPLATE
		template <typename Predicate, typename OutIter>
	size_t TREE_QUAL::query(const Predicate &predicate, OutIter out_it) const {

		size_t foundCount = 0;
		queryRec(m_root, predicate, foundCount, out_it);
		return foundCount;
	}

	TREE_TEMPLATE
		template <typename OutIter>
	size_t TREE_QUAL::nearest(const T point[2], T radius, OutIter out_it) const {
		size_t foundCount = 0;
		nearestRec(m_root, point, radius, foundCount, out_it);

		return foundCount;
	}

	TREE_TEMPLATE
		template <typename OutIter>
	size_t TREE_QUAL::k_nearest(const T point[Dimension], uint32_t k, OutIter out_it) const {
		size_t foundCount = 0;
		
		// incremental nearest neighbor search
		std::priority_queue<NearestDistance> queue;

		queue.emplace(m_root, 0);
		while (!queue.empty())
		{
			const NearestDistance element = queue.top();
			queue.pop();
			if (element.isObject())
			{
				if (!queue.empty() && distance(point, element.object_bbox()) > queue.top().distance)
				{
					queue.push(element);
				}
				else
				{
					*out_it = element.node->values[element.index];
					++out_it;
					if (++foundCount == k)
					{
						break;
					}
				}
			}
			else if (element.isLeaf())
			{
				for (count_type index = 0; index < element.node->count; ++index)
				{
					const bbox_type& bbox = element.node->bboxes[index];
					RealType d = distance(point, bbox);
					queue.emplace(element.node, d, index);
				}
			}
			else
			{
				// is a branch
				for (count_type index = 0; index < element.node->count; ++index)
				{
					const bbox_type& bbox = element.node->bboxes[index];
					RealType d = distance(point, bbox);
					queue.emplace(element.node->children[index], d);
				}
			}
		}

		return foundCount;
	}

	TREE_TEMPLATE
		void TREE_QUAL::setQueryTargetLevel(int level) { m_queryTargetLevel = level; }

	TREE_TEMPLATE
		size_t TREE_QUAL::count() const {
		assert(m_root);
		assert(m_count == countImpl(*m_root));

		return m_count;
	}

	TREE_TEMPLATE
		size_t TREE_QUAL::bytes(bool estimate /*= true*/) const {
		size_t size = sizeof(RTree);

		if (!m_count)
			return size;

		if (estimate)
			return size + nodeCount(count()) * sizeof(node_type);

		depth_iterator it = const_cast<RTree &>(*this).dbegin();
		size_t n = 0;
		for (; !it.isNull(); ++it) {
			++n;
		}

		return size + n * sizeof(node_type);
	}

	TREE_TEMPLATE
		size_t TREE_QUAL::nodeCount(size_t numberOfItems) {
		const double k = min_child_items;
		const double invK1 = 1.0 / (k - 1.0);

		const double depth = levels(numberOfItems);
		// perfectly balanced k-ary tree formula
		double count = std::pow(k, depth + 1.0) - 1;
		count *= invK1;
		return std::max(count * 0.5, 1.0);
	}

	TREE_TEMPLATE
		size_t TREE_QUAL::branchCount(size_t numberOfItems) {
		size_t count = nodeCount(numberOfItems) *
			(min_child_items + max_child_items / min_child_items);
		return count;
	}

	TREE_TEMPLATE
		size_t TREE_QUAL::levels() const {
		assert(m_root);
		return m_root->level;
	}

	TREE_TEMPLATE
		double TREE_QUAL::levels(size_t numberOfItems) {
		static const double invLog = 1 / std::log(min_child_items);
		const double depth = std::log(numberOfItems) * invLog - 1;
		return depth;
	}

	TREE_TEMPLATE
		typename TREE_QUAL::bbox_type TREE_QUAL::bbox() const {
		assert(m_root);
		return m_root->cover();
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
		void TREE_QUAL::countRec(const node_type &node, size_t &count) const {
		if (node.isBranch()) // not a leaf node
		{
			for (count_type index = 0; index < node.count; ++index) {
				assert(node.children[index] != NULL);
				countRec(*node.children[index], count);
			}
		}
		else // A leaf node
		{
			count += node.count;
		}
	}

	TREE_TEMPLATE
		void TREE_QUAL::translateRec(node_type &node, const T point[Dimension]) {
		for (count_type index = 0; index < node.count; ++index) {
			node.bboxes[index].translate(point);
		}

		if (node.isBranch()) // not a leaf node
		{
			for (count_type index = 0; index < node.count; ++index) {
				assert(node.children[index] != NULL);
				translateRec(*node.children[index], point);
			}
		}
		// A leaf node
	}

	TREE_TEMPLATE
		size_t TREE_QUAL::countImpl(const node_type &node) const {
		size_t count = 0;
		countRec(node, count);
		return count;
	}

	TREE_TEMPLATE
		void TREE_QUAL::clear(bool recursiveCleanup /*= true*/) {

#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
		if (allocator_type::is_overflowable)
			recursiveCleanup &= !m_allocator.overflowed();
#endif

		if (recursiveCleanup && m_root)
			clearRec(m_root);

		m_root = detail::allocate(m_allocator, 0);
		m_count = 0;
	}

	TREE_TEMPLATE
		void TREE_QUAL::clearRec(const node_ptr_type node) {
		assert(node);
		assert(node->level >= 0);

		if (node->isBranch()) // This is an internal node in the tree
		{
			for (count_type index = 0; index < node->count; ++index) {
				clearRec(node->children[index]);
			}
		}
		detail::deallocate(m_allocator, node);
	}

	// Inserts a new data rectangle into the index structure.
	// Recursively descends tree, propagates splits back up.
	// Returns 0 if node was not split.  Old node updated.
	// If node was split, returns 1 and sets the pointer pointed to by
	// new_node to point to the new node.  Old node updated to become one of two.
	// The level argument specifies the number of steps up from the leaf
	// level to insert; e.g. a data rectangle goes in at level = 0.
	TREE_TEMPLATE
		template <typename Predicate>
	bool TREE_QUAL::insertRec(const branch_type &branch, const Predicate &predicate,
		node_type &node, node_ptr_type &newNode, bool &added,
		int level) {
		assert(level >= 0 && level <= node.level);

		// TODO: profile and test a non-recursive version

		// recurse until we reach the correct level for the new record. data records
		// will always be called with level == 0 (leaf)
		if (node.level > level) {
			// Still above level for insertion, go down tree recursively
			node_ptr_type otherNode = NULL;

			// find the optimal branch for this record
			const count_type index = pickBranch(branch.bbox, node);

			// recursively insert this record into the picked branch
			assert(node.children[index]);
			bool childWasSplit = insertRec(branch, predicate, *node.children[index],
				otherNode, added, level);

			if (!childWasSplit) {
				// Child was not split. Merge the bounding box of the new record with the
				// existing bounding box
				if (added)
					node.bboxes[index] = branch.bbox.extended(node.bboxes[index]);
				return false;
			}
			else if (otherNode) {
				// Child was split. The old branches are now re-partitioned to two nodes
				// so we have to re-calculate the bounding boxes of each node
				node.bboxes[index] = node.children[index]->cover();
				branch_type branch;
				branch.child = otherNode;
				branch.bbox = otherNode->cover();

				// The old node is already a child of node. Now add the newly-created
				// node to node as well. node might be split because of that.
				return addBranch(branch, node, &newNode);
			}
#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
			else if (allocator_type::is_overflowable) {
				// overflow error
				return false;
			}
#endif
		}
		else if (node.level == level) {
			// Check if we should add the branch
			if (!detail::checkInsertPredicate(predicate, node)) {
				added = false;
				return false;
			}

			// We have reached level for insertion. Add bbox, split if necessary
			return addBranch(branch, node, &newNode);
		}

		// Should never occur
		assert(0);
		return false;
	}

	// Insert a data rectangle into an index structure.
	// insertImpl provides for splitting the root;
	// returns 1 if root was split, 0 if it was not.
	// The level argument specifies the number of steps up from the leaf
	// level to insert; e.g. a data rectangle goes in at level = 0.
	// insertRec does the recursion.
	TREE_TEMPLATE
		template <typename Predicate>
	bool TREE_QUAL::insertImpl(const branch_type &branch,
		const Predicate &predicate, int level) {
		assert(m_root);
		assert(level >= 0 && level <= m_root->level);
#ifndef NDEBUG
		for (int index = 0; index < Dimension; ++index) {
			assert(branch.bbox.min[index] <= branch.bbox.max[index]);
		}
#endif

		node_ptr_type newNode = NULL;
		bool added = true;

		// Check if root was split
		if (insertRec(branch, predicate, *m_root, newNode, added, level)) {
#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
			if (allocator_type::is_overflowable && newNode == NULL) // overflow
				return false;
#endif

			// Grow tree taller and new root
			node_ptr_type newRoot = detail::allocate(m_allocator, m_root->level + 1);

			branch_type branch;
			// add old root node as a child of the new root
			branch.bbox = m_root->cover();
			branch.child = m_root;
			addBranch(branch, *newRoot, NULL);

			// add the split node as a child of the new root
			branch.bbox = newNode->cover();
			branch.child = newNode;
			addBranch(branch, *newRoot, NULL);
			// set the new root as the root node
			m_root = newRoot;
		}
		return added;
	}

	TREE_TEMPLATE
		void TREE_QUAL::copyRec(const node_ptr_type src, node_ptr_type dst) {
		*dst = *src;
		for (count_type index = 0; index < src->count; ++index) {
			const node_ptr_type srcCurrent = src->children[index];
			if (srcCurrent) {
				node_ptr_type dstCurrent = dst->children[index] =
					detail::allocate(m_allocator, srcCurrent->level);
				copyRec(srcCurrent, dstCurrent);
			}
		}
	}

	// Add a branch to a node.  Split the node if necessary.
	// Returns 0 if node not split.  Old node updated.
	// Returns 1 if node split, sets *new_node to address of new node.
	// Old node updated, becomes one of two.
	TREE_TEMPLATE
		bool TREE_QUAL::addBranch(const branch_type &branch, node_type &node,
			node_dptr_type newNode) const {
		if (node.addBranch(branch)) {
			return false;
		}

		assert(newNode);
		splitNode(node, branch, newNode);

		return true;
	}

	// Pick a branch.  Pick the one that will need the smallest increase
	// in area to accomodate the new rectangle.  This will result in the
	// least total area for the covering rectangles in the current node.
	// In case of a tie, pick the one which was smaller before, to get
	// the best resolution when searching.
	TREE_TEMPLATE
		typename TREE_QUAL::count_type
		TREE_QUAL::pickBranch(const bbox_type &bbox, const node_type &node) const {
		bool firstTime = true;
		real_type increase;
		real_type bestIncr = (real_type)-1;
		real_type area;
		real_type bestArea;
		count_type best;
		bbox_type tempbbox_type;

		for (count_type index = 0; index < node.count; ++index) {
			const bbox_type &curbbox_type = node.bboxes[index];
			area = curbbox_type.template volume<bbox_volume_mode, RealType>();
			tempbbox_type = bbox.extended(curbbox_type);
			increase =
				tempbbox_type.template volume<bbox_volume_mode, RealType>() - area;
			if ((increase < bestIncr) || firstTime) {
				best = index;
				bestArea = area;
				bestIncr = increase;
				firstTime = false;
			}
			else if ((increase == bestIncr) && (area < bestArea)) {
				best = index;
				bestArea = area;
				bestIncr = increase;
			}
		}
		assert(node.count);
		return best;
	}

	// Split a node.
	// Divides the nodes branches and the extra one between two nodes.
	// Old node is one of the new ones, and one really new one is created.
	// Tries more than one method for choosing a partition, uses best result.
	TREE_TEMPLATE
		void TREE_QUAL::splitNode(node_type &node, const branch_type &branch,
			node_dptr_type newNodePtr) const {
		assert(newNodePtr);
		node_ptr_type &newNode = *newNodePtr;

		// Could just use local here, but member or external is faster since it is
		// reused
		m_parVars.clear();

		// Load all the branches into a buffer, initialize old node
		getBranches(node, branch, m_parVars);

		// Find partition
		choosePartition(m_parVars);

		// Create a new node to hold (about) half of the branches
		newNode = detail::allocate(m_allocator, node.level);

#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
		if (allocator_type::is_overflowable && !newNode)
			return;
#endif

		assert(newNode);
		// Put branches from buffer into 2 nodes according to the chosen partition
		node.count = 0;
		loadNodes(node, *newNode, m_parVars);

		assert((node.count + newNode->count) == m_parVars.maxFill());
	}

	// Load branch buffer with branches from full node plus the extra branch.
	TREE_TEMPLATE
		void TREE_QUAL::getBranches(const node_type &node, const branch_type &branch,
			BranchVars &branchVars) const {
		assert(node.count == max_child_items);

		// Load the branch buffer
		for (size_t index = 0; index < max_child_items; ++index) {
			branch_type &branch = branchVars.branches[index];
			branch.bbox = node.bboxes[index];
			branch.value = node.values[index];
			branch.child = node.children[index];
		}
		branchVars.branches[max_child_items] = branch;

		// Calculate bbox containing all in the set
		branchVars.coverSplit = branchVars.branches[0].bbox;
		for (int index = 1; index < max_child_items + 1; ++index) {
			branchVars.coverSplit.extend(branchVars.branches[index].bbox);
		}
		branchVars.coverSplitArea =
			branchVars.coverSplit.template volume<bbox_volume_mode, RealType>();
	}

	// Method #0 for choosing a partition:
	// As the seeds for the two groups, pick the two rects that would waste the
	// most area if covered by a single rectangle, i.e. evidently the worst pair
	// to have in the same group.
	// Of the remaining, one at a time is chosen to be put in one of the two groups.
	// The one chosen is the one with the greatest difference in area expansion
	// depending on which group - the bbox most strongly attracted to one group
	// and repelled from the other.
	// If one group gets too full (more would force other group to violate min
	// fill requirement) then other group gets the rest.
	// These last are the ones that can go in either group most easily.
	TREE_TEMPLATE
		void TREE_QUAL::choosePartition(PartitionVars &partitionVars) const {
		real_type biggestDiff;
		count_type chosen;
		int group, betterGroup;

		pickSeeds(partitionVars);

		while (((partitionVars.totalCount()) < partitionVars.maxFill()) &&
			(partitionVars.count[0] < partitionVars.diffFill()) &&
			(partitionVars.count[1] < partitionVars.diffFill())) {
			biggestDiff = (real_type)-1;
			for (count_type index = 0; index < partitionVars.maxFill(); ++index) {
				if (PartitionVars::ePartitionUnused != partitionVars.partitions[index])
					continue;

				const bbox_type &curbbox_type = partitionVars.branches[index].bbox;
				bbox_type bbox0 = curbbox_type.extended(partitionVars.cover[0]);
				bbox_type bbox1 = curbbox_type.extended(partitionVars.cover[1]);
				real_type growth0 = bbox0.template volume<bbox_volume_mode, RealType>() -
					partitionVars.area[0];
				real_type growth1 = bbox1.template volume<bbox_volume_mode, RealType>() -
					partitionVars.area[1];
				real_type diff = growth1 - growth0;
				if (diff >= 0) {
					group = 0;
				}
				else {
					group = 1;
					diff = -diff;
				}

				if (diff > biggestDiff) {
					biggestDiff = diff;
					chosen = index;
					betterGroup = group;
				}
				else if ((diff == biggestDiff) && (partitionVars.count[group] <
					partitionVars.count[betterGroup])) {
					chosen = index;
					betterGroup = group;
				}
			}
			classify(chosen, betterGroup, partitionVars);
		}

		// If one group too full, put remaining bboxs in the other
		if (partitionVars.totalCount() < partitionVars.maxFill()) {
			if (partitionVars.count[0] >= partitionVars.diffFill()) {
				group = 1;
			}
			else {
				group = 0;
			}
			for (count_type index = 0; index < partitionVars.maxFill(); ++index) {
				if (PartitionVars::ePartitionUnused == partitionVars.partitions[index]) {
					classify(index, group, partitionVars);
				}
			}
		}

		assert(partitionVars.totalCount() == partitionVars.maxFill());
		assert((partitionVars.count[0] >= partitionVars.minFill()) &&
			(partitionVars.count[1] >= partitionVars.minFill()));
	}

	// Copy branches from the buffer into two nodes according to the partition.
	TREE_TEMPLATE
		void TREE_QUAL::loadNodes(node_type &nodeA, node_type &nodeB,
			const PartitionVars &partitionVars) const {
		node_ptr_type targetNodes[] = { &nodeA, &nodeB };
		for (count_type index = 0; index < partitionVars.maxFill(); ++index) {
			assert(partitionVars.partitions[index] == 0 ||
				partitionVars.partitions[index] == 1);

			int targetNodeIndex = partitionVars.partitions[index];

			// It is assured that addBranch here will not cause a node split.
			bool nodeWasSplit = addBranch(partitionVars.branches[index],
				*targetNodes[targetNodeIndex], NULL);
			(void)(nodeWasSplit);
			assert(!nodeWasSplit);
		}
	}

	TREE_TEMPLATE
		void TREE_QUAL::pickSeeds(PartitionVars &partitionVars) const {
		count_type seed0, seed1;
		real_type worst, waste;
		real_type area[max_child_items + 1];

		for (size_t index = 0; index < partitionVars.maxFill(); ++index) {
			area[index] = partitionVars.branches[index]
				.bbox.template volume<bbox_volume_mode, RealType>();
		}

		worst = -partitionVars.coverSplitArea - 1;
		for (count_type indexA = 0; indexA < partitionVars.maxFill() - 1; ++indexA) {
			for (count_type indexB = indexA + 1; indexB < partitionVars.maxFill();
				++indexB) {
				bbox_type onebbox_type = partitionVars.branches[indexA].bbox.extended(
					partitionVars.branches[indexB].bbox);
				waste = onebbox_type.template volume<bbox_volume_mode, RealType>() -
					area[indexA] - area[indexB];
				if (waste > worst) {
					worst = waste;
					seed0 = indexA;
					seed1 = indexB;
				}
			}
		}

		classify(seed0, 0, partitionVars);
		classify(seed1, 1, partitionVars);
	}

	// Put a branch in one of the groups.
	TREE_TEMPLATE
		void TREE_QUAL::classify(count_type index, int group,
			PartitionVars &partitionVars) const {
		assert(PartitionVars::ePartitionUnused == partitionVars.partitions[index]);

		partitionVars.partitions[index] = group;

		// Calculate combined bbox
		if (partitionVars.count[group] == 0) {
			partitionVars.cover[group] = partitionVars.branches[index].bbox;
		}
		else {
			partitionVars.cover[group] =
				partitionVars.branches[index].bbox.extended(partitionVars.cover[group]);
		}

		// Calculate volume of combined bbox
		partitionVars.area[group] =
			partitionVars.cover[group].template volume<bbox_volume_mode, RealType>();

		++partitionVars.count[group];
	}

	// Delete a data rectangle from an index structure.
	// Pass in a pointer to a bbox_type, the id of the record, ptr to ptr to root
	// node.
	// Returns 0 if record not found, 1 if success.
	// removeImpl provides for eliminating the root.
	TREE_TEMPLATE
		bool TREE_QUAL::removeImpl(const bbox_type &bbox, const ValueType &value) {
		assert(m_root);

		std::vector<node_ptr_type> reInsertList;
		if (removeRec(bbox, value, m_root, reInsertList)) {
			// Found and deleted a data item
			// Reinsert any branches from eliminated nodes
			for (size_t i = 0; i < reInsertList.size(); ++i) {
				node_ptr_type tempNode = reInsertList[i];

				for (count_type index = 0; index < tempNode->count; ++index) {
					branch_type branch;
					branch.bbox = tempNode->bboxes[index];
					branch.value = tempNode->values[index];
					branch.child = tempNode->children[index];
					insertImpl(branch, spatial::detail::DummyInsertPredicate(), tempNode->level);
				}
				m_allocator.deallocate(tempNode);
			}

			// Check for redundant root (not leaf, 1 child) and eliminate TODO replace
			// if with while? In case there is a whole branch of redundant roots...
			if (m_root->count == 1 && m_root->isBranch()) {
				node_ptr_type tempNode = m_root->children[0];

				assert(tempNode);
				m_allocator.deallocate(m_root);
				m_root = tempNode;
			}
			return true;
		}

		return false;
	}

	// Delete a rectangle from non-root part of an index structure.
	// Called by removeImpl.  Descends tree recursively,
	// merges branches on the way back up.
	// Returns 0 if record not found, 1 if success.
	TREE_TEMPLATE
		bool TREE_QUAL::removeRec(const bbox_type &bbox, const ValueType &value,
			node_ptr_type node,

			std::vector<node_ptr_type> &reInsertList) {
		assert(node);
		assert(node->level >= 0);

		if (node->isBranch()) // not a leaf node
		{
			for (count_type index = 0; index < node->count; ++index) {
				if (!bbox.overlaps(node->bboxes[index]))
					continue;

				node_ptr_type currentNode = node->children[index];
				if (removeRec(bbox, value, currentNode, reInsertList)) {
					if (currentNode->count >= min_child_items) {
						// child removed, just resize parent bbox
						node->bboxes[index] = currentNode->cover();
					}
					else {
						// child removed, not enough entries in node, eliminate node
						reInsertList.push_back(currentNode);
						node->disconnectBranch(index);
					}
					return true;
				}
			}
		}
		else // A leaf node
		{
			for (count_type index = 0; index < node->count; ++index) {
				if (node->values[index] == value) {
					node->disconnectBranch(index); 
					return true;
				}
			}
		}
		return false;
	}

	// Search in an index tree or subtree for all data retangles that overlap the
	// argument rectangle and stop at first node that is fully containted by the
	// bbox.
	TREE_TEMPLATE
		template <typename Predicate, typename OutIter>
	void TREE_QUAL::queryHierachicalRec(node_ptr_type node,
		const Predicate &predicate,
		size_t &foundCount, OutIter it) const {
		assert(node);
		assert(node->level >= 0);

		if (node->level <= m_queryTargetLevel) {
			// This is a target or lower level node
			for (count_type index = 0; index < node->count; ++index) {
				if (predicate(node->bboxes[index])) {
					*it = node->values[index];
					++it;
					++foundCount;
				}
			}
		}
		else {
			for (count_type index = 0; index < node->count; ++index) {
				const bbox_type &nodeBBox = node->bboxes[index];
				// Branch is fully contained, dont search further
				if (predicate.bbox.contains(nodeBBox)) {
					*it = node->values[index];
					++it;
					++foundCount;
				}
				else if (predicate.bbox.overlaps(nodeBBox)) {
					queryHierachicalRec(node->children[index], predicate, foundCount, it);
				}
			}
		}
	}

	TREE_TEMPLATE
		template <typename Predicate, typename OutIter>
	void TREE_QUAL::queryRec(node_ptr_type node, const Predicate &predicate,
		size_t &foundCount, OutIter it) const {
		assert(node);
		assert(node->level >= 0);

		if (node->isLeaf()) {
			for (count_type index = 0; index < node->count; ++index) {
				if (predicate(node->bboxes[index])) {
					*it = node->values[index];
					++it;
					++foundCount;
				}
			}
		}
		else {
			for (count_type index = 0; index < node->count; ++index) {
				const bbox_type &nodeBBox = node->bboxes[index];

				// HACK I'm a monster
				if (predicate(nodeBBox)) {
					queryRec(node->children[index], predicate, foundCount, it);
				}
			}
		}
	}

	TREE_TEMPLATE
		template <typename OutIter>
	void TREE_QUAL::nearestRec(node_ptr_type node, const T point[2], T radius,
		size_t &foundCount, OutIter it) const {
		SPATIAL_TREE_STATIC_ASSERT(Dimension == 2, "Only for 2 dimensions!");

		assert(node);
		assert(node->level >= 0);

		if (node->isLeaf()) {
			BranchDistance branches[max_child_items];

			count_type current = 0;
			for (count_type index = 0; index < node->count; ++index) {
				const bbox_type &nodeBBox = node->bboxes[index];

				if (nodeBBox.overlaps(point, radius)) {
					BranchDistance &branch = branches[current++];
					branch.index = index;

					// use euclidean distance
					T center[2];
					nodeBBox.center(center);
					branch.distance = distance(point, center);
				}
			}

			// sort branches based on distances
			std::sort(branches, branches + current);

			// save results
			for (count_type i = 0; i < current; ++i) {
				BranchDistance &branch = branches[i];

				*it = node->values[branch.index];
				++it;
				++foundCount;
			}
		}
		else {
			// compute distance for each branch
			BranchDistance branches[max_child_items];
			for (count_type index = 0; index < node->count; ++index) {
				const bbox_type &nodeBBox = node->bboxes[index];

				BranchDistance &branch = branches[index];
				// check if the radius overlaps with the node's bbox
				if (nodeBBox.overlaps(point, radius)) {
					// use euclidean distance
					T center[2];
					nodeBBox.center(center);
					branch.distance = distance(point, center);
				}
				else
					// discarded branch
					branch.distance = -1;

				branch.index = index;
			}

			// sort branches based on distances
			std::sort(branches, branches + node->count);
			// prune branches
			count_type first = node->count;
			for (count_type index = 0; index < node->count; ++index) {
				BranchDistance &branch = branches[index];
				if (branch.distance >= 0) {
					first = index;
					break;
				}
			}

			// iterate through the active branches
			for (count_type index = first; index < node->count; ++index) {
				BranchDistance &branch = branches[index];
				nearestRec(node->children[branch.index], point, radius, foundCount, it);
			}
		}
	}

	TREE_TEMPLATE
		typename TREE_QUAL::node_iterator TREE_QUAL::root() {
		return node_iterator(m_root);
	}

	TREE_TEMPLATE
		typename TREE_QUAL::depth_iterator TREE_QUAL::dbegin() {
		typedef detail::Stack<node_type, count_type> base_it_type;

		depth_iterator it;

		node_ptr_type first = m_root;
		while (first) {
			if (first->level == 1) // penultimate level
			{
				if (first->count) {
					it.push(first, 0, base_it_type::eNormal);
				}
				break;
			}
			else if (first->count > 0) {
				it.push(first, 0, base_it_type::eNormal);
			}

			first = first->children[0];
		}
		return it;
	}

	TREE_TEMPLATE
		typename TREE_QUAL::leaf_iterator TREE_QUAL::lbegin() {
		leaf_iterator it;

		node_ptr_type first = m_root;
		while (first) {
			if (first->isBranch() && first->count > 1) {
				it.push(first, 1); // Descend sibling branch later
			}
			else if (first->isLeaf()) {
				if (first->count) {
					it.push(first, 0);
				}
				break;
			}

			first = first->children[0];
		}
		return it;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TREE_TEMPLATE
		int TREE_QUAL::base_iterator::level() const {
		assert(m_tos > 0);
		const typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->level;
	}

	TREE_TEMPLATE
		bool TREE_QUAL::base_iterator::valid() const { return (m_tos > 0); }

	TREE_TEMPLATE
		ValueType &TREE_QUAL::base_iterator::operator*() {
		assert(valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->values[curTos.branchIndex];
	}

	TREE_TEMPLATE
		const ValueType &TREE_QUAL::base_iterator::operator*() const {
		assert(valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->values[curTos.branchIndex];
	}

	TREE_TEMPLATE
		const typename TREE_QUAL::bbox_type &TREE_QUAL::base_iterator::bbox() const {
		assert(valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->bboxes[curTos.branchIndex];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TREE_TEMPLATE
		TREE_QUAL::node_iterator::node_iterator() : m_index(0), m_node(NULL) {}

	TREE_TEMPLATE
		TREE_QUAL::node_iterator::node_iterator(node_ptr_type node)
		: m_index(0), m_node(node) {
		assert(node);
	}

	TREE_TEMPLATE
		int TREE_QUAL::node_iterator::level() const {
		assert(m_node);
		return m_node->level;
	}

	TREE_TEMPLATE
		bool TREE_QUAL::node_iterator::valid() const {
		assert(m_node);
		return (m_index < m_node->count);
	}

	TREE_TEMPLATE
		ValueType &TREE_QUAL::node_iterator::operator*() {
		assert(valid());
		return m_node->values[m_index];
	}

	TREE_TEMPLATE
		const ValueType &TREE_QUAL::node_iterator::operator*() const {
		assert(valid());
		return m_node->values[m_index];
	}

	TREE_TEMPLATE
		const typename TREE_QUAL::bbox_type &TREE_QUAL::node_iterator::bbox() const {
		return m_node->bboxes[m_index];
	}

	TREE_TEMPLATE
		void TREE_QUAL::node_iterator::next() {
		assert(valid());
		++m_index;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TREE_TEMPLATE
		void TREE_QUAL::depth_iterator::next() {
		for (;;) {
			if (m_tos <= 0)
				break;

			// Copy stack top cause it may change as we use it
			const typename base_type::StackElement curTos = pop();

			if (curTos.node->level == 1) // penultimate level
			{
				// Keep walking through data while we can
				if (curTos.branchIndex + 1 < curTos.node->count) {
					// There is more data, just point to the next one
					push(curTos.node, curTos.branchIndex + 1);
					break;
				}

				// No more data, so it will fall back to previous level
				if (m_tos > 0) {
					m_stack[m_tos - 1].status = base_type::eBranchTraversed;
				}
			}
			else {
				count_type currentBranchIndex = curTos.branchIndex;
				switch (curTos.status) {
				case base_type::eBranchTraversed: {
					// Revisit inner branch after visiting the upper levels
					push(curTos.node, curTos.branchIndex, base_type::eNextBranch);
					return;
				}
				case base_type::eNextBranch: {
					if (curTos.branchIndex + 1 < curTos.node->count) {
						// Push sibling on for future tree walk
						// This is the 'fall back' node when we finish with the current level
						push(curTos.node, ++currentBranchIndex, base_type::eNormal);
					}
					else {
						// No more data, so it will fall back to previous level
						if (m_tos > 0) {
							m_stack[m_tos - 1].status = base_type::eBranchTraversed;
						}
						break;
					}
				}
				case base_type::eNormal: {
					// Since cur node is not a leaf, push first of next level to get deeper
					// into the tree
					node_ptr_type nextLevelnode = curTos.node->children[currentBranchIndex];
					push(nextLevelnode, 0, base_type::eNormal);

					while (nextLevelnode->level != 1) {
						nextLevelnode = nextLevelnode->children[0];
						push(nextLevelnode, 0, base_type::eNormal);
					}

					//  We pushed until traget level at TOS
					return;
				}
				}
			}
		}
	}

	TREE_TEMPLATE
		typename TREE_QUAL::node_iterator TREE_QUAL::depth_iterator::child() {
		assert(m_tos > 0);
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return node_iterator(curTos.node->children[curTos.branchIndex]);
	}

	TREE_TEMPLATE
		typename TREE_QUAL::node_iterator TREE_QUAL::depth_iterator::current() {
		assert(m_tos > 0);
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return node_iterator(curTos.node);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TREE_TEMPLATE
		void TREE_QUAL::leaf_iterator::next() {
		for (;;) {
			if (m_tos <= 0)
				break;

			// Copy stack top cause it may change as we use it
			const typename base_type::StackElement curTos = pop();

			if (curTos.node->isLeaf()) {
				// Keep walking through data while we can
				if (curTos.branchIndex + 1 < curTos.node->count) {
					// There is more data, just point to the next one
					push(curTos.node, curTos.branchIndex + 1);
					break;
				}
				// No more data, so it will fall back to previous level
			}
			else {
				if (curTos.branchIndex + 1 < curTos.node->count) {
					// Push sibling on for future tree walk
					// This is the 'fall back' node when we finish with the current level
					push(curTos.node, curTos.branchIndex + 1);
				}
				// Since cur node is not a leaf, push first of next level to get deeper
				// into the tree
				node_ptr_type nextLevelnode = curTos.node->children[curTos.branchIndex];
				push(nextLevelnode, 0);

				// If we pushed on a new leaf, exit as the data is ready at TOS
				if (nextLevelnode->isLeaf())
					break;
			}
		}
	}

#ifdef TREE_DEBUG_TAG

	TREE_TEMPLATE
		std::string &TREE_QUAL::base_iterator::tag() {
		assert(valid());
		typename base_type::StackElement &curTos = m_stack[m_tos - 1];
		return curTos.node->debugTags[curTos.branchIndex];
	}

	TREE_TEMPLATE
		std::string &TREE_QUAL::node_iterator::tag() {
		assert(valid());
		return m_node->debugTags[m_index];
	}

#endif

#undef TREE_TEMPLATE
#undef TREE_QUAL
} // namespace spatial
