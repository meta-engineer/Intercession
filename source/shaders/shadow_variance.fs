#version 330 core
out vec4 FragColor;

// do I need to in the right variables if i don't need them?

// TODO: ingest texture of object to discard/deal with transparency
void main()
{
    float depth = gl_FragCoord.z;

    // change variance like changing bias based on light angle
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    float moment2 = depth * depth + 0.25*(dx*dx + dy*dy);

    // store depth calc as usual, then store depth^2 to calculate variance
    FragColor = vec4(depth, moment2, 0.0, 1.0);
}