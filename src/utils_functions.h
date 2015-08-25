#ifndef __UTILS_FUNCTIONS_H
#define __UTILS_FUNCTIONS_H

#include "STL_inc.h"
#include "EIGEN_inc.h"
#include "dirent.h"

inline void glVertex(RowVector3 v){glVertex3f(v[0], v[1], v[2]);}
inline void glNormal(RowVector3 v){glNormal3f(v[0], v[1], v[2]);}
inline bool isEqual(vector<int> vec0, vector<int> vec1)
{
	bool result = true;
	for(int i=0;i<vec0.size();i++)
	{
		bool found = false;
		for(int j=0;j<vec1.size();j++)
		{
			if(vec0[i] == vec1[j])
			{
				found = true;
				break;
			}
		}
		if(!found) result = false;
	}
	return result;
}
//zhiping luo
inline bool pointInTriangle(const RowVector3 & p, const RowVector3 & p0, 
	const RowVector3 & p1, const RowVector3 & p2, ScalarType & u, ScalarType & v, ScalarType & w)
{
	RowVector3 v0 = p1-p0, v1 = p2 - p0, v2 = p - p0;
    ScalarType d00 = v0.dot(v0);
	ScalarType d01 = v0.dot(v1);
	ScalarType d11 = v1.dot(v1);
	ScalarType d20 = v2.dot(v0);
	ScalarType d21 = v2.dot(v1);
    ScalarType invDenom = 1.0 / (d00 * d11 - d01 * d01);
    v = (d11 * d20 - d01 * d21) * invDenom;
    w = (d00 * d21 - d01 * d20) * invDenom;
    u = 1.0f - v - w;
	return (v>=0) && (w>=0) && (w+v<=1);
}

inline void QuaternionToMatrix44(const Quaternion & q, double * mat) {
	Matrix33 m = q.toRotationMatrix().transpose();
	mat[0] = m(0,0); mat[1] = m(0,1); mat[2] = m(0,2); mat[3] = 0; 
	mat[4] = m(1,0); mat[5] = m(1,1); mat[6] = m(1,2); mat[7] = 0; 
	mat[8] = m(2,0); mat[9] = m(2,1); mat[10] = m(2,2); mat[11] = 0; 
	mat[12] = 0; mat[13] = 0; mat[14] = 0; mat[15] = 1; 
}

inline void compute_bounding_box_and_centroid(const PointMatrixType & vertices, RowVector3 & min_point, RowVector3 & max_point, RowVector3 & centroid)
{
  centroid = vertices.colwise().sum()/vertices.rows();
  min_point = vertices.colwise().minCoeff();
  max_point = vertices.colwise().maxCoeff();
}

#undef min
#undef max

inline void get_scale_and_shift_to_fit_mesh(const PointMatrixType & vertices, float& zoom, RowVector3 & shift)
{
  //Compute mesh centroid
  RowVector3 centroid;
  RowVector3 min_point;
  RowVector3 max_point;
  compute_bounding_box_and_centroid(vertices, min_point, max_point, centroid);
  
  shift = centroid;
  double x_scale = fabs(max_point[0] - min_point[0]);
  double y_scale = fabs(max_point[1] - min_point[1]);
  double z_scale = fabs(max_point[2] - min_point[2]);
  zoom = 2.7f/ (float)std::max(z_scale,std::max(x_scale,y_scale));
}

inline bool exists(const string filename) {
	std::ifstream ifile(filename);
	bool result = ifile.is_open();
	ifile.close();
	return result;
}

inline void findFiles(string dirName, string extName, vector<string> & files) {

	DIR *pxDir = opendir(dirName.c_str());
	struct dirent *pxItem = NULL;
	if(pxDir != NULL) {
		files.clear();
		while(pxItem = readdir(pxDir)) {
			string fn(pxItem->d_name);
			if(fn.find(extName) != string::npos) {
				files.push_back(fn);
			}
		}
		closedir(pxDir);
	}
	else
		cout << "Unable to open specified directory." << endl;
}

inline void transpose(float *m)
{
	for (int j=0;j<4;j++) {
	for (int i=0;i<4;i++) {
		if (i<j) { continue; }
		int k  = i+j*4;
		int t  = j+i*4;
		std::swap( m[k],m[t] );
	}
	}
}

inline string IntToString (int Number) {
     std::ostringstream ss;
     ss << Number;
     return ss.str();
}

inline bool inTheList(const vector<int> & l, int i) {
	return (std::find(l.begin(),l.end(),i) != l.end());
}

inline vector<string> split(string str,string pattern)
{
	string::size_type pos;
	vector<string> result;
	str+=pattern;
	int size=str.size();
	for(int i=0; i<size; i++)
	{
		pos=str.find(pattern,i);
		if(pos<size)
		{
			string s=str.substr(i,pos-i);
			result.push_back(s);
			i=pos+pattern.size()-1;
		}
	}
	return result;
}

inline ScalarType clamp(ScalarType x, ScalarType a, ScalarType b) {
	return ((x<a)?a:((x>b)?b:x));
}
//zhiping luo
inline Quaternion AccurateAverageQuaternion(const Quaternion * q, int m_numQuaternions){
	Quaternion averageRotation;	
	Matrix44 M;
	M.setConstant(4,4,0);
	for(int i=0;i<m_numQuaternions;i++)
	{
		Quaternion r = q[i];
		Vector4 rev(r.x(),r.y(),r.z(),r.w());
		Matrix44 m = rev*rev.transpose();
		M += m;
	}
	M *= 1.0f/(float)m_numQuaternions;
	Eigen::EigenSolver<Matrix44> es(M);
	vector<ScalarType> eigenvalues;
	int num_eigens = es.eigenvalues().rows();
	for(int i=0;i<num_eigens;i++)
	{
		eigenvalues.push_back(es.eigenvalues()[i].real());
	}
	std::sort(eigenvalues.begin(),eigenvalues.end());
	for(int i=0;i<num_eigens;i++)
	{
		if(es.eigenvalues()[i].real() == eigenvalues[num_eigens-1])
		{
            Vector4 vec = es.eigenvectors().col(i).real();
			averageRotation.w() = vec[3];
			averageRotation.x() = vec[0];
			averageRotation.y() = vec[1];
			averageRotation.z() = vec[2];
			break;
		}
	}
	return averageRotation;
}
#endif