
#include "../test/custom_allocator.h"
#include "spatial_index_benchmark.hpp"

#include <QuadTree.h>
#include <RTree.h>

namespace {

std::string const lib("thst");

struct ArrayIndexable {

  ArrayIndexable(const sibench::boxes2d_t &array) : array(array) {}

  const sibench::coord_t *min(const uint32_t index) const {
    return array[index].min;
  }
  const sibench::coord_t *max(const uint32_t index) const {
    return array[index].max;
  }

private:
  const sibench::boxes2d_t &array;
};

#ifdef SIBENCH_RTREE_SPLIT_QUADTREE
const int quadtree_factor = 64;

template <int max_capacity>
using qtree_t = spatial::QuadTree<sibench::coord_t, sibench::id_type,
                                  max_capacity, ArrayIndexable>;

#else

#ifdef SIBENCH_RTREE_SPLIT_QUADRATIC_SPHERE
const int kVolumeMode = spatial::box::eSphericalVolume;
#else
const int kVolumeMode = spatial::box::eNormalVolume;
#endif

using tree_bbox_type = spatial::BoundingBox<sibench::coord_t, 2>;
template <int max_capacity>
using tree_node_type =
    spatial::detail::Node<sibench::id_type, tree_bbox_type, max_capacity>;

#ifdef SIBENCH_RTREE_LOAD_CUSTOM
template <int max_capacity>
using tree_allocator_type =
    test::tree_allocator<tree_node_type<max_capacity>, false>;
#else
template <int max_capacity>
using tree_allocator_type = test::heap_allocator<tree_node_type<max_capacity>>;
#endif //#ifdef SIBENCH_RTREE_LOAD_CUSTOM

template <int min_capacity, int max_capacity>
using rtree_t =
    spatial::RTree<sibench::coord_t, sibench::id_type, 2, max_capacity,
                   min_capacity, ArrayIndexable, kVolumeMode, sibench::coord_t,
                   tree_allocator_type<max_capacity>>;
#endif // #ifdef SIBENCH_RTREE_SPLIT_QUADTREE

template <typename T>
void print_statistics(std::ostream &os, std::string const &lib, T const &i) {
  os << sibench::get_banner(lib) << " stats: levels=" << i.levels()
     << " values=" << i.count()
#ifndef SIBENCH_RTREE_SPLIT_QUADTREE
     << " nodes=" << i.allocator().count
#ifdef SIBENCH_RTREE_LOAD_CUSTOM
     << " estimated nodes=" << i.allocator().buffer.size()
#endif
#endif
     << std::endl;
}
} // unnamed namespace

template <class TreeClass>
void setCapacity(const TreeClass &tree, sibench::result_info &res) {
#ifdef SIBENCH_RTREE_SPLIT_QUADTREE
  res.max_capacity = res.min_capacity = TreeClass::max_items / quadtree_factor;
#else
  res.min_capacity = TreeClass::min_items;
  res.max_capacity = TreeClass::max_items;
#endif
}

template <class TreeClass>
sibench::result_info benchmark_load(const sibench::boxes2d_t &boxes,
                                    TreeClass &tree) {

  sibench::result_info res;
  setCapacity(tree, res);

  typedef std::vector<sibench::id_type> box_values_t;
  auto const iterations = boxes.size();
  box_values_t vboxes(iterations);
  std::iota(vboxes.begin(), vboxes.end(), 0);

#if defined(SIBENCH_RTREE_LOAD_ITR) || defined(SIBENCH_RTREE_LOAD_CUSTOM)
  auto const marks = sibench::benchmark(
      "insert", iterations, vboxes,
      [&tree](box_values_t const &boxes, std::size_t iterations) {
        assert(iterations <= boxes.size());
        tree.insert(boxes.cbegin(), boxes.cbegin() + iterations);
      });

  res.accumulate(marks);
#else
#error Unknown tree loading method
#endif

#if SIBENCH_DEBUG_PRINT_INFO == 1
  sibench::print_result(std::cout, lib, marks);
  print_statistics(std::cout, lib, tree);
#endif

  return res;
}

