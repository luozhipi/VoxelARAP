#include "Support.h"
#include "Mesh.h"

#undef min
#undef max

static Point projectPointOnSegment(const Point & p, const Segment & seg) {
	Point q = seg.supporting_line().projection(p);
	if( K::Vector_2(seg.start(),q) * K::Vector_2(seg.start(),seg.target()) <= 0 ) {
		return seg.start();
	}
	else if( K::Vector_2(seg.target(),q) * K::Vector_2(seg.target(),seg.source()) <= 0 ) {
		return seg.target();
	}
	else return q;
}

bool Support::isStanding(const RowVector3 & p) const {
	Point q(p.dot(dirU),p.dot(dirV));
	return standingZone.has_on_bounded_side(q);
	return true;
}

bool Support::isStable(const RowVector3 & p) const {
	return (computeToppling(p) >= angleObj);
}

ScalarType Support::computeToppling(const RowVector3 & p) const {
	Point q(p.dot(dirU),p.dot(dirV));
	double minDist = std::numeric_limits<double>::infinity();

	for(Polygon_2::Edge_const_iterator ei = standingZone.edges_begin() ; ei != standingZone.edges_end() ; ++ei) {
		double d = CGAL::squared_distance(q,*ei);
		
		if(d < minDist) minDist = d;
	}
	
	minDist = sqrt(minDist);
	ScalarType height = -dir.dot(p-target);
	
	return 180.0*atan(minDist/height)/M_PI;
}

ScalarType Support::computeDeviation(const RowVector3 & p) const {
	RowVector3 dirCC = p - target;
	dirCC.normalize();
	return 180.0*acos(clamp(dirCC.dot(dir),0.0,1.0))/M_PI;
}

RowVector3 Support::getCentroid() const {

	RowVector2 c = RowVector2::Zero();
	double totalEdgeLength = 0;
	
	for(Polygon_2::Edge_const_iterator ei = standingZone.edges_begin() ; ei != standingZone.edges_end() ; ++ei) {
		double edgeLength = sqrt(ei->squared_length());
		totalEdgeLength += edgeLength;
		c += 0.5 * edgeLength * RowVector2(ei->start().x(),ei->start().y());
		c += 0.5 * edgeLength * RowVector2(ei->end().x(),ei->end().y());
	}
	c /= totalEdgeLength;

	return -supportHeight * dir + c[0] * dirU + c[1] * dirV;
}

void Support::updateStandingZone(const Mesh & mesh) {
	ScalarType lowestH = std::numeric_limits<ScalarType>::infinity();

	for(int i = 0 ; i < mesh.nbV ; i++) {
		lowestH = std::min(lowestH,-dir.dot(mesh.V(i)));
	}
	
	supportHeight = lowestH + threshold;

	vector<Point> projVertices;

	for(int i = 0 ; i < mesh.nbV ; i++) {
		if(supportHeight > -dir.dot(mesh.V(i))) {
			vertices.push_back(i);
			const RowVector3 & p = mesh.V(i);
			projVertices.push_back(Point(p.dot(dirU),p.dot(dirV)));
		}
	}
	vector<Point> hull;
	CGAL::convex_hull_2( projVertices.begin(), projVertices.end(), std::back_inserter(hull) );

	for(unsigned int i = 0 ; i < hull.size() ; ++i) {
		standingZone.push_back(hull[i]);
	}
	
	skeleton = CGAL::create_interior_straight_skeleton_2(standingZone);
	maxOffset = 0;
	minimalZone.clear();
	for( Skeleton::Vertex_const_iterator vi = skeleton->vertices_begin() ; vi != skeleton->vertices_end() ; ++vi ) {
		 maxOffset = std::max(vi->time(),maxOffset);
	}
	for( Skeleton::Vertex_const_iterator vi = skeleton->vertices_begin() ; vi != skeleton->vertices_end() ; ++vi ) {
		if(vi->time() == maxOffset) minimalZone.push_back(vi->point());
	}
	
	if(minimalZone.size() == 1) target = -supportHeight * dir + minimalZone[0].x() * dirU + minimalZone[0].y() * dirV;
	if(minimalZone.size() == 2) target = -supportHeight * dir + 0.5*(minimalZone[0].x()+minimalZone[1].x()) * dirU + 0.5*(minimalZone[0].y()+minimalZone[1].y()) * dirV;
}

void Support::updateStabilityZoneAndTarget(const RowVector3 & p) {
	
	Point q(p.dot(dirU),p.dot(dirV));
	ScalarType height = -dir.dot(p-target);
	ScalarType offset = std::max(1e-4,height * tan(M_PI*angleObj/180.0));

	Point projTarget;
	if(offset < maxOffset) {
		stabilityZone = *(CGAL::create_offset_polygons_2<Polygon_2>(offset,*skeleton)[0]);
		
		double minDist = std::numeric_limits<double>::infinity();
		Segment minDistSeg;
		for(Polygon_2::Edge_const_iterator ei = stabilityZone.edges_begin() ; ei != stabilityZone.edges_end() ; ++ei) {
			double d = CGAL::squared_distance(q,*ei);
		
			if(d < minDist) {
				minDist = d;
				minDistSeg = *ei;
			}
		}
		projTarget = projectPointOnSegment(q,minDistSeg);
	}
	else {
		stabilityZone.clear();
		if(minimalZone.size() == 1)
			projTarget = minimalZone[0];
		else if(minimalZone.size() == 2)
			projTarget = projectPointOnSegment(q,Segment(minimalZone[0],minimalZone[1]));
		else 
			cout << "Error : the minimal stability zone must be a point or a segment" << endl;
	}

	target = -supportHeight * dir + projTarget.x() * dirU + projTarget.y() * dirV;

}

void Support::draw() const {
	if(standingZone.size() >= 3) 
	{
		glColor3f(1.0f,0.64f,0.0f);
		glBegin(GL_POLYGON);
		glNormal3f(-dir[0],-dir[1],-dir[2]);
		for(unsigned int i = 0 ; i < standingZone.size() ; ++i) {
			RowVector3 p = -supportHeight * dir + standingZone[i].x() * dirU + standingZone[i].y() * dirV;
			glVertex3f(p[0],p[1],p[2]);
		}
		glEnd();
	}
	if(stabilityZone.size() >= 3) {
		glColor3f(0.0f,1.0f,0.0f);
		glBegin(GL_POLYGON);
		glNormal3f(-dir[0],-dir[1],-dir[2]);
		for(unsigned int i = 0 ; i < stabilityZone.size() ; ++i) {
			RowVector3 p = -(supportHeight+0.001) * dir + stabilityZone[i].x() * dirU + stabilityZone[i].y() * dirV;
			glVertex3f(p[0],p[1],p[2]);
		}
		glEnd();
	}
}
	void Support::save(string _filename)
	{
		ofstream fout;
		fout.open(_filename, std::ios::out);
		if(fout.fail()) {
		   cout << "I/O error" << endl;
		}
		else {
			for(unsigned int i = 0 ; i < vertices.size() ; ++i) {
				fout << vertices[i] << endl;
			}
		}
		fout.close();
	}