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
    float textureGloss = textureSpecular.x + textureSpecular.y + textureSpecular.z + 1* 15;

    // clamp ambient to highest single ambient source?
    vec3 outColor = vec3(0.0);

    for (int i = 0; i < min(num_ray_lights, MAX_RAY_LIGHTS); i++)
        outColor += CalcRayLight(rLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    for (int i = 0; i < min(num_point_lights, MAX_POINT_LIGHTS); i++)
        outColor += CalcPointLight(pLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    for (int i = 0; i < min(num_spot_lights, MAX_SPOT_LIGHTS); i++)
        outColor += CalcSpotLight(sLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);

    FragColor = vec4(outColor, textureDiffuse4.a);
}

vec3 CalcRayLight(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample)
{
    // no attenuation needed
    vec3 ambient = rLight.ambient * diffuseSample;

    vec3 lightDir = normalize(-rLight.direction);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = rLight.diffuse * diff * diffuseSample;

    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), glossSample);
    vec3 specular = rLight.specular * spec * specularSample;

    return (ambient + diffuse + specular);
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
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), glossSample ); //material.shininess);
    vec3 specular = pLight.specular * spec * specularSample * falloff;

    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample)
{
    float dist = length(sLight.position - FragPos);
    float falloff = 1 / (sLight.attenuation.x + sLight.attenuation.y * dist + sLight.attenuation.z * (dist*dist));

    vec3 ambient = sLight.ambient * diffuseSample * falloff;

    vec3 lightDir = normalize(sLight.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = sLight.diffuse * diff * diffuseSample * falloff;

    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), glossSample);
    vec3 specular = sLight.specular * spec * specularSample * falloff;

    // spotlight mask
    float theta = dot(lightDir, normalize(-sLight.direction));
    float epsilon = (sLight.innerCos - sLight.outerCos);
    float intensity = clamp((theta - sLight.outerCos) / epsilon, 0.0, 1.0);
    diffuse *= intensity;
    specular *= intensity;

    return (ambient + diffuse + specular);
}
