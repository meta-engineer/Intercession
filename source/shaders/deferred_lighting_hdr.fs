#version 330
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 HDRColor;

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
uniform sampler2DShadow shadow_map;
uniform mat4 light_transform;

// TODO: shadow cube map matches to pLights[0]
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

// lighting calculations now need to accept the non-uniforms from GeomBuffer
vec3 CalcRayLight(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos, int shadowMapIndex);
vec3 CalcPointLight(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos);
vec3 CalcSpotLight(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos);
vec3 CalcSkyboxReflection(vec3 reflectSample, vec3 norm, vec3 fragPos);

float ShadowCalculation(vec4 lightFragPos, vec3 lightDir, vec3 fragNorm);
float OmniShadowCalculation(PointLight pLight, vec3 fragPos);

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
    float blinn_ratio = 8;
    float textureGloss = 16 * blinn_ratio;//textureSpecular.x + textureSpecular.y + textureSpecular.z + 1* 20;

    // clamp ambient to highest single ambient source?
    vec3 outColor = vec3(0.0);

    for (int i = 0; i < min(num_ray_lights, MAX_RAY_LIGHTS); i++)
        outColor += CalcRayLight(rLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss, FragPos, 0);
    for (int i = 0; i < min(num_point_lights, MAX_POINT_LIGHTS); i++)
        outColor += CalcPointLight(pLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss, FragPos);
    for (int i = 0; i < min(num_spot_lights, MAX_SPOT_LIGHTS); i++)
        outColor += CalcSpotLight(sLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss, FragPos);
    
    if (use_cube_map == true)
    {
        outColor += CalcSkyboxReflection(textureSpecular, norm, FragPos);
    }

    // NOTE: this should not be further post-processed once gamma-corrected
    //  if we have another pass (like bloom) then we could do it there?
    //  though it might now matter
    outColor = pow(outColor, vec3(1.0/gamma));
    FragColor = vec4(outColor, 1.0);    // always 1.0 because deferred shading is only opaque

    // only colors in HDR range
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        HDRColor = vec4(FragColor.rgb, 1.0);
    else
        HDRColor = vec4(0.0, 0.0, 0.0, 1.0);
}


vec3 CalcSkyboxReflection(vec3 reflectSample, vec3 norm, vec3 fragPos)
{
    vec3 I = normalize(fragPos - viewPos);
    // reflection
    vec3 R = reflect(I, normalize(norm));
    // refraction
    //vec3 R = refract(I, normalize(norm), 1.00/1.52); // air to glass ratio

    return texture(cube_map, R).xyz * (reflectSample.x + reflectSample.y, reflectSample.z)/3;
}

vec3 CalcRayLight(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos, int shadowMapIndex)
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
    float shadow = ShadowCalculation(light_transform * vec4(fragPos,1.0), lightDir, norm);

    return (ambient + (1.0-shadow) * (diffuse + specular));
}

vec3 CalcPointLight(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos)
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

    float shadow = OmniShadowCalculation(pLight, fragPos);

    return (ambient + (1.0-shadow) * (diffuse + specular));
}

vec3 CalcSpotLight(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, vec3 fragPos)
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

float OmniShadowCalculation(PointLight pLight, vec3 fragPos)
{
    vec3 lightToFrag = fragPos - pLight.position;
    float currentDepth = length(lightToFrag);
    
    float bias = 0.05;

    // ***** single texel samplerCubeShadow sampling that spot on shadow cube map *****
    float shadow = texture(shadow_cube_map, vec4(lightToFrag, (currentDepth-bias) / light_far_plane));

    return 1.0 - shadow;
}