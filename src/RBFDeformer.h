#ifndef _RBFDEFORMER_H
#define _RBFDEFORMER_H

#include "MIS_inc.h"

class RBFDeformer{
public:
	RBFDeformer(const BoxGrid & m_voxels);
	~RBFDeformer(){};
	void rbf_fit(vector<RowVector3> samples);
	void updateFunc(vector<ScalarType> f);
	void deform(BoxGrid & m_voxels, Mesh & m_mesh);
	ScalarType eval(const RowVector3 p);
	float time;
private:
	ScalarType g(const ScalarType d_squared) const;
	VectorX A;
	vector<RowVector3> P;
	Eigen::FullPivLU< MatrixXX> lu;
	unsigned int M;
};

#endif