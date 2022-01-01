#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 HdrColor;

uniform vec3 lightColor;

void main()
{
    FragColor = vec4(lightColor, 1.0);

    // only colors in HDR range
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        HdrColor = vec4(FragColor.rgb, 1.0);
    else
        HdrColor = vec4(0.0, 0.0, 0.0, 1.0);
}
