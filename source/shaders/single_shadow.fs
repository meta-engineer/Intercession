#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace_Ray_0;

// All textures must be in the single Material instance "material"
// and use this TYPE_map_X format
struct Material {
    sampler2D   diffuse_map_0;
    sampler2D   specular_map_0;
    //float       shininess;

    // optionally select normal map if available
    bool        enable_normal_map_0;
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
uniform sampler2DShadow shadowMap_ray_0;
//uniform sampler2D shadowMap_ray_0;

// TODO: shadow sube map matches to pLights[0]
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

vec3 calc_ray_light(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, int shadowMapIndex);
vec3 calc_point_light(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample);
vec3 calc_spot_light(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample);
vec3 calc_skybox_reflection(vec3 reflectSample);

float frustrum_shadow_calculation(vec4 fragPosLightSpace, vec3 lightDir, vec3 fragNorm);
float omni_shadow_calculation(vec3 fragPos, PointLight pLight);

void main()
{
    // pre-calculate parameters
    vec4 textureDiffuse4 = texture(material.diffuse_map_0, TexCoord); // use diffuse map for alpha
    if (textureDiffuse4.a < 0.01)
        discard;

    // use surface normal
    vec3 norm;
    if (material.enable_normal_map_0)
    {
        norm = texture(material.normal_map_0, TexCoord).rgb;
        norm = normalize(norm * 2.0 - 1.0);
        // is this thing on?
        //FragColor = vec4(norm, 1.0);
        //return;
    }
    else
    {
        norm = normalize(Normal);
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 textureDiffuse = vec3(textureDiffuse4);
    vec3 textureSpecular= vec3(texture(material.specular_map_0, TexCoord));
    
    // no shininess from model imports so we'll generate a makeshift one for now
    // + 1 to avoid zeroing FragColor. color of (0.5, 0.5, 0.5) should be about 32? so factor of 15?
    float blinnRatio = 8;
    float textureGloss = 16 * blinnRatio;//textureSpecular.x + textureSpecular.y + textureSpecular.z + 1* 20;

    // clamp ambient to highest single ambient source?
    vec3 outColor = vec3(0.0);

    for (int i = 0; i < min(numRayLights, MAX_RAY_LIGHTS); i++)
        outColor += calc_ray_light(rLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss, 0);
    for (int i = 0; i < min(numPointLights, MAX_POINT_LIGHTS); i++)
        outColor += calc_point_light(pLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    for (int i = 0; i < min(numSpotLights, MAX_SPOT_LIGHTS); i++)
        outColor += calc_spot_light(sLights[i], norm, viewDir, textureDiffuse, textureSpecular, textureGloss);
    
    if (environmentCubemap_enable == true)
    {
        outColor += calc_skybox_reflection(textureSpecular);
    }

    // NOTE: this should not be further post-processed once gamma-corrected
    outColor = pow(outColor, vec3(1.0/gamma));
    FragColor = vec4(outColor, textureDiffuse4.a);
}

vec3 calc_skybox_reflection(vec3 reflectSample)
{
    vec3 I = normalize(FragPos - viewPos);
    // reflection
    vec3 R = reflect(I, normalize(Normal));
    // refraction
    //vec3 R = refract(I, normalize(Normal), 1.00/1.52); // air to glass ratio

    return texture(environmentCubemap, R).xyz * (reflectSample.x + reflectSample.y, reflectSample.z)/3;
}

vec3 calc_ray_light(RayLight rLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample, int shadowMapIndex)
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
    float shadow = frustrum_shadow_calculation(FragPosLightSpace_Ray_0, lightDir, norm);

    return (ambient + (1.0-shadow) * (diffuse + specular));
}

vec3 calc_point_light(PointLight pLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample)
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

    float shadow = omni_shadow_calculation(FragPos, pLight);

    return (ambient + (1.0-shadow) * (diffuse + specular));
}

vec3 calc_spot_light(SpotLight sLight, vec3 norm, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float glossSample)
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

// What is this? just a random "kernel" thats pre-computed?
vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);

