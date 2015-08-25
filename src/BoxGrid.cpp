#include "BoxGrid.h"
#include "Mesh.h"
#include "Handles.h"
#include "NodeHandles.h"
#include "NodeDeformable.h"
#include "utils_functions.h"
#include "slvoxlib.h"
#include "rendering_functions.h"
#include "extract_rotation_pseudo_edge.h"

static const int g_quadCube[6][4] = {
	{0,1,3,2},
	{0,4,5,1},
	{0,2,6,4},
	{4,6,7,5},
	{2,3,7,6},
	{1,5,7,3}
};

BoxGrid::BoxGrid(const Mesh & mesh,const int res,string filename)
{
	initRes(res);
	if(exists(filename))
		initFromFile(filename);
	else
		initVoxels(mesh,res);
	initStructure();
	compute();
}
void BoxGrid::initVoxels(const Mesh & mesh, const int res) {
	
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];	
    slvox_set_voxel_res( res );
	slvox_begin();
	mesh.glDrawForVoxelizer(res);
	slvox_end();  
	unsigned char *vox = new unsigned char[res*res*res];
	slvox_getvoxels( vox );
	m_boxArray.setAllTo(-1);
	nnzBoxes = 0;
	for (int x=0; x<Xs; x++) {
		for (int y=0; y<Ys; y++) {
			for (int z=0; z<Zs; z++) {
				if(vox[res*res*z+res*y+x] != 0) {
					m_boxArray(x,y,z) = nnzBoxes++;
				}
			}
		}
	}
	delete vox;	
	for(int i = 0 ; i < mesh.nbV ; ++i) {
		RowVector3i t = RowVector3i::Zero();
		if(getBoxCoordsContainingPoint(mesh.V(i),t)) {
			if(m_boxArray(t[0],t[1],t[2]) == -1) {
				m_boxArray(t[0],t[1],t[2]) = nnzBoxes++;
			}
		}
		else {
			cout << "Error !" << endl;
		}
	}
}
void BoxGrid::initRes(const int res)
{
	resolution = res;
	m_size[0] = m_size[1] = m_size[2] = res;
	m_lowerLeft.setZero();
	m_upperRight.setConstant(1.0f);	
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];
	m_boxArray.init(Xs, Ys, Zs);
	m_nodeArray.init(Xs+1, Ys+1, Zs+1);
	m_frac = (m_upperRight-m_lowerLeft).cwiseQuotient(RowVector3(Xs,Ys,Zs));
}

