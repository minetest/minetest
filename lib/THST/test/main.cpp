
#include "custom_allocator.h"

#define SPATIAL_TREE_ALLOCATOR 2

#include <QuadTree.h>
#include <RTree.h>
#include <iostream>
#include <numeric>
#include <sstream>

template <typename T> struct Point {
	union {
		T data[2];
		struct {
			T x, y;
		};
	};

	void set(T x, T y) {
		this->x = x;
		this->y = y;
	}

	inline float distance(const Point point) const {

		float d = data[0] - point.data[0];
		d *= d;
		for (int i = 1; i < 2; i++)
		{
			float temp = data[i] - point.data[i];
			d += temp * temp;
		}
		return std::sqrt(d);
	}
};

template <typename T> struct Box2 {
	T min[2];
	T max[2];

	explicit operator spatial::BoundingBox<int, 2>() const {
		return spatial::BoundingBox<int, 2>(min, max);
	}

	bool operator==(const Box2& other) const
	{
		return min[0] == other.min[0] && min[1] == other.min[1] && max[0] == other.max[0] && max[1] == other.max[1];
	}
};

template <typename T>
std::ostream &operator<<(std::ostream &stream, const Box2<T> &bbox) {
	stream << "min: " << bbox.min[0] << " " << bbox.min[1] << " ";
	stream << "max: " << bbox.max[0] << " " << bbox.max[1];
	return stream;
}

struct Object {
	spatial::BoundingBox<int, 2> bbox;
	std::string name;
};

const Box2<int> kBoxes[] = {
	{{16, 0}, {32, 16}},   {{0, 0}, {8, 8}},      {{4, 4}, {6, 8}},
	{{4, 2}, {8, 4}},      {{3, 3}, {12, 16}},    {{0, 0}, {64, 32}},
	{{32, 32}, {64, 128}}, {{128, 0}, {256, 64}}, {{120, 64}, {250, 128}} };

