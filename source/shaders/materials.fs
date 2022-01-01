#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec3 OutColor;
in vec2 TexCoord;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct PointLight {
    vec3 position;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform sampler2D texture;

uniform PointLight light;

uniform vec3 viewPos;

void main()
{
    // distance factor
    float falloff = 1;// / length(light.position - FragPos);

    // ambient
    vec3 ambient = light.ambient * material.ambient * falloff;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * material.diffuse * falloff;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular * falloff;

    FragColor = vec4(ambient + diffuse + specular, 1.0) * texture(texture, TexCoord);
}