//oct-tree
void BoxGrid::initStructure() {	
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];
	m_nodeArray.setAllTo(-1);
	nnzNodes = 0;
	for (int x=0; x<Xs+1; x++) {
		for (int y=0; y<Ys+1; y++) {
			for (int z=0; z<Zs+1; z++) {
				bool occupiedNeighboringBox = false;
				for (int dx=-1; dx<1; dx++) {
					for (int dy=-1; dy<1; dy++) {
						for (int dz=-1; dz<1; dz++) {
							if (m_boxArray.validIndices(x + dx, y + dy, z + dz)) {
								if (m_boxArray(x + dx, y + dy, z + dz) != -1) occupiedNeighboringBox = true;								
							}
						}
					}
				}				
				if (occupiedNeighboringBox) m_nodeArray(x, y, z) = nnzNodes++;
			}
		}
	}
	//zhiping luo
	m_nodes.assign(nnzNodes, 0);
	for (int x=0; x<Xs+1; x++) {
		for (int y=0; y<Ys+1; y++) {
			for (int z=0; z<Zs+1; z++) {
				const int nIdx = m_nodeArray(x, y, z);
				if (nIdx != -1) {
					m_nodes[nIdx] = new NodeDeformable( m_lowerLeft + m_frac.cwiseProduct(RowVector3(x,y,z)) );
				}
			}
		}
	}

	m_nodeNodes.setConstant(nnzNodes,6,-1);
	for (int x=0; x<Xs+1; x++) {
		for (int y=0; y<Ys+1; y++) {
			for (int z=0; z<Zs+1; z++) {
				if(m_nodeArray(x,y,z) == -1) continue;
				int idNode = m_nodeArray(x,y,z);
				int k = 0;
				for (int dw=-1; dw<=1; dw+=2) {
					if (m_nodeArray.validIndices(x + dw, y, z)) {
						m_nodeNodes(idNode,k) = m_nodeArray(x + dw, y, z);								
					}
					k++;
					if (m_nodeArray.validIndices(x, y + dw, z)) {
						m_nodeNodes(idNode,k) = m_nodeArray(x, y + dw, z);								
					}
					k++;
					if (m_nodeArray.validIndices(x, y, z + dw)) {
						m_nodeNodes(idNode,k) = m_nodeArray(x, y, z + dw);								
					}
					k++;
				}
			}
		}
	}

	m_boxNodes.setConstant(nnzBoxes, 8, -1);
	for (int x=0; x<Xs; x++) {
		for (int y=0; y<Ys; y++) {
			for (int z=0; z<Zs; z++) {
				const int idBox = m_boxArray(x,y,z);
				if (idBox == -1) continue;
				int cnt = 0;
				for (int dx=0; dx<2; dx++) {
					for (int dy=0; dy<2; dy++) {
						for (int dz=0; dz<2; dz++) {
							m_boxNodes(idBox,cnt++) = m_nodeArray(x + dx, y + dy, z + dz);
						}
					}
				}
			}
		}
	}

	m_boxBoxes.setConstant(nnzBoxes,6,-1);
	for (int x=0; x<Xs; x++) {
		for (int y=0; y<Ys; y++) {
			for (int z=0; z<Zs; z++) {
				if(m_boxArray(x,y,z) == -1) continue;
				int idBox = m_boxArray(x,y,z);
				int k = 0;
				for (int dw=-1; dw<=1; dw+=2) {
					if (m_boxArray.validIndices(x + dw, y, z)) {
						m_boxBoxes(idBox,k) = m_boxArray(x + dw, y, z);								
					}
					k++;
					if (m_boxArray.validIndices(x, y + dw, z)) {
						m_boxBoxes(idBox,k) = m_boxArray(x, y + dw, z);								
					}
					k++;
					if (m_boxArray.validIndices(x, y, z + dw)) {
						m_boxBoxes(idBox,k) = m_boxArray(x, y, z + dw);								
					}
					k++;
				}
			}
		}
	}

	m_depth.assign(nnzBoxes,-1);
	std::queue<int> voxBFS; // breadth first search from the surface octants
	for (int x=0; x<Xs; x++) {
		for (int y=0; y<Ys; y++) {
			for (int z=0; z<Zs; z++) {
				if(m_boxArray(x,y,z) == -1) continue;
				int idBox = m_boxArray(x,y,z);
				bool hull = false;
				for (int dx=-1; !hull && dx<=1; ++dx) {
					for (int dy=-1; !hull && dy<=1; ++dy) {
						for (int dz=-1; !hull && dz<=1; ++dz) {
							if (dx == 0 && dy == 0 && dz == 0) continue;
							if (!m_boxArray.validIndices(x + dx, y + dy, z + dz) || m_boxArray(x + dx, y + dy, z + dz) == -1) {
								hull = true;	
							}
						}
					}
				}			
				if(hull) {
					m_depth[idBox] = 0;
					voxBFS.push(idBox);
				}
			}
		}
	}
	while(!voxBFS.empty()) {
		int idBox = voxBFS.front();
		voxBFS.pop();
		for(int i = 0 ; i < 6 ; ++i) {
			int idNeighbor = m_boxBoxes(idBox,i);
			if(idNeighbor != -1 && m_depth[idNeighbor] == -1) {
				m_depth[idNeighbor] = m_depth[idBox]+1;
				voxBFS.push(idNeighbor);
			}
		}
	}
}

void BoxGrid::initFromFile(string filename) {
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];
	ifstream fin;
	fin.open(filename,std::ios::in);
	if(fin.fail()) {
		cout << "I/O error" << endl;
	}
	else {
		m_boxArray.setAllTo(-1);
		nnzBoxes = 0;
		for (int x=0; x<Xs; x++) {
			for (int y=0; y<Ys; y++) {
				for (int z=0; z<Zs; z++) {
					int idBox;
					fin >> idBox;
					if(idBox >= 0) {
						m_boxArray(x,y,z) = idBox;
						nnzBoxes++;
					}
				}
			}
		}
	}
	fin.close();
}
void BoxGrid::saveToFile(string voxfile) const {
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];
	ofstream fout;
	fout.open(voxfile, std::ios::out);
	if(fout.fail()) {
		cout << "I/O error" << endl;
	}
	else {
		for (int x=0; x<Xs; x++) {
			for (int y=0; y<Ys; y++) {
				for (int z=0; z<Zs; z++) {
					fout <<	m_boxArray(x,y,z) << " ";
				}
				fout << endl;
			}
		}
	}
	fout.close();
}
void BoxGrid::freeAll()
{
	m_nodeArray.free();
	m_boxArray.free();
	for(unsigned int i = 0 ; i < m_nodes.size() ; ++i)
		if(m_nodes[i]) delete m_nodes[i];
	m_nodes.clear();
}

