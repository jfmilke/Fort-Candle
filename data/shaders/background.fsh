
uniform samplerCube uTexCubeMap;
uniform mat4 uInvProj;
uniform mat4 uInvView;
uniform vec3 uLightDir;
uniform float uRuntime;

in vec2 vPosition;

out vec3 fColor;
out vec4 fBright; // the sky does not bloom
out uint fPickID; // and cannot be picked

void main()
{
    vec3 uLightDir = normalize(vec3(-1, 1, 1));
    vec4 near = uInvProj * vec4(vPosition * 2 - 1, 0, 1);
    vec4 far = uInvProj * vec4(vPosition * 2 - 1, 0.5, 1);

    near /= near.w;
    far /= far.w;

    near = uInvView * near;
    far = uInvView * far;

    fColor = texture(uTexCubeMap, (far - near).xyz).rgb;
    fBright = vec4(0.0);
    fPickID = uint(-1);

    vec3 sunColor = vec3(0.9,0.9,0.7);
    float dotVL = max(0, dot(uLightDir, normalize(far - near).xyz));

    //fColor = pow(fColor, vec3(1 / 2.2));
}