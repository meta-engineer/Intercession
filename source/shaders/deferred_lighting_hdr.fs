#version 330
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 HdrColor;

in vec2 TexCoord;

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

uniform sampler2D GPosition;
uniform sampler2D GNormal;
uniform sampler2D GColorSpec;

uniform vec3 viewPos;
uniform float gamma = 2.2;

// TODO: shadow map matches to rLights[0] only right now
uniform sampler2DShadow shadowMap_ray_0;
uniform mat4 lightTransform_ray_0;

// TODO: shadow cube map matches to pLights[0]
uniform samplerCubeShadow shadowCubemap_point_0;
//uniform samplerCube shadowCubemap_point_0;
uniform float shadowFarPlane_point_0;

// NVIDIA throws if there is an uninitialized cubemap
// so assume only ever 1 environment map
uniform bool environmentCubemap_enable;
uniform samplerCube environmentCubemap;

#define MAX_RAY_LIGHTS 1
uniform int numRayLights;
uniform RayLight rLights[MAX_RAY_LIGHTS];

#define MAX_POINT_LIGHTS 4
uniform int numPointLights;
uniform PointLight pLights[MAX_POINT_LIGHTS];

#define MAX_SPOT_LIGHTS 2
uniform int numSpotLights;
uniform SpotLight sLights[MAX_SPOT_LIGHTS];

// lighting calculations now need to accept the non-uniforms from GeomBuffer
vec3 calc_ray_light(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos, int shadowMapIndex);
vec3 calc_point_light(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos);
vec3 calc_spot_light(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos);
vec3 calc_skybox_reflection(vec3 reflectSample, vec3 norm, vec3 fragPos);

float frustrum_shadow_calculation(vec4 fragPosLightSpace, vec3 lightDir, vec3 fragNorm);
float omni_shadow_calculation(PointLight pLight, vec3 fragPos);

void main()
{
    // derive in valuse from geom buffer
    vec3 FragPos = texture(GPosition, TexCoord).rgb;
    vec3 norm = texture(GNormal, TexCoord).rgb;
    vec3 textureDiffuse = texture(GColorSpec, TexCoord).rgb;
    // identical vec3 to be consistent with previous lighting calcs
    vec3 textureSpecular = vec3(texture(GColorSpec, TexCoord).a);
    // then do as usual

    vec3 viewDir = normalize(viewPos - FragPos);

    // TODO: look into setting this as uniform/material. or wait for PBR
    float blinnRatio = 8;
    float textureGloss = 16 * blinnRatio;//textureSpecular.x + textureSpecular.y + textureSpecular.z + 1* 20;

    // clamp ambient to highest single ambient source?
    vec3 outColor = vec3(0.0);

    for (int i = 0; i < min(numRayLights, MAX_RAY_LIGHTS); i++)
        outColor += calc_ray_light(rLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss, FragPos, 0);
    for (int i = 0; i < min(numPointLights, MAX_POINT_LIGHTS); i++)
        outColor += calc_point_light(pLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss, FragPos);
    for (int i = 0; i < min(numSpotLights, MAX_SPOT_LIGHTS); i++)
        outColor += calc_spot_light(sLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss, FragPos);
    
    if (environmentCubemap_enable == true)
    {
        outColor += calc_skybox_reflection(textureSpecular, norm, FragPos);
    }

    // NOTE: this should not be further post-processed once gamma-corrected
    //  if we have another pass (like bloom) then we could do it there?
    //  though it might now matter
    outColor = pow(outColor, vec3(1.0/gamma));
    FragColor = vec4(outColor, 1.0);    // always 1.0 because deferred shading is only opaque

    // only colors in HDR range
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        HdrColor = vec4(FragColor.rgb, 1.0);
    else
        HdrColor = vec4(0.0, 0.0, 0.0, 1.0);
}


vec3 calc_skybox_reflection(vec3 reflectSample, vec3 norm, vec3 fragPos)
{
    vec3 I = normalize(fragPos - viewPos);
    // reflection
    vec3 R = reflect(I, normalize(norm));
    // refraction
    //vec3 R = refract(I, normalize(norm), 1.00/1.52); // air to glass ratio

    return texture(environmentCubemap, R).xyz * (reflectSample.x + reflectSample.y, reflectSample.z)/3;
}

vec3 calc_ray_light(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos, int shadowMapIndex)
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

    // only 1 shadowMap_ray_0 so don't use index
    float shadow = frustrum_shadow_calculation(lightTransform_ray_0 * vec4(fragPos,1.0), lightDir, norm);

    return (ambient + (1.0-shadow) * (diffuse + specular));
}

vec3 calc_point_light(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos)
{
    // distance factor
    float dist = length(pLight.position - fragPos);
    float falloff = 1 / (pLight.attenuation.x + pLight.attenuation.y * dist + pLight.attenuation.z * (dist*dist));

    // ambient (using light ambient but material diffuse)
    vec3 ambient = pLight.ambient * diffuseSample * falloff;
    
    // diffuse (using diffuse map)
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = pLight.diffuse * diff * diffuseSample * falloff;

    // specular
    // now blinn-phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), glossSample ); //material.shininess);
    vec3 specular = pLight.specular * spec * specularSample * falloff;

    float shadow = omni_shadow_calculation(pLight, fragPos);

    return (ambient + (1.0-shadow) * (diffuse + specular));
}

vec3 calc_spot_light(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos)
{
    float dist = length(sLight.position - fragPos);
    float falloff = 1 / (sLight.attenuation.x + sLight.attenuation.y * dist + sLight.attenuation.z * (dist*dist));

    vec3 ambient = sLight.ambient * diffuseSample * falloff;

    vec3 lightDir = normalize(sLight.position - fragPos);
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
float frustrum_shadow_calculation(vec4 fragPosLightSpace, vec3 lightDir, vec3 fragNorm)
{
    // perspective divide (match to virtual screen)
    vec3 projCoord = fragPosLightSpace.xyz / fragPosLightSpace.w;
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
    vec2 texelSize = 1.0 / textureSize(shadowMap_ray_0, 0);

    // ***** single texel sampler2DShadow sampling that spot on shadow map *****
    shadow = texture(shadowMap_ray_0, projCoord-bias);
    
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

float omni_shadow_calculation(PointLight pLight, vec3 fragPos)
{
    vec3 lightToFrag = fragPos - pLight.position;
    float currentDepth = length(lightToFrag);
    
    float bias = 0.05;

    // ***** single texel samplerCubeShadow sampling that spot on shadow cube map *****
    float shadow = texture(shadowCubemap_point_0, vec4(lightToFrag, (currentDepth-bias) / shadowFarPlane_point_0));

    return 1.0 - shadow;
}