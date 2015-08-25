#ifndef __BOXDEFORMABLE_H
#define __BOXDEFORMABLE_H

#include "MIS_inc.h"

class BoxDeformable{

public:
	RowVector3 m_restPose;
	map<IndexType, ScalarType> m_coords; 
	ScalarType m_sumCoords;
	RowVector3 m_currentPose;

	BoxDeformable(const RowVector3 & restPose) {
		m_restPose = restPose;
		m_currentPose = m_restPose;
		m_sumCoords = 0;
	}
	~BoxDeformable() {
		m_coords.clear();
	}

	void pushWeight(IndexType idNode, ScalarType w) {
		m_coords[idNode] = w;
		m_sumCoords += w;
	}
	ScalarType getCoord(IndexType idNode){ return m_coords[idNode]; }
	ScalarType getSumCoords() const { return m_sumCoords; }

	void normalizeWeights() {
		map<IndexType, ScalarType>::iterator it;
		for(it = m_coords.begin();it!=m_coords.end();it++) {
			it->second /= m_sumCoords;
		}
		m_sumCoords = 1.0;
	}

	void computeCurrentPose(const BoxGrid & m_voxels, const NodeHandles & m_nodehandles, int mode);
	void computeCurrentPose(const RowVector3 & pose);

	const RowVector3 & getRestPose() const { return m_restPose; }
	const RowVector3 & getCurrentPose() const { return m_currentPose; }

};



#endif