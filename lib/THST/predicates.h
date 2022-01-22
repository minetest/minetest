//
//  predicates.h
//
//

#pragma once

#include "bbox.h"

namespace spatial {

namespace detail {
struct intersects_tag;
struct contains_tag;
struct within_tag;

template <typename T, int Dimension, typename OperationTag>
struct CheckPredicateHelper;

template <typename T, int Dimension>
struct CheckPredicateHelper<T, Dimension, intersects_tag> {
  typedef spatial::BoundingBox<T, Dimension> box_t;

  inline bool operator()(const box_t &predicateBBox,
                         const box_t &valueBBox) const {
    return predicateBBox.overlaps(valueBBox);
  }
};

template <typename T, int Dimension>
struct CheckPredicateHelper<T, Dimension, contains_tag> {
  typedef spatial::BoundingBox<T, Dimension> box_t;

  inline bool operator()(const box_t &predicateBBox,
                         const box_t &valueBBox) const {
    return predicateBBox.contains(valueBBox);
  }
};

template <typename T, int Dimension>
struct CheckPredicateHelper<T, Dimension, within_tag> {
  typedef spatial::BoundingBox<T, Dimension> box_t;

  inline bool operator()(const box_t &predicateBBox,
                         const box_t &valueBBox) const {
    return predicateBBox.contains(valueBBox.min);
  }
};

template <typename T, int Dimension, typename OperationTag>
bool checkPredicate(const spatial::BoundingBox<T, Dimension> &predicateBBox,
                    const spatial::BoundingBox<T, Dimension> &valueBBox) {
  return CheckPredicateHelper<T, Dimension, OperationTag>()(predicateBBox,
                                                            valueBBox);
}
};

template <typename T, int Dimension, typename OperationTag>
struct SpatialPredicate {
  typedef spatial::BoundingBox<T, Dimension> box_t;
  typedef OperationTag op_t;

  SpatialPredicate(const box_t &bbox) : bbox(bbox) {}
  inline bool operator()(const box_t &valueBBox) const {
    return detail::checkPredicate<T, Dimension, OperationTag>(bbox, valueBBox);
  }

  box_t bbox;
};

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::intersects_tag>
intersects(const spatial::BoundingBox<T, Dimension> &bbox) {
  return SpatialPredicate<T, Dimension, detail::intersects_tag>(bbox);
}
template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::intersects_tag>
intersects(const T min[Dimension], const T max[Dimension]) {
  typedef SpatialPredicate<T, Dimension, detail::intersects_tag> predicate_t;
  return predicate_t(typename predicate_t::box_t(min, max));
}

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::contains_tag>
contains(const spatial::BoundingBox<T, Dimension> &bbox) {
  return SpatialPredicate<T, Dimension, detail::contains_tag>(bbox);
}
template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::contains_tag>
contains(const T min[Dimension], const T max[Dimension]) {
  typedef SpatialPredicate<T, Dimension, detail::contains_tag> predicate_t;
  return predicate_t(typename predicate_t::box_t(min, max));
}

///@note To be used with points, as it only check the min point of the
/// bounding boxes of the leaves.
template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::within_tag>
within(const spatial::BoundingBox<T, Dimension> &bbox) {
  return SpatialPredicate<T, Dimension, detail::within_tag>(bbox);
}
template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::within_tag>
within(const T min[Dimension], const T max[Dimension]) {
  typedef SpatialPredicate<T, Dimension, detail::within_tag> predicate_t;
  return predicate_t(typename predicate_t::box_t(min, max));
}

} // namespace spatial
