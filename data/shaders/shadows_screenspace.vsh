uniform mat4 uLightSpaceMat;
uniform mat4x3 uModel;

in vec3 aPosition;
// in vec3 aNormal;
// in vec3 aTangent;
// in vec2 aTexCoord;

// out vec3 vWorldPos;
// out vec3 vNormal;
// out vec3 vTangent;
// out vec2 vTexCoord;

void main()
{
    // assume uModel has no non-uniform scaling
    // vNormal = mat3(uModel) * aNormal;
    // vTangent = mat3(uModel) * aTangent;

    // y-flip to match blenders uv-mapping
    // vTexCoord = aTexCoord;
    // vTexCoord.y = 1.0 - aTexCoord.y;

    vec3 vWorldPos = uModel * vec4(aPosition, 1.0);
    gl_Position = uLightSpaceMat * vec4(vWorldPos, 1);
}
