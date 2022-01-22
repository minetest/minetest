//
// Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MLOSKOT_SPATIAL_INDEX_BENCHMARK_HPP_INCLUDED
#define MLOSKOT_SPATIAL_INDEX_BENCHMARK_HPP_INCLUDED
#ifdef _MSC_VER
#if (_MSC_VER == 1700)
#define _VARIADIC_MAX 6
#endif
#define NOMINMAX
#endif // _MSC_VER
#include "high_resolution_timer.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sibench {

//
// Default benchmark settings
//
std::size_t const max_iterations = 1000000;
std::size_t const max_capacities = 100;
std::size_t const capacity_step = 2;
std::size_t const max_insertions = max_iterations;
std::size_t const max_random_insertions = 20000;
double const insert_step = 0.10;
std::size_t const random_runs = 200;
std::size_t const max_queries = std::size_t(max_insertions * 0.1);
std::size_t const constant_max_capacity = 4;
std::size_t const constant_min_capacity = 2;
std::size_t const query_size = 100;

typedef uint32_t id_type;

#ifndef NDEBUG
#define SIBENCH_DEBUG_PRINT_INFO 1
#endif

enum class rtree_variant { rstar, linear, quadratic, rtree, quadtree };

inline std::pair<rtree_variant, std::string> get_rtree_split_variant() {
#ifdef SIBENCH_RTREE_SPLIT_LINEAR
  return std::make_pair(rtree_variant::linear, "L");
#elif SIBENCH_RTREE_SPLIT_QUADRATIC
  return std::make_pair(rtree_variant::quadratic, "Q");
#elif SIBENCH_RTREE_SPLIT_QUADRATIC_SPHERE
  return std::make_pair(rtree_variant::quadratic, "QS");
#elif SIBENCH_RTREE_SPLIT_RSTAR
  return std::make_pair(rtree_variant::rstar, "R");
#elif SIBENCH_RTREE_SPLIT_QUADTREE
  return std::make_pair(rtree_variant::quadtree, "QT");
#else
#error Unknown rtree split algorithm
#endif
}

inline std::string get_rtree_load_variant() {
#ifdef SIBENCH_RTREE_LOAD_ITR
  return "ITR";
#elif SIBENCH_RTREE_LOAD_BLK
  return "BLK";
#elif SIBENCH_RTREE_LOAD_CUSTOM
  return "CST";
#else
#error Unknown rtree loading method
#endif
}

inline std::string get_banner(std::string const &lib) {
  return lib + "(" + get_rtree_split_variant().second + ',' +
         get_rtree_load_variant() + ")";
}

//
// Generators of random objects
//
typedef float coord_t;
typedef std::vector<coord_t> coords_t;
typedef std::tuple<coord_t, coord_t> point2d_t;
typedef std::vector<point2d_t> points2d_t;
typedef struct box2d {
  coord_t min[2];
  coord_t max[2];
} box2d_t;
typedef std::vector<box2d_t> boxes2d_t;

template <typename T> struct random_generator {
  typedef typename std::uniform_real_distribution<T>::result_type result_type;

  T const max;
  std::mt19937 gen;
  std::uniform_real_distribution<T> dis;

  random_generator(std::size_t n)
      : max(static_cast<T>(n / 2)),
        gen(1) // generate the same succession of results for everyone
               // (unsigned
        // int)std::chrono::system_clock::now().time_since_epoch().count())
        ,
        dis(-max, max) {}

  result_type operator()() { return dis(gen); }

private:
  random_generator(random_generator const &) /*= delete*/;
  random_generator &operator=(random_generator const &) /*= delete*/;
};

inline coords_t generate_coordinates(std::size_t n) {
  random_generator<float> rg(n);
  coords_t coords;
  coords.reserve(n);
  for (decltype(n) i = 0; i < n; ++i) {
    coords.emplace_back(rg());
  }
  return coords;
}

inline points2d_t generate_points(std::size_t n) {
  auto coords = generate_coordinates(n * 2);
  points2d_t points;
  points.reserve(n);
  auto s = coords.size();
  for (decltype(s) i = 0; i < s; i += 2) {
    points.emplace_back(coords[i], coords[i + 1]);
  }
  return points;
}

inline boxes2d_t generate_boxes(std::size_t n) {
  random_generator<float> rg(n);

  boxes2d_t boxes;
  boxes.reserve(n);
  for (decltype(n) i = 0; i < n; ++i) {
    auto const x = rg();
    auto const y = rg();
    boxes.emplace_back();
    auto &box = boxes.back();
    float const size = 0.5f;
    box.min[0] = x - size;
    box.min[1] = y - size;
    box.max[0] = x + size;
    box.max[1] = y + size;
  }
  return boxes;
}

