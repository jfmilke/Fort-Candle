// Separable Gaussian blur (35-tap, sigma ~5.67, linear-sampled). Run once
// horizontally and once vertically per iteration; ping-pong between two
// targets for a wider spread.
//
// Equivalent to a 35-tap discrete Gaussian (radius 17). Hardware bilinear
// interpolation folds each adjacent texel pair into one fetch, so 35 texel
// taps are served by 19 texture() calls (1 center + 9 pairs * 2 sides; the
// outermost texel at offset 17 is an unpaired lone tap).
//
// REQUIRES: uTex must use GL_LINEAR min/mag filtering, or the fractional
// offsets snap to nearest and the weights are wrong.

uniform sampler2DRect uTex;
uniform vec2 uTexSize;    // size of this target in texels
uniform vec2 uDirection;  // blur step in texels: (s, 0) horizontal, (0, s) vertical

in vec2 vPosition;
out vec4 fBloom;

const float kCenterWeight = 0.0705417099;
const float kWeights[9] = float[](
    0.1357339535, 0.1163030126, 0.0880672294, 0.0589327601, 0.0348510360,
    0.0182132595, 0.0084114060, 0.0034328403, 0.0007836476);
const float kOffsets[9] = float[](
    1.4883239224, 3.4727778102, 5.4572843087, 7.4418730026, 9.4265728474,
    11.4114119552, 13.3964173947, 15.3816150075, 17.0000000000);

void main()
{
    vec2 uv = vPosition * uTexSize;

    vec3 result = texture(uTex, uv).rgb * kCenterWeight;
    for (int i = 0; i < 9; ++i)
    {
        vec2 step = uDirection * kOffsets[i];
        result += texture(uTex, uv + step).rgb * kWeights[i];
        result += texture(uTex, uv - step).rgb * kWeights[i];
    }
    fBloom = vec4(result, 1.0);
}
