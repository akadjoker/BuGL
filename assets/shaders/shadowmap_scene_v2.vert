#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uLightMVP;

out vec3 vWorldNormal;
out vec3 vWorldTangent;
out vec3 vWorldPos;
out vec2 vUV;
out vec4 vLightClipPos;

void main()
{
    mat3 nrmMat = mat3(transpose(inverse(uModel)));
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vWorldNormal = normalize(nrmMat * aNormal);
    vWorldTangent = normalize(nrmMat * aTangent);
    vUV = aUV;
    vLightClipPos = uLightMVP * vec4(aPos, 1.0);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
