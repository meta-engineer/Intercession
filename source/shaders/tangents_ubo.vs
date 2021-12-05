#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;


layout (std140) uniform view_transforms
{
    mat4 world_to_view;     // offset  0 bytes, size 64 bytes
    mat4 projection;        // offset 64 bytes, size 64 bytes
};
uniform mat4 model_to_world;

// TODO: dealing with multiple light transforms
uniform mat4 light_transform;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightTransform;

out mat3 TBN;

void main()
{


 
    gl_Position = projection * world_to_view * model_to_world * vec4(aPos, 1.0);
    FragPos = vec3(model_to_world * vec4(aPos, 1.0));

    mat3 modelVector = transpose(inverse(mat3(model_to_world)));

    Normal = normalize(modelVector * aNorm);
    TexCoord = aTexCoord;
    FragPosLightTransform = light_transform * vec4(FragPos, 1.0);
    
    // Calculate trangent space matrix (change-of-basis)
    vec3 T = normalize(modelVector * aTangent);
    // "re-orthogonalize" T with respect to N (i guess ebcause N is considered ground-truth)
    T = normalize(T - dot(T,Normal) * Normal);
    // generate B
    vec3 B = cross(Normal,T);

    TBN = mat3(T, B, Normal);
}