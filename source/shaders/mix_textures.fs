#version 330 core
out vec4 FragColor;
in vec3 OutColor;
in vec2 TexCoord;

uniform float globalAlpha;
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), globalAlpha);
}