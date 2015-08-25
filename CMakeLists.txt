cmake_minimum_required(VERSION 2.6)
project(VoxARAP)

SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS_RELEASE})
SET(EXE_NAME ${PROJECT_NAME})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin)

INCLUDE_DIRECTORIES(
               ${CMAKE_CURRENT_SOURCE_DIR} 
			   include
			   include/FreeImage/Dist 
			   include/CGAL4.4 
			   include/SOIL 
			   include/embree 
			   include/boost_1_42/boost
			   include/CGAL4.4/auxiliary/gmp/include
			   )
LINK_DIRECTORIES(
               ${CMAKE_CURRENT_SOURCE_DIR}/lib 
			   ${CMAKE_CURRENT_SOURCE_DIR}/include/boost_1_42/lib
			   )			   

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/src source_files)
SET(dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
file( GLOB header_files "${dir}/*.h" )
SET(solution ${source_files} ${header_files})
ADD_EXECUTABLE(${EXE_NAME} ${solution})
SET(
 LIBRARIES
 "AntTweakBar"
 "glut32"
 "glew32"
 "sys"
 "embree"
 "slvoxlib"
 "slcrystal"
 "glux"
 "LibSL"
 "SOIL"
 "LibSL_gl4"
 "opengl32"
 "winmm"
 "psapi"
 "shlwapi"
 "libmpfr-4"
 "libgmp-10"
 "CGAL_Core-vc100-mt-4.4"
 "CGAL-vc100-mt-4.4"
 "FreeImage"
 "user32"
 )
TARGET_LINK_LIBRARIES(${EXE_NAME} ${LIBRARIES})