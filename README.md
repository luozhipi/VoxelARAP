![Alt text](/voxARAP.bmp)


Volumetric As-Rigid-As-Possible surface deformation/modeling with voxels. This program provides a solution for computing volumetric
deformations using as-rigid-as-possible energy minimization, and then the arget surface deformations via computing various space deformations.

Core:
1: As-rigid-as-possible energy minimization
2: Space deformations including RBF embedding

Dependencies:
1: CGAL 4.4
2: embree
3: Eigen

you can copy their head files into the `Include` directories.

usage:
1: ALT+E to create point handles
2:ALT+T to translate point handels for deformation

Based on the point handels, this program is easily extended to blend skinning+ARAP, the results from skinning can be used as initial 
guess of the ARAP iterative minimization, also provides a way as rigging to manipulate the voxel grid, in turn the mesh object.

acknowledgement:
Many thanks to Olga Diamanti and Alec Jacobson for sharing their knowledge, by the basic framework of OpenGL viewer for instance.
