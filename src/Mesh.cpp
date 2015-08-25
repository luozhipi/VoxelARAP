#include "Mesh.h"
#include "config.h"
#include "readMesh.h"
#include "BoxDeformable.h"
#include "BoxGrid.h"
#include "NodeHandles.h"

	Mesh::Mesh(string filename) {
		has_texture = false;
		string mtl_file;
		readMeshfile(filename,vertices,faces,uvfaces,uvs,has_texture,mtl_file);
		if(has_texture)
		{
			readMaterial(tDiffuseTexture,mtl_file);
		}
		nbV = vertices.rows();
		nbF = faces.rows();
		boxDeformInfo.assign(nbV,0);
		InitNeighboringData();
		ComputeBoundingBox();
		for(IndexType i = 0 ; i < nbV ; ++i) {
			boxDeformInfo[i] = new BoxDeformable(vertices.row(i));
		}
		corner_threshold = 64;
		ComputeNormals();
		has_embedweight=false;
	}

	Mesh::~Mesh() {
	}

	void Mesh::InitNeighboringData() {
		vertex_to_faces.clear();
		vertex_to_faces.resize(nbV);
		vertex_to_vertices.clear();
		vertex_to_vertices.resize(nbV);
		for(int fi = 0; fi < nbF; fi++) {
			for(IndexType j = 0 ; j < 3 ; ++j) { 
				vertex_to_faces[F(fi,j)].push_back(fi);
				IndexType v0 = F(fi,j);
				IndexType v1 = F(fi,(j+1)%3);
				IndexType v2 = F(fi,(j+2)%3);
				if(!inTheList(vertex_to_vertices[v0],v1)) vertex_to_vertices[v0].push_back(v1);
				if(!inTheList(vertex_to_vertices[v0],v2)) vertex_to_vertices[v0].push_back(v2);
			}
		}
		face_to_faces.setConstant(nbF,3,INDEX_NULL);
		for(int fi = 0 ; fi < nbF ; ++fi) {
			for(int i = 0 ; i < 3 ; ++i) {
				IndexType v0 = faces(fi,i);
				IndexType v1 = faces(fi,(i+1)%3);
				for(unsigned int j = 0 ; j < vertex_to_faces[v0].size() ; ++j) {
					IndexType fj = vertex_to_faces[v0][j];
					if(fj != fi && inTheList(vertex_to_faces[v1],fj)) {
						face_to_faces(fi,i) = fj;
						break;
					}
				}
			}
		}
	}
	void Mesh::ComputeNormals() {
		FNormals.setZero(nbF,3);
		#pragma omp parallel for
		for(int fi = 0; fi < nbF; ++fi) {
			const RowVector3 & p0 = getCurrentPose(faces(fi,0));
			const RowVector3 & p1 = getCurrentPose(faces(fi,1));
			const RowVector3 & p2 = getCurrentPose(faces(fi,2));		
			FNormals.row(fi) = ((p1-p0).cross(p2-p0)).normalized();
		}
		VNormals.setZero(nbV,3);
		#pragma omp parallel for
		for(int vi = 0; vi < nbV; ++vi) {
			RowVector3 n = RowVector3::Zero();
			for(IndexType j = 0; j < vfNbNeighbors(vi); ++j) {
				n += FNormals.row(vfNeighbor(vi,j));
			}
			VNormals.row(vi) = n.normalized();
		}
		ComputeCornerNormals();
	}
	void Mesh::ComputeCornerNormals() {
		double t = cos(clamp(corner_threshold,0.0,180.0)*M_PI/180.0);
		CNormals.setZero(nbF*3,3);
		#pragma omp parallel for
		for(int fi = 0 ; fi < nbF ; ++fi) {
			for(int j = 0 ; j < 3 ; ++j) {
				IndexType vj = faces(fi,j);
				RowVector3 n = FNormals.row(fi);
				for(int k = 0; k < vfNbNeighbors(vj); ++k) {
					IndexType fk = vfNeighbor(vj,k);
					if(fk != fi && FNormals.row(fi).dot(FNormals.row(fk)) >= t)
						n += FNormals.row(fk);
				}
				CNormals.row(3*fi+j) = n.normalized();
			}
		}
	}
	RowVector3 Mesh::transformedPosition(const RowVector3 & p) const 
	{
		return scale*(p-center) + RowVector3(0.5,0.5,0.5);
	}
	void Mesh::ComputeBoundingBox() {
		bmin = bmax = vertices.row(0);	
		for(int i=1; i < nbV; i++) {
			bmin = bmin.cwiseMin(vertices.row(i));
			bmax = bmax.cwiseMax(vertices.row(i));    
		}
		RowVector3 length = (bmax - bmin).cwiseInverse();
		ScalarType min_scale = 0.95*length.minCoeff();
		scale = 0.98*length.minCoeff();
		center = (bmin+bmax)/2.0;
		for(int i=0; i < nbV; i++) {
			vertices.row(i) = min_scale*(vertices.row(i)-center) + RowVector3(0.5,0.5,0.5);
		}
		bmin = bmax = vertices.row(0);	
		for(int i=1; i < nbV; i++) {
			bmin = bmin.cwiseMin(vertices.row(i));
			bmax = bmax.cwiseMax(vertices.row(i));    
		}
	}
	RowVector3 Mesh::getHigherVertex() const {
		ScalarType highestHeight = -std::numeric_limits<ScalarType>::infinity();
        const RowVector3 & gravity = RowVector3(0,-1,0);
		int idBest = -1;
		for(int i = 0 ; i < nbV ; i++) {
			ScalarType height = -gravity.dot(vertices.row(i));
			if(height > highestHeight) {
				highestHeight = height;
				idBest = i;
			}
		}	
		return vertices.row(idBest);
	}	
	//zhiping luo
	ScalarType Mesh::computeRestVolume()
	{
		ScalarType volume = 0;
		for(int i=0;i<nbF;i++)
		{
			IndexType i1= F(i,0);IndexType i2= F(i,1);IndexType i3= F(i,2);
			RowVector3 v1 = getRestPose(i1);
			RowVector3 v2 = getRestPose(i2);
			RowVector3 v3 = getRestPose(i3);
			volume += (1.0/6.0)*(v1.dot(v2.cross(v3)));
		}
		//cout<<"rest volume "<<volume<<endl;
		return volume;
	}
	ScalarType Mesh::computeCurrentVolume()
	{
		ScalarType volume = 0;
		for(int i=0;i<nbF;i++)
		{
			IndexType i1= F(i,0);IndexType i2= F(i,1);IndexType i3= F(i,2);
			RowVector3 v1 = getCurrentPose(i1);
			RowVector3 v2 = getCurrentPose(i2);
			RowVector3 v3 = getCurrentPose(i3);
			volume += (1.0/6.0)*(v1.dot(v2.cross(v3)));
		}
		//cout<<"current volume "<<volume<<endl;
		return volume;
	}
	ScalarType Mesh::computeRelativeVolume()
	{
		ScalarType result = computeCurrentVolume()/computeRestVolume();
		return result*100.0;
	}
	ScalarType Mesh::computeVolumeLoss()
	{
		ScalarType result = abs(computeCurrentVolume()-computeRestVolume())/computeRestVolume();
		return result;
	}
	void Mesh::glDrawForVoxelizer(const int res) const {
		
		glDisable(GL_LIGHTING);
		glColor4f(1.0f,1.0f,1.0f,1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBegin(GL_TRIANGLES);
		for(int i = 0 ; i < nbF ; ++i) {
			for(int j = 0 ; j < 3 ; ++j) {
				const RowVector3 & p = V(faces(i,j));
				glVertex3f(res*p[0],res*p[1],res*p[2]);
			}
		}
		glEnd();
	}
	void Mesh::draw()
	{
		for(IndexType fi = 0; fi < nbF ; ++fi) {
			glBegin(GL_TRIANGLES);
			for(IndexType j = 0; j < 3; ++j)
			{
				IndexType vj = F(fi,j);
				const RowVector3 & p = getCurrentPose(vj);
				const RowVector3 & n = cN(fi,j);
				if(textured())
				{
					const RowVector2 & tex = FT(fi,j);
					glTexCoord2f(tex[0],tex[1]);
		    		glVertexAttrib2f(1,tex[0],tex[1]); 
				}
				glNormal3f(n[0],n[1],n[2]);
				glVertexAttrib3f(1,n[0],n[1],n[2]); 
				glVertex3f(p[0],p[1],p[2]);
			}
			glEnd();
		}
	}
	void Mesh::render(bool wire)
	{
		if(wire)
		{
			glLineWidth((GLfloat)1.0f);
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(1.0, 1.0);
			draw();
			glDisable(GL_POLYGON_OFFSET_FILL);
			glDisable(GL_LIGHTING);
			glDisable(GL_LIGHT0);
			glColor3f (0.0, 0.0, 0.0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			draw();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
		}
		else draw();
	}
	void Mesh::computeEmbedWeight(const BoxGrid & m_voxels)
	{
		for(int idV = 0;idV<nbV;idV++)
		{
			RowVector3 P = getRestPose(idV);
			RowVector3 t;
			int idBox = m_voxels.getBoxContainingPoint(P,t);
			if(idBox == -1) {
				cout << "Error cannot create embedding Weights for this mesh vertex" << endl;
				return;
			}
			for(int i = 0 ; i < 8 ; ++i) 
			{
				int idNode = m_voxels.getBoxNodes(idBox, i);
				float alpha = 1.0f;
				alpha *= (((i/4) % 2 == 0)?(1.0f-t[0]):t[0]);
				alpha *= (((i/2) % 2 == 0)?(1.0f-t[1]):t[1]);
				alpha *= ((i % 2 == 0)?(1.0f-t[2]):t[2]);
				boxDeformInfo[idV]->pushWeight(idNode, alpha);
			}
			boxDeformInfo[idV]->normalizeWeights();
		}
		has_embedweight = true;
	}
	const RowVector3 & Mesh::getCurrentPose(int idV) const
	{
		return boxDeformInfo[idV]->getCurrentPose();
	}
	const RowVector3 & Mesh::getRestPose(int idV) const
	{
		return boxDeformInfo[idV]->getRestPose();
	}
	void Mesh::updateGeometry(BoxGrid & m_voxels, NodeHandles & m_nodehandles, int mode)
	{
		#pragma omp parallel for
		for(int i = 0 ; i < nbV ; ++i) {
			boxDeformInfo[i]->computeCurrentPose(m_voxels, m_nodehandles, mode);
		}
		ComputeNormals();
	}
	void Mesh::updateGeometry(int i, const RowVector3 & pose)
	{
		boxDeformInfo[i]->computeCurrentPose(pose);
	}
	RowVector3 Mesh::getLowestVertex(const RowVector3 & gravity) const {
		ScalarType lowestHeight = std::numeric_limits<ScalarType>::infinity();
		int idBest = -1;
		for(int i = 0 ; i < nbV ; i++) {
			ScalarType height = -gravity.dot(vertices.row(i));
			if(height < lowestHeight) {
				lowestHeight = height;
				idBest = i;
			}
		}	
		return vertices.row(idBest);
	}