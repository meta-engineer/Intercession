#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model_to_world;
uniform mat4 world_to_view;
uniform mat4 projection;

void main()
{

    vec3 FragPos = vec3(model_to_world * vec4(aPos, 1.0));
    vec3 Normal = mat3(transpose(inverse(model_to_world))) * aNorm;
    
    // extrude vertex position along vertex normal
    // this only works if the normals are smooth, continuous, and ideal
    gl_Position = projection * world_to_view * model_to_world * vec4(aPos + aNorm*0.08, 1.0);
}