//
// Benchmark running routines
//
struct result_info {
  std::string step;
  double min;
  double max;
  double sum;
  double count;
  std::size_t min_capacity;
  std::size_t max_capacity;
  std::size_t iterations;

  result_info(char const *step = "")
      : step(step), min(-1), max(-1), sum(0), count(0), min_capacity(0),
        max_capacity(0), iterations(0) {}

  void accumulate(result_info const &r) {
    min = min < 0 ? r.min : (std::min)(min, r.min);
    max = max < 0 ? r.max : (std::max)(max, r.max);
    sum += r.sum;
    count += r.count;

    assert(min <= max);
  }

  template <typename Timer> void set_mark(Timer const &t) {
    auto const m = t.elapsed();
    sum += m;
    count++;
    min = min < 0 ? m : (std::min)(m, min);
    max = max < 0 ? m : (std::max)(m, max);

    assert(min <= max);
  }

  double avg() const { return sum / count; }
};

inline std::ostream &operator<<(std::ostream &os, result_info const &r) {
  os << r.step << " " << r.iterations << " in " << r.min << " to " << r.max
     << " sec" << std::endl;
  return os;
}

inline std::ostream &print_result(std::ostream &os, std::string const &lib,
                                  result_info const &r) {
  os << std::setw(15) << std::setfill(' ') << std::left
     << sibench::get_banner(lib) << ": " << r;
  return os;
}

inline std::ostream &print_result(std::ostream &os, std::string const & /*lib*/,
                                  result_info const &load,
                                  result_info const &query) {
  assert(load.min_capacity == query.min_capacity);
  assert(load.max_capacity == query.max_capacity);

  std::streamsize wn(5), wf(10);
  os << std::left << std::setfill(' ') << std::fixed << std::setprecision(6)
     << std::setw(wn) << load.max_capacity << std::setw(wn) << load.min_capacity
     << std::setw(wf) << load.min << std::setw(wf) << query.min << std::endl;
  return os;
}

inline std::ostream &print_insert_result(std::ostream &os,
                                         std::string const & /*lib*/,
                                         result_info const &res) {
  std::streamsize wn(5), wf(10);
  os << std::left << std::setfill(' ') << std::fixed << std::setprecision(6)
     << std::setw(wn) << res.max_capacity << std::setw(wn) << res.min_capacity
     << std::setw(wn * 2 + 1) << res.iterations << std::setw(wf)
     << std::setprecision(3) << res.avg() * 1000.0 << std::setw(wf)
     << std::setprecision(5) << res.sum << std::endl;
  return os;
}

inline std::ostream &print_result_header(std::ostream &os,
                                         std::string const &lib) {
  std::streamsize const wn(5), wf(10), vn(2);
  os << sibench::get_banner(lib) << ' ' << std::setw(wn * vn + wf * vn)
     << std::setfill('-') << ' ' << std::endl;
  os << std::left << std::setfill(' ') << std::setw(wn * vn) << "capacity"
     << std::setw(wf) << "load" << std::setw(wf) << "query" << std::endl;
  return os;
}

inline std::ostream &print_insert_result_header(std::ostream &os,
                                                std::string const &lib) {
  std::streamsize const wn(5), wf(10), vn(2);
  os << sibench::get_banner(lib) << ' ' << std::setw(wn * vn + wf * vn)
     << std::setfill('-') << ' ' << std::endl;
  os << std::left << std::setfill(' ') << std::setw(wn * vn) << "capacity"
     << std::setw(wn * vn + 1) << "insertions" << std::setw(wf) << "avg"
     << std::setw(wf) << "total" << std::endl;
  return os;
}

inline std::ostream &print_query_count(std::ostream &os, std::string const &lib,
                                       size_t i) {
  os << sibench::get_banner(lib) << " stats: found=" << i << std::endl;
  return os;
}

inline std::ostream &print_insert_count(std::ostream &os,
                                        std::string const &lib, size_t i,
                                        size_t t) {
  os << sibench::get_banner(lib) << " stats: insertions=" << i << " total=" << t
     << std::endl;
  return os;
}

template <typename Container, typename Operation>
inline result_info benchmark(char const *step, std::size_t iterations,
                             Container const &objects, Operation op) {
  result_info r(step);
  r.iterations = iterations;
  {
    util::high_resolution_timer t;
    op(objects, iterations);
    r.set_mark(t);
  }
  return r;
}

} // namespace sibench

#endif
