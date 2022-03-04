#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
// ingest for consistency, but dont use
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

layout (std140) uniform viewTransforms
{
    mat4 world_to_view;     // offset  0 bytes, size 64 bytes
    mat4 projection;        // offset 64 bytes, size 64 bytes
};
uniform mat4 model_to_world;

// use same geom shader mechanism to show tangents instead
uniform bool showTangent = false;

// interface block to be in'd by geometryShader
out VS_OUT {
    vec3 Normal;
} vs_out;

void main()
{
    // No projection to prepare for geometryShader
    gl_Position = /* projection */ world_to_view * model_to_world * vec4(aPos, 1.0);

    vec3 attr = aNorm;
    if (showTangent) attr = aTangent;

    vs_out.Normal = normalize(mat3(transpose(inverse(world_to_view * model_to_world))) * attr);
}