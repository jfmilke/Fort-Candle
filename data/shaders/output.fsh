uniform sampler2DRect uTexColor;
uniform sampler2DRect uTexBright;
uniform sampler2DRect uTexBloom;
uniform sampler2DRect uTexDepth;
//uniform sampler2DShadow uTexShadowMap;

uniform mat4 uInvView;
uniform mat4 uInvProj;
uniform vec3 uCamPos;
uniform float uNear;
uniform float uFar;

uniform bool uEnableTonemap;
uniform bool uEnableGamma;
uniform bool uShowPostProcess;
uniform bool uEnableSepia;
uniform bool uEnableVignette;
uniform bool uEnableDesaturate;
uniform bool uEnableBloom;
uniform float uDesaturationStrength;
uniform float uSepiaStrength;
uniform vec3 uVignetteColor;
uniform float uVignetteStrength;
uniform float uVignetteSoftness;
uniform float uVignetteRadius;
uniform vec3 uDistFadeColor;
uniform float uDistFadeStrength;

in vec2 vTexCoord;
out vec3 fColor;

const vec3 SEPIA = vec3(1.2, 1.0, 0.8);

uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float wang_float(uint hash)
{
    return hash / float(0x7FFFFFFF) / 2.0;
}

vec3 dither(in vec3 color, vec2 uv, int divisor) {
    uint seed = uint(uv.x) + uint(uv.y) * 8096;
    float r = wang_float(wang_hash(seed * 3 + 0));
    float g = wang_float(wang_hash(seed * 3 + 1));
    float b = wang_float(wang_hash(seed * 3 + 2));
    vec3 random = vec3(r, g, b);

    return color + (random - .5) / divisor;
}

//
// Some Tonemappers to play with
//

float linearDepth(float z)
{
    vec4 sp = vec4(vTexCoord * 2 - 1, z * 2 - 1, 1.0);
    vec4 vp = uInvProj * sp;
    vp /= vp.w;
    vec4 wp = uInvView * vp;
    return distance(uCamPos, wp.xyz) / uFar;
}

float tuneLinearDepth(float z)
{
    return (1/z - 1/uNear)/(1/uFar - 1/uNear);
}

