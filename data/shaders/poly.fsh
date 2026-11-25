
#include "brdf.glsli"

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
};

struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

#define MAX_POINTLIGHTS 256

layout (std140) uniform PointLightUBO   // Pointlights
{
  PointLight ubPointLights [MAX_POINTLIGHTS];
};

// Textures
uniform sampler2D uTexAlbedo;
uniform samplerCube uSkyCubeMap;
uniform samplerCube uPointShadowMap;
uniform sampler2DShadow uDirShadowMap;

// Time
uniform float uRuntime;
uniform float uGametime;

// Light
uniform bool uEnableAmbientL;
uniform bool uEnableDirectionalL;
uniform bool uEnablePointL;
uniform DirectionalLight uSunLight;
uniform vec3 uAmbientLight;         // Ambient
uniform vec3 uLightPos;             // Shadowing Point Light
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uLightRadius;
uniform int uPointLightCount;       // Check which pointlights are valid

// Misc
uniform uint uPickingID;
uniform uint uAlbedoBiasPacked;

// Render Parameters
uniform bool uRenderNormals;
uniform bool uPointShadows;
uniform bool uDirShadows;

// Coordinates
uniform vec3 uCamPos;
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec2 vTexCoord;

// Transformations
uniform mat4 uShadowViewProj;   // world -> sun shadow map clip space
uniform float uDirShadowTexel;  // edge length of a sun shadow map texel in world units

// Fog of War
uniform bool uFogOfWar;
in float vFogDepth;

// Output Targets
out vec3 fColor;
out vec4 fBright;
out uint fPickID;

// Model properties
uniform float uMetalness;
uniform float uReflectivity;

uniform float uFar;

vec3 fog_color = vec3(0.8, 0.9, 0.0) * 0.8 + vec3(0, 0, 1);

vec4 unpackSaturatedFloat4(uint value)
{
    vec4 res;
    res.w = float((value >> 0) & 0xFF) / 255.f;
    res.z = float((value >> 8) & 0xFF) / 255.f;
    res.y = float((value >> 16) & 0xFF) / 255.f;
    res.x = float((value >> 24) & 0xFF) / 255.f;
    return res;
}

float getLinearAttenuation(float lightDistance, float lightRadius)
{
    return smoothstep(lightRadius, 0.0, lightDistance);
}

float getInverseSquareFalloffAttenuation(float lightDistance, float lightRadius)
{
    const float res = pow2(saturate(1 - pow4(lightDistance / lightRadius))) / (pow2(lightDistance) + 1.0);
    return saturate(res);
}

vec3 shadingDiffuseOrenNayar(vec3 N, vec3 V, vec3 L, float roughness, vec3 albedo) 
{
    float dotVL = dot(L, V);
    float dotLN = dot(L, N);
    float dotNV = dot(N, V);

    float s = dotVL - dotLN * dotNV;
    float t = mix(1.0, max(dotLN, dotNV), step(0.0, s));

    float sigma2 = roughness * roughness;
    vec3 A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));    
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    return albedo * max(0.0, dotLN) * (A + B * s / t);
}

vec3 ambient(vec3 lightColor, float strength)
{
    return lightColor * strength;
}

vec3 diffuse(vec3 lightColor, vec3 L, vec3 N)
{
    float diff = max(dot(N, L), 0.0);

    return diff * lightColor;
}

vec3 specular_BlinnPhong(vec3 lightColor, vec3 L, vec3 V, vec3 N, float shininess)
{
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    return lightColor * spec;
}

vec3 desaturate(vec3 color, float strength)
{
    float lum = dot(color, vec3(0.3, 0.59, 0.11));

    return lerp(color, vec3(lum), strength);
}

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float PointShadow(vec3 fragPos)
{
    vec3 fragToLight = fragPos - uLightPos;

    float currentDepth = length(fragToLight);

    // Grid based Poisson Sampling
    float shadow = 0.0;
    float bias = 0.05;
    int samples = 20;
    float viewDistance = length(uCamPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / uFar)*25) / 50.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(uPointShadowMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= uFar;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    //return closestDepth;
    return 1-shadow;
}

// Fraction of sunlight that reaches worldPos (1 = lit); N is the surface normal, L the direction towards the sun.
float DirShadow(vec3 worldPos, vec3 N, vec3 L)
{
    float cosNL = dot(N, L);
    if (cosNL <= 0.0)
        return 1.0; // turned away from the sun: gets no sunlight anyway

    // look up slightly off the surface (along the normal, more at grazing angles, and towards the sun) against acne at silhouettes
    vec3 offsetPos = worldPos + (N * (1.0 - cosNL) + L * 0.5) * uDirShadowTexel;

    vec4 shadowPos = uShadowViewProj * vec4(offsetPos, 1.0);
    vec3 p = shadowPos.xyz / shadowPos.w * 0.5 + 0.5;

    // outside the shadow map's depth range: nothing in the map can shade it
    if (p.z <= 0.0 || p.z >= 1.0)
        return 1.0;

    vec2 border = abs(p.xy * 2.0 - 1.0);
    float borderDist = max(border.x, border.y);
    if (borderDist >= 1.0)
        return 1.0;

    // 3x3 PCF; the depth compare texture is filtered linearly, so each lookup is itself a bilinear 2x2 PCF
    vec2 texel = 1.0 / vec2(textureSize(uDirShadowMap, 0));
    float lit = 0.0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            lit += texture(uDirShadowMap, vec3(p.xy + vec2(x, y) * texel, p.z));
    lit /= 9.0;

    // fade out towards the border of the shadow map instead of ending in a hard line
    return mix(lit, 1.0, smoothstep(0.9, 1.0, borderDist));
}

