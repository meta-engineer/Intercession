#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4 aBoneWeights;

// NOTE: instance transforms (mat4) will be location 6,7,8,9
//  In the base of instace with no bones, locations 4 & 5 will be blank
//  Hopefully this doesn't have a performance impact if they're not packed well

layout (std140) uniform view_transforms
{
    mat4 world_to_view;     // offset  0 bytes, size 64 bytes
    mat4 projection;        // offset 64 bytes, size 64 bytes
};
uniform mat4 model_to_world;

// Bones per Vertex Buffer (aka per mesh)
const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;       // because we use a vec4
uniform mat4 finalBonesMatrices[MAX_BONES];

// TODO: dealing with multiple light transforms
uniform mat4 light_transform;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightTransform;
out mat3 TBN;

void main()
{
    vec4 boned_pos = vec4(0.0f);
    vec4 boned_norm = vec4(0.0f);
    vec4 boned_tang = vec4(0.0f);
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if (aBoneIDs[i] == -1)
        {
            // skipping if no supplied bone
            continue;
        }
        if (aBoneIDs[i] >= MAX_BONES)
        {
            // disable bone influence if too many bones found?
            // this may cause headaches later remember this!
            boned_pos = vec4(aPos, 1.0f);
            boned_norm = vec4(aNorm, 1.0f);
            break;
        }

        vec4 local_pos = finalBonesMatrices[aBoneIDs[i]] * aBoneWeights[i] * vec4(aPos, 1.0f);
        boned_pos  += local_pos;
        boned_norm += mat3(finalBonesMatrices[aBoneIDs[i]]) * aNorm;
        boned_tang += mat3(finalBonesMatrices[aBoneIDs[i]]) * aTangent;
    }

 
    gl_Position = projection * world_to_view * model_to_world * boned_pos;
    FragPos = vec3(model_to_world * boned_pos);

    TexCoord = aTexCoord;
    FragPosLightTransform = light_transform * vec4(FragPos, 1.0);
    
    mat3 normalMatrix = transpose(inverse(mat3(model_to_world)));
    Normal = normalize(normalMatrix * vec3(boned_norm));
    
    // Calculate trangent space matrix (change-of-basis)
    vec3 T = normalize(mat3(model_to_world) * vec3(boned_tang));
    vec3 N = normalize(mat3(model_to_world) * vec3(boned_norm));
    // "re-orthogonalize" T with respect to N (i guess because N is considered ground-truth)
    T = normalize(T - dot(T,N) * N);
    // generate B
    vec3 B = cross(T,N);

    TBN = transpose(mat3(T, B, N));
}