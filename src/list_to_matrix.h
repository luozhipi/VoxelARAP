#ifndef IGL_LIST_TO_MATRIX_H
#define IGL_LIST_TO_MATRIX_H

#include <vector>
#include <cassert>
#include <cstdio>

#include <Eigen/Dense>

namespace igl
{
  // Convert a list (std::vector) of row vectors of the same length to a matrix
  // Template: 
  //   T  type that can be safely cast to type in Mat via '='
  //   Mat  Matrix type, must implement:
  //     .resize(m,n)
  //     .row(i) = Row
  // Inputs:
  //   V  a m-long list of vectors of size n
  // Outputs:
  //   M  an m by n matrix
  // Returns true on success, false on errors
  template <typename T, class Mat>
  inline bool list_to_matrix(const std::vector<std::vector<T > > & V,Mat & M);
  // Vector wrapper
  template <typename T, class Mat>
  inline bool list_to_matrix(const std::vector<T > & V,Mat & M);
}


template <typename T, class Mat>
inline bool igl::list_to_matrix(const std::vector<std::vector<T > > & V,Mat & M)
{
  // number of rows
  int m = V.size();
  if(m == 0)
  {
    fprintf(stderr,"Error: list_to_matrix() list is empty()\n");
    return false;
  }
  int n = 3;
  // Resize output
  M.resize(m,n);

  // Loop over rows
  for(int i = 0;i<m;i++)
  {
    // Loop over cols
    for(int j = 0;j<n;j++)
    {
      M(i,j) = V[i][j];
    }
  }

  return true;
}

template <typename T, class Mat>
inline bool igl::list_to_matrix(const std::vector<T > & V,Mat & M)
{
  // number of rows
  int m = V.size();
  if(m == 0)
  {
    fprintf(stderr,"Error: list_to_matrix() list is empty()\n");
    return false;
  }
  // Resize output
  M.resize(m,1);

  // Loop over rows
  for(int i = 0;i<m;i++)
  {
    M(i) = V[i];
  }

  return true;
}


#endif
