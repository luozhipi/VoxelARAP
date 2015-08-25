#pragma once

#include "MIS_inc.h"

class ARAPDeformer{

public:
	ARAPDeformer(const BoxGrid & m_voxel);
	~ARAPDeformer(){}
	void BuildAndFactor(const BoxGrid & m_voxel);
	void SVDRotation(const BoxGrid & m_voxel);
	void setOriPose(const BoxGrid & m_voxel)
	{
		for(IndexType i = 0; i<nbN;i++){
			oriVoxel[i] = m_voxel.getRestPose(i);
		}
	}
	void setXYZ(const BoxGrid & m_voxel){
		for(IndexType i = 0; i<nbN;i++){
			RowVector3 p = m_voxel.getCurrentPose(i);
			xyz(i, 0) = p[0];xyz(i, 1) = p[1];xyz(i, 2) = p[2];
		}
	}
	void Deform(BoxGrid & m_voxel, int ARAPIteration = 1);
	void reset();

private:
	int nbN;

	vector<Matrix33> R;
	RowMatrixX3 xyz;
	RowMatrixX3 b;
	Eigen::SparseMatrix<double> At;
	//A direct sparse LLT Cholesky factorizations
	Eigen::SimplicialLLT< Eigen::SparseMatrix<double> > solver;
	bool isSolverReady;
	vector<bool> isAnchorPoint;
	vector<bool> isControlPoint;
	vector<RowVector3> oriVoxel;

public:

	void SetAnchor( const int & nid ){ isAnchorPoint[nid] = true; isSolverReady = false;}
	bool isAnchor(const int & nid){return isAnchorPoint[nid];}
	void SetControl( const int & nid){
		isControlPoint[nid] = true;
		isSolverReady = false;
	}
	bool isControl(const int & nid){return isControlPoint[nid];}

	void ClearAnchors(){
		for (int i = 0; i< nbN; i++)
			isAnchorPoint[i] = false;
		isSolverReady = false;
	}

	void ClearControl(){
		for (int i = 0; i< nbN; i++)
			isControlPoint[i] = false;
		isSolverReady = false;
	}

	void ClearAll(){
		ClearAnchors();
		ClearControl();
	}
};