template <class TreeClass>
sibench::result_info benchmark_query(const sibench::boxes2d_t &boxes,
                                     const TreeClass &tree) {
  size_t query_found = 0;

  sibench::result_info res;
  setCapacity(tree, res);

  sibench::result_info const marks = sibench::benchmark(
      "query", sibench::max_queries, boxes,
      [&tree, &query_found](sibench::boxes2d_t const &boxes,
                            std::size_t iterations) {
        std::vector<sibench::id_type> results;
        results.reserve(iterations);

        for (size_t i = 0; i < iterations; ++i) {
          results.clear();
          auto const &box = boxes[i];
          sibench::coord_t min[2] = {box.min[0] - sibench::query_size,
                                     box.min[1] - sibench::query_size};
          sibench::coord_t max[2] = {box.max[0] + sibench::query_size,
                                     box.max[1] + sibench::query_size};

          tree.query(spatial::intersects<2>(min, max),
                     std::back_inserter(results));

          query_found += results.size();
        }
      });

  res.accumulate(marks);

#if SIBENCH_DEBUG_PRINT_INFO == 1
  sibench::print_result(std::cout, lib, marks);
  sibench::print_query_count(std::cout, lib, query_found);
#endif

  return res;
}

#ifndef SIBENCH_RTREE_SPLIT_QUADTREE
sibench::result_info benchmark_random(const sibench::boxes2d_t &boxes,
                                      size_t iterations) {

  typedef rtree_t<8, 16> tree_t;
  ArrayIndexable indexable(boxes);

#if SIBENCH_RTREE_LOAD_CUSTOM
  tree_t tree(indexable, tree_t::allocator_type(), false);
  size_t nodeCount = tree_t::nodeCount(iterations);
  tree.allocator().resize(std::max(nodeCount * 3, (size_t)10));
#else
  tree_t tree(indexable);
#endif

  size_t insert_count = 0, total_count = 0;

  sibench::result_info res;
  setCapacity(tree, res);
  res.iterations = iterations;

  size_t start = 0;
  for (int i = 0; i < sibench::random_runs; ++i) {
    sibench::result_info const marks = sibench::benchmark(
        "random", iterations, boxes,
        [&tree, &start, &insert_count, &total_count](
            sibench::boxes2d_t const &boxes, std::size_t iterations) {

#ifdef SIBENCH_RTREE_LOAD_CUSTOM
          tree.allocator().index = tree.allocator().count = 0;
          // cleanup was handled by the allocator
          tree.clear(false);
#else
          tree.clear();
#endif

          start = (start + iterations) % boxes.size();

          auto end = start + iterations;

          if (end > boxes.size()) {
            start = boxes.size() - iterations;
            end = boxes.size();
          }

          assert(end <= boxes.size());
          for (size_t i = start; i < end; ++i) {
            assert(iterations <= boxes.size());
            auto const &currentBox = boxes[i];

            bool added = tree.insert(
                i, [&currentBox](const decltype(tree)::bbox_type &bbox) {
                  const decltype(tree)::bbox_type cbbox(currentBox.min,
                                                        currentBox.max);
                  return !bbox.overlaps(cbbox);
                });

            insert_count += added;
            ++total_count;
          }
        });
    res.accumulate(marks);
  }

#if SIBENCH_DEBUG_PRINT_INFO == 1
  sibench::print_insert_count(std::cout, lib, insert_count, total_count);
#endif

  return res;
}
#endif

