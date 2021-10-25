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
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;

uniform PointLight light;
uniform vec3 viewPos;

void main()
{
    // distance factor
    float falloff = 1;// / length(light.position - FragPos);

    // ambient (using light ambient but material diffuse)
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord)) * falloff;

    // diffuse (using diffuse map)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoord)) * falloff;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoord)) * falloff;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}