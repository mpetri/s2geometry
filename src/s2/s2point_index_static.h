// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Author: ericv@google.com (Eric Veach)

#ifndef S2_S2POINT_INDEX_STATIC_H_
#define S2_S2POINT_INDEX_H_

#include <tuple>
#include <type_traits>
#include "s2/s2cell_id.h"
#include "s2/util/compressed_maps.h"

// S2PointIndex maintains an index of points sorted by leaf S2CellId.  Each
// point can optionally store auxiliary data such as an integer or pointer.
// This can be used to map results back to client data structures.
//
// The class supports adding or removing points dynamically, and provides a
// seekable iterator interface for navigating the index.
//
// You can use this class in conjuction with S2ClosestPointQuery to find the
// closest index points to a given query point.  For example:
//
// void Test(const vector<S2Point>& index_points,
//           const vector<S2Point>& target_points) {
//   // The template argument allows auxiliary data to be attached to each
//   // point (in this case, the array index).
//   S2PointIndex<int> index;
//   for (int i = 0; i < index_points.size(); ++i) {
//     index.Add(index_points[i], i);
//   }
//   S2ClosestPointQuery<int> query(&index);
//   query.mutable_options()->set_max_results(5);
//   for (const S2Point& target_point : target_points) {
//     S2ClosestPointQueryPointTarget target(target_point);
//     for (const auto& result : query.FindClosestPoints(&target)) {
//       // The Result class contains the following methods:
//       //   distance() is the distance to the target.
//       //   point() is the indexed point.
//       //   data() is the auxiliary data.
//       DoSomething(target_point, result);
//     }
//   }
// }
//
// The Data argument defaults to an empty class, which uses no additional
// space beyond the S2Point itself.  In this case the Data argument is
// required.  For example:
//
//   S2PointIndex<> index;
//   index.Add(point);
//
// Points can be added or removed from the index at any time by calling Add()
// or Remove().  However when the index is modified, you must call Init() on
// each iterator before using it again (or simply create a new iterator).
//
//   index.Add(new_point, 123456);
//   it.Init(&index);
//   it.Seek(target.range_min());
//
// You can also access the index directly using the iterator interface.  For
// example, here is how to iterate through all the points in a given S2CellId
// "target_id":
//
//   S2PointIndex<int>::Iterator it(&index);
//   it.Seek(target_id.range_min());
//   for (; !it.done() && it.id() <= target_id.range_max(); it.Next()) {
//     DoSomething(it.id(), it.point(), it.data());
//   }
//
// TODO(ericv): Consider adding an S2PointIndexRegion class, which could be
// used to efficiently compute coverings of a collection of S2Points.
//
// REQUIRES: "Data" has default and copy constructors.
// REQUIRES: "Data" has operator== and operator<.
template <class Data = std::tuple<>,class MapType = ef_map>
class S2PointIndexStatic {
 public:
  // PointData is essentially std::pair with named fields.  It stores an
  // S2Point and its associated data, taking advantage of the "empty base
  // optimization" to ensure that no extra space is used when Data is empty.
  class PointData {
   public:
    PointData() {}  // Needed by STL
    PointData(const S2Point& point, const Data& data) : tuple_(point, data) {}

    const S2Point& point() const { return std::get<0>(tuple_); }
    const Data& data() const { return std::get<1>(tuple_); }

    friend bool operator==(const PointData& x, const PointData& y) {
      return x.tuple_ == y.tuple_;
    }
    friend bool operator<(const PointData& x, const PointData& y) {
      return x.tuple_ < y.tuple_;
    }

   private:
    // Note that std::tuple has special template magic to ensure that Data
    // doesn't take up any space when it is empty.  (This is not true if you
    // simply declare a member of type Data.)
    std::tuple<S2Point, Data> tuple_;
  };

  // Default constructor.
  S2PointIndexStatic();

  // Returns the number of points in the index.
  int num_points() const;

  // Adds the given point to the index.  Invalidates all iterators.
  void Add(const S2Point& point, const Data& data);
  void Add(const PointData& point_data);

  // Convenience function for the case when Data is an empty class.
  void Add(const S2Point& point);

  // Resets the index to its original empty state.  Invalidates all iterators.
  void Clear();

  void Freeze();

 private:
  // Defined here because the Iterator class below uses it.
  using Map = MapType<S2CellId, PointData>;

 public:
  class Iterator {
   public:
    // Default constructor; must be followed by a call to Init().
    Iterator();

    // Convenience constructor that calls Init().
    explicit Iterator(const S2PointIndexStatic* index);

