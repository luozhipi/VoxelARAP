#ifndef __SUPPORT_H
#define __SUPPORT_H

#include "MIS_inc.h"

#include "cgal_functions.h"

class Support {

protected:
	vector<int> vertices;
	bool mode;
	Polygon_2 standingZone;
	Polygon_2 stabilityZone;
	vector<Point> minimalZone;
	RowVector3 suspendPoint;	
	SkeletonPtr skeleton;
	ScalarType maxOffset;

	ScalarType supportHeight;
	
	RowVector3 target;

	RowVector3 dir, dirU, dirV;
	ScalarType threshold;
	ScalarType angleObj;

public:
	Support(ScalarType t, ScalarType a) : threshold(t), angleObj(a) 
	{
		mode = true;
		dir = RowVector3(0,-1,0);
		dirU = RowVector3(1,0,0);
		dirV  = RowVector3(0,0,1);
	}
	~Support() {}
	void clear() {
		vertices.clear();
		standingZone.clear();
		stabilityZone.clear();
		minimalZone.clear();
		supportHeight = std::numeric_limits<ScalarType>::infinity();
	}
	
	const RowVector3 & getDir() const { return dir; }
	const RowVector3 & getDirU() const { return dirU; }
	const RowVector3 & getDirV() const { return dirV; }
	void setDir(const RowVector3 & a, const RowVector3 & b, const RowVector3 & c) {
		dir = a;
		dirU = b;
		dirV = c;
	}
	ScalarType getThreshold() const { return threshold; }
	void setThreshold(ScalarType t) { threshold = t; }
	ScalarType getAngleObj() const { return angleObj; }
	void setAngleObj(ScalarType angle) { angleObj = angle; }

	const RowVector3 & getTarget() const { return target; }
	void setTarget(const RowVector3 & p) { target = p; }

	int nbVertices() const { return vertices.size(); }
	int getVertex(int j) const { return vertices[j]; }


	void updateStandingZone(const Mesh & mesh);
	void updateStabilityZoneAndTarget(const RowVector3 & p);

	bool isStanding(const RowVector3 & p) const;
	bool isStable(const RowVector3 & p) const;
	
	RowVector3 getCentroid() const;
	ScalarType computeToppling(const RowVector3 & p) const;
	ScalarType computeDeviation(const RowVector3 & p) const;
	
	RowVector3 projectOnSupport(const RowVector3 & p) const {
		return -supportHeight * dir + p.dot(dirU) * dirU + p.dot(dirV) * dirV;
	}
	void setMode(bool _mode) { mode = _mode; }
	bool isStandingMode() const { return mode; }
	bool isSuspendedMode() const { return !mode; }
	void setSupendPoint(const RowVector3 & p) {
		suspendPoint = p;
	}
	void draw() const;
	void save(string filename);
};

#endif