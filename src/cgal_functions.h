#ifndef __CGAL_FUNCTIONS_H
#define __CGAL_FUNCTIONS_H

#include <boost/shared_ptr.hpp>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/create_offset_polygons_2.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K ;

typedef K::Point_2					Point;
typedef K::Segment_2				Segment;
typedef CGAL::Polygon_2<K>			Polygon_2;
typedef CGAL::Straight_skeleton_2<K> Skeleton;

typedef boost::shared_ptr<Skeleton> SkeletonPtr;

#endif