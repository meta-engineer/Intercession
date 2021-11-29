#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightTransform;

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
uniform float gamma = 2.2;
// TODO: shadow map matches to rLights[0] only right now
uniform sampler2DShadow shadow_map;
//uniform sampler2D shadow_map;

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
vec3 CalcSkyboxReflection(vec3 reflectSample);

float ShadowCalculation(vec4 lightFragPos, vec3 lightDir, vec3 fragNorm);

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
        outColor += CalcSkyboxReflection(textureSpecular);
    }

    // NOTE: this should not be further post-processed once gamma-corrected
    outColor = pow(outColor, vec3(1.0/gamma));
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

    // We can use a normal sampler2D and do anti-aliasing
    //  But, sampler2DShadow does a bilinear filtering of neighboring values on hardware!
    //  sampler2DShadow's overloaded texture() returns shadow value implicitly...

    // extra multisampling
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadow_map, 0);

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
            shadow += texture(shadow_map, (projCoord + vec3(i*texelSize.x, j*texelSize.y, -bias)));
            // sampler2D
            //shadow += currentDepth-bias > texture(shadow_map, projCoord.xy + vec2(i*texelSize.x, j*texelSize.y)).r ? 0.0 : 1.0;
        }
    }
    shadow /= pow(kernel_radius*2 + 1, 2);


    // ***** poisson sampling *****
    //for (int i = 0; i < 4; i++)
    //{
    //    int rIndex = int(16.0*PseudoRandom(floor(FragPos.xyz*1000.0), i))%16;
    //    // sampler2DShadow
    //    //shadow += texture(shadow_map, vec3(projCoord.xy + poissonDisk[rIndex]*texelSize, projCoord.z-bias));
    //    // sampler2D
    //    shadow += currentDepth-bias > texture(shadow_map, projCoord.xy + poissonDisk[rIndex]*texelSize).r ? 0.0 : 1.0;
    //}
    //shadow /= 4;

    // ***** single texel sampler2DShadow sampling that spot on shadow map *****
    //shadow = texture(shadow_map, projCoord-bias);

    // ***** doing linear sampling ourselves with sampler2D *****
    // offset by 0.5 to move to corner of pixels not centre
    //vec2 pixelPos = projCoord.xy / texelSize + vec2(0.5);
    //vec2 posDecimal = fract(pixelPos);
    //vec2 startTexel = (pixelPos - posDecimal) * texelSize;
    //float samples[4];
    //samples[0] = currentDepth-bias > texture(shadow_map, startTexel + vec2(0,          0          )).r ? 0.0 : 1.0;
    //samples[1] = currentDepth-bias > texture(shadow_map, startTexel + vec2(texelSize.x,0          )).r ? 0.0 : 1.0;
    //samples[2] = currentDepth-bias > texture(shadow_map, startTexel + vec2(0,          texelSize.y)).r ? 0.0 : 1.0;
    //samples[3] = currentDepth-bias > texture(shadow_map, startTexel + vec2(texelSize.x,texelSize.y)).r ? 0.0 : 1.0;
    //// interpolate in 2 dimensions
    //float mixL = mix(samples[0], samples[2], posDecimal.y);
    //float mixR = mix(samples[1], samples[3], posDecimal.y);
    //shadow = mix(mixL, mixR, posDecimal.x);

    // ***** variance shadow map *****
    // TODO: this is likely a better method to use for future
    //  See Notes.md to follow Benny's tutorial
    //// sample depth and depth variance
    //vec2 moments = texture(shadow_map, projCoord.xy).xy;
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
