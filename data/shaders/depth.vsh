uniform mat4x3 uModel;

in vec3 aPosition;

uniform mat4 uShadowMatrix;

out vec3 vWorldPos;

void main() { 
    vWorldPos = uModel * vec4(aPosition, 1.0);
    gl_Position = uShadowMatrix * vec4(vWorldPos, 1);
}