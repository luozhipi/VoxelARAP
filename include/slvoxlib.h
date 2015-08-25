// SL 2012-12-30
#pragma once

/* call once to initialize */
void slvox_init(int voxRes,int hRes = 4096,bool instrument = false); 

/* call once to terminate */
void slvox_terminate(); 

/* call to clear the content of the voxelizer */
void slvox_clear(); 

/* call to start drawing an object into the voxelizer (may be called multiple times, each add voxels) */
void slvox_begin( const float *viewMatrix = 0 /*optional*/, const float *projMatrix = 0 /*optional*/, float znear=0, float zfar=0  );

/* call to change model matrix */
void slvox_set_model_matrix( const float *modelMatrix );

/* call to stop drawing an object */
void slvox_end();

/* retrieve voxels from the GPU (slow!) */
void slvox_getvoxels( unsigned char *voxels /*3D arrays of size voxRes x voxRes x voxRes*/ );

/* change voxel resolution (erases all prior voxels) */
void slvox_set_voxel_res(unsigned int vRes);

/* set code to compute per-voxel data (has to be called before init) */
void slvox_set_voxel_data_code(const char *str);

/* get low level pointer to internal tables -- ADVANCED ONLY */

enum e_BufferName { 
  Hash      = 0, 
  MaxAge    = 1, 
  DepthCull = 2, 
  VoxelData = 3,
  NumIter   = 4,
  NumCulled = 5
};

void *slvox_get_table(e_BufferName bufname);

/* get voxelizer shader -- ADVANCED ONLY */
void *slvox_get_shader();

/* get offset table -- ADVANCED ONLY */
const unsigned int *slvox_get_offsets( int *num_offsets );

/* return number of fragment in hash -- SLOW */
unsigned int slvox_get_num_fragments();

/* return hash density -- SLOW */
float slvox_get_hash_density();

/* return max age -- SLOW */
int  slvox_get_max_age();

/* get tweak bar -- ADVANCED ONLY */
void *slvox_get_shader_tweak_bar();

enum e_Mode { 
  _32Bits          = 0, 
  _64Bits          = 1, 
  _64BitsEarlyCull = 2, 
};

/* select which mode is used
0: 32 bits (no data)
1: 64 bits (data)
2: 64 bits early cull (transparency)
*/
void slvox_select_mode(e_Mode mode);
