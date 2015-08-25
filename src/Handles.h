#ifndef __HANDLES_H
#define __HANDLES_H

#include "MIS_inc.h"

class Handles {

private:	
	vector<IndexType> constraints;
	vector<int> containingBox;
	int m_numHandles;
	RowMatrixX3 m_restposes;
	RowMatrixX3 m_translations;

public:
	Handles(const Config & cfg,const BoxGrid & m_vox);
	~Handles() {}
	void update(const Config & cfg,const BoxGrid & m_vox);
	void reset() {
		m_translations = m_restposes;
	}
	RowVector3 getRestPose(int j) const{return m_restposes.row(j);}
	int getNumHandles() const { return m_numHandles; }
	IndexType getConstraint(int j) const{return constraints[j];}
	IndexType getConstainBox(int j) const{return containingBox[j];}
	void setT(int j, const RowVector3 & t) {
		m_translations.row(j) = t;
	}
	RowVector3 getHandlePose (int j) const
	{
		return m_translations.row(j); 
	}
};

#endif