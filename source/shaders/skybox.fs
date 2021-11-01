#version 330 core
out vec4 FragColor;

in vec3 TexCoord;

uniform samplerCube skybox_texture;

void main()
{    
    FragColor = texture(skybox_texture, TexCoord);
}