#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoord;

uniform mat4 world_to_view;
uniform mat4 projection;

void main()
{
    TexCoord = aPos;
    vec4 pos = projection * world_to_view * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // maximize depth value
}  