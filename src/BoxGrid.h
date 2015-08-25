#ifndef __BOXGRID_H
#define __BOXGRID_H

#include "MIS_inc.h"
#include "Array3D.h"

// Basic data structures for a 3D grid of regular boxes (not necessarily equilateral -- though some methods silently assume square boxes)
// Some boxes can be empty, so we distinguish all elements (i.e. full 3D array) and non-empty ones (carving a subset of the 3D array)
class BoxGrid {

public:
	BoxGrid(const Mesh & mesh, const int res,string filename);
	~BoxGrid() {
		freeAll();
	}

	void initRes(const int res);
	void initVoxels(const Mesh & mesh, const int res);
	void initFromFile(string filename);
	void saveToFile(string voxfile) const;
	void initStructure();

	int getNumNodes() const { return nnzNodes; }
	int getNumBoxes() const { return nnzBoxes; }

	int getBoxContainingPoint(const RowVector3 & P, RowVector3 & t) const;
	bool getBoxCoordsContainingPoint(const RowVector3 & P, RowVector3i & t) const;
	int getNodeClosestToPoint(const RowVector3 & P) const;

	const RowVector3 & getCurrentPose(int idNode) const;
	const RowVector3 & getRestPose(int idNode) const;
	NodeDeformable* getDeformableNode(int j) const{return m_nodes[j];}
	void updateVoxel(Handles &m_handles);
	void updateVoxel(int idNode, const RowVector3 & pos);
	void computeBoxPositions();
	RowMatrixX3::ConstRowXpr getBoxPosition(int idBox) const { return m_boxPositions.row(idBox); }
	RowVector3 getRestBoxPosition(int idBox) const;

	bool isHull(int idBox) const { return m_depth[idBox] == 0; }
	int getBoxArray(int i, int j, int k){return m_boxArray(i,j,k);}
	int getBoxBoxes(int idBox, int idNeighbor) const { return m_boxBoxes(idBox,idNeighbor); }
	int getNodeNodes(int idNode, int idNeighbor) const { return m_nodeNodes(idNode,idNeighbor); }
	int getNodeValence(int idNode) const 
	{
		int number=0;
		for(int i=0;i<6;i++)
		{
			if(m_nodeNodes(idNode,i)!=-1)
				number++;
		}
		return number;
	}
	int getBoxNodes(int idBox, int idNeighbor) const { return m_boxNodes(idBox,idNeighbor); }
	void compute();
	void computeTransformation(NodeHandles & m_nodehandles);
	IndexType F(int idF, int idV) const;
	const RowVector3 & V(int idF, int idV) const;
	const RowVector3 & V(int idV) const;
	void draw();
	void renderLattice();
	void renderWire();
	void computeAnchors(const Mesh & m_mesh);
	int getAnchor(int j) const{return anchors[j];}
	int getNbAnchor()const{return anchors.size();}
	int getAnchorNode(int j) const{return anchorNodes[j];}
	int getNbAnchorNode()const{return anchorNodes.size();}
	void renderAnchor();
protected:
	Vector3i m_size; // number of boxes in x,y,z dimensions
	int resolution;
	RowVector3 m_lowerLeft, m_upperRight; // placement in 3D space
	RowVector3 m_frac;
	Array3D<int> m_nodeArray; // nodes 
	Array3D<int> m_boxArray; // boxes
	int nnzBoxes, nnzNodes, nnzEdges[3];
	void freeAll();
	vector<NodeDeformable*> m_nodes;
	vector<int> m_depth; // boxes depth (0 is hull, 1 the neighbors of the hull, 2...)
	RowMatrixX3 m_boxPositions; // positions of boxes (isobarycenter)
	MatrixX8i m_boxNodes; // numBoxes x 8 int matrix of node indices (incident to a given box)
	MatrixX6i m_nodeNodes; // numNodes x 6 int matrix of node indices (incident to a given node)
	MatrixX6i m_boxBoxes;
	QuadMatrixType faces;
	IndexType nbF; 
	vector<int> anchors;
	vector<int> anchorNodes;
};

#endif
