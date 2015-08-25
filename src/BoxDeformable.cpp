#include "BoxDeformable.h"
#include "BoxGrid.h"
#include "NodeHandles.h"

void BoxDeformable::computeCurrentPose(const BoxGrid & m_voxels, const NodeHandles & m_nodehandles, int mode)
{
	if(mode == 0)
	{
		m_currentPose = m_restPose;
		map<IndexType, ScalarType>::iterator it;
		for(it = m_coords.begin();it!=m_coords.end();it++) 
		{
			int idNode = it->first;
			RowVector3 d = m_voxels.getCurrentPose(idNode) - m_voxels.getRestPose(idNode);
			m_currentPose += it->second*d;
		}
	}
	else if(mode == 1)
	{
		m_currentPose = RowVector3::Zero();
	    RowVector3 Tjv;
		map<IndexType, ScalarType>::iterator it;
		for(it = m_coords.begin();it!=m_coords.end();it++) 
		{
			int idNode = it->first;
			m_nodehandles.getTransformedLBS(idNode,m_restPose,Tjv);
			m_currentPose += it->second * Tjv;
		}
	}
}

void BoxDeformable::computeCurrentPose(const RowVector3 & pose)
{
	m_currentPose = pose;
}