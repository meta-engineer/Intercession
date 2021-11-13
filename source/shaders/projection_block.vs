#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;

// std140 is a standard byte layout for the struct
// The aligned byte offset of a variable must be a multiple of its base alignment (size)
// mat4's are an array of 4 vec4's so it must be on a multiple of 16
layout (std140) uniform view_transforms
{
    mat4 world_to_view;     // offset  0 bytes, size 64 bytes
    mat4 projection;        // offset 64 bytes, size 64 bytes
};
uniform mat4 model_to_world;

// interface block to be in'd by geometryShader
out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} vs_out;

void main()
{
    // gl_Position is predefined as the shader's output?
    gl_Position = projection * world_to_view * model_to_world * vec4(aPos, 1.0);
    vs_out.FragPos = vec3(model_to_world * vec4(aPos, 1.0));

    vs_out.Normal = mat3(transpose(inverse(model_to_world))) * aNorm;
    vs_out.TexCoord = aTexCoord;
}