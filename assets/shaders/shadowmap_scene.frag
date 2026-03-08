#version 330 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec4 vLightClipPos;

uniform sampler2D uShadowMap;
uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform vec3 uBaseColor;

out vec4 FragColor;

float shadowFactor(vec4 lightClipPos, vec3 normal, vec3 lightDir)
{
    vec3 proj = lightClipPos.xyz / lightClipPos.w;
    proj = proj * 0.5 + 0.5;

    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0 || proj.z > 1.0)
    {
        return 0.0;
    }

    float bias = max(0.0012 * (1.0 - dot(normal, lightDir)), 0.00025);
    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;

    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            vec2 uv = proj.xy + vec2(x, y) * texel;
            float depth = texture(uShadowMap, uv).r;
            shadow += (proj.z - bias > depth) ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0);

    float shadow = shadowFactor(vLightClipPos, N, L);

    vec3 ambient = 0.22 * uBaseColor;
    vec3 direct = (1.0 - shadow) * (uBaseColor * diff * 0.90 + vec3(1.0) * spec * 0.25);

    FragColor = vec4(ambient + direct, 1.0);
}