template <int max_capacity, int min_capacity>
void benchmark_run(const sibench::boxes2d_t &boxes,
                   const spatial::BoundingBox<sibench::coord_t, 2> &world_box) {

  ArrayIndexable indexable(boxes);
#if SIBENCH_RTREE_SPLIT_QUADTREE
  qtree_t<max_capacity * quadtree_factor> tree(world_box.min, world_box.max,
                                               indexable);

#elif SIBENCH_RTREE_SPLIT_QUADRATIC || SIBENCH_RTREE_SPLIT_QUADRATIC_SPHERE

#if SIBENCH_RTREE_LOAD_CUSTOM
  typedef rtree_t<min_capacity, max_capacity> tree_t;
  tree_t tree(indexable, typename tree_t::allocator_type(), false);
  size_t nodeCount =
      rtree_t<min_capacity, max_capacity>::nodeCount(sibench::max_insertions);
  tree.allocator().resize(nodeCount * 1.5);
  tree.clear();
#else
  rtree_t<min_capacity, max_capacity> tree(indexable);
#endif

#else
#error Unknown tree split method
#endif //#ifdef SIBENCH_RTREE_SPLIT_QUADTREE
  sibench::result_info load_r = benchmark_load(boxes, tree);
  sibench::result_info query_r = benchmark_query(boxes, tree);

  // single line per run
  sibench::print_result(std::cout, lib, load_r, query_r);
}

int main() {
  try {

    sibench::print_result_header(std::cout, lib);

#ifdef SIBENCH_THST_RTREE_PARAMS_CT
    // Generate random objects for indexing
    auto const boxes = sibench::generate_boxes(sibench::max_insertions);
    spatial::BoundingBox<sibench::coord_t, 2> world_box;
    world_box.init();
    for (const auto &box : boxes) {
      world_box.extend(box.min);
      world_box.extend(box.max);
    }

    std::size_t const max_capacity = sibench::constant_max_capacity;
    std::size_t const min_capacity = sibench::constant_min_capacity;
    benchmark_run<max_capacity, min_capacity>(boxes, world_box);
    benchmark_run<max_capacity * 2, min_capacity * 2>(boxes, world_box);
    benchmark_run<max_capacity * 3, min_capacity * 3>(boxes, world_box);
    benchmark_run<max_capacity * 4, min_capacity * 4>(boxes, world_box);
    benchmark_run<max_capacity * 5, min_capacity * 5>(boxes, world_box);
    benchmark_run<max_capacity * 6, min_capacity * 6>(boxes, world_box);
    benchmark_run<max_capacity * 8, min_capacity * 8>(boxes, world_box);
    benchmark_run<max_capacity * 9, min_capacity * 9>(boxes, world_box);
    benchmark_run<max_capacity * 11, min_capacity * 11>(boxes, world_box);
    benchmark_run<max_capacity * 12, min_capacity * 12>(boxes, world_box);
    benchmark_run<max_capacity * 16, min_capacity * 16>(boxes, world_box);
    benchmark_run<max_capacity * 20, min_capacity * 20>(boxes, world_box);
    benchmark_run<max_capacity * 24, min_capacity * 24>(boxes, world_box);
    benchmark_run<max_capacity * 28, min_capacity * 28>(boxes, world_box);
    benchmark_run<max_capacity * 32, min_capacity * 32>(boxes, world_box);
    benchmark_run<max_capacity * 36, min_capacity * 36>(boxes, world_box);
    benchmark_run<max_capacity * 40, min_capacity * 40>(boxes, world_box);
#else
#error Only compile time support for rtree/quadtree
#endif //#ifdef SIBENCH_THST_RTREE_PARAMS_CT

#ifndef SIBENCH_RTREE_SPLIT_QUADTREE
    sibench::print_insert_result_header(std::cout, lib);
    for (int insertions = 10; insertions < sibench::max_random_insertions;
         insertions += insertions * sibench::insert_step) {

      sibench::result_info random_r = benchmark_random(boxes, insertions);
      sibench::print_insert_result(std::cout, lib, random_r);
    }
#endif

    return EXIT_SUCCESS;
  } catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
  }
  return EXIT_FAILURE;
}
