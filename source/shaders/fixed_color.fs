#version 330 core
out vec4 FragColor;

in vec4 vertexColor; // passed from vertex shader. type AND name indentical

void main()
{
    FragColor = vertexColor;
}