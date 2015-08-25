#ifndef IGL_READOBJ_H
#define IGL_READOBJ_H

#include <Eigen/Core>
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>
#include <fstream>
#include "list_to_matrix.h"

namespace igl 
{
  //! Read a mesh from an ascii obj file
  // Inputs:
  //   str  path to .obj file
  // Outputs:
  //   V  eigen matrix #V by 3
  //   F  eigen matrix #F by 3
  //
  // KNOWN BUG: This only knows how to read *triangle* meshes. It will probably
  // crash or give garbage on anything else.
  //
  // KNOWN BUG: This only knows how to face lines without normal or texture
  // indices. It will probably crash or give garbage on anything else.
  template <typename DerivedV, typename DerivedF>
  inline bool readOBJ(
    const std::string str,
    Eigen::PlainObjectBase<DerivedV>& V,
    Eigen::PlainObjectBase<DerivedF>& F);

  template <typename DerivedV, typename DerivedF>
  inline bool readOBJ(
    const std::string str,
    Eigen::PlainObjectBase<DerivedV>& V,
    Eigen::PlainObjectBase<DerivedF>& F,
    Eigen::PlainObjectBase<DerivedV>& N);

  template <typename DerivedV, typename DerivedF, typename DerivedT>
  inline bool readOBJ(
    const std::string str,
    Eigen::PlainObjectBase<DerivedV>& V,
    Eigen::PlainObjectBase<DerivedF>& F,Eigen::PlainObjectBase<DerivedF>& FT,
	Eigen::PlainObjectBase<DerivedT>& T, bool & has_texture, std::string & mtl_file);
}

template <typename DerivedV, typename DerivedF>
inline bool igl::readOBJ(
                             const std::string str,
                             Eigen::PlainObjectBase<DerivedV>& V,
                             Eigen::PlainObjectBase<DerivedF>& F)
{
  std::vector<std::vector<double> > vV;
  std::vector<std::vector<int> > vF;

  // Open file, and check for error
  FILE * obj_file = fopen(str.c_str(),"r");
  if(NULL==obj_file)
  {
    fprintf(stderr,"IOError: %s could not be opened...\n",
            str.c_str());
    return false;
  }
  // File open was succesfull so clear outputs
  vV.clear();
  vF.clear();
  
  // variables an constants to assist parsing the .obj file
  // flag for whether vertex texture coordinates exist in file
  bool has_texture = false;
  // flag for whether vertex normals exist in file
  bool has_normals = false;
  // Constant strings to compare against
  std::string v("v");
  std::string vn("vn");
  std::string vt("vt");
  std::string f("f");
  std::string tic_tac_toe("#");
#ifndef LINE_MAX
#  define LINE_MAX 2048
#endif
  
  char line[LINE_MAX];
  int line_no = 1;
  while (fgets(line, LINE_MAX, obj_file) != NULL) 
  {
    char type[LINE_MAX];
    // Read first word containing type
    if(sscanf(line, "%s",type) == 1)
    {
      // Get pointer to rest of line right after type
      char * l = &line[strlen(type)];
      if(type == v)
      {
        double x[4];
        int count = 
        sscanf(l,"%lf %lf %lf %lf\n",&x[0],&x[1],&x[2],&x[3]);
        if(count != 3)
        {
          fprintf(stderr, 
                  "Error: readOBJ() vertex on line %d should have 3 coordinates", 
                  line_no);
          fclose(obj_file);
          return false;
        }
        std::vector<double > vertex(count);
        for(int i = 0;i<count;i++)
        {
          vertex[i] = x[i];
        }
        vV.push_back(vertex);
      }else if(type == vn)
      {
        has_normals = true;
        double x[3];
        int count = 
        sscanf(l,"%lf %lf %lf\n",&x[0],&x[1],&x[2]);
        if(count != 3)
        {
          fprintf(stderr, 
                  "Error: readOBJ() normal on line %d should have 3 coordinates", 
                  line_no);
          fclose(obj_file);
          return false;
        }
      }else if(type == vt)
      {
        has_texture = true;
        double x[3];
        int count = 
        sscanf(l,"%lf %lf %lf\n",&x[0],&x[1],&x[2]);
        if(count != 2 && count != 3)
        {
          fprintf(stderr, 
                  "Error: readOBJ() vertex on line %d should have 2 or 3 coordinates", 
                  line_no);
          fclose(obj_file);
          return false;
        }
      }else if(type == f)
      {
        std::vector<int > f;
        // Read each "word" after type
        char word[LINE_MAX];
        int offset;
        while(sscanf(l,"%s%n",word,&offset) == 1)
        {
          // adjust offset
          l += offset;
          // Process word
          unsigned int i,it,in;
          if(sscanf(word,"%u/%u/%u",&i,&it,&in) == 3)
          {
            f.push_back(i-1);
          }else if(sscanf(word,"%u/%u",&i,&it) == 2)
          {
            f.push_back(i-1);
          }else if(sscanf(word,"%u//%u",&i,&in) == 2)
          {
            f.push_back(i-1);
          }else if(sscanf(word,"%u",&i) == 1)
          {
            f.push_back(i-1);
          }else
          {
            fprintf(stderr,
                    "Error: readOBJ() face on line %d has invalid element format\n",
                    line_no);
            fclose(obj_file);
            return false;
          }
        }
        if(f.size()>0)
        {
          // No matter what add each type to lists so that lists are the
          // correct lengths
          vF.push_back(f);
        }else
        {
          fprintf(stderr,
                  "Error: readOBJ() face on line %d has invalid format\n", line_no);
          fclose(obj_file);
          return false;
        }
      }else if(strlen(type) >= 1 && type[0] == '#')
      {
        //ignore comments
      }else
      {
        //ignore any other lines
       // fprintf(stderr,
          //      "Warning: readOBJ() ignored non-comment line %d:\n  %s",
           //     line_no,
           //     line);
      }
    }else
    {
      // ignore empty line
    }
    line_no++;
  }
  fclose(obj_file);
  
  bool V_rect = igl::list_to_matrix(vV,V);
  if(!V_rect)
  {
    // igl::list_to_matrix(vV,V) already printed error message to std err
    return false;
  }
  bool F_rect = igl::list_to_matrix(vF,F);
  if(!F_rect)
  {
    // igl::list_to_matrix(vF,F) already printed error message to std err
    return false;
  }
  // Legacy
  if(F.cols() != 3)
  {
    fprintf(stderr,
            "Error: readOBJ(filename,V,F) is meant for reading triangle-only"
            " meshes. This mesh has faces all with size %d. See readOBJ.h for other"
            " options.\n",
            (int)F.cols());
    return false;
  }
  return true;
}


