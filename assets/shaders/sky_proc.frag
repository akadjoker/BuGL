#version 330 core

in vec3 vDir;

uniform vec3 uSunDir;
uniform vec3 uSkyTop;
uniform vec3 uSkyHorizon;
uniform vec3 uSkyGround;

out vec4 FragColor;

float sat(float x)
{
    return clamp(x, 0.0, 1.0);
}

void main()
{
    vec3 dir = normalize(vDir);
    float h = sat(dir.y * 0.5 + 0.5);
    float horizonBand = exp(-abs(dir.y) * 9.0);

    vec3 upper = mix(uSkyHorizon, uSkyTop, smoothstep(0.0, 1.0, h));
    vec3 lower = mix(uSkyGround, uSkyHorizon, smoothstep(0.0, 1.0, h * 2.0));
    vec3 sky = (dir.y >= 0.0) ? upper : lower;

    vec3 sunDir = normalize(uSunDir);
    float sunDot = max(dot(dir, sunDir), 0.0);
    float sunCore = pow(sunDot, 1800.0);
    float sunGlow = pow(sunDot, 64.0);

    vec3 color = sky;
    color += vec3(1.0, 0.90, 0.72) * sunCore * 2.8;
    color += vec3(1.0, 0.75, 0.45) * sunGlow * 0.35;
    color += vec3(0.08, 0.10, 0.14) * horizonBand * 0.35;

    FragColor = vec4(color, 1.0);
}
