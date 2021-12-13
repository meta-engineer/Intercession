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

// keep TBN for mapping, but remove lighting transforms
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out mat3 TBN;

void main()
{


    
    gl_Position = projection * world_to_view * model_to_world * vec4(aPos, 1.0);
    FragPos = vec3(model_to_world * vec4(aPos, 1.0));

    TexCoord = aTexCoord;
    
    mat3 normalMatrix = transpose(inverse(mat3(model_to_world)));
    Normal = normalize(normalMatrix * aNorm);
    
    // Calculate trangent space matrix (change-of-basis)
    vec3 T = normalize(mat3(model_to_world) * aTangent);
    vec3 N = normalize(mat3(model_to_world) * aNorm);
    // "re-orthogonalize" T with respect to N (i guess because N is considered ground-truth)
    T = normalize(T - dot(T,N) * N);
    // generate B
    vec3 B = cross(T,N);

    TBN = transpose(mat3(T, B, N));
}