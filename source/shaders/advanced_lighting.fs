#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// All textures must be in the single Material instance "material"
// and use this TYPE_map_X format
struct Material {
    sampler2D   diffuse_map_0;
    sampler2D   specular_map_0;
    //float       shininess;
};

struct PointLight {
    float intensity;
    vec3 position;
    vec3 attenuation;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct RayLight {
    float intensity;
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    float intensity;
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

vec3 CalcRayLight(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample);
vec3 CalcPointLight(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample);
vec3 CalcSpotLight(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample);
vec3 CalcSkyboxReflection(vec3 reflectSample);

void main()
{
    // pre-calculate parameters
    vec4 textureDiffuse4 = texture(material.diffuse_map_0, TexCoord); // use diffuse map for alpha
    if (textureDiffuse4.a < 0.01)
        discard;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 textureDiffuse = vec3(textureDiffuse4);
    vec3 textureSpecular= vec3(texture(material.specular_map_0, TexCoord));
    
    // no shininess from model imports so we'll generate a makeshift one for now
    // + 1 to avoid zeroing FragColor. color of (0.5, 0.5, 0.5) should be about 32? so factor of 15?
    float textureGloss = 32;//textureSpecular.x + textureSpecular.y + textureSpecular.z + 1* 20;

    // clamp ambient to highest single ambient source?
    vec3 outColor = vec3(0.0);

    for (int i = 0; i < min(num_ray_lights, MAX_RAY_LIGHTS); i++)
        outColor += CalcRayLight(rLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    for (int i = 0; i < min(num_point_lights, MAX_POINT_LIGHTS); i++)
        outColor += CalcPointLight(pLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    for (int i = 0; i < min(num_spot_lights, MAX_SPOT_LIGHTS); i++)
        outColor += CalcSpotLight(sLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    
    if (use_cube_map == true)
    {
        outColor += CalcSkyboxReflection(textureSpecular);
    }

    FragColor = vec4(outColor, textureDiffuse4.a);
}

vec3 CalcSkyboxReflection(vec3 reflectSample)
{
    vec3 I = normalize(FragPos - viewPos);
    // reflection
    vec3 R = reflect(I, normalize(Normal));
    // refraction
    //vec3 R = refract(I, normalize(Normal), 1.00/1.52); // air to glass ratio

    return texture(cube_map, R).xyz * (reflectSample.x + reflectSample.y, reflectSample.z)/3;

}

vec3 CalcRayLight(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample)
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

    return (ambient + diffuse + specular) * rLight.intensity;
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

    return (ambient + diffuse + specular) * pLight.intensity;
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

    return (ambient + diffuse + specular) * sLight.intensity;
}
