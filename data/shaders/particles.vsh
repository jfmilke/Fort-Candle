uniform mat4 uProj;
uniform mat4 uView;

// Per vertex
in vec3 aPosition;

// Per instance
in vec4 aTranslation;
in vec4 aColor;
in float aRotation;
in float aScale;
in float aBlend;

out vec3 vColor;
out vec3 vWorldPos;
out float vBlend;

void main()
{
    // Construct model matrix from translation, y-rotation & uniform scaling
    mat4x3 modelMatrix;
    modelMatrix[0][0] = aScale * cos(aRotation);
    modelMatrix[0][1] = 0.0;
    modelMatrix[0][2] = -aScale * sin(aRotation);
    modelMatrix[1][0] = 0.0;
    modelMatrix[1][1] = aScale;
    modelMatrix[1][2] = 0.0;
    modelMatrix[2][0] = aScale * sin(aRotation);
    modelMatrix[2][1] = 0.0;
    modelMatrix[2][2] = aScale * cos(aRotation);
    modelMatrix[3] = vec3(0,0,0);

    vBlend = aBlend;
    vColor = vec3(aColor);
    vWorldPos = modelMatrix * vec4(aPosition, 1.0) + vec3(aTranslation.x, aTranslation.y, aTranslation.z);
    gl_Position = uProj * uView * vec4(vWorldPos, 1.0);
}
