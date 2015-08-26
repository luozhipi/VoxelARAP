![Alt text](/voxARAP.bmp)


Volumetric As-Rigid-As-Possible surface deformation/modeling with voxels. This `visual studio 2010` program provides a solution for computing volumetric
deformations using as-rigid-as-possible energy minimization, and then the target surface deformations via computing various space deformations.

Core:

1: As-rigid-as-possible energy minimization

2: Space deformations including RBF embedding, vertex blending

Dependencies:

1: CGAL 4.4

2: embree

3: Eigen

4: boost 1.42

you can copy their head files into the `Include` directories.

usage:

1: ALT+E to create point handles

2: ALT+T to translate point handels for manipulation

Based on the point handels, this program is easily extended to blend_skinning+ARAP. The results from skinning can be used as an initial guess of the ARAP iterative minimization, also providing a way as rigging to manipulate the voxel grid, in turn the mesh object.

acknowledgement:
Many thanks to Olga Diamanti and Alec Jacobson for sharing their knowledge, by the basic framework of OpenGL viewer for instance.
