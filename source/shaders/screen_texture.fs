#version 330 core
out vec4 FragColor;
  
in vec2 TexCoord;

uniform sampler2D screenTexture;

void main()
{
    // applies greyscale
    FragColor = texture(screenTexture, TexCoord);
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    FragColor = vec4(average, average, average, 1.0);
}
