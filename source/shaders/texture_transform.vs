#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 OutColor;
out vec2 TexCoord;

uniform mat4 transform;

void main()
{
    // gl_Position is predefined as the shader's output?
    gl_Position = transform * vec4(aPos.x, aPos.y, aPos.z, 1.0);

    OutColor = aColor;
    TexCoord = aTexCoord;
}