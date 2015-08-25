#ifndef __NODEHANDLES_H
#define __NODEHANDLES_H

#include "MIS_inc.h"

class NodeHandles {

private:	

	int m_numHandles;
	RowMatrixX3 m_restposes;
	RowMatrixX3 m_translations;
	RowMatrixX4 m_rotations;

public:
	NodeHandles(const BoxGrid & m_vox);
	~NodeHandles() {}

	void reset() {
		m_translations = m_restposes;
		m_rotations.setZero(m_numHandles,4);
		for(int i=0;i<m_numHandles;i++)
		{
			m_rotations.row(i) = RowVector4(1.0,0,0,0);
		}
	}
	RowVector3 getRestPose(int j) const{return m_restposes.row(j);}
	int getNumHandles() const { return m_numHandles; }
	
	void getTransformedLBS(int j, const RowVector3 & v0, RowVector3 & v) const {
		RowVector4 tmp = m_rotations.row(j);
		Quaternion r(tmp[0], tmp[1], tmp[2], tmp[3]);
		double R[16];
		QuaternionToMatrix44(r,R);
		RowVector3 P = v0-m_restposes.row(j);
		RowVector3 PT(0.0f,0.0f,0.0f);
		PT[0] = R[0]*P[0] + R[1]*P[1] + R[2]*P[2];
		PT[1] = R[4]*P[0] + R[5]*P[1] + R[6]*P[2];
		PT[2] = R[8]*P[0] + R[9]*P[1] + R[10]*P[2];
		v = PT + m_translations.row(j);
	}
	void getFullTransform(int j, RowVector3 & t, Quaternion & r, ScalarType & s) const{
		RowVector4 tmp = m_rotations.row(j);
		r.w() = tmp[0];
		r.x() = tmp[1];
		r.y() = tmp[2];
		r.z() = tmp[3];
	    t = m_translations.row(j);
	}
	void setT(int j, const RowVector3 & t) {
		m_translations.row(j) = t;
	}
	RowVector3 getPose (int j) const
	{
		return m_translations.row(j); 
	}
	void setR(int j, const Quaternion & r){
		m_rotations.row(j) = RowVector4(r.w(), r.x(), r.y(), r.z());
	}
};

#endif