int BoxGrid::getBoxContainingPoint(const RowVector3 & P, RowVector3 & t) const
{
	if(P.cwiseMin(m_lowerLeft) != m_lowerLeft || P.cwiseMax(m_upperRight) != m_upperRight) {
		cout << "Error : point is outside the voxelgrid" << endl;
		//P = P.cwiseMax(m_lowerLeft).cwiseMin(m_upperRight);
		return -1;
	}
	RowVector3 floatIndices = (P - m_lowerLeft).cwiseQuotient(m_frac);
	RowVector3i intIndices((int)floor(floatIndices[0]),(int)floor(floatIndices[1]),(int)floor(floatIndices[2]));	
	if(!m_boxArray.validIndices(intIndices[0],intIndices[1],intIndices[2])) {
		cout << "Error : position in voxelgrid incorrect" << endl;
		return -1;
	}
	t = floatIndices - RowVector3(intIndices[0],intIndices[1],intIndices[2]);
	return m_boxArray(intIndices[0],intIndices[1],intIndices[2]);
}

bool BoxGrid::getBoxCoordsContainingPoint(const RowVector3 & P, RowVector3i & t) const
{
	if(P.cwiseMin(m_lowerLeft) != m_lowerLeft || P.cwiseMax(m_upperRight) != m_upperRight) {
		cout << "Error : point is outside the voxelgrid" << endl;
		//P = P.cwiseMax(m_lowerLeft).cwiseMin(m_upperRight);
		return false;
	}
	RowVector3 floatIndices = (P - m_lowerLeft).cwiseQuotient(m_frac);
	t = RowVector3i((int)floor(floatIndices[0]),(int)floor(floatIndices[1]),(int)floor(floatIndices[2]));
	
	if(!m_boxArray.validIndices(t[0],t[1],t[2])) {
		cout << "Error : position in voxelgrid incorrect" << endl;
		return false;
	}
	return true;
}