template <typename DerivedV, typename DerivedF>
inline bool igl::readOBJ(
                             const std::string str,
                             Eigen::PlainObjectBase<DerivedV>& V,
                             Eigen::PlainObjectBase<DerivedF>& F,
                             Eigen::PlainObjectBase<DerivedV>& N)
{
  std::vector<std::vector<double> > vV;
  std::vector<std::vector<int> > vF;
  std::vector<std::vector<double> > vN;

  // Open file, and check for error
  FILE * obj_file = fopen(str.c_str(),"r");
  if(NULL==obj_file)
  {
    fprintf(stderr,"IOError: %s could not be opened...\n",
            str.c_str());
    return false;
  }
  // File open was succesfull so clear outputs
  vV.clear();
  vF.clear();
  vN.clear();
  
  // variables an constants to assist parsing the .obj file
  // flag for whether vertex texture coordinates exist in file
  bool has_texture = false;
  // flag for whether vertex normals exist in file
  bool has_normals = false;
  // Constant strings to compare against
  std::string v("v");
  std::string vn("vn");
  std::string vt("vt");
  std::string f("f");
  std::string tic_tac_toe("#");
#ifndef LINE_MAX
#  define LINE_MAX 2048
#endif
  
  char line[LINE_MAX];
  int line_no = 1;
  while (fgets(line, LINE_MAX, obj_file) != NULL) 
  {
    char type[LINE_MAX];
    // Read first word containing type
    if(sscanf(line, "%s",type) == 1)
    {
      // Get pointer to rest of line right after type
      char * l = &line[strlen(type)];
      if(type == v)
      {
        double x[4];
        int count = 
        sscanf(l,"%lf %lf %lf %lf\n",&x[0],&x[1],&x[2],&x[3]);
        if(count != 3 && count != 4)
        {
          fprintf(stderr, 
                  "Error: readOBJ() vertex on line %d should have 3 or 4 coordinates", 
                  line_no);
          fclose(obj_file);
          return false;
        }
        std::vector<double > vertex(count);
        for(int i = 0;i<count;i++)
        {
          vertex[i] = x[i];
        }
        vV.push_back(vertex);
      }else if(type == vn)
      {
        has_normals = true;
        double x[3];
        int count = 
        sscanf(l,"%lf %lf %lf\n",&x[0],&x[1],&x[2]);
        if(count != 3)
        {
          fprintf(stderr, 
                  "Error: readOBJ() normal on line %d should have 3 coordinates", 
                  line_no);
          fclose(obj_file);
          return false;
        }
        std::vector<double > normal(count);
        for(int i = 0;i<count;i++)
        {
          normal[i] = x[i];
        }
        vN.push_back(normal);
      }else if(type == vt)
      {
        has_texture = true;
        double x[3];
        int count = 
        sscanf(l,"%lf %lf %lf\n",&x[0],&x[1],&x[2]);
        if(count != 2 && count != 3)
        {
          fprintf(stderr, 
                  "Error: readOBJ() vertex on line %d should have 2 or 3 coordinates", 
                  line_no);
          fclose(obj_file);
          return false;
        }
      }else if(type == f)
      {
        std::vector<int > f;
        // Read each "word" after type
        char word[LINE_MAX];
        int offset;
        while(sscanf(l,"%s%n",word,&offset) == 1)
        {
          // adjust offset
          l += offset;
          // Process word
          unsigned int i,it,in;
          if(sscanf(word,"%u/%u/%u",&i,&it,&in) == 3)
          {
            f.push_back(i-1);
          }else if(sscanf(word,"%u/%u",&i,&it) == 2)
          {
            f.push_back(i-1);
          }else if(sscanf(word,"%u//%u",&i,&in) == 2)
          {
            f.push_back(i-1);
          }else if(sscanf(word,"%u",&i) == 1)
          {
            f.push_back(i-1);
          }else
          {
            fprintf(stderr,
                    "Error: readOBJ() face on line %d has invalid element format\n",
                    line_no);
            fclose(obj_file);
            return false;
          }
        }
        if(f.size()>0)
        {
          // No matter what add each type to lists so that lists are the
          // correct lengths
          vF.push_back(f);
        }else
        {
          fprintf(stderr,
                  "Error: readOBJ() face on line %d has invalid format\n", line_no);
          fclose(obj_file);
          return false;
        }
      }else if(strlen(type) >= 1 && type[0] == '#')
      {
        //ignore comments
      }else
      {
        //ignore any other lines
        fprintf(stderr,
                "Warning: readOBJ() ignored non-comment line %d:\n  %s",
                line_no,
                line);
      }
    }else
    {
      // ignore empty line
    }
    line_no++;
  }
  fclose(obj_file);
  
  bool V_rect = igl::list_to_matrix(vV,V);
  if(!V_rect)
  {
    // igl::list_to_matrix(vV,V) already printed error message to std err
    return false;
  }
  bool F_rect = igl::list_to_matrix(vF,F);
  if(!F_rect)
  {
    // igl::list_to_matrix(vF,F) already printed error message to std err
    return false;
  }
  
  if(!vN.empty())
  {
    bool VN_rect = igl::list_to_matrix(vN,N);
    if(!VN_rect)
    {
      // igl::list_to_matrix(vV,V) already printed error message to std err
      return false;
    }
  }

  // Legacy
  if(F.cols() != 3)
  {
    fprintf(stderr,
            "Error: readOBJ(filename,V,F) is meant for reading triangle-only"
            " meshes. This mesh has faces all with size %d. See readOBJ.h for other"
            " options.\n",
            (int)F.cols());
    return false;
  }
  return true;
}

