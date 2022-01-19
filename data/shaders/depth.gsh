
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 uShadowMatrices[6];

out vec3 vWorldPos;

void main()
{
    for (int face = 0; face < 6; ++face)
    {
        gl_Layer = face;            // built-in variable that specifies to which face we render.
        for (int i = 0; i < 3; ++i) // for each triangle vertex
        {
            vWorldPos = gl_in[i].gl_Position.xyz;
            gl_Position = uShadowMatrices[face] * vec4(vWorldPos, 1.0);
            EmitVertex();
        }
        EndPrimitive();
    }
}