#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

struct Material {
    sampler2D   diffuse;
    sampler2D   specular;
    float       shininess;
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

uniform Material material;

uniform SpotLight sLight;
uniform PointLight pLight;
uniform vec3 viewPos;

void main()
{
    // distance factor
    float dist = length(pLight.position - FragPos);
    float falloff = 1 / (pLight.attenuation.x + pLight.attenuation.y * dist + pLight.attenuation.z * (dist*dist));

    // ambient (using light ambient but material diffuse)
    vec3 ambient = pLight.ambient * vec3(texture(material.diffuse, TexCoord)) * falloff;

    // diffuse (using diffuse map)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(pLight.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = pLight.diffuse * diff * vec3(texture(material.diffuse, TexCoord)) * falloff;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = pLight.specular * spec * vec3(texture(material.specular, TexCoord)) * falloff;

    FragColor = vec4(ambient + diffuse + specular, 1.0);

    // spotlight
    // Better way to add additional lights?
    dist = length(sLight.position - FragPos);
    falloff = 1;//1 / (sLight.attenuation.x + sLight.attenuation.y * dist + sLight.attenuation.z * (dist*dist));

    // clamp ambient to highest single ambient source?
    ambient = sLight.ambient * vec3(texture(material.diffuse, TexCoord)) * falloff;

    lightDir = normalize(sLight.position - FragPos);
    diff = max(dot(norm, lightDir), 0.0);
    diffuse = sLight.diffuse * diff * vec3(texture(material.diffuse, TexCoord)) * falloff;

    reflectDir = reflect(-lightDir, norm);
    spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    specular = sLight.specular * spec * vec3(texture(material.specular, TexCoord)) * falloff;

    // spotlight mask
    float theta = dot(lightDir, normalize(-sLight.direction));
    float epsilon = (sLight.innerCos - sLight.outerCos);
    float intensity = clamp((theta - sLight.outerCos) / epsilon, 0.0, 1.0);
    diffuse *= intensity;
    specular *= intensity;

    FragColor += vec4(ambient + diffuse + specular, 1.0);
}
