#ifndef __MIS_COMMON_H
#define __MIS_COMMON_H

#include "MIS_inc.h"
#include "embree_intersector.h"
namespace MIS {

	extern Config * m_config;
	extern Mesh * m_mesh;
	extern BoxGrid * m_voxels;
	extern Handles * m_handles;
	extern Support * m_support;
	extern NodeHandles * m_nodehandles;
	extern ARAPDeformer * m_ARAP;
	extern EmbreeIntersector * m_embree;
	extern RBFDeformer * m_rbf;
	extern int embed_mode;
	extern bool wire;
	extern float time_voxel;
	extern float time_skin;
	extern float volume;
	extern float loss;
	bool isConfigLoaded();
	bool isReady();
	void initialize();
	void init_ARAP();
	void update_ARAP();
	void init_RBF();
	bool isVoxelized();
	void voxelize();
	void updateVoxelGrid();
	void updateSurface();
	void uninitialize();
};
#endif