#ifndef __EMBREE_INTERSECTOR_H
#define __EMBREE_INTERSECTOR_H

#include "EIGEN_inc.h"
#include "embree\include\embree.h"
#include "embree\common\ray.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define strcasecmp lstrcmpiA
#pragma warning(disable:4297) // function assumed not to throw an exception but does
#endif

class EmbreeIntersector
{
public:
  EmbreeIntersector(const PointMatrixType & V, const FaceMatrixType & F);
  virtual ~EmbreeIntersector();
  
  bool intersectRay(embree::Ray &ray) const;
  
private:
  embree::RTCGeometry* _accel;
  embree::RTCIntersector1* _intersector;
};

inline embree::Vec3f toVec3f(const Vector3 &p) { return embree::Vec3f((float)p[0], (float)p[1], (float)p[2]); }

#endif //EMBREE_INTERSECTOR_H