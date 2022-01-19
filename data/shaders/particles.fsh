
in vec3 vColor;
in vec3 vWorldPos;
in float vBlend;

out vec3 fColor;
out vec4 fBright;

vec3 saturate(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }

vec3 desaturate(vec3 color, float strength)
{
    float lum = dot(color, vec3(0.3, 0.59, 0.11));

    return mix(color, vec3(lum), strength);
}

void main()
{
    fColor = mix(vColor * 0.2, vColor, vBlend);

    fBright = vec4(fColor, 1.0);        
}