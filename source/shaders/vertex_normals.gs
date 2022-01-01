#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (std140) uniform viewTransforms
{
    mat4 world_to_view;     // offset  0 bytes, size 64 bytes
    mat4 projection;        // offset 64 bytes, size 64 bytes
};

in VS_OUT {
    vec3 Normal;
} gs_in[];

out vec4 vertexColor;

const float MAGNITUDE = 0.1;

void main()
{
    vec4 flat_color = vec4(0.0, 0.8, 0.8, 1.0);
    for (int i = 0; i < 3; i++)
    {
        gl_Position = projection *  gl_in[i].gl_Position;
        vertexColor = flat_color;
        EmitVertex();

        gl_Position  = projection * (gl_in[i].gl_Position + vec4(gs_in[i].Normal, 0.0) * MAGNITUDE);
        vertexColor = flat_color;
        EmitVertex();

        EndPrimitive();
    }
}