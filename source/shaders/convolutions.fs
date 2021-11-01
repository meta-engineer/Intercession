#version 330 core
out vec4 FragColor;
  
in vec2 TexCoord;

uniform sampler2D screenTexture;

// texture is in NDC so we should sample ~ distance of 1 pixel (dimensions of screen)
const float offset = 1.0 / 400.0;

void main()
{
    // gaussian blur kernel
    float kernel[9] = float[](
        -1, -1, -1,
        -1,  8, -1,
        -1, -1, -1
    );

    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );
    vec3 sampleWindow[9];
    for (int i = 0; i < 9; i++)
    {
        sampleWindow[i] = vec3(texture(screenTexture, TexCoord + offsets[i]));
    }

    vec3 color = vec3(0.0);
    for(int i = 0; i < 9; i++)
    {
        color += sampleWindow[i] * kernel[i];
    }

    FragColor = vec4(color, 1.0);
    float average = (FragColor.r + FragColor.g + FragColor.b) / 3.0;
    FragColor = vec4(average, average, average, 1.0);
}
