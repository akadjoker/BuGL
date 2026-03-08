#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uLightMVP;

out vec3 vNormal;
out vec3 vWorldPos;
out vec4 vLightClipPos;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vLightClipPos = uLightMVP * vec4(aPos, 1.0);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
