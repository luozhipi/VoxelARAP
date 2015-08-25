#include "NodeHandles.h"
#include "BoxGrid.h"

	NodeHandles::NodeHandles(const BoxGrid & m_vox) {
		m_numHandles = m_vox.getNumNodes();
		m_restposes.setZero(m_numHandles,3);
		for(unsigned int idNode = 0 ; idNode < m_numHandles ; idNode++) 
		{
			RowVector3 pos = m_vox.getRestPose(idNode);
			m_restposes.row(idNode) = pos;
		}
		reset();
	}