#ifndef __NODEDEFORMABLE_H
#define __NODEDEFORMABLE_H

#include "MIS_inc.h"

class NodeDeformable {

public:
	RowVector3 m_restPose;
	RowVector3 m_currentPose;

	NodeDeformable(const RowVector3 & restPose) {
		m_restPose = restPose;
		m_currentPose = m_restPose;
	}
	~NodeDeformable() {
	}
	void computeCurrentPose(const RowVector3 & pose);

	const RowVector3 & getRestPose() const { return m_restPose; }
	const RowVector3 & getCurrentPose() const { return m_currentPose; }

};



#endif