vec3 uc2_transform(in vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

//
// Uncharted 2 Tonemapper

vec3 tonemap_uc2(in vec3 color)
{
    float W = 11.2;

    color *= 2;  // Hardcoded Exposure Adjustment

    float exposure_bias = 2.0f;
    vec3 curr = uc2_transform(exposure_bias*color);

    vec3 white_scale = vec3(1,1,1)/uc2_transform(vec3(W,W,W));
    vec3 ccolor = curr*white_scale;

    return ccolor;
}

//
// Filmic tonemapper

vec3 tonemap_filmic(in vec3 color)
{
    color = max(vec3(0,0,0), color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);

    return color;
}

//
// Metal Gear Solid V tonemapper

vec3 tonemap_mgsv(in vec3 color)
{
    float A = 0.6;
    float B = 0.45333;
    vec3 mapped = min(vec3(1,1,1), A + B - ( (B * B) / (color - A + B) ));
    vec3 condition = vec3(color.r > A, color.g > A, color.b > A);

    return mapped * condition + color * (1 - condition);
}


// accurate linear to sRGB conversion (often simplified to pow(x, 1/2.24))
vec3 applySRGBCurve(vec3 x)
{
    // accurate linear to sRGB conversion (often simplified to pow(x, 1/2.24))
    return mix(
        vec3(1.055) * pow(x, vec3(1.0/2.4)) - vec3(0.055), 
        x * vec3(12.92), 
        lessThan(x, vec3(0.0031308))
    );
}

vec3 desaturate(vec3 color, float strength)
{
    float lum = dot(color, vec3(0.3, 0.59, 0.11));

    return mix(color, vec3(lum), strength);
}

vec3 vignette(vec3 color, vec3 vignetteColor, float radius, float softness, vec2 size, float alpha)
{
    vec2 cDist = gl_FragCoord.xy / size - vec2(0.5); // Distance to center
    float len = length(cDist);
    float vignette = smoothstep(radius, radius - softness, len);
    return color = mix(color, color * vignette + (1.0 -  vignette) * vignetteColor, alpha);
}

vec3 sepia(vec3 color, float strength)
{
    float lum = dot(color, vec3(0.3, 0.59, 0.11));
    return mix(color, vec3(lum) * SEPIA, strength);
}

vec3 distfade(vec3 color, float depth, vec3 fadeColor, float fadeStrength)
{
    return mix(color, fadeColor, min(linearDepth(depth) * fadeStrength, 1.0));
}

float lookup(vec2 coord, float dx, float dy)
{
    vec2 uv = coord + vec2(dx, dy);
    vec3 c = texture(uTexColor, uv).rgb;
	
	// return as luma
    return 0.2126*c.r + 0.7152*c.g + 0.0722*c.b;
}

void main()
{    
    vec4 n[9];
    vec2 size = textureSize(uTexColor);
    vec2 coord = gl_FragCoord.xy;

    // sample scene HDR color and depth
    vec3 color = texture(uTexColor, gl_FragCoord.xy).rgb;
    vec3 bright = texture(uTexBright, gl_FragCoord.xy).rgb;
    vec3 bloom = max(texture(uTexBloom, gl_FragCoord.xy).rgb, 0.0);
    float depth = texture(uTexDepth, gl_FragCoord.xy).x;

    color += bloom;

    // tonemap
    if (uEnableTonemap)
    {
        color = tonemap_uc2(color);
    }
    
    
    // conversion to sRGB
    if (uEnableGamma)
    {
        color = applySRGBCurve(color);
    }

    // some random additional effect
    if (uShowPostProcess && depth < 1)
    {
        color = 1 - color; // invert color for foreground if true
    }

    // Desaturate colors
    if (uEnableDesaturate)
        color = desaturate(color, uDesaturationStrength);

    // Create sepia effect
    if (uEnableSepia)
        color = sepia(color, uSepiaStrength);

    // Create vignette effect
    if (uEnableVignette)
        color = vignette(color, uVignetteColor, uVignetteRadius, uVignetteSoftness, size, uVignetteStrength);

    color = dither(color, gl_FragCoord.xy, 100); // Divisor is originally 255, but this looks much better (on an 8bit screen)
    
    color = distfade(color, depth, uDistFadeColor, uDistFadeStrength);

        // simple sobel edge detection
    vec2 p = gl_FragCoord.xy;
    float gx = 0.0;
    gx += -1.0 * lookup(p, -1.0, -1.0);
    gx += -2.0 * lookup(p, -1.0,  0.0);
    gx += -1.0 * lookup(p, -1.0,  1.0);
    gx +=  1.0 * lookup(p,  1.0, -1.0);
    gx +=  2.0 * lookup(p,  1.0,  0.0);
    gx +=  1.0 * lookup(p,  1.0,  1.0);
    
    float gy = 0.0;
    gy += -1.0 * lookup(p, -1.0, -1.0);
    gy += -2.0 * lookup(p,  0.0, -1.0);
   	gy += -1.0 * lookup(p,  1.0, -1.0);
    gy +=  1.0 * lookup(p, -1.0,  1.0);
    gy +=  2.0 * lookup(p,  0.0,  1.0);
    gy +=  1.0 * lookup(p,  1.0,  1.0);
    
    float g = sqrt(gx*gx + gy*gy) * 0.2;
    float g2 = g / 2.0;

    g = max(mix(g, 0.0, linearDepth(depth) * 3.0), 0.0);
    
    color -= vec3(g, g, g); 

    fColor = color;
    //float depthValue = texture(uTexShadowMap, gl_FragCoord.xy).r;
    //fColor = vec3(tuneLinearDepth(linearDepth(depth)));
    //fColor = vec3(linearDepth(depth));
}
