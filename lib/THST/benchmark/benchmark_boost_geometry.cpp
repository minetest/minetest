//
// Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
#include "spatial_index_benchmark.hpp"
#ifdef SIBENCH_RTREE_LOAD_BLK
#define BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL 1
#endif
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>
// enable internal debugging utilities
#include "../config.h"

#include <boost/geometry/index/detail/rtree/utilities/print.hpp>
#include <boost/geometry/index/detail/rtree/utilities/statistics.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<sibench::coord_t, 2, bg::cs::cartesian> point_t;
typedef bg::model::box<point_t> box_t;

namespace {

#ifdef SIBENCH_BGI_RTREE_PARAMS_CT
std::string const lib("bgi_ct");
#elif SIBENCH_BGI_RTREE_PARAMS_RT
std::string const lib("bgi");
#else
#error Boost.Geometry rtree parameters variant unknown
#endif

struct ArrayIndexable {
  typedef sibench::id_type V;
  typedef box_t result_type;

  ArrayIndexable(const sibench::boxes2d_t &array) : array(&array) {}

  result_type operator()(sibench::id_type id) const {
    const auto &bbox = (*array)[id];
    return result_type(point_t(bbox.min[0], bbox.min[1]),
                       point_t(bbox.max[0], bbox.max[1]));
  }

private:
  const sibench::boxes2d_t *array;
};

#ifdef SIBENCH_RTREE_SPLIT_LINEAR
template <int max_capacity, int min_capacity>
using parameters_t = bgi::linear<max_capacity, min_capacity>;
#elif SIBENCH_RTREE_SPLIT_QUADRATIC
template <int max_capacity, int min_capacity>
using parameters_t = bgi::quadratic<max_capacity, min_capacity>;
#else
template <int max_capacity, int min_capacity>
using parameters_t = bgi::rstar<max_capacity, min_capacity>;
#endif
template <int max_capacity, int min_capacity>
using rtree_t =
    bgi::rtree<sibench::id_type, parameters_t<max_capacity, min_capacity>,
               ArrayIndexable>;

template <typename T>
void print_statistics(std::ostream &os, std::string const &lib, T const &i) {
  using bgi::detail::rtree::utilities::statistics;
  auto const s = statistics(i);
  os << sibench::get_banner(lib) << " stats: levels=" << boost::get<0>(s)
     << ", nodes=" << boost::get<1>(s) << ", leaves=" << boost::get<2>(s)
     << ", values=" << boost::get<3>(s) << ", values_min=" << boost::get<4>(s)
     << ", values_max=" << boost::get<5>(s) << std::endl;
}
}

template <typename Parameters>
void setCapacity(const Parameters &parameters, sibench::result_info &info) {
  info.min_capacity = Parameters::min_elements;
  info.max_capacity = Parameters::max_elements;
}

void setCapacity(const bgi::dynamic_linear &parameters,
                 sibench::result_info &info) {
  info.min_capacity = parameters.get_min_elements();
  info.max_capacity = parameters.get_max_elements();
}

void setCapacity(const bgi::dynamic_quadratic &parameters,
                 sibench::result_info &info) {
  info.min_capacity = parameters.get_min_elements();
  info.max_capacity = parameters.get_max_elements();
}

void setCapacity(const bgi::dynamic_rstar &parameters,
                 sibench::result_info &info) {
  info.min_capacity = parameters.get_min_elements();
  info.max_capacity = parameters.get_max_elements();
}

template <class TreeClass>
sibench::result_info benchmark_load(const sibench::boxes2d_t &boxes,
                                    TreeClass &rtree) {

  sibench::result_info load_r;
  setCapacity(rtree.parameters(), load_r);

  typedef std::vector<sibench::id_type> box_values_t;
  auto const iterations = boxes.size();
  box_values_t vboxes(iterations);
  std::iota(vboxes.begin(), vboxes.end(), 0);

#ifdef SIBENCH_RTREE_LOAD_BLK
  auto const marks = sibench::benchmark(
      "load", iterations, vboxes,
      [&rtree](box_values_t const &boxes, std::size_t iterations) {
        assert(iterations <= boxes.size());
        rtree = TreeClass(boxes.cbegin(), boxes.cbegin() + iterations, //
                          rtree.parameters(), rtree.indexable_get());
      });
#else
  auto const marks = sibench::benchmark(
      "load", iterations, vboxes,
      [&rtree](box_values_t const &boxes, std::size_t iterations) {
        assert(iterations <= boxes.size());
        rtree.insert(boxes.cbegin(), boxes.cbegin() + iterations);
      });
#endif

  load_r.accumulate(marks);

#if SIBENCH_DEBUG_PRINT_INFO == 1
  sibench::print_result(std::cout, lib, marks);
  print_statistics(std::cout, lib, rtree);
#endif

  return load_r;
}

template <class TreeClass>
sibench::result_info benchmark_query(const sibench::boxes2d_t &boxes,
                                     const TreeClass &rtree) {
  std::size_t query_found = 0;

  sibench::result_info query_r;
  setCapacity(rtree.parameters(), query_r);

  auto const marks = sibench::benchmark(
      "query", sibench::max_queries, boxes,
      [&rtree, &query_found](sibench::boxes2d_t const &boxes,
                             std::size_t iterations) {
        std::vector<sibench::id_type> results;
        results.reserve(iterations);

        for (std::size_t i = 0; i < iterations; ++i) {
          results.clear();

          auto const &box = boxes[i];
          point_t p1(box.min[0] - sibench::query_size,
                     box.min[1] - sibench::query_size);
          point_t p2(box.max[0] + sibench::query_size,
                     box.max[1] + sibench::query_size);
          box_t region(p1, p2);

          rtree.query(bgi::intersects(region), std::back_inserter(results));
          query_found += results.size();
        }
      });

  query_r.accumulate(marks);

#if SIBENCH_DEBUG_PRINT_INFO == 1
  sibench::print_result(std::cout, lib, marks);
  sibench::print_query_count(std::cout, lib, query_found);
#endif
  return query_r;
}