void main()
{
    // "Globals"
    const float viewDist = 25;
    const vec2 uv = vTexCoord;

    // Light Parameters
    const float specularity = 3;

    // local dirs
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 N = normalize(vNormal);
    vec3 T = normalize(vTangent);
    vec3 B = normalize(cross(N, T));
    vec3 DL = normalize(uSunLight.direction);   // Directional Light
    vec3 PL = normalize(uLightPos - vWorldPos); // Shadow-casting point light

    if (uRenderNormals)
    {
        fColor = saturate(N);
        return;
    }

    // Material Parameters
    float metalness = 0.0;      // Microfacet Parameter
    float roughness = 1.0;      // Microfacet Parameter
    float matAO     = 1.0;

    roughness = clamp(roughness, 0.025, 1.0);

    // read color texture and apply bias
    vec3 albedo = texture(uTexAlbedo, uv).rgb;
    vec4 albedoBias = unpackSaturatedFloat4(uAlbedoBiasPacked);
    // NOTE: while albedo is automatically converted to linear because we loaded it as an sRGB texture, 
    // we have to do it manually for our color bias here
    albedo = saturate(lerp(albedo, pow(albedoBias.rgb, vec3(2.2)), albedoBias.a));

    // Important angles
    float dotNV = dot(N, V); // cos: Normal, View

    // Fresnel
    vec3 F0 = lerp(vec3(M_F_DIELECTRIC), albedo, metalness);

    // Calculate fragment shade
    vec3 resultColor = vec3(0);

    // Indirect Diffuse
    if (uEnableAmbientL)
        resultColor += calculateIndirectDiffuseLight(uAmbientLight, F0, dotNV, N, V, metalness, 1.0, albedo);

    // Sun
    if (uEnableDirectionalL)
        if (uDirShadows)
        {
            resultColor += calculateDirectLight(DL, V, dotNV, uSunLight.color, N, F0, matAO, roughness, metalness, albedo) * DirShadow(vWorldPos, N, DL);
        } 
        else 
        {
            resultColor += calculateDirectLight(DL, V, dotNV, uSunLight.color, N, F0, matAO, roughness, metalness, albedo);
        }

    if (uEnablePointL)
    {
        // Shadow-casting point light
        const float Ldistance = length(uLightPos - vWorldPos);
        const vec3 Lradiance = uLightColor * uLightIntensity * getInverseSquareFalloffAttenuation(Ldistance, uLightRadius);
        //const vec3 Lradiance = vec3(5, 10, 6) * getInverseSquareFalloffAttenuation(Ldistance, Lradius);

        if (uPointShadows) 
        {
            resultColor += calculateDirectLight(PL, V, dotNV, Lradiance, N, F0, matAO, roughness, metalness, albedo) * PointShadow(vWorldPos);
        } 
        else 
        {
            resultColor += calculateDirectLight(PL, V, dotNV, Lradiance, N, F0, matAO, roughness, metalness, albedo);
        }

        // Instance Lights
        for (int i = 0; i < uPointLightCount; i++)
        {   
            PointLight pl = ubPointLights[i];
            const float PLdistance = length(pl.position - vWorldPos);
            const vec3 PLradiance = pl.color * pl.intensity * getInverseSquareFalloffAttenuation(PLdistance, pl.radius);
            // Alternative falloff:
            //float attLin = 1.0;
            //float attConst = 0.5;
            //float attQuad = 0.05;
            //const vec3 PLradiance = pl.color * 1 / (attConst + attLin * PLdistance + attQuad * PLdistance * PLdistance);

            PL = normalize(pl.position - vWorldPos);

            resultColor += calculateDirectLight(PL, V, dotNV, PLradiance, N, F0, matAO, roughness, metalness, albedo);
        }
    }
    
    // resultColor = vec3(ShadowCalculation(vWorldPos) / uFar);

    // write outputs
    fColor = max(resultColor, vec3(0.0));
    fPickID = uPickingID;

    if (uFogOfWar)
    {
        fColor *= (1 / 1 - pow(2.71828,(length(uCamPos.xz - vWorldPos.xz) - viewDist)*0.3));
    }

    float brightness = dot(fColor, vec3(0.2126, 0.7152, 0.0722));

    if (brightness > 2.0)
        fBright = vec4(fColor, 1.0);
    else
        fBright = vec4(0.0, 0.0, 0.0, 0.0);
}
