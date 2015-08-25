// Used for shadow lookup
varying vec4 ShadowCoord;

varying vec3 N;
varying vec3 v;

void main()
{
   ShadowCoord= gl_TextureMatrix[7] * gl_Vertex;
  
   v = vec3(gl_ModelViewMatrix * gl_Vertex);       
   N = normalize(gl_NormalMatrix * gl_Normal);
   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;  
   gl_FrontColor = gl_Color;
}

