#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightTransform;
in mat3 TBN;

// All textures must be in the single Material instance "material"
// and use this TYPE_map_X format
struct Material {
    sampler2D   diffuse_map_0;
    sampler2D   specular_map_0;
    //float       shininess;

    // optionally select normal map if available
    bool        set_normal_map_0;
    sampler2D   normal_map_0;
};

struct PointLight {
    vec3 position;
    vec3 attenuation;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct RayLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 attenuation;
    float innerCos;
    float outerCos;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 viewPos;
uniform Material material;
uniform float gamma = 2.2;
// TODO: shadow map matches to rLights[0] only right now
uniform sampler2DShadow shadow_map;
//uniform sampler2D shadow_map;

// TODO: shadow sube map matches to pLights[0]
uniform samplerCubeShadow shadow_cube_map;
//uniform samplerCube shadow_cube_map;
uniform float light_far_plane;

// NVIDIA throws if there is an uninitialized cubemap
// so assume only ever 1 environment map
uniform bool use_cube_map;
uniform samplerCube cube_map;

#define MAX_RAY_LIGHTS 1
uniform int num_ray_lights;
uniform RayLight rLights[MAX_RAY_LIGHTS];

#define MAX_POINT_LIGHTS 4
uniform int num_point_lights;
uniform PointLight pLights[MAX_POINT_LIGHTS];

#define MAX_SPOT_LIGHTS 2
uniform int num_spot_lights;
uniform SpotLight sLights[MAX_SPOT_LIGHTS];

vec3 CalcRayLight(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, int shadowMapIndex);
vec3 CalcPointLight(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample);
vec3 CalcSpotLight(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample);
vec3 CalcSkyboxReflection(vec3 reflectSample, vec3 norm);

float ShadowCalculation(vec4 lightFragPos, vec3 lightDir, vec3 fragNorm);
float OmniShadowCalculation(vec3 fragPos, PointLight pLight);

void main()
{
    // pre-calculate parameters
    vec4 textureDiffuse4 = texture(material.diffuse_map_0, TexCoord); // use diffuse map for alpha
    if (textureDiffuse4.a < 0.01)
        discard;

    // use surface normal
    vec3 norm;
    if (material.set_normal_map_0)
    {
        // texture values should be in range [0,1]
        norm = texture(material.normal_map_0, TexCoord).xyz;
        // transform to range [-1,1]
        norm = (2.0 * norm - vec3(1.0));
        // "naive" method using matrix multiple in each fragment, what is the performance cost?
        norm = normalize(TBN * norm);
    }
    else
    {
        norm = Normal;
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 textureDiffuse = vec3(textureDiffuse4);
    vec3 textureSpecular= vec3(texture(material.specular_map_0, TexCoord));
    
    // no shininess from model imports so we'll generate a makeshift one for now
    // + 1 to avoid zeroing FragColor. color of (0.5, 0.5, 0.5) should be about 32? so factor of 15?
    float blinn_ratio = 8;
    float textureGloss = 16 * blinn_ratio;//textureSpecular.x + textureSpecular.y + textureSpecular.z + 1* 20;

    // clamp ambient to highest single ambient source?
    vec3 outColor = vec3(0.0);

    for (int i = 0; i < min(num_ray_lights, MAX_RAY_LIGHTS); i++)
        outColor += CalcRayLight(rLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss, 0);
    for (int i = 0; i < min(num_point_lights, MAX_POINT_LIGHTS); i++)
        outColor += CalcPointLight(pLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    for (int i = 0; i < min(num_spot_lights, MAX_SPOT_LIGHTS); i++)
        outColor += CalcSpotLight(sLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    
    if (use_cube_map == true)
    {
        outColor += CalcSkyboxReflection(textureSpecular, norm);
    }

    // NOTE: this should not be further post-processed once gamma-corrected
    outColor = pow(outColor, vec3(1.0/gamma));
    FragColor = vec4(outColor, textureDiffuse4.a);
}

vec3 CalcSkyboxReflection(vec3 reflectSample, vec3 norm)
{
    vec3 I = normalize(FragPos - viewPos);
    // reflection
    vec3 R = reflect(I, normalize(norm));
    // refraction
    //vec3 R = refract(I, normalize(norm), 1.00/1.52); // air to glass ratio

    return texture(cube_map, R).xyz * (reflectSample.x + reflectSample.y, reflectSample.z)/3;
}

vec3 CalcRayLight(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, int shadowMapIndex)
{
    // no attenuation needed
    vec3 ambient = rLight.ambient * diffuseSample;

    vec3 lightDir = normalize(-rLight.direction);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = rLight.diffuse * diff * diffuseSample;

    // now blinn-phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), glossSample );
    vec3 specular = rLight.specular * spec * specularSample;

    // only 1 shadow_map so don't use index
    float shadow = ShadowCalculation(FragPosLightTransform, lightDir, norm);

    return (ambient + (1.0-shadow) * (diffuse + specular));
}

vec3 CalcPointLight(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample)
{
    // distance factor
    float dist = length(pLight.position - FragPos);
    float falloff = 1 / (pLight.attenuation.x + pLight.attenuation.y * dist + pLight.attenuation.z * (dist*dist));

    // ambient (using light ambient but material diffuse)
    vec3 ambient = pLight.ambient * diffuseSample * falloff;
    
    // diffuse (using diffuse map)
    vec3 lightDir = normalize(pLight.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = pLight.diffuse * diff * diffuseSample * falloff;

    // specular
    // now blinn-phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), glossSample ); //material.shininess);
    vec3 specular = pLight.specular * spec * specularSample * falloff;

    float shadow = OmniShadowCalculation(FragPos, pLight);

    return (ambient + (1.0-shadow) * (diffuse + specular));
}

vec3 CalcSpotLight(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample)
{
    float dist = length(sLight.position - FragPos);
    float falloff = 1 / (sLight.attenuation.x + sLight.attenuation.y * dist + sLight.attenuation.z * (dist*dist));

    vec3 ambient = sLight.ambient * diffuseSample * falloff;

    vec3 lightDir = normalize(sLight.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = sLight.diffuse * diff * diffuseSample * falloff;

    // now blinn-phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), glossSample );
    vec3 specular = sLight.specular * spec * specularSample * falloff;

    // spotlight mask
    float theta = dot(lightDir, normalize(-sLight.direction));
    float epsilon = (sLight.innerCos - sLight.outerCos);
    float intensity = clamp((theta - sLight.outerCos) / epsilon, 0.0, 1.0);
    diffuse *= intensity;
    specular *= intensity;

    return (ambient + diffuse + specular);
}

// TODO: separate this into different functions for each technique
float ShadowCalculation(vec4 lightFragPos, vec3 lightDir, vec3 fragNorm)
{
    // perspective divide (match to virtual screen)
    vec3 projCoord = lightFragPos.xyz / lightFragPos.w;
    // transform to [0,1] NDC
    projCoord = projCoord * 0.5 + 0.5;
    // values outside far plane always in lights
    if (projCoord.z > 1.0)
        return 0.0;

    // get depth of transformed spot
    float currentDepth = projCoord.z;
    // bias into surface. this no longer works with sampler2DShadow?
    //max(0.0005 * (1.0 - dot(fragNorm, lightDir)), 0.0001);
    float bias = 0.0005;

    // extra multisampling
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadow_map, 0);

    // ***** single texel sampler2DShadow sampling that spot on shadow map *****
    shadow = texture(shadow_map, projCoord-bias);
    
    return 1.0 - shadow;
}

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);  

float OmniShadowCalculation(vec3 fragPos, PointLight pLight)
{
    vec3 lightToFrag = fragPos - pLight.position;
    float currentDepth = length(lightToFrag);
    
    float bias = 0.05;

    // ***** single texel samplerCubeShadow sampling that spot on shadow cube map *****
    float shadow = texture(shadow_cube_map, vec4(lightToFrag, (currentDepth-bias) / light_far_plane));

    return 1.0 - shadow;
}
