#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uLightMVP0;
uniform mat4 uLightMVP1;
uniform mat4 uLightMVP2;

out vec3 vWorldNormal;
out vec3 vWorldTangent;
out vec3 vWorldPos;
out vec2 vUV;
out vec4 vLightClipPos0;
out vec4 vLightClipPos1;
out vec4 vLightClipPos2;
out float vViewDepth;

void main()
{
    mat3 nrmMat = mat3(transpose(inverse(uModel)));
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vec4 viewPos = uView * worldPos;

    vWorldPos = worldPos.xyz;
    vWorldNormal = normalize(nrmMat * aNormal);
    vWorldTangent = normalize(nrmMat * aTangent);
    vUV = aUV;
    vLightClipPos0 = uLightMVP0 * vec4(aPos, 1.0);
    vLightClipPos1 = uLightMVP1 * vec4(aPos, 1.0);
    vLightClipPos2 = uLightMVP2 * vec4(aPos, 1.0);
    vViewDepth = -viewPos.z;

    gl_Position = uMVP * vec4(aPos, 1.0);
}