int BoxGrid::getNodeClosestToPoint(const RowVector3 & P) const
{
	if(P.cwiseMin(m_lowerLeft) != m_lowerLeft || P.cwiseMax(m_upperRight) != m_upperRight) {
		cout << "Error : point is outside the voxelgrid" << endl;
		//P = P.cwiseMax(m_lowerLeft).cwiseMin(m_upperRight);
		return -1;
	}
	RowVector3 floatIndices = (P - m_lowerLeft).cwiseQuotient(m_frac);
	RowVector3i intIndices((int)floor(floatIndices[0]),(int)floor(floatIndices[1]),(int)floor(floatIndices[2]));
	if(!m_boxArray.validIndices(intIndices[0],intIndices[1],intIndices[2])) {
		cout << "Error : position in voxelgrid incorrect" << endl;
		return -1;
	}
	RowVector3 t = floatIndices - RowVector3(intIndices[0],intIndices[1],intIndices[2]);
	int dx = (t[0]<=0.5)?0:1;
	int dy = (t[1]<=0.5)?0:1;
	int dz = (t[2]<=0.5)?0:1;
	return m_nodeArray(intIndices[0]+dx,intIndices[1]+dy,intIndices[2]+dz);
}
	
	const RowVector3 & BoxGrid::getCurrentPose(int idNode) const {
		return m_nodes[idNode]->getCurrentPose();
	}
	const RowVector3 & BoxGrid::getRestPose(int idNode) const {
		return m_nodes[idNode]->getRestPose();
	}
	void BoxGrid::updateVoxel(int idNode,const RowVector3 & pos)
	{
		m_nodes[idNode]->computeCurrentPose(pos);
	}
	void BoxGrid::updateVoxel(Handles &m_handles)
	{
		for(int i=0;i<m_handles.getNumHandles();i++)
		{
			IndexType idBox = m_handles.getConstainBox(i);
			for(int j=0;j<8;j++)
			{
				RowVector3 p = m_handles.getHandlePose(i);
				m_nodes[m_boxNodes(idBox, j)]->computeCurrentPose(p);
			}
		}
	}
	static const int g_triangulateCube[12][3] = {
		{1,2,0},{1,3,2},
		{4,7,5},{4,6,7},
		{3,6,2},{3,7,6},
		{5,1,0},{5,0,4},
		{0,2,6},{0,6,4},
		{5,7,3},{5,3,1}
	};

	void BoxGrid::computeBoxPositions() {
		m_boxPositions.setZero(getNumBoxes(),3);
		#pragma omp parallel for
		for(int i = 0 ; i < getNumBoxes() ; i++) {
			for(int k = 0 ; k < 8 ; ++k) {
				m_boxPositions.row(i) += getCurrentPose(m_boxNodes(i,k));
			}
		}
		m_boxPositions *= 0.125;
	}

	RowVector3 BoxGrid::getRestBoxPosition(int idBox) const
	{
		RowVector3 p = RowVector3::Zero();
		for(int k = 0 ; k < 8 ; ++k) {
			p += getRestPose(m_boxNodes(idBox,k));
		}
		p *= 0.125;
		return p;
	}

	void BoxGrid::compute()
	{
		nbF = 0;	
		for(int i = 0 ; i < getNumBoxes() ; ++i) {
			{
				for(int j = 0 ; j < 6 ; ++j)
				{
					//if(voxels.getBoxBoxes(i,j) != -1 && !voxels.isFilled(voxels.getBoxBoxes(i,j)))
					{
						nbF++;
					}
				}
			}
		}
		faces.setZero(nbF,4);			
		int idF = 0;
		for(int i = 0 ; i < getNumBoxes() ; ++i) {
			{
				for(int j = 0 ; j < 6 ; ++j) {
					//if(voxels.getBoxBoxes(i,j) != -1 && !voxels.isFilled(voxels.getBoxBoxes(i,j))) 
					{
						for(int k = 0 ; k < 4 ; ++k)
							faces(idF,k) = getBoxNodes(i,g_quadCube[j][k]);
						idF++;
					}
				}
			}
		}
	}
	IndexType BoxGrid::F(int idF, int idV) const {
		return faces(idF,idV);
	}

	const RowVector3 & BoxGrid::V(int idF, int idV) const {
		return getCurrentPose(faces(idF,idV));
	}

	const RowVector3 & BoxGrid::V(int idV) const {
		return getCurrentPose(idV);
	}
	void BoxGrid::draw()
	{
		for(IndexType fi = 0; fi < nbF ; ++fi) {
			glBegin(GL_QUADS);
			const RowVector3 & v0 = V(fi,0);
			const RowVector3 & v1 = V(fi,1);
			const RowVector3 & v2 = V(fi,2);
			const RowVector3 & v3 = V(fi,3);
			RowVector3 n = (v3-v1).cross(v0-v1);
			n.normalize();
			glNormal3f(n[0],n[1],n[2]);
			glVertexAttrib3f(1,n[0],n[1],n[2]);
			glVertex3d(v0[0], v0[1], v0[2]);
			glVertex3d(v1[0], v1[1], v1[2]);
			glVertex3d(v2[0], v2[1], v2[2]);
			glVertex3d(v3[0], v3[1], v3[2]);
			glEnd();
		}
	}

	void BoxGrid::renderLattice()
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glLineWidth((GLfloat)1.0f);
		glColor3f (0.45, 0.45, 0.45);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        draw();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
	}

	void BoxGrid::renderWire()
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		//glColor4f(238.0/255.0,250.0/255.0,0.0,138.0/255.0);
		glColor4f(115.0/255.0,115.0/255.0,115.0/255.0,255.0/255.0);
		glPolygonOffset(1.0, 1.0);
		draw();
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glLineWidth((GLfloat)2.0f);
		glColor3f (0.0, 0.0, 0.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		draw();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
	}
	void BoxGrid::renderAnchor()
	{
		glColor4f(1.0,0,0,1.0);
		glBegin(GL_QUADS);
		for(int i=0;i<anchors.size();i++)
		{
			int idBox = anchors[i];
			RowVector3 p[8];
			p[0] = getCurrentPose(getBoxNodes(idBox,7));
			p[1] = getCurrentPose(getBoxNodes(idBox,6));
			p[2] = getCurrentPose(getBoxNodes(idBox,5));
			p[3] = getCurrentPose(getBoxNodes(idBox,4));
			p[4] = getCurrentPose(getBoxNodes(idBox,3));
			p[5] = getCurrentPose(getBoxNodes(idBox,2));
			p[6] = getCurrentPose(getBoxNodes(idBox,1));
			p[7] = getCurrentPose(getBoxNodes(idBox,0));
			glNormal((p[3]-p[7]).cross(p[6]-p[7]));//bottom
			glVertex(p[7]);
			glVertex(p[3]);
			glVertex(p[2]);
			glVertex(p[6]);
			glNormal((p[4]-p[5]).cross(p[1]-p[5]));//top
			glVertex(p[5]);
			glVertex(p[4]);
			glVertex(p[0]);
			glVertex(p[1]);
			glNormal((p[5]-p[7]).cross(p[3]-p[7]));//far
			glVertex(p[7]);
			glVertex(p[5]);
			glVertex(p[1]);
			glVertex(p[3]);
			glNormal((p[1]-p[3]).cross(p[2]-p[3]));//right
			glVertex(p[3]); 
			glVertex(p[1]);
			glVertex(p[0]);
			glVertex(p[2]);
			glNormal((p[2]-p[6]).cross(p[4]-p[6]));//front
			glVertex(p[6]);
			glVertex(p[2]);
			glVertex(p[0]);
			glVertex(p[4]);
			glNormal((p[6]-p[7]).cross(p[5]-p[7]));//left
			glVertex(p[7]);
			glVertex(p[6]);
			glVertex(p[4]);
			glVertex(p[5]);
		}
		glEnd();
		RowVector4 color(1.0,0.0,0.0,1.0);
		for(int i=0;i<anchorNodes.size();i++)
		{
			RowVector3 p = getCurrentPose(anchorNodes[i]);
			paintPoint(p, 0.007,color);
		}
	}
	void BoxGrid::computeAnchors(const Mesh & m_mesh)
	{
		for(int i=0;i<m_mesh.getNbAnchors();i++)
		{
			int indice = m_mesh.getAnchor(i);
			RowVector3 p = m_mesh.V(indice);
			RowVector3 t;
			int idBox = getBoxContainingPoint(p,t);
			anchors.push_back(idBox);
			for(int j = 0; j < 8;j++)
			{
				int idNode =getBoxNodes(idBox,j);
				if(!inTheList(anchorNodes,idNode))
				{
					anchorNodes.push_back(idNode);
				}
			}
		}
	}
	//zhiping luo
	void BoxGrid::computeTransformation(NodeHandles & m_nodehandles)
	{
		vector<int> nodes;
		for(int idBox = 0; idBox <nnzBoxes; idBox++)
		{
			if(isHull(idBox))
			{
				for(int i = 0; i< 8; i++){
					int idNode = getBoxNodes(idBox, i);
					if(!inTheList(nodes, idNode)){
						nodes.push_back(idNode);
				    	RowVector3 P = getCurrentPose(idNode);
						RowVector3 P_rest = getRestPose(idNode);
						int m_numEdges = 0;
						Quaternion quaternions[6];
						for(int j = 0; j<6;j++){
							int idNeighborNode = getNodeNodes(idNode, j);
							if(idNeighborNode!=-1){
								float tmp[4];
								RowVector3 P1 = getRestPose(idNeighborNode);
								RowVector3 P2 = getCurrentPose(idNeighborNode);
								RowVector3 P3 = (P_rest - P1).normalized();
								RowVector3 P4 = (P - P2).normalized();
								extract_rotation_pseudo_edge(tmp, P3[0],P3[1],P3[2],P4[0],P4[1],P4[2]);
								Quaternion r(tmp[3],tmp[0],tmp[1],tmp[2]);
								r.normalize();
								quaternions[m_numEdges] = r;
								m_numEdges++;
							}
						}
						Quaternion r = AccurateAverageQuaternion(quaternions,m_numEdges);
						m_nodehandles.setR(idNode, r);
						m_nodehandles.setT(idNode, P);
					}
				}
			}
		}
		nodes.clear();
	}