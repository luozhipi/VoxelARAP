varying vec3 N;
varying vec3 v;
uniform sampler2D sampler;
void main (void)
{
	vec3 L = normalize(gl_LightSource[0].position.xyz - v);
	vec3 E = normalize(-v);
	vec3 R = normalize(-reflect(L,N));
	vec4 Iamb = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 Idiff = gl_LightSource[0].diffuse * gl_Color
					* max(dot(N,L),0.0);
	vec4 Ispec = gl_LightSource[0].specular * gl_FrontMaterial.specular
					* pow(max(dot(R,E),0.0),gl_FrontMaterial.shininess);
	vec4 vTexColor = texture2D(sampler, vec2(gl_TexCoord[0])).bgra;
	// write Total Color:
	gl_FragColor =clamp(Iamb + Ispec,0.0,1.0)*vTexColor;
}