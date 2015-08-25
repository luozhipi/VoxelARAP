#ifndef __READMESH_H
#define __READMESH_H

#include "readOBJ.h"
#include "MIS_inc.h"
#include "texture.h"

inline void readMeshfile(string filename, PointMatrixType & vertices, FaceMatrixType & faces, FaceMatrixType & uvfaces, UVMatrixType & uvs,bool & has_texture, string & mtl_file)
{
	size_t last_dot = filename.rfind('.');
	if(last_dot == std::string::npos)
		printf("Error: No file extension found in %s.\n",filename);

	string extension = filename.substr(last_dot+1);
	if(extension == "obj" || extension =="OBJ") {
		if(!igl::readOBJ(filename, vertices, faces, uvfaces, uvs, has_texture, mtl_file))
			printf("Error: Unreadable mesh file.\n");
	}
	else {
		printf("Error: %s is not a recognized file type.\n",extension.c_str());
	}
}

inline bool readMaterial(CTexture & tDiffuseTexture, string & file)
{
	string mtlfile = string(TEXTURE_DIR).append(file);
		// Open file, and check for error
	FILE * mtl_file = fopen(mtlfile.c_str(),"r");
	if(NULL==mtl_file)
	{
	fprintf(stderr,"IOError: %s could not be opened...\n",
			mtlfile.c_str());
	return false;
	}
	string map_Kd("map_Kd");
#ifndef LINE_MAX
#  define LINE_MAX 2048
#endif
	char line[LINE_MAX];
	while (fgets(line, LINE_MAX, mtl_file) != NULL) 
	{
		char type[LINE_MAX];
		// Read first word containing type
		if(sscanf(line, "%s",type) == 1)
		{
			// Get pointer to rest of line right after type
			char * l = &line[strlen(type)];
			if(type == map_Kd)
			{
				string sLine = line;
				int from = sLine.find(map_Kd)+7;
				string sTextureName = string(TEXTURE_DIR).append(sLine.substr(from, sLine.size()-from-1));
				tDiffuseTexture.loadTexture2D(sTextureName, true);
				tDiffuseTexture.setFiltering(TEXTURE_FILTER_MAG_BILINEAR, TEXTURE_FILTER_MIN_NEAREST_MIPMAP);
			}

		}
	}
	fclose(mtl_file);
	return true;
}
#endif