#version 330 core
out vec4 FragColor;

uniform vec4 globalColor; // passed by opengl code.

void main()
{
    FragColor = globalColor;
}