#version 330 core
out vec4 FragColor;

in vec3 TexCoord;

uniform samplerCube skybox_cubemap;
uniform float gamma = 2.2;

void main()
{
    // NOTE: this should not be further post-processed once gamma-corrected
    vec4 outColor = texture(skybox_cubemap, TexCoord);
    FragColor = vec4( pow(outColor.rgb, vec3(1.0/gamma)), outColor.a);
}