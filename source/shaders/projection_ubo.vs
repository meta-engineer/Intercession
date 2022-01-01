#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;

layout (std140) uniform viewTransforms
{
    mat4 world_to_view;     // offset  0 bytes, size 64 bytes
    mat4 projection;        // offset 64 bytes, size 64 bytes
};
uniform mat4 model_to_world;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    // gl_Position is predefined as the shader's output?
    gl_Position = projection * world_to_view * model_to_world * vec4(aPos, 1.0);
    FragPos = vec3(model_to_world * vec4(aPos, 1.0));

    Normal = mat3(transpose(inverse(model_to_world))) * aNorm;
    TexCoord = aTexCoord;
}