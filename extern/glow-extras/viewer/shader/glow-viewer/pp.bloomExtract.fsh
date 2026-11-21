// Bloom bright-pass: extract HDR pixels above a luminance threshold into a
// (typically downsampled) target. The contribution is luminance-normalized so
// bright colors keep their hue instead of washing out towards white.

uniform sampler2DRect uTexHDR;
uniform vec2 uSrcSize; // size of uTexHDR in texels (full res)
uniform float uThreshold;

in vec2 vPosition;
out vec4 fBloom;

void main()
{
    vec3 c = texture(uTexHDR, vPosition * uSrcSize).rgb;
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    float contrib = max(l - uThreshold, 0.0) / max(l, 1e-4);
    fBloom = vec4(c * contrib, 1.0);
}
