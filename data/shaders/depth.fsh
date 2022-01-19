
in vec3 vWorldPos;

uniform vec3 uLightPos;
uniform float uFar;

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(vWorldPos - uLightPos);
    
    // map to [0;1] range by dividing by far plane
    lightDistance = lightDistance / uFar;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}  