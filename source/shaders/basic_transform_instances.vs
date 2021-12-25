#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 6) in mat4 aInstanceTransform;

uniform mat4 model_to_world;

void main()
{
    // apply instance transform first, then collective transform (model_to_world);
    mat4 instance_to_world = model_to_world * aInstanceTransform;

    gl_Position = instance_to_world * vec4(aPos, 1.0);
}  
