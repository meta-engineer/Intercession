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

layout (std140) uniform viewTransforms
{
    mat4 world_to_view;     // offset  0 bytes, size 64 bytes
    mat4 projection;        // offset 64 bytes, size 64 bytes
};
uniform mat4 model_to_world;

// TODO: can we define a shader header that concatenates this to VS and FS?
// otherwise they have to be matched manually
#define MAX_RAY_LIGHTS 1
#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS 2
// Its suggested to do this matrix multiply in the vertex shader
// TODO: convert this to array for all ray lights
uniform mat4 lightTransform_ray_0;

// Bones per Vertex Buffer (aka per mesh)
const int MAX_BONES = 64;
const int MAX_BONE_INFLUENCE = 4;       // because we use a vec4
uniform mat4 final_bone_matrices[MAX_BONES];


out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace_Ray_0;
out mat3 TBN;

void main()
{
    mat4 model_to_bone = mat4(1.0f);
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if (aBoneIDs[i] <= -1)
        {
            // skipping if no supplied bone
            continue;
        }
        if (aBoneIDs[i] >= MAX_BONES)
        {
            // disable bone influence if too many bones found?
            model_to_bone = mat4(1.0f);
            break;
        }

        model_to_bone += final_bone_matrices[aBoneIDs[i]] * aBoneWeights[i];
    }
 
    gl_Position = projection * world_to_view * model_to_world * model_to_bone * vec4(aPos, 1.0);
    FragPos = vec3(model_to_world * model_to_bone * vec4(aPos, 1.0));

    TexCoord = aTexCoord;
    FragPosLightSpace_Ray_0 = lightTransform_ray_0 * vec4(FragPos, 1.0);
    
    mat3 normalMatrix = transpose(inverse(mat3(model_to_world)));
    Normal = normalize(normalMatrix * aNorm);
    
    // Calculate trangent space matrix (change-of-basis)
    vec3 T = normalize(mat3(model_to_world) * aTangent);
    vec3 N = normalize(mat3(model_to_world) * aNorm);
    // "re-orthogonalize" T with respect to N (i guess because N is considered ground-truth)
    T = normalize(T - dot(T,N) * N);
    // generate B
    vec3 B = cross(T,N);

    TBN = mat3(T, B, N);
}