#version 330 core
in vec4 FragPos;

uniform vec3 source_pos;
uniform float source_far_plane;

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(FragPos.xyz - source_pos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / source_far_plane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}  