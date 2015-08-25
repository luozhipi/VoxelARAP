#include "NodeDeformable.h"
#include "Handles.h"

void NodeDeformable::computeCurrentPose(const RowVector3 & pose)
{
	m_currentPose = pose;
}