#version 330 core
out vec4 FragColor;

in vec3 TexCoord;

uniform samplerCube cube_map;

void main()
{    
    FragColor = texture(cube_map, TexCoord);
}