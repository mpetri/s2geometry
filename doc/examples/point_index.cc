// Copyright 2017 Google Inc. All Rights Reserved.
// Author: ericv@google.com (Eric Veach)
//
// This example shows how to build and query an in-memory index of points
// using S2PointIndex.

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include "s2/base/commandlineflags.h"
#include "s2/s2earth.h"
#include "s2/s1chord_angle.h"
#include "s2/s2closest_point_query.h"
#include "s2/s2point_index.h"
#include "s2/s2point_index_static.h"
#include "s2/s2testing.h"
#include "s2/util/compressed_maps/compressed_maps.h"

DEFINE_int32(num_index_points, 10000, "Number of points to index");
DEFINE_int32(num_queries, 10000, "Number of queries");
DEFINE_double(query_radius_km, 100, "Query radius in kilometers");

int main(int argc, char **argv) {
  // Build an index containing random points anywhere on the Earth.
  S2PointIndex<int> index;
  for (int i = 0; i < FLAGS_num_index_points; ++i) {
    index.Add(S2Testing::RandomPoint(), i);
  }

  using static_index_type = S2PointIndexStaticEF<int>;

  static_index_type index_static;
  {
    static_index_type::builder index_static_builder;
    for (int i = 0; i < FLAGS_num_index_points; ++i) {
      index_static_builder.Add(S2Testing::RandomPoint(), i);
    }
    index_static_builder.build(index_static);
  }

  std::cout << "index bytes = " << index.bytes_used() << std::endl;
  std::cout << "index_static bytes = " << index_static.bytes_used() << std::endl;

  // Create a query to search within the given radius of a target point.
  S2ClosestPointQuery<int> query(&index);
  query.mutable_options()->set_max_distance(
      S1Angle::Radians(S2Earth::KmToRadians(FLAGS_query_radius_km)));

  S2ClosestPointQuery<int,static_index_type> query2(&index_static);
  query2.mutable_options()->set_max_distance(
      S1Angle::Radians(S2Earth::KmToRadians(FLAGS_query_radius_km)));


  // Repeatedly choose a random target point, and count how many index points
  // are within the given radius of that point.
  int64_t num_found = 0;
  for (int i = 0; i < FLAGS_num_queries; ++i) {
    S2ClosestPointQuery<int>::PointTarget target(S2Testing::RandomPoint());
    num_found += query.FindClosestPoints(&target).size();
  }
  int64_t num_found2 = 0;
  for (int i = 0; i < FLAGS_num_queries; ++i) {
    S2ClosestPointQuery<int>::PointTarget target(S2Testing::RandomPoint());
    num_found2 += query2.FindClosestPoints(&target).size();
  }

  std::printf("Found %" PRId64 " points in %d queries\n",
              num_found, FLAGS_num_queries);

  std::printf("Found2 %" PRId64 " points in %d queries\n",
              num_found2, FLAGS_num_queries);
  return  0;
}
