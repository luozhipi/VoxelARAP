#ifndef __RENDERING_FUNCTIONS_H
#define __RENDERING_FUNCTIONS_H

#include "MIS_inc.h"
#include "readOBJ.h"

	class objModel {
	public:
		objModel(string filename) {
			igl::readOBJ(filename, V, F, N);
		}
		
		void draw() const;

	private:
		PointMatrixType V;
		FaceMatrixType F;
		PointMatrixType N;

	};
	void paintTrackball(const double radius);
    void paint3DFrame(const double radius);
    void paint2DFrameSL(const double radius, const RowVector3 & pos);
	void paintPoint(const RowVector3 & a, const double radius, const RowVector4 & color);
	void paintCylinder(const RowVector3 & a, const RowVector3 & b, const double radius, const RowVector4 & color);
	void paintCone(const RowVector3 & bottom, const RowVector3 & top, const double radius, const RowVector4 & color);
	void paintArrow(const RowVector3 & from, const RowVector3 & to, const double radius, const RowVector4 & color);
	void paintArrowSL(const RowVector3 & from, const RowVector3 & to, const double radius, const RowVector4 & color);
	void paintPointSL(const RowVector3 & a, const double radius, const RowVector4 & color);
	void paintConeSL(const RowVector3 & bottom, const RowVector3 & top, const double radius, const RowVector4 & color);	
	void paintCylinderSL(const RowVector3 & a, const RowVector3 & b, const double radius, const RowVector4 & color);
#endif