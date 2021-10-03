#version 330 core
out vec4 FragColor;
in vec3 ourColor;

uniform float globalAlpha;

void main()
{
    FragColor = vec4(ourColor, globalAlpha);
}