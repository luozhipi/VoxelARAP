/*
*
*@author Zhiping Luo
*@date 28/05/14
*recipes£º
* http://www.alglib.net/interpolation/introductiontorbfs.php
*Presence of polynomial term help to reproduce global behavior of the function
*
*Linear systems with large dense matrices are hard to solve
*
*Non-compact basis function, which is non-zero at any point.its main drawback, very 
*often makes this basis unsuitable for medium and large scale problems
*
*matrix is symmetric (or Hermitian, if A is complex) positive definite(all eigenvalues>0), could use something like Cholesky decomposition(which is a special LU)
*
*Cholesky decomposition is roughly twice as efficient as the LU decomposition 
*for solving systems of linear equations
*
*LU mostly is more stable than SVD
*/

#include "RBFDeformer.h"
#include "BoxGrid.h"
#include "Mesh.h"
#include "timer.h"


RBFDeformer::RBFDeformer(const BoxGrid & m_voxels)
{
	vector<RowVector3> samples;
	for(int i=0;i< m_voxels.getNumBoxes();i++)
	{
		if(m_voxels.isHull(i))
		{
			RowVector3 p = m_voxels.getRestBoxPosition(i);
			samples.push_back(p);
		}
	}
	rbf_fit(samples);
}
//G is symmetric but positive definite
void RBFDeformer::rbf_fit(vector<RowVector3> samples)
{
	M = samples.size();
	P = samples;
	MatrixXX G;
	G.setConstant(M+4,M+4,0);
	ScalarType g_mm;
	#pragma omp parallel for
	for (int i = 0; i < M-1; i++)
	{
		for (int j = i+1; j < M; j++)
		{
			RowVector3 diff = samples[i] - samples[j];
			g_mm = g(diff.squaredNorm());
			G(i,j) = g_mm;
			G(j,i) = g_mm;
		}
	}
	RowVector3 sample;
	#pragma omp parallel for
	for (int i = 0; i < M; i++)
	{
		sample = samples[i];
		G( i, M ) = 1;
		G( i, M+1 ) = sample[0];
		G( i, M+2 ) = sample[1];
		G( i, M+3 ) = sample[2];
		G( M, i ) = 1;
		G( M+1, i ) = sample[0];
		G( M+2, i ) = sample[1];
		G( M+3, i ) = sample[2];
	}
	lu = Eigen::FullPivLU< MatrixXX>(G);
}
ScalarType RBFDeformer::eval(const RowVector3 p)
{
	ScalarType sum = 0.0f;
	RowVector3 diff;
	#pragma omp parallel for
	for (int i = 0; i < M; i++)
	{
		igl::Timer timer = igl::Timer();
		timer.start();
		diff = p - P[i];
		float gg = g(diff.squaredNorm());
		timer.stop();
		time += timer.getElapsedTimeInSec();
		sum += A[i] * gg;
	}
	sum += A[M] + A[M+1]*p[0] + A[M+2]*p[1] + A[M+3]*p[2];
	return sum;
}

ScalarType RBFDeformer::g(const ScalarType d_squared) const
{	
	return pow(sqrt(d_squared),3);
}

void RBFDeformer::updateFunc(vector<ScalarType> f)
{
	VectorX F;
	F.setConstant(M+4,1,0);
	#pragma omp parallel for
	for (int i = 0; i < M; i++)
		F[i] = f[i];
	A = lu.solve(F);
}

void RBFDeformer::deform(BoxGrid & m_voxels, Mesh & m_mesh)
{
	time = 0;
	m_voxels.computeBoxPositions();
    vector<ScalarType> Fx;
    vector<ScalarType> Fy;
    vector<ScalarType> Fz;
	for(int i=0;i< m_voxels.getNumBoxes();i++)
	{
		if(m_voxels.isHull(i))
		{
			RowVector3 p = m_voxels.getBoxPosition(i);
			RowVector3 f = p - m_voxels.getRestBoxPosition(i);
			Fx.push_back(f[0]);
			Fy.push_back(f[1]);
			Fz.push_back(f[2]);
		}
	}
	vector<RowVector3> poses;
	updateFunc(Fx);
	for(int i = 0; i<m_mesh.nbV;i++)
	{
		const RowVector3 & p = m_mesh.getRestPose(i);
		ScalarType px = eval(p);
		poses.push_back(RowVector3(px, 0,0));
	}
	updateFunc(Fy);
	for(int i = 0; i<m_mesh.nbV;i++)
	{
		const RowVector3 & p = m_mesh.getRestPose(i);
		ScalarType py = eval(p);
		poses[i][1] = py;
	}
	updateFunc(Fz);
	for(int i = 0; i<m_mesh.nbV;i++)
	{
		const RowVector3 & p = m_mesh.getRestPose(i);
		ScalarType pz = eval(p);
		poses[i][2] = pz;
	}
	for(int i=0;i<m_mesh.nbV;i++)
	{
		const RowVector3 & p = m_mesh.getRestPose(i);
		m_mesh.updateGeometry(i,poses[i]+p);
	}
	m_mesh.ComputeNormals();
}