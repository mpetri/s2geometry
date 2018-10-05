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


#ifndef S2_S2POINT_INDEX_STATIC_H_
#define S2_S2POINT_INDEX_STATIC_H_

#include "s2/s2cell_id.h"
#include <tuple>
#include <type_traits>
#include "s2/util/compressed_maps/compressed_maps.h"

template <class Data, template<typename,typename> class MapType>
class S2PointIndexStatic {
public:
    // PointData is essentially std::pair with named fields.  It stores an
    // S2Point and its associated data, taking advantage of the "empty base
    // optimization" to ensure that no extra space is used when Data is empty.
    class PointData {
    public:
        PointData() {} // Needed by STL
        PointData(const S2Point& point, const Data& data)
            : tuple_(point, data)
        {
        }

        const S2Point& point() const { return std::get<0>(tuple_); }
        const Data& data() const { return std::get<1>(tuple_); }

        friend bool operator==(const PointData& x, const PointData& y)
        {
            return x.tuple_ == y.tuple_;
        }
        friend bool operator<(const PointData& x, const PointData& y)
        {
            return x.tuple_ < y.tuple_;
        }

    private:
        // Note that std::tuple has special template magic to ensure that Data
        // doesn't take up any space when it is empty.  (This is not true if you
        // simply declare a member of type Data.)
        std::tuple<S2Point, Data> tuple_;
    };

    // Default constructor.
    S2PointIndexStatic() {};

    // Returns the number of points in the index.
    int num_points() const
    {
        m_map.size();
    }

    size_t bytes_used() const {
        return m_map.bytes_used();
    }

private:
    // Defined here because the Iterator class below uses it.
    using Map = MapType<S2CellId, PointData>;
    Map m_map;
public:
    class builder {
    public:
        void build(S2PointIndexStatic& static_index)
        {
            Map(map_).swap(static_index.m_map);
        }

        void Add(const PointData& point_data)
        {
            S2CellId id(point_data.point());
            map_.emplace(std::make_pair(id, point_data));
        }

        void Add(const S2Point& point, const Data& data)
        {
            Add(PointData(point, data));
        }

        void Add(const S2Point& point)
        {
            static_assert(std::is_empty<Data>::value, "Data must be empty");
            Add(point, {});
        }

    private:
        std::map<S2CellId, PointData> map_;
    };

    class Iterator {
    public:
        Iterator();

        // Convenience constructor that calls Init().
        explicit Iterator(const S2PointIndexStatic* index) {
            Init(index);
        }

        // Initializes an iterator for the given S2PointIndex.  If the index is
        // non-empty, the iterator is positioned at the first cell.
        //
        // This method may be called multiple times, e.g. to make an iterator
        // valid again after the index is modified.
        void Init(const S2PointIndexStatic* index) {
            map_ = &index->m_map;
            iter_ = map_->begin();
            end_ = map_->end();
        }

        // The S2CellId for the current index entry.
        // REQUIRES: !done()
        S2CellId id() const {
            S2_DCHECK(!done());
            return iter_->first;
        }

        // The point associated with the current index entry.
        // REQUIRES: !done()
        const S2Point& point() const {
            S2_DCHECK(!done());
            return iter_->second.point();
        }

        // The client-supplied data associated with the current index entry.
        // REQUIRES: !done()
        const Data& data() const {
            S2_DCHECK(!done());
            return iter_->second.data();
        }

        // The (S2Point, data) pair associated with the current index entry.
        const PointData& point_data() const {
            S2_DCHECK(!done());
            return iter_->second;
        }

        // Returns true if the iterator is positioned past the last index entry.
        bool done() const {
            return iter_ == end_;
        }

        // Positions the iterator at the first index entry (if any).
        void Begin() {
            iter_ = map_->begin();
        }

        // Positions the iterator so that done() is true.
        void Finish() {
            iter_ = end_;
        }

        // Advances the iterator to the next index entry.
        // REQUIRES: !done()
        void Next() {
            S2_DCHECK(!done());
            ++iter_;
        }

        // If the iterator is already positioned at the beginning, returns false.
        // Otherwise positions the iterator at the previous entry and returns true.
        bool Prev() {
            if (iter_ == map_->begin())
                return false;
            --iter_;
            return true;
        }

        // Positions the iterator at the first entry with id() >= target, or at the
        // end of the index if no such entry exists.
        void Seek(S2CellId target) {
            iter_ = map_->lower_bound(target);
        }

    private:
        const Map* map_ = nullptr;
        typename Map::const_iterator iter_, end_;
    };

private:
    friend class Iterator;

    S2PointIndexStatic(const S2PointIndexStatic&) = delete;
    void operator=(const S2PointIndexStatic&) = delete;
};


#endif // S2_S2POINT_INDEX_STATIC_H_
