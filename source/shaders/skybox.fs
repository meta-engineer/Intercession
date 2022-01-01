#version 330 core
out vec4 FragColor;

in vec3 TexCoord;

uniform samplerCube skybox_cubemap;

void main()
{    
    FragColor = texture(skybox_cubemap, TexCoord);
}