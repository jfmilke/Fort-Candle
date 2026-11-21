// Bloom composite: add the blurred bloom to the accumulated HDR image, then
// apply the exact same tonemap + gamma as pp.accum and write the final output.
// Bloom is added BEFORE tonemapping so it rolls off naturally and bleeds over
// neighbouring (and nearer) geometry like a real glow.

uniform sampler2DRect uTexAccum; // accumulated HDR (full res)
uniform sampler2DRect uTexBloom; // blurred bloom (downsampled)
uniform sampler2DRect uTexDepth;

uniform vec2 uBloomSize; // size of uTexBloom in texels
uniform float uBloomStrength;

uniform bool uReverseZEnabled;
uniform bool uEnableTonemap;
uniform float uTonemapExposure;
uniform bool uForceAlphaOne;

in vec2 vPosition;
out vec4 fOutput;

const mat3 ACESInputMat =
mat3(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04824, 0.01566, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat =
mat3(
    1.60475, -0.10208, -0.00327,
    -0.53108, 1.10813, -0.07276,
    -0.07367, -0.00605, 1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFittedTonemap(vec3 inputColor, float exposure)
{
    inputColor *= exposure;
    inputColor = ACESInputMat * inputColor;
    inputColor = RRTAndODTFit(inputColor);
    inputColor = ACESOutputMat * inputColor;
    inputColor = clamp(inputColor, 0.0, 1.0);
    return inputColor;
}

// Exact sRGB OETF (linear -> sRGB), piecewise. Expects c in [0, 1].
vec3 linearToSrgb(vec3 c)
{
    bvec3 cutoff = lessThanEqual(c, vec3(0.0031308));
    vec3 low  = c * 12.92;
    vec3 high = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, vec3(cutoff));
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    float d = texture(uTexDepth, fragCoord).x;

    vec4 color = texture(uTexAccum, fragCoord);
    vec3 bloom = texture(uTexBloom, vPosition * uBloomSize).rgb;
    color.rgb += uBloomStrength * bloom;

    if (uReverseZEnabled)
    {
        if (d > 0 && uEnableTonemap) color = vec4(ACESFittedTonemap(color.xyz, uTonemapExposure), 1.0);
        else color = clamp(color, 0.0, 1.0);
    }
    else
    {
        if (d < 1 && uEnableTonemap) color = vec4(ACESFittedTonemap(color.xyz, uTonemapExposure), 1.0);
        else color = clamp(color, 0.0, 1.0);
    }

    // sRGB encode (display transfer function)
    color.rgb = linearToSrgb(color.rgb);

    if (uForceAlphaOne)
        color.a = 1.0;

    fOutput = color;
}
