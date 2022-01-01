#version 330 core
out vec4 FragColor;
in vec3 OutColor;
in vec2 TexCoord;

uniform float globalAlpha;
uniform sampler2D texture;

void main()
{
    FragColor = texture(texture, TexCoord) * vec4(OutColor, globalAlpha);
}