
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
uniform bool uShadowsPass;
uniform bool uPointShadows;
uniform bool uDirShadows;

// Coordinates
uniform vec3 uCamPos;
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec2 vTexCoord;

// Transformations
uniform mat4 uShadowViewProj;

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

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
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

float DirShadow(vec3 worldPos, vec3 N, vec3 L)
{
    vec3 offset = L * 0.01; // to prevent shadow acne
    vec4 shadowPos = uShadowViewProj * vec4(worldPos, 1.0f);
    shadowPos /= shadowPos.w;

    //if (any(greaterThan(abs(shadowPos.xyz), vec3(0.99f))))
    //    return 1.0f; // no shadow outside

    bvec3 gr = greaterThan(abs(shadowPos.xyz), vec3(0.99f));
    if (gr.x || gr.y || gr.z)
        return 1.0f; // no shadow outside

    float dotNL = dot(N, L);
    float bias = max(0.05 * (1.0 - dotNL), 0.005);
    
    shadowPos.xyz = shadowPos.xyz * 0.5 + 0.5;
    
    float closestDepth = texture(uDirShadowMap, shadowPos.xyz); 
    float currentDepth = shadowPos.z;

    // Random Poisson Sampling 
    float shadow = 0.0;
    for (int i = 0; i < 4; i++) {
        int index = int(16.0*random(gl_FragCoord.xyy, i))%16;
        if (texture(uDirShadowMap, shadowPos.xyz + vec3(poissonDisk[index]/700.0, 0))  >  shadowPos.z )
        {
            shadow+=0.3;
        }
    }
    shadow = clamp(shadow, 0, 1);

    // // Grid based Poisson Sampling
    // float shadow = 0.0;
    // float bias = 0.05;
    // int samples = 20;
    // float viewDistance = length(uCamPos - worldPos);
    // float diskRadius = (1.0 + (viewDistance / uFar)*25) / 50.0;
    // for(int i = 0; i < samples; ++i)
    // {
    //     float closestDepth = texture(uDirShadowMap, shadowPos.xyz + gridSamplingDisk[i]/2000);
    //     //closestDepth *= uFar;   // undo mapping [0;1]
    //     if(currentDepth > closestDepth)
    //         shadow += 1.0;
    // }
    // shadow /= float(samples);
    // shadow = 1-shadow;


    //float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    if(shadowPos.z > 1.0) // check far plane
        shadow = 0.0;

    return shadow;
}

void main()
{
    if (uShadowsPass)
    {
        fColor = vec3(1.0);
        return;
    }

    // "Globals"
    const float viewDist = 25;
    const vec2 uv = vTexCoord;

    // Light Paramters
    const float specularity = 3;

    // local dirs
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 N = normalize(vNormal);
    vec3 T = normalize(vTangent);
    vec3 B = normalize(cross(N, T));
    vec3 DL = normalize(uSunLight.direction);   // Directional Light
    vec3 PL = normalize(uLightPos - vWorldPos); // Demo Point Light

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
    float dotNV = dot(N, V); // cos: Normal, Vertex

    // Frescnel
    vec3 F0 = lerp(vec3(M_F_DIELECTRIC), albedo, metalness);

    // Calculate fragment shade
    vec3 resultColor = vec3(0);

    // Indirect Diffuse
    if (uEnableAmbientL)
        resultColor += calculateIndirectDiffuseLight(uAmbientLight, F0, dotNV, N, V, metalness, 1.0, albedo);

    vec3 Lsun = normalize(vec3(-1, 1, 1));
    vec3 radianceSun = vec3(1.0, 0.9, 0.75) * 0.5;

    // Sun
    if (uEnableDirectionalL)
        if (uDirShadows) 
        {
            resultColor += calculateDirectLight(DL, V, dotNV, uSunLight.color, N, F0, matAO, roughness, metalness, albedo) * DirShadow(vWorldPos, N, Lsun);
        } 
        else 
        {
            resultColor += calculateDirectLight(DL, V, dotNV, uSunLight.color, N, F0, matAO, roughness, metalness, albedo);
        }

    if (uEnablePointL)
    {
        // Demo Pointlight
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
