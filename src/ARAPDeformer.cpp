/*
*@Author Zhiping Luo 09/10/2014
*ARAP on voxelgrid
*Eigen solver of the variational non-linear ARAP optimization problem
*fast SVD of rotation part
*/
#include "BoxGrid.h"
#include "ARAPDeformer.h"

ARAPDeformer::ARAPDeformer(const BoxGrid & m_voxel)
{
	nbN = m_voxel.getNumNodes();
	oriVoxel.resize(nbN);
	isAnchorPoint.resize(nbN);
	isControlPoint.resize(nbN);
	ClearAll();
}

void ARAPDeformer::BuildAndFactor(const BoxGrid & m_voxel)
{
	b.setConstant(nbN,3,0.);
	xyz.setConstant(nbN, 3, 0.);
	R.clear();
	R.assign(nbN, Matrix33::Identity());
	// L matrix, n by n, uniform weights
	Eigen::SparseMatrix<double> A(nbN, nbN);
	for (int i=0;i<nbN;i++)
	{
		ScalarType weight=0;
		if(!(isAnchorPoint[i] || isControlPoint[i]))
		{
			for(int nn=0;nn<6;nn++)
			{
				int j = m_voxel.getNodeNodes(i,nn);
				if(j!=-1)
				{
					weight += 1.0;
					A.coeffRef(i,j) = -1.0;
				}
			} 
		}
		else
			weight = 1.0f;
		A.coeffRef(i,i) = weight;
	}
	At = A.transpose();
	solver.compute(At * A);
	isSolverReady = true;
}

void ARAPDeformer::reset()
{
	isSolverReady = false;
}
void ARAPDeformer::SVDRotation(const BoxGrid & m_voxel)
{
	Matrix33 eye = Matrix33::Identity();
	for (int i=0; i<nbN;i++)
	{
		int valence = m_voxel.getNodeValence(i);
		RowVector3 pi = oriVoxel[i];
		RowVector3  pii = xyz.row(i);
		Matrix3X P, Q;
		P.setConstant(3, valence, 0.);
		Q.setConstant(3, valence, 0.);
		int index = 0;
		for(int nv=0; nv<6;nv++)
		{
			IndexType j = m_voxel.getNodeNodes(i,nv);
			if(j!=-1)
			{
				RowVector3  pj = oriVoxel[j];
				RowVector3  pjj = xyz.row(j);
				P.col(index) = (pi - pj) * 1.0;
				Q.col(index) = pii-pjj;
				index++;
			}
		}

		// Compute the 3 by 3 covariance matrix:
		// actually S = (P * W * Q.t()); W is already considerred in the previous step (P=P*W)
		MatrixXX S = (P * Q.transpose());
		// Compute the singular value decomposition S = UDV.t
		Eigen::JacobiSVD<MatrixXX> svd(S, Eigen::ComputeThinU | Eigen::ComputeThinV); // X = U * D * V.t()
		MatrixXX V = svd.matrixV();
		MatrixXX Ut = svd.matrixU().transpose();
		eye(2,2) = (V * Ut).determinant();	// remember: Eigen starts from zero index
		// determinant of R == 1 (sin^2+cos^2) if no change volume, area...
		// V*U.t may be reflection (determinant = -1). in this case, we need to change the sign of 
		// column of U corresponding to the smallest singular value (3rd column)
		R[i] = (V * eye * Ut); //Ri = (V * eye * U.t());
	}
}

void ARAPDeformer::Deform(BoxGrid &m_voxel, int ARAPIteration /*= 1*/ )
{
	// ARAP iteration
	for(int iter = 0; iter < ARAPIteration; iter++)
	{	
		// update vector b3 = wij/2 * (Ri+Rj) * (pi - pj), where pi and pj are coordinates of the original voxel
		if(iter > 0)
			SVDRotation(m_voxel);
		for (int i=0; i<nbN;i++)
		{
			RowVector3 p = m_voxel.getCurrentPose(i);
			if(!(isAnchorPoint[i] || isControlPoint[i]))
			{
				p = RowVector3::Zero();
				for(int j=0;j<6;j++)
				{ 
					IndexType ii = m_voxel.getNodeNodes(i,j);
					if(ii!=-1)
					{
						RowVector3 pij = oriVoxel[i] - oriVoxel[ii];
						Vector3 pijv(pij[0],pij[1],pij[2]);
						Vector3 RijPij = (R[i] + R[ii])*pijv;
						RowVector3 RijPijMat(RijPij[0],RijPij[1],RijPij[2]);
						p += RijPijMat * (1.0 / 2.0);
					}
				}
			}
			b.row(i) = p;
		}
		// SOLVE for x, y, and z
		for(int k = 0; k < 3; k++)
			xyz.col(k) = solver.solve(At * b.col(k));
	}
	for(int i=0;i<nbN;i++)
	{
		m_voxel.updateVoxel(i,xyz.row(i));
	}
}
