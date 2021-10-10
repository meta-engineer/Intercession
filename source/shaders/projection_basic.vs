#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

uniform mat4 model_to_world;
uniform mat4 world_to_view;
uniform mat4 projection;

void main()
{
    // gl_Position is predefined as the shader's output?
    gl_Position = projection * world_to_view * model_to_world * vec4(aPos, 1.0);

    ourColor = aColor;
    TexCoord = aTexCoord;
}