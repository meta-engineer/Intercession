#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform samplerCube skybox_texture;

void main()
{
    vec3 I = normalize(FragPos - viewPos);
    // reflection
    vec3 R = reflect(I, normalize(Normal));
    // refraction
    //vec3 R = refract(I, normalize(Normal), 1.00/1.52); // air to glass ratio

    FragColor = vec4(texture(skybox_texture, R).xyz, 1.0);
}
