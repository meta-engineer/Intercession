#version 330 core
layout (location = 0) out vec3 GPosition;
layout (location = 1) out vec3 GNormal;
layout (location = 2) out vec4 GColorSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in mat3 TBN;

struct Material {
    sampler2D   diffuse_map_0;
    sampler2D   specular_map_0;
    
    bool        enable_normal_map_0;
    sampler2D   normal_map_0;

    bool        enable_displace_map_0;
    sampler2D   displace_map_0;
};

uniform vec3 viewPos;
uniform Material material;
uniform float heightMapScale = 0.1;

vec2 DisplaceMapping(vec2 texCoords, vec3 viewDir);

void main()
{
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 tangentViewDir = normalize(TBN * viewDir);
    vec2 remappedTexCoord = TexCoord;
    if (material.enable_displace_map_0)
    {
        remappedTexCoord = DisplaceMapping(TexCoord, tangentViewDir);
        if(remappedTexCoord.x > 1.0 || remappedTexCoord.y > 1.0 || remappedTexCoord.x < 0.0 || remappedTexCoord.y < 0.0)
            discard;
    }

    if (texture(material.diffuse_map_0, remappedTexCoord).a < 0.01)
        discard;

    GPosition = FragPos;

    GNormal = Normal;
    if (material.enable_normal_map_0)
    {
        GNormal = texture(material.normal_map_0, remappedTexCoord).xyz;
        GNormal = (2.0 * GNormal - vec3(1.0));
        GNormal = normalize(TBN * GNormal);
    }

    GColorSpec.rgb = texture(material.diffuse_map_0, remappedTexCoord).rgb;
    GColorSpec.a = texture(material.specular_map_0, remappedTexCoord).r;
}


vec2 DisplaceMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightMapScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(material.displace_map_0, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(material.displace_map_0, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material.displace_map_0, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}