    // Initializes an iterator for the given S2PointIndex.  If the index is
    // non-empty, the iterator is positioned at the first cell.
    //
    // This method may be called multiple times, e.g. to make an iterator
    // valid again after the index is modified.
    void Init(const S2PointIndexStatic* index);

    // The S2CellId for the current index entry.
    // REQUIRES: !done()
    S2CellId id() const;

    // The point associated with the current index entry.
    // REQUIRES: !done()
    const S2Point& point() const;

    // The client-supplied data associated with the current index entry.
    // REQUIRES: !done()
    const Data& data() const;

    // The (S2Point, data) pair associated with the current index entry.
    const PointData& point_data() const;

    // Returns true if the iterator is positioned past the last index entry.
    bool done() const;

    // Positions the iterator at the first index entry (if any).
    void Begin();

    // Positions the iterator so that done() is true.
    void Finish();

    // Advances the iterator to the next index entry.
    // REQUIRES: !done()
    void Next();

    // If the iterator is already positioned at the beginning, returns false.
    // Otherwise positions the iterator at the previous entry and returns true.
    bool Prev();

    // Positions the iterator at the first entry with id() >= target, or at the
    // end of the index if no such entry exists.
    void Seek(S2CellId target);

   private:
    const Map* map_;
    typename Map::const_iterator iter_, end_;
  };

 private:
  friend class Iterator;
  Map map_;

  S2PointIndexStatic(const S2PointIndexStatic&) = delete;
  void operator=(const S2PointIndexStatic&) = delete;
};


//////////////////   Implementation details follow   ////////////////////


template <class Data>
S2PointIndexStatic<Data>::S2PointIndexStatic() {
}

template <class Data>
inline int S2PointIndexStatic<Data>::num_points() const {
  return map_.size();
}

template <class Data>
void S2PointIndexStatic<Data>::Add(const PointData& point_data) {
  S2CellId id(point_data.point());
  map_.insert(std::make_pair(id, point_data));
}

template <class Data>
void S2PointIndexStatic<Data>::Add(const S2Point& point, const Data& data) {
  Add(PointData(point, data));
}

template <class Data>
void S2PointIndexStatic<Data>::Add(const S2Point& point) {
  static_assert(std::is_empty<Data>::value, "Data must be empty");
  Add(point, {});
}

template <class Data>
void S2PointIndexStatic<Data>::Clear() {
  map_.clear();
}

template <class Data>
void S2PointIndexStatic<Data>::Freeze() {
  map_.freeze();
}

template <class Data>
inline S2PointIndexStatic<Data>::Iterator::Iterator() : map_(nullptr) {
}

template <class Data>
inline S2PointIndexStatic<Data>::Iterator::Iterator(
    const S2PointIndexStatic<Data>* index) {
  Init(index);
}

template <class Data>
inline void S2PointIndexStatic<Data>::Iterator::Init(
    const S2PointIndexStatic<Data>* index) {
  map_ = &index->map_;
  iter_ = map_->begin();
  end_ = map_->end();
}

template <class Data>
inline S2CellId S2PointIndexStatic<Data>::Iterator::id() const {
  S2_DCHECK(!done());
  return iter_->first;
}

template <class Data>
inline const S2Point& S2PointIndexStatic<Data>::Iterator::point() const {
  S2_DCHECK(!done());
  return iter_->second.point();
}

template <class Data>
inline const Data& S2PointIndexStatic<Data>::Iterator::data() const {
  S2_DCHECK(!done());
  return iter_->second.data();
}

template <class Data>
inline const typename S2PointIndexStatic<Data>::PointData&
S2PointIndexStatic<Data>::Iterator::point_data() const {
  S2_DCHECK(!done());
  return iter_->second;
}

template <class Data>
inline bool S2PointIndexStatic<Data>::Iterator::done() const {
  return iter_ == end_;
}

template <class Data>
inline void S2PointIndexStatic<Data>::Iterator::Begin() {
  iter_ = map_->begin();
}

template <class Data>
inline void S2PointIndexStatic<Data>::Iterator::Finish() {
  iter_ = end_;
}

template <class Data>
inline void S2PointIndexStatic<Data>::Iterator::Next() {
  S2_DCHECK(!done());
  ++iter_;
}

template <class Data>
inline bool S2PointIndexStatic<Data>::Iterator::Prev() {
  if (iter_ == map_->begin()) return false;
  --iter_;
  return true;
}

template <class Data>
inline void S2PointIndexStatic<Data>::Iterator::Seek(S2CellId target) {
  iter_ = map_->lower_bound(target);
}

#endif  // S2_S2POINT_INDEX_STATIC_H_
