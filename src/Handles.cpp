//
#include "Handles.h"
#include "config.h"
#include "BoxGrid.h"
#include "Mesh.h"
#include "rendering_functions.h"

	Handles::Handles(const Config & cfg,const BoxGrid & m_vox) {
		m_numHandles = cfg.getNbHandles();
		m_restposes.setZero(m_numHandles,3);
		constraints.clear();
		containingBox.clear();
		for(unsigned int i = 0 ; i < cfg.getNbHandles() ; ++i) 
		{
			int idNode = m_vox.getNodeClosestToPoint(cfg.getHandle(i));
			RowVector3 t;
			int idBox = m_vox.getBoxContainingPoint(cfg.getHandle(i),t);
			RowVector3 pos = m_vox.getRestPose(idNode);
			m_restposes.row(i) = pos;
			constraints.push_back(idNode);
			containingBox.push_back(idBox);
		}
		reset();
	}

	void Handles::update(const Config & cfg,const BoxGrid & m_vox) {
		m_numHandles = cfg.getNbHandles();
		m_restposes.setZero(m_numHandles,3);
		constraints.clear();
		containingBox.clear();
		for(unsigned int i = 0 ; i < cfg.getNbHandles() ; ++i) 
		{
			int idNode = m_vox.getNodeClosestToPoint(cfg.getHandle(i));
			RowVector3 t;
			int idBox = m_vox.getBoxContainingPoint(cfg.getHandle(i),t);
			RowVector3 pos = m_vox.getRestPose(idNode);
			m_restposes.row(i) = pos;
			constraints.push_back(idNode);
			containingBox.push_back(idBox);
		}
		reset();
	}