template <typename DerivedV, typename DerivedF, typename DerivedT>
inline bool igl::readOBJ(
                             const std::string str,
                             Eigen::PlainObjectBase<DerivedV>& V,
                             Eigen::PlainObjectBase<DerivedF>& F,Eigen::PlainObjectBase<DerivedF>& FT,
							 Eigen::PlainObjectBase<DerivedT>& T, bool & has_texture,std::string & mtl_file)
{
  std::vector<std::vector<double> > vV;
  std::vector<std::vector<int> > vF;
  std::vector<std::vector<int> > vFT;
  std::vector<std::vector<double> > vT;

  // Open file, and check for error
  FILE * obj_file = fopen(str.c_str(),"r");
  if(NULL==obj_file)
  {
    fprintf(stderr,"IOError: %s could not be opened...\n",
            str.c_str());
    return false;
  }
  // File open was succesfull so clear outputs
  vV.clear();
  vF.clear();
  vT.clear();
  
  // variables an constants to assist parsing the .obj file
  // flag for whether vertex texture coordinates exist in file
  has_texture = false;
  // Constant strings to compare against
  std::string v("v");
  std::string vn("vn");
  std::string vt("vt");
  std::string f("f");
  std::string mtl("mtllib");
  std::string tic_tac_toe("#");
#ifndef LINE_MAX
#  define LINE_MAX 2048
#endif
  
  char line[LINE_MAX];
  int line_no = 1;
  while (fgets(line, LINE_MAX, obj_file) != NULL) 
  {
    char type[LINE_MAX];
    // Read first word containing type
    if(sscanf(line, "%s",type) == 1)
    {
      // Get pointer to rest of line right after type
      char * l = &line[strlen(type)];
      if(type == v)
      {
        double x[4];
        int count = 
        sscanf(l,"%lf %lf %lf %lf\n",&x[0],&x[1],&x[2],&x[3]);
        if(count != 3 && count != 4)
        {
          fprintf(stderr, 
                  "Error: readOBJ() vertex on line %d should have 3 or 4 coordinates", 
                  line_no);
          fclose(obj_file);
          return false;
        }
        std::vector<double > vertex(count);
        for(int i = 0;i<count;i++)
        {
          vertex[i] = x[i];
        }
        vV.push_back(vertex);
      }else if(type == vn)
      {
      }else if(type == vt)
      {
        has_texture = true;
        double x[3];
        int count = 
        sscanf(l,"%lf %lf %lf\n",&x[0],&x[1],&x[2]);
        if(count != 2 && count != 3)
        {
          fprintf(stderr, 
                  "Error: readOBJ() vertex on line %d should have 2 or 3 coordinates", 
                  line_no);
          fclose(obj_file);
          return false;
        }
        std::vector<double > tex(count);
        for(int i = 0;i<count;i++)
        {
          tex[i] = x[i];
        }
        vT.push_back(tex);
      }
	  else if(type == f)
      {
        std::vector<int > f;
		std::vector<int > ft;
        // Read each "word" after type
        char word[LINE_MAX];
        int offset;
        while(sscanf(l,"%s%n",word,&offset) == 1)
        {
          // adjust offset
          l += offset;
          // Process word
          unsigned int i,it,in;
          if(sscanf(word,"%u/%u/%u",&i,&it,&in) == 3)
          {
            f.push_back(i-1);
			ft.push_back(it-1);
          }else if(sscanf(word,"%u/%u",&i,&it) == 2)
          {
            f.push_back(i-1);
			ft.push_back(it-1);
          }else if(sscanf(word,"%u//%u",&i,&in) == 2)
          {
            f.push_back(i-1);
          }else if(sscanf(word,"%u",&i) == 1)
          {
            f.push_back(i-1);
          }else
          {
            fprintf(stderr,
                    "Error: readOBJ() face on line %d has invalid element format\n",
                    line_no);
            fclose(obj_file);
            return false;
          }
        }
        if(f.size()>0)
        {
          // No matter what add each type to lists so that lists are the
          // correct lengths
          vF.push_back(f);
        }else
        {
          fprintf(stderr,
                  "Error: readOBJ() face on line %d has invalid format\n", line_no);
          fclose(obj_file);
          return false;
        }
		if(ft.size()>0)
		{
			vFT.push_back(ft);
		}
      }
	  else if(type == mtl)
	  {
		string sLine = line;
		int from = sLine.find(mtl)+7;
		mtl_file = sLine.substr(from, sLine.size()-from-1);
	  }
	  else if(strlen(type) >= 1 && type[0] == '#')
      {
      }else
      {
      }
    }else
    {
    }
    line_no++;
  }
  fclose(obj_file);
  
  bool V_rect = igl::list_to_matrix(vV,V);
  if(!V_rect)
  {
    return false;
  }
  bool F_rect = igl::list_to_matrix(vF,F);
  if(!F_rect)
  {
    return false;
  }
  if(!vFT.empty())
  {
    bool VFT_rect = igl::list_to_matrix(vFT,FT);
    if(!VFT_rect)
    {
      return false;
    }
  }
  if(!vT.empty())
  {
	  bool VT_rect = igl::list_to_matrix(vT, T);
	  if(!VT_rect)
	  {
		  return false;
	  }
  }
  // Legacy
  if(F.cols() != 3)
  {
    fprintf(stderr,
            "Error: readOBJ(filename,V,F) is meant for reading triangle-only"
            " meshes. This mesh has faces all with size %d. See readOBJ.h for other"
            " options.\n",
            (int)F.cols());
    return false;
  }
  return true;
}

#endif