float PseudoRandom(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
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

    // We can use a normal sampler2D and do anti-aliasing
    //  But, sampler2DShadow does a bilinear filtering of neighboring values on hardware!
    //  sampler2DShadow's overloaded texture() returns shadow value implicitly...

    // extra multisampling
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap_ray_0, 0);

    // *****contact hardening *****
    // increate texelSize spending on difference in depth
    //  I think frontface culling needs to be used for thick objects at certain angles?
    //  how can i compare depths when sampler2DShadow's texture() doesn't return depth value anymore?

    // ***** regular kernel blur *****
    const int kernel_radius = 1;
    for (int i = -kernel_radius; i <= kernel_radius; i++)
    {
        for (int j = -kernel_radius; j <= kernel_radius; j++)
        {
            // sampler2DShadow
            shadow += texture(shadowMap_ray_0, (projCoord + vec3(i*texelSize.x, j*texelSize.y, -bias)));
            // sampler2D
            //shadow += currentDepth-bias > texture(shadowMap_ray_0, projCoord.xy + vec2(i*texelSize.x, j*texelSize.y)).r ? 0.0 : 1.0;
        }
    }
    shadow /= 9;//pow(kernel_radius*2 + 1, 2);


    // ***** poisson sampling *****
    //for (int i = 0; i < 4; i++)
    //{
    //    int rIndex = int(16.0*PseudoRandom(floor(FragPos.xyz*1000.0), i))%16;
    //    // sampler2DShadow
    //    //shadow += texture(shadowMap_ray_0, vec3(projCoord.xy + poissonDisk[rIndex]*texelSize, projCoord.z-bias));
    //    // sampler2D
    //    shadow += currentDepth-bias > texture(shadowMap_ray_0, projCoord.xy + poissonDisk[rIndex]*texelSize).r ? 0.0 : 1.0;
    //}
    //shadow /= 4;

    // ***** single texel sampler2DShadow sampling that spot on shadow map *****
    //shadow = texture(shadowMap_ray_0, projCoord-bias);

    // ***** doing linear sampling ourselves with sampler2D *****
    // offset by 0.5 to move to corner of pixels not centre
    //vec2 pixelPos = projCoord.xy / texelSize + vec2(0.5);
    //vec2 posDecimal = fract(pixelPos);
    //vec2 startTexel = (pixelPos - posDecimal) * texelSize;
    //float samples[4];
    //samples[0] = currentDepth-bias > texture(shadowMap_ray_0, startTexel + vec2(0,          0          )).r ? 0.0 : 1.0;
    //samples[1] = currentDepth-bias > texture(shadowMap_ray_0, startTexel + vec2(texelSize.x,0          )).r ? 0.0 : 1.0;
    //samples[2] = currentDepth-bias > texture(shadowMap_ray_0, startTexel + vec2(0,          texelSize.y)).r ? 0.0 : 1.0;
    //samples[3] = currentDepth-bias > texture(shadowMap_ray_0, startTexel + vec2(texelSize.x,texelSize.y)).r ? 0.0 : 1.0;
    //// interpolate in 2 dimensions
    //float mixL = mix(samples[0], samples[2], posDecimal.y);
    //float mixR = mix(samples[1], samples[3], posDecimal.y);
    //shadow = mix(mixL, mixR, posDecimal.x);

    // ***** variance shadow map *****
    // TODO: this is likely a better method to use for future
    //  See Notes.md to follow Benny's tutorial
    //// sample depth and depth variance
    //vec2 moments = texture(shadowMap_ray_0, projCoord.xy).xy;
    //// get difference in depth
    //float d = currentDepth - moments.x;
    //// get binary shadow value
    //float p = d > 0 ? 0.0 : 1.0;
    //// compute variance from depth^2
    //float variance = max(moments.y - moments.x * moments.x, 0.00002);
    //float pMax = variance / (variance + d*d);
    //// LinearStep()
    //float low = 0.1;
    //float high = 1.0;
    //pMax = (pMax-low)/(high-low);
    //pMax = clamp(pMax, 0.0, 1.0);
    //// clamp to most shadow of 1.0
    //shadow = min(max(p, pMax), 1.0);
    
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

float omni_shadow_calculation(vec3 fragPos, PointLight pLight)
{
    vec3 lightToFrag = fragPos - pLight.position;
    float currentDepth = length(lightToFrag);
    
    float bias = 0.05;

    // ***** single texel samplerCubeShadow sampling that spot on shadow cube map *****
    float shadow = texture(shadowCubemap_point_0, vec4(lightToFrag, (currentDepth-bias) / shadowFarPlane_point_0));
    
    // ***** single texel samplerCube *****
    //float closestDepth = texture(shadowCubemap_point_0, lightToFrag).r;
    //closestDepth *= shadowFarPlane_point_0;
    //float shadow = currentDepth-bias > closestDepth ? 0.0 : 1.0;

    // ***** regular kernel blur samplerCubeShadow *****
    //float shadow = 0.0;
    //// how to calulate this intelligently?
    ////vec2 texelSize = 1.0 / textureSize(shadowCubemap_point_0, 0);
    //float radius = (1.0 + length(viewPos-fragPos) / shadowFarPlane_point_0) / 100.0;
    //for (float i = -1; i <= 1; i+=2)
    //{
    //    for (float j = -1; j <= 1; j+=2)
    //    {
    //        for (float k = -1; k <= 1; k+=2)
    //        {
    //            //samplerCubeShadow
    //            vec3 offset = vec3(i,j,k) * radius;
    //            shadow += texture(shadowCubemap_point_0, vec4(lightToFrag + offset, (currentDepth-bias) / shadowFarPlane_point_0));
    //        }
    //    }
    //}
    //shadow /= 8; //= 2^3

    // ***** regular kernel blur SamplerCube *****
    //float shadow = 0.0;
    //float samples = 20;
    //float radius = (1.0 + length(viewPos-fragPos) / shadowFarPlane_point_0) / 25.0;
    //for (int i = 0; i < samples; i++)
    //{
    //    float closestDepth = texture(shadowCubemap_point_0, lightToFrag + sampleOffsetDirections[i] * radius).r;
    //    closestDepth *= shadowFarPlane_point_0;
    //    shadow += currentDepth-bias > closestDepth ? 0.0 : 1.0;
    //}
    //shadow /= samples;

    return 1.0 - shadow;
}
