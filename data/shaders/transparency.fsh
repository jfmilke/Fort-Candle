
#include "brdf.glsli"

uniform sampler2D uTexAlbedo;
uniform vec3 uLightPos;
uniform vec3 uCamPos;
uniform uint uPickingID;
uniform uint uAlbedoBiasPacked;
uniform bool uRenderNormals;
uniform bool uFogOfWar;

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec2 vTexCoord;

out vec3 fColor;
out uint fPickID;

#define ENABLE_SUN              1
#define ENABLE_POINTLIGHT       1
#define ENABLE_AMBIENT          1

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

void main()
{
    // local dirs
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 N = normalize(vNormal);
    vec3 T = normalize(vTangent);
    vec3 B = normalize(cross(N, T));

    float metalness = 0.0;
    float roughness = 1.0;
    float matAO = 1.0;

    const vec2 uv = vTexCoord;

    float viewDist = 25;

    if (uRenderNormals)
    {
        fColor = saturate(N);
        return;
    }

    // read color texture and apply bias
    vec3 albedo = texture(uTexAlbedo, uv).rgb;
    vec4 albedoBias = unpackSaturatedFloat4(uAlbedoBiasPacked);
    // NOTE: while albedo is automatically converted to linear because we loaded it as an sRGB texture, 
    // we have to do it manually for our color bias here
    albedo = saturate(lerp(albedo, pow(albedoBias.rgb, vec3(2.2)), albedoBias.a));

    // angle between surface normal and outgoing light direction.
    const float cosLo = saturate(dot(N, V));
	// fresnel reflectance at normal incidence (for metals use albedo color).
	const vec3 F0 = lerp(vec3(M_F_DIELECTRIC), albedo, metalness);

    vec3 resultColor = vec3(0);

    // hardcoded directional light
#if ENABLE_SUN
    {
        vec3 Lsun = normalize(vec3(-1, 1, 1));
        vec3 radianceSun = vec3(1.0, 0.9, 0.75) * 0.5;

        resultColor += calculateDirectLight(Lsun, V, cosLo, radianceSun, N, F0, matAO, roughness, metalness, albedo);
    }
#endif

    // point light
#if ENABLE_POINTLIGHT
    {
        const vec3 L = normalize(uLightPos - vWorldPos);
        const float Ldistance = length(uLightPos - vWorldPos);
        const float Lradius = 50.0;

        // compute radiance by applying attenuation
        const vec3 Lradiance = vec3(5, 10, 6) * getInverseSquareFalloffAttenuation(Ldistance, Lradius);
        resultColor += calculateDirectLight(L, V, cosLo, Lradiance, N, F0, matAO, roughness, metalness, albedo);
    }
#endif

    // hardcoded indirect diffuse term
#if ENABLE_AMBIENT
    vec3 constantAmbientIrradiance = vec3(0.75, 1.0, 0.9) * 0.15;
    resultColor += calculateIndirectDiffuseLight(constantAmbientIrradiance, F0, cosLo, N, V, metalness, matAO, albedo);
#endif

if (uFogOfWar) {
    resultColor *= (1 / 1 - pow(2.71828,(length(uCamPos.xz - vWorldPos.xz) - viewDist)*0.3));
}

    // write outputs
    fColor = max(resultColor, vec3(0.0));
    fPickID = uPickingID;
}
