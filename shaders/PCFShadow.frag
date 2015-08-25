uniform sampler2DShadow ShadowMap;

varying vec4 ShadowCoord;
varying vec3 N;
varying vec3 v;

// This define the value to move one pixel left or right
uniform float xPixelOffset ;

// This define the value to move one pixel up or down
uniform float yPixelOffset ;

float lookup( vec2 offSet)
{
	// Values are multiplied by ShadowCoord.w because shadow2DProj does a W division for us.
	vec4 tc = ShadowCoord + vec4(offSet.x * xPixelOffset * ShadowCoord.w, offSet.y * yPixelOffset * ShadowCoord.w, 0.05, 0.0);
	//if(tc.x / tc.w < 0.01 || tc.x / tc.w > 0.99) return 1;
	//if(tc.y / tc.w < 0.01 || tc.y / tc.w > 0.99) return 1;
	return shadow2DProj(ShadowMap, tc).w;
}

void main()
{	

	// Used to lower moir?pattern and self-shadowing
	//shadowCoordinateWdivide.z += ;
	
	
	float shadow = 1;
	
	// Avoid counter shadow
	if (ShadowCoord.w > 1.0)
	{
		// Simple lookup, no PCF
					//shadow = lookup(vec2(0.0,0.0));
		




		// 8x8 kernel PCF
					
					float x,y;
					for (y = -3.5 ; y <=3.5 ; y+=1.0)
						for (x = -3.5 ; x <=3.5 ; x+=1.0)
							shadow += lookup(vec2(x,y));
					
					shadow /= 64.0 ;
					
		




		// 8x8 PCF wide kernel (step is 10 instead of 1)
					/*
					float x,y;
					for (y = -30.5 ; y <=30.5 ; y+=10.0)
						for (x = -30.5 ; x <=30.5 ; x+=10.0)
							shadow += lookup(vec2(x,y));
					
					shadow /= 64.0 ;
					*/
	



		// 4x4 kernel PCF
		/*
		float x,y;
		for (y = -1.5 ; y <=1.5 ; y+=1.0)
			for (x = -1.5 ; x <=1.5 ; x+=1.0)
				shadow += lookup(vec2(x,y));
		
		shadow /= 16.0 ;
		*/
		


		// 4x4  PCF wide kernel (step is 10 instead of 1)
					/*
					float x,y;
					for (y = -10.5 ; y <=10.5 ; y+=10.0)
						for (x = -10.5 ; x <=10.5 ; x+=10.0)
							shadow += lookup(vec2(x,y));
					
					shadow /= 16.0 ;
					*/
		
		// 4x4  PCF dithered
					/*
					// use modulo to vary the sample pattern
					vec2 o = mod(floor(gl_FragCoord.xy), 2.0);
				
					shadow += lookup(vec2(-1.5, 1.5) + o);
					shadow += lookup(vec2( 0.5, 1.5) + o);
					shadow += lookup(vec2(-1.5, -0.5) + o);
					shadow += lookup(vec2( 0.5, -0.5) + o);
					shadow *= 0.25 ;
					*/
	}

	vec3 L = normalize(gl_LightSource[0].position.xyz - v);
	vec3 E = normalize(-v);
	vec3 R = normalize(-reflect(L,N));

	//calculate Ambient Term:
	vec4 Iamb = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;

	//calculate Diffuse Term:
	vec4 Idiff = gl_LightSource[0].diffuse * gl_Color
					* max(dot(N,L), 0.0);

	// calculate Specular Term:
	//vec4 Ispec = gl_LightSource[0].specular * gl_FrontMaterial.specular
	//				* pow(max(dot(R,E),0.0),gl_FrontMaterial.shininess);

	// write Total Color:
	gl_FragColor =  clamp(clamp(shadow+0.4,0.0,1.0) * (Iamb + Idiff),0.0,1.0);
  
}
