#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;    // ingest for consistency, but dont use

layout (std140) uniform view_transforms
{
    mat4 world_to_view;     // offset  0 bytes, size 64 bytes
    mat4 projection;        // offset 64 bytes, size 64 bytes
};
uniform mat4 model_to_world;

// interface block to be in'd by geometryShader
out VS_OUT {
    vec3 Normal;
} vs_out;

void main()
{
    // No projection to prepare for geometryShader
    gl_Position = /* projection */ world_to_view * model_to_world * vec4(aPos, 1.0);

    vs_out.Normal = normalize(mat3(transpose(inverse(world_to_view * model_to_world))) * aNorm);
}