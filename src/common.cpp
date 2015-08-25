#include "common.h"
#include "Mesh.h"
#include "config.h"
#include "Handles.h"
#include "BoxGrid.h"
#include "ARAPDeformer.h"
#include "RBFDeformer.h"
#include "NodeHandles.h"

namespace MIS {
	Config * m_config = 0;
	Mesh * m_mesh = 0;
	BoxGrid * m_voxels = 0;
	Handles * m_handles = 0;
	Support * m_support = 0;
	EmbreeIntersector *m_embree = 0;
	NodeHandles * m_nodehandles = 0;
	ARAPDeformer * m_ARAP = 0;
	RBFDeformer * m_rbf = 0;
	int embed_mode = 0;
	bool wire = false;
	float time_voxel=0.0;
	float time_skin=0.0;
	float volume = 100;
	float loss = 0.0;
	bool isConfigLoaded() { return m_config != 0; }
	bool isVoxelized() { return m_voxels != 0; }
	bool isSupported(){return m_support!=0;}
	bool isReady(){return m_handles !=0;}
	void initialize()
	{
		m_handles = new Handles(*m_config, *m_voxels);
		m_nodehandles = new NodeHandles(*m_voxels);
	}
	void voxelize() {
		cout << "Voxelizing mesh" << endl;
		const int res = m_config->getVoxRes();
		const string voxfile = m_config->getVoxfile();
		if(m_voxels==0)
			m_voxels = new BoxGrid(*m_mesh,res,voxfile);
		cout << "Voxelized mesh" << endl;
	}
	void updateVoxelGrid()
	{
	    time_voxel=0.0;
		igl::Timer timer = igl::Timer();
		timer.start();
		if(m_ARAP!=0)
		{
		//ARAP method
			m_voxels->updateVoxel(*m_handles);
			m_ARAP->setXYZ(*m_voxels);
			m_ARAP->SVDRotation(*m_voxels);
			m_ARAP->Deform(*m_voxels,5);
		}
		timer.stop();
		time_voxel = timer.getElapsedTimeInSec();
		updateSurface();
	}
	void updateSurface()
	{
		time_skin=0.0;
		m_voxels->computeTransformation(*m_nodehandles);
		igl::Timer timer = igl::Timer();
		timer.start();
		//embedding
		if(embed_mode<2 && m_mesh->has_embedweight)
		{
			m_mesh->updateGeometry(*m_voxels, *m_nodehandles, embed_mode);
		}
		else if(embed_mode==2 && m_rbf!=0)
		{
			//RBF
			m_rbf->deform(*m_voxels, *m_mesh);
		}
		timer.stop();
		time_skin = timer.getElapsedTimeInSec();
		if(embed_mode==3 && m_rbf!=0)
			time_skin -= m_rbf->time;//minus the precompute time
		volume = m_mesh->computeRelativeVolume();
		loss = m_mesh->computeVolumeLoss();
	}
	void init_ARAP()
	{
		igl::Timer timer = igl::Timer();
		timer.start();
		m_ARAP = new ARAPDeformer(*m_voxels);
		for(int i = 0;i< m_handles->getNumHandles();i++)
		{
			IndexType idBox = m_handles->getConstainBox(i);
			for(int j=0;j<8;j++)
			{
				m_ARAP->SetControl(m_voxels->getBoxNodes(idBox, j));
			}
		}
		for(int i = 0;i< m_voxels->getNbAnchorNode();i++)
		{
			int idNode = m_voxels->getAnchorNode(i);
			m_ARAP->SetAnchor(idNode);
		}
		m_ARAP->setOriPose(*m_voxels);
		m_ARAP->BuildAndFactor(*m_voxels);
		timer.stop();
		cout<<"==ARAP init time: "<<timer.getElapsedTimeInSec()<<" s=="<<endl;
	}
	void update_ARAP()
	{
		for(int i = 0;i< m_handles->getNumHandles();i++)
		{
			IndexType idBox = m_handles->getConstainBox(i);
			for(int j=0;j<8;j++)
			{
				m_ARAP->SetControl(m_voxels->getBoxNodes(idBox, j));
			}
		}
		for(int i = 0;i< m_voxels->getNbAnchorNode();i++)
		{
			int idNode = m_voxels->getAnchorNode(i);
			m_ARAP->SetAnchor(idNode);
		}
		m_ARAP->setOriPose(*m_voxels);
		m_ARAP->BuildAndFactor(*m_voxels);
	}
	void init_RBF()
	{
		igl::Timer timer = igl::Timer();
		timer.start();
		m_rbf = new RBFDeformer(*m_voxels);
		timer.stop();
		cout<<"==RBF init time: "<<timer.getElapsedTimeInSec()<<" s=="<<endl;
	}
	void uninitialize() {		
		if(m_mesh) { delete m_mesh; m_mesh = 0; }
		if(m_config) { delete m_config; m_config = 0; }
		if(m_voxels) { delete m_voxels; m_voxels = 0; }
		if(m_handles) { delete m_handles; m_handles = 0; }
		if(m_ARAP) { delete m_ARAP; m_ARAP = 0; }
		if(m_nodehandles){delete m_nodehandles;m_nodehandles=0;}
		if(m_support) { delete m_support; m_support = 0; }
		if(m_rbf){delete m_rbf;m_rbf=0;}
	}
};