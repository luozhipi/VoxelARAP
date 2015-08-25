#include "embree_intersector.h"

using namespace embree;

EmbreeIntersector::EmbreeIntersector(const PointMatrixType & V, const FaceMatrixType & F)
{
  rtcInit();
  rtcStartThreads();

  size_t numVertices = V.rows();
  size_t numTriangles = F.rows();
  _accel = rtcNewTriangleMesh (numTriangles, numVertices, "default");
  RTCTriangle* triangles = (RTCTriangle*) rtcMapTriangleBuffer(_accel);
  RTCVertex*   vertices  = (RTCVertex*)   rtcMapPositionBuffer(_accel);
  
  for(int i = 0; i < (int)F.rows(); ++i)
    triangles[i] = RTCTriangle((int)F(i,0),(int)F(i,1),(int)F(i,2),i);

  for(int i = 0; i < (int)V.rows(); ++i) {
    vertices[i] = RTCVertex((float)V(i,0),(float)V(i,1),(float)V(i,2));
  }
  
  rtcUnmapTriangleBuffer(_accel);
  rtcUnmapPositionBuffer(_accel);

  rtcBuildAccel(_accel, "default");
  _intersector = rtcQueryIntersector1(_accel, "default");
}

EmbreeIntersector::~EmbreeIntersector()
{
	rtcFreeMemory();
}

bool EmbreeIntersector::intersectRay(Ray &ray) const
{
	_intersector->intersect(ray);
	return ray.id0 != -1;
}