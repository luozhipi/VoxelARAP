#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H

#include "MIS_inc.h"

class Config {

private:
	string filename;
	string meshfile;
	Quaternion meshRotation;
	vector<RowVector3> handles;
	int voxRes;
public:
	Config(const string _meshfile);
	~Config();
	void saveConfig(const string _filename);
	string getConfigfile() const { 
		string configfile = string(PRINT_DIR).append(meshfile);
		removeExt(configfile);
		configfile.append(CONFIG_EXT);
		return configfile; }

	string getMeshName() const {return meshfile;}
	string getMeshfile() const { return string(MESH_DIR).append(meshfile); }
	string getVoxfile() const {
		string voxfile = string(PRINT_DIR).append(meshfile);
		removeExt(voxfile);
		voxfile.append("_").append(IntToString(voxRes)).append(VOX_EXT);
		return voxfile;
	}
	const unsigned int getNbHandles() const { return handles.size(); }
	const RowVector3 & getHandle(int i) const { return handles[i]; }
	const Quaternion & getMeshRotation() const { return meshRotation; }
	void setMeshRotation(const Quaternion & quat) {
		meshRotation = quat;
	}
	void deleteHandle(int i) {
		handles.erase(handles.begin()+i);
	}
	void addHandle(const RowVector3 & v) {
		handles.push_back(v);
	}
	void moveHandle(int i, const RowVector3 & t) {
		handles[i] += t;
	}
	int getVoxRes() const { return voxRes; }
	void setVoxRes(int _voxRes) { voxRes = _voxRes; }
};


#endif