int main() {
	std::vector<Object> objects;
	std::vector<Box2<int>> boxes;

	size_t i = 0;
	std::cout << "Creating objects:\n";
	for (const auto &bbox : kBoxes) {
		boxes.push_back(bbox);
		objects.emplace_back();
		Object &obj = objects.back();

		std::stringstream ss;
		ss << "object" << i++;
		obj.name = ss.str();
		obj.bbox = (spatial::BoundingBox<int, 2>)bbox;

		std::cout << obj.name << " " << obj.bbox << "\n";
	}
	std::cout << std::endl;

	// create a quad tree with the given box
	spatial::BoundingBox<int, 2> bbox((spatial::box::empty_init()));
	Point<int> point;
	point.set(0, 0);
	bbox.extend(point.data);
	point.set(256, 128);
	bbox.extend(point.data);

	typedef spatial::RTree<int, Box2<int>, 2, 4, 2> rtree_box_t;
	typedef spatial::QuadTree<int, Box2<int>, 2> qtree_box_t;
	qtree_box_t qtree(bbox.min, bbox.max);
	rtree_box_t rtree;

	// insert elements into the trees
	{
		rtree.count();
		rtree = rtree_box_t(std::begin(kBoxes), std::end(kBoxes));
		qtree =
			qtree_box_t(bbox.min, bbox.max, std::begin(kBoxes), std::end(kBoxes));

		rtree.clear();
		qtree.clear();

		// or construction via insert
		qtree.insert(std::begin(kBoxes), std::end(kBoxes));
		rtree.insert(std::begin(kBoxes), std::end(kBoxes));
		Box2<int> box = { {7, 3}, {14, 6} };
		qtree.insert(box);
		rtree.insert(box);

#ifdef SPATIAL_TREE_CPP11
		// insert only if predicate is always valid
		Box2<int> box2 = { {7, 4}, {14, 6} };
		bool wasAdded =
			rtree.insert(box2, [&box2](const decltype(rtree)::bbox_type &bbox) {
			const decltype(rtree)::bbox_type cbbox(box2.min, box2.max);
			return !bbox.overlaps(cbbox);
		});
		std::cout << "Condition insert: " << wasAdded << "\n";
#endif
		std::cout << "Created trees, element count: " << rtree.count() << "\n";
		std::cout << std::endl;

		// remove an element
		bool removed = rtree.remove(box);
		assert(removed);
	}

	// query for results within the search box
	{
		Box2<int> searchBox = { {0, 0}, {8, 31} };
		std::vector<Box2<int>> results;
		rtree.query(spatial::intersects<2>(searchBox.min, searchBox.max),
			std::back_inserter(results));
		std::cout << "Overlaps results: " << std::endl;
		for (const auto &res : results)
			std::cout << res << "\n";

		results.clear();
		qtree.query(spatial::intersects<2>(searchBox.min, searchBox.max),
			std::back_inserter(results));

		results.clear();
		rtree.query(spatial::contains<2>(searchBox.min, searchBox.max),
			std::back_inserter(results));
		std::cout << "Contains results: " << std::endl;
		for (const auto &res : results)
			std::cout << res << "\n";
		std::cout << std::endl;
	}

	// use a custom indexable for custom objects
	struct Indexable {
		const int *min(const Object &value) const { return value.bbox.min; }
		const int *max(const Object &value) const { return value.bbox.max; }
	};
	{

		spatial::QuadTree<int, Object, 2, Indexable> qtree(bbox.min, bbox.max);
		qtree.insert(objects.begin(), objects.end());

		spatial::RTree<int, Object, 2, 4, 2, Indexable> rtree;
		rtree.insert(objects.begin(), objects.end());
	}

	// leaf traversal of the tree
	{
		spatial::RTree<int, Object, 2, 4, 2, Indexable> rtree;
		rtree.insert(objects.begin(), objects.end());

		std::cout << "Initial order:\n";
		for (const auto &obj : objects) {
			std::cout << obj.name << "\n";
		}
		std::cout << "Leaf tree traversal order:\n";
		// gives the spatial partitioning order within the tree
		for (auto it = rtree.lbegin(); it.valid(); it.next()) {
			std::cout << (*it).name << "\n";
		}
		std::cout << std::endl;
	}

	// depth traversal of the tree
	{
		spatial::RTree<int, Object, 2, 4, 2, Indexable> rtree;
		rtree.insert(objects.begin(), objects.end());

		std::cout << "Depth tree traversal: "
			<< "\n";
		std::cout << std::endl;

		assert(rtree.levels() > 0);
		for (auto it = rtree.dbegin(); it.valid(); it.next()) {
			std::string parentName;
			spatial::BoundingBox<int, 2> parentBBox;

			// traverse current children of the parent node(i.e. upper level)
			for (auto nodeIt = it.child(); nodeIt.valid(); nodeIt.next()) {
				std::cout << "level: " << nodeIt.level() << " " << (*nodeIt).name
					<< "\n";
				parentName += (*nodeIt).name + " + ";
				parentBBox.extend(nodeIt.bbox());
			}
			(*it).name = parentName;
			(*it).bbox = parentBBox;
			std::cout << "level: " << it.level() << " " << parentName << "\n";
		}
		std::cout << std::endl;

		// special hierarchical query
		{
			std::cout << "Hierarchical query: "
				<< "\n";
			std::vector<Object> results;
			Box2<int> searchBox = { {0, 0}, {8, 31} };
			rtree.hierachical_query(
				spatial::intersects<2>(searchBox.min, searchBox.max),
				std::back_inserter(results));
			for (const auto &res : results)
				std::cout << res.name << "\n";
		}
		std::cout << std::endl;

		// nearest neighbor query
		{
			std::cout << "Nearest radius query: "
				<< "\n";
			std::vector<Object> results;
			Point<int> p = { {0, 0} };
			rtree.nearest(
				p.data, 100,
				std::back_inserter(results));
			for (const auto& res : results) {
				Point<int>  center;
				res.bbox.center(center.data);
				std::cout << res.name << " : " << p.distance(center) << "\n";
			}

			std::cout << "Nearest knn query: "
				<< "\n";

			results.clear();
			rtree.k_nearest(
				p.data, 3,
				std::back_inserter(results));
			for (const auto& res : results)
				std::cout << res.name << " : " << res.bbox.distance(p.data) << "\n";
		}
		std::cout << std::endl;
	}

	// use a custom indexable for array and indices as values
	{
		struct ArrayIndexable {

			ArrayIndexable(const std::vector<Box2<int>> &array) : array(array) {}

			const int *min(const uint32_t index) const { return array[index].min; }
			const int *max(const uint32_t index) const { return array[index].max; }

			const std::vector<Box2<int>> &array;
		};

		ArrayIndexable indexable(boxes);

		typedef uint32_t IndexType;
		spatial::RTree<int, IndexType, 2, 4, 2, ArrayIndexable> rtree(indexable);

		std::vector<uint32_t> indices(boxes.size());
		std::iota(indices.begin(), indices.end(), 0);
		rtree.insert(indices.begin(), indices.end());

		indices.clear();
		Box2<int> searchBox = { {0, 0}, {8, 31} };
		rtree.query(spatial::intersects<2>(searchBox.min, searchBox.max),
			std::back_inserter(indices));
		std::cout << "Object query results: " << std::endl;
		for (auto index : indices)
			std::cout << "index: " << index << " " << objects[index].name << " "
			<< objects[index].bbox << "\n";
		std::cout << std::endl;

		// custom allocator example
#if SPATIAL_TREE_ALLOCATOR == SPATIAL_TREE_DEFAULT_ALLOCATOR
		{
			std::cout << "Custom allocator example:\n";

			const int kMaxKeysPerNode = 4;
			const int kVolumeMode = spatial::box::eSphericalVolume;

			typedef spatial::BoundingBox<int, 2> tree_bbox_type;
			typedef spatial::detail::Node<uint32_t, tree_bbox_type, kMaxKeysPerNode>
				tree_node_type;
			typedef test::tree_allocator<tree_node_type> tree_allocator_type;

			typedef spatial::RTree<int, uint32_t, 2, kMaxKeysPerNode,
				kMaxKeysPerNode / 2, ArrayIndexable, kVolumeMode,
				double, tree_allocator_type>
				CustomTree;

			CustomTree rtree(indexable, tree_allocator_type(), true);

			indices.resize(boxes.size());
			std::iota(indices.begin(), indices.end(), 0);
			rtree.insert(indices.begin(), indices.end());
			std::cout << std::endl;
		}
#endif
	}

	return 0;
}
