#version 450 core

out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
}; 

struct DirectionalLight {  
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;


    float constant;
    float linear;
    float quadratic;
};


in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;


uniform vec3 viewPos;
uniform Material material;

uniform DirectionalLight directionalLight;
#define NR_POINT_LIGHTS 64
uniform PointLight pointLights[NR_POINT_LIGHTS];

uniform int uPointLightCount;

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir);  
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);  


void main() {    
    vec3 normalDir = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);

    vec3 result = calcDirectionalLight(directionalLight, normalDir, viewDir);

    for (int i = 0; i < uPointLightCount; i++) {
        result += calcPointLight(pointLights[i], normalDir, fragPos, viewDir);
    }

    FragColor = vec4(result, 1.0);
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 2.0);
    
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, texCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, texCoords));

    return (ambient + diffuse + specular);
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 2.0);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / 
          (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, texCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, texCoords));

     ambient *= attenuation;
     diffuse *= attenuation;
     specular *= attenuation;

     return (ambient + diffuse + specular);
}
