#ifndef __MIS_INCLUDE_H
#define __MIS_INCLUDE_H

#include "STL_inc.h"
#include "EIGEN_inc.h"
#include "GL_inc.h"
#include "timer.h"

#include <omp.h>


class Mesh;
class BoxGrid;
class BoxDeformable;
class Handles;
class NodeHandles;
class Config;
class Support;
class ARAPDeformer;
class RBFDeformer;
class NodeDeformable;

#undef min
#undef max
#include "utils_functions.h"
const static IndexType INDEX_NULL = 999999;
#define DEFAULT_FLATTEN_THRESHOLD 0.02
#define DEFAULT_ANGLE_OBJ 0.0

#define MESH_DIR "mesh/"
#define PRINT_DIR "print/"
#define TEXTURE_DIR "textures/"
#define MESH_EXT_LENGTH 4
#define OBJ_EXT ".obj"
#define VOX_EXT ".vox"
#define CONFIG_EXT ".mis"

inline void removeExt(string & s) {
	s.erase(s.cend()-MESH_EXT_LENGTH,s.cend());
}


inline vector<RowVector3> generateColor()
{
	vector<RowVector3> colors;
	colors.push_back(RowVector3(0,0,1.0));
   	colors.push_back(RowVector3(1.0,0,0));
   	colors.push_back(RowVector3(0,1.0,0));
   	colors.push_back(RowVector3(0,0,0.1724));
   	colors.push_back(RowVector3(1.0,0.1034,0.7241));
   	colors.push_back(RowVector3(1.0,0.8276,0));
	colors.push_back(RowVector3(0,0.3448,0));
   	colors.push_back(RowVector3(0.5172,0.5172,1.0));
   	colors.push_back(RowVector3(0.6207,0.3103,0.2759));
 	colors.push_back(RowVector3(0,1.0,0.7586));
  	colors.push_back(RowVector3(0,0.5172,0.5862));
  	colors.push_back(RowVector3(0,0,0.4828));
  	colors.push_back(RowVector3(0.5862,0.8276,0.3103));
  	colors.push_back(RowVector3(0.9655,0.6207,0.8621));
  	colors.push_back(RowVector3(0.8276,0.0690,1.0000));
  	colors.push_back(RowVector3(0.4828,0.1034,0.4138));
  	colors.push_back(RowVector3(0.9655,0.0690,0.3793));
  	colors.push_back(RowVector3(1.0000,0.7586,0.5172));
  	colors.push_back(RowVector3(0.1379,0.1379,0.0345));
  	colors.push_back(RowVector3(0.5517,0.6552,0.4828));
  	colors.push_back(RowVector3(0.9655,0.5172,0.0345));
  	colors.push_back(RowVector3(0.5172,0.4483,0));
 	colors.push_back(RowVector3(0.4483,0.9655,1.0));
  	colors.push_back(RowVector3(0.6207,0.7586,1.0));
  	colors.push_back(RowVector3(0.4483,0.3793,0.4828));
  	colors.push_back(RowVector3(0.6207,0,0));
 	colors.push_back(RowVector3(0,0.3103,1.0));
  	colors.push_back(RowVector3(0,0.2759,0.5862));
 	colors.push_back(RowVector3(0.8276,1.0,0));
 	colors.push_back(RowVector3(0.7241,0.3103,0.8276));
	return colors;
}
#endif