sibench::result_info benchmark_random(const sibench::boxes2d_t &boxes,
                                      size_t iterations) {

  ArrayIndexable indexable(boxes);
  rtree_t<16, 8> tree(parameters_t<16, 8>(), indexable);

  size_t insert_count = 0, total_count = 0;

  sibench::result_info res;
  setCapacity(tree.parameters(), res);
  res.iterations = iterations;

  size_t start = 0;
  for (int i = 0; i < sibench::random_runs; ++i) {
    sibench::result_info const marks = sibench::benchmark(
        "random", iterations, boxes,
        [&tree, &start, &insert_count, &total_count](
            sibench::boxes2d_t const &boxes, std::size_t iterations) {

          tree.clear();

          start = (start + iterations) % boxes.size();

          auto end = start + iterations;

          if (end > boxes.size()) {
            start = boxes.size() - iterations;
            end = boxes.size();
          }

          assert(end <= boxes.size());
          for (size_t i = start; i < end; ++i) {
            assert(iterations <= boxes.size());
            auto const &box = boxes[i];
            point_t p1(box.min[0], box.min[1]);
            point_t p2(box.max[0], box.max[1]);
            box_t region(p1, p2);

            if (tree.query(bgi::intersects(region),
                           spatial::detail::dummy_iterator()) == 0) {
              tree.insert(i);
              ++insert_count;
            }
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

void benchmark_run_rt(const sibench::boxes2d_t &boxes) {
  for (std::size_t next_capacity = 4; next_capacity <= sibench::max_capacities;
       next_capacity += sibench::capacity_step) {
    double const fill_factor = 0.5;
    std::size_t const min_capacity = next_capacity;
    std::size_t const max_capacity =
        std::size_t(std::floor(min_capacity / fill_factor));
    {
      ArrayIndexable indexable(boxes);

#ifdef SIBENCH_RTREE_SPLIT_LINEAR
      typedef bgi::dynamic_linear rtree_parameters_t;
#elif SIBENCH_RTREE_SPLIT_QUADRATIC
      typedef bgi::dynamic_quadratic rtree_parameters_t;
#else
      typedef bgi::dynamic_rstar rtree_parameters_t;
#endif
      typedef bgi::rtree<sibench::id_type, rtree_parameters_t, ArrayIndexable>
          rtree_t;

      rtree_parameters_t rtree_parameters(max_capacity, min_capacity);
      rtree_t rtree(rtree_parameters, indexable);

      sibench::result_info load_r = benchmark_load(boxes, rtree);
      sibench::result_info query_r = benchmark_query(boxes, rtree);

      // single line per run
      sibench::print_result(std::cout, lib, load_r, query_r);

    } // for capacity
  }
}

template <int max_capacity, int min_capacity>
void benchmark_run_ct(const sibench::boxes2d_t &boxes) {

  ArrayIndexable indexable(boxes);
  rtree_t<max_capacity, min_capacity> rtree(
      parameters_t<max_capacity, min_capacity>(), indexable);

  sibench::result_info load_r = benchmark_load(boxes, rtree);
  sibench::result_info query_r = benchmark_query(boxes, rtree);

  // single line per run
  sibench::print_result(std::cout, lib, load_r, query_r);
}

int main() {
  try {

    sibench::print_result_header(std::cout, lib);

    // Generate random objects for indexing
    auto const boxes = sibench::generate_boxes(sibench::max_insertions);

#ifdef SIBENCH_BGI_RTREE_PARAMS_RT
    benchmark_run_rt(boxes);
#elif SIBENCH_BGI_RTREE_PARAMS_CT
    std::size_t const max_capacity = sibench::constant_max_capacity;
    std::size_t const min_capacity = sibench::constant_min_capacity;
    benchmark_run_ct<max_capacity, min_capacity>(boxes);
    benchmark_run_ct<max_capacity * 2, min_capacity * 2>(boxes);
    benchmark_run_ct<max_capacity * 3, min_capacity * 3>(boxes);
    benchmark_run_ct<max_capacity * 4, min_capacity * 4>(boxes);
    benchmark_run_ct<max_capacity * 5, min_capacity * 5>(boxes);
    benchmark_run_ct<max_capacity * 6, min_capacity * 6>(boxes);
    benchmark_run_ct<max_capacity * 8, min_capacity * 8>(boxes);
    benchmark_run_ct<max_capacity * 9, min_capacity * 9>(boxes);
    benchmark_run_ct<max_capacity * 11, min_capacity * 11>(boxes);
    benchmark_run_ct<max_capacity * 12, min_capacity * 12>(boxes);
    benchmark_run_ct<max_capacity * 16, min_capacity * 16>(boxes);
    benchmark_run_ct<max_capacity * 20, min_capacity * 20>(boxes);
    benchmark_run_ct<max_capacity * 24, min_capacity * 24>(boxes);
    benchmark_run_ct<max_capacity * 28, min_capacity * 28>(boxes);
    benchmark_run_ct<max_capacity * 32, min_capacity * 32>(boxes);
    benchmark_run_ct<max_capacity * 36, min_capacity * 36>(boxes);
    benchmark_run_ct<max_capacity * 40, min_capacity * 40>(boxes);
#else
#error "Unknown params!"
#endif

#ifndef SIBENCH_RTREE_LOAD_BLK
    // bulk load cannot be used since only dynamic insertions
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
