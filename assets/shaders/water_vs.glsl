//uniform mat4	View;
uniform mat4	WorldViewProj;  // World * View * Projection transformation
uniform mat4	WorldReflectionViewProj;  // World * Reflection View * Projection transformation

uniform float	WaveLength;

uniform float	Time;
uniform float	WindForce;
uniform vec2	WindDirection;

// Vertex shader output structure
varying vec2 bumpMapTexCoord;
varying vec3 refractionMapTexCoord;
varying vec3 reflectionMapTexCoord;
varying vec3 position3D;

void main()
{
	//color = gl_Color;

	// transform position to clip space
	vec4 pos = WorldViewProj * gl_Vertex;
	gl_Position = pos;
	
	// calculate vawe coords
	bumpMapTexCoord = gl_MultiTexCoord0.xy / WaveLength + Time * WindForce * WindDirection;

	// refraction texcoords
	refractionMapTexCoord.x = 0.5 * (pos.w + pos.x);
	refractionMapTexCoord.y = 0.5 * (pos.w + pos.y);
	refractionMapTexCoord.z = pos.w;
								
	// reflection texcoords
	pos = WorldReflectionViewProj * gl_Vertex;
	reflectionMapTexCoord.x = 0.5 * (pos.w + pos.x);
	reflectionMapTexCoord.y = 0.5 * (pos.w + pos.y);
	reflectionMapTexCoord.z = pos.w;
	
	// position of the vertex
	position3D = gl_Vertex.xyz;
}