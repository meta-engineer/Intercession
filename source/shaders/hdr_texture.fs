#version 330 core
out vec4 FragColor;
  
in vec2 TexCoord;

uniform sampler2D screenTexture;
uniform float exposure = 2.0;

void main()
{
    // because the hdr buffer is rgba i think we need to ensure alpha is 1.0 here
    vec3 hdrColor = texture(screenTexture, TexCoord).rgb;

    // reinhard tone mappping
    //vec3 ldrColor = hdrColor / (hdrColor + vec3(1.0));

    // exposure tone mapping
    vec3 ldrColor = vec3(1.0) - exp(-hdrColor * exposure);
    
    // NOTE: we could instead do gamma correction here aswell (gamma always comes last)
    FragColor = vec4(ldrColor, 1.0);
}
