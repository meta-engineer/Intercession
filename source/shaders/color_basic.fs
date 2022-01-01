#version 330 core
out vec4 FragColor;
in vec3 OutColor;

uniform float globalAlpha;

void main()
{
    FragColor = vec4(OutColor, globalAlpha);
}