const float LOG2 = 1.442695;

uniform vec3		CameraPosition;  // Position of main position
uniform float		WaveHeight;

uniform vec4		WaterColor;
uniform float		ColorBlendFactor;

uniform sampler2D	WaterBump; //coverage
uniform sampler2D	RefractionMap; //coverage
uniform sampler2D	ReflectionMap; //coverage

uniform bool		FogEnabled;
uniform int			FogMode;

varying vec2 bumpMapTexCoord;
varying vec3 refractionMapTexCoord;
varying vec3 reflectionMapTexCoord;
varying vec3 position3D;
	
void main()
{
	//bump color
	vec4 bumpColor = texture2D(WaterBump, bumpMapTexCoord);
	vec2 perturbation = WaveHeight * (bumpColor.rg - 0.5);
	
	//refraction
	vec2 ProjectedRefractionTexCoords = clamp(refractionMapTexCoord.xy / refractionMapTexCoord.z + perturbation, 0.0, 1.0);
	//calculate final refraction color
	vec4 refractiveColor = texture2D(RefractionMap, ProjectedRefractionTexCoords );
	
	//reflection
	vec2 ProjectedReflectionTexCoords = clamp(reflectionMapTexCoord.xy / reflectionMapTexCoord.z + perturbation, 0.0, 1.0);
	//calculate final reflection color
	vec4 reflectiveColor = texture2D(ReflectionMap, ProjectedReflectionTexCoords );

	//fresnel
	vec3 eyeVector = normalize(CameraPosition - position3D);
	vec3 upVector = vec3(0.0, 1.0, 0.0);
	
	//fresnel can not be lower than 0
	float fresnelTerm = max( dot(eyeVector, upVector), 0.0 );
	
	float fogFactor = 1.0;
	
	if (FogEnabled)
	{
		float z = gl_FragCoord.z / gl_FragCoord.w;

		if (FogMode == 1) //exp
		{
			float fogFactor = exp2(-gl_Fog.density * z * LOG2);
			fogFactor = clamp(fogFactor, 0.0, 1.0);
		}
		else if (FogMode == 0) //linear
		{
			fogFactor = (gl_Fog.end - z) / (gl_Fog.end - gl_Fog.start);
		}
		else if (FogMode == 2) //exp2
		{
			float fogFactor = exp2(-gl_Fog.density * gl_Fog.density * z * z * LOG2);
			fogFactor = clamp(fogFactor, 0.0, 1.0);
		}
	}
	
	vec4 combinedColor = refractiveColor * fresnelTerm + reflectiveColor * (1.0 - fresnelTerm);
	
	vec4 finalColor = ColorBlendFactor * WaterColor + (1.0 - ColorBlendFactor) * combinedColor;
	
	gl_FragColor = mix(gl_Fog.color, finalColor, fogFactor );
}
