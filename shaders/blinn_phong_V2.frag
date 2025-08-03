#version 450 core

out vec4 FragColor;

in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;

uniform sampler2D diffuseMap;
uniform vec3 viewPos;

uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;

#define NR_POINT_LIGHTS 64
uniform vec3 pointLightPositions[NR_POINT_LIGHTS];
uniform vec3 pointLightColors[NR_POINT_LIGHTS];
uniform int uPointLightCount;

const float ambientStrength = 0.1;
const float shininess       = 64.0;

vec3 BlinnPhong(vec3 N, vec3 V, vec3 L, vec3 lightCol) {
    // diffuse term
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse  = diff * lightCol;
    // specular term
    vec3 H        = normalize(L + V);
    float spec    = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = spec * lightCol;
    return diffuse + specular;
}

vec3 CalcDirLight(vec3 N, vec3 V) {
    vec3 L     = normalize(-dirLightDirection);
    vec3 ambient = ambientStrength * dirLightColor;
    vec3 phong   = BlinnPhong(N, V, L, dirLightColor);
    return ambient + phong;
}

vec3 CalcPointLight(vec3 N, vec3 V, vec3 P, vec3 lightPos, vec3 lightCol) {
    vec3 L    = normalize(lightPos - P);
    vec3 phong = BlinnPhong(N, V, L, lightCol);

    float d    = length(lightPos - P);
    float att  = 1.0 / (1.0 + 0.00000198f*d + 0.00000155f*d*d);
    return phong * att;
}

void main() {
    vec3 N     = normalize(normal);
    vec3 V     = normalize(viewPos - fragPos);
    vec4 tex = texture(diffuseMap, texCoords);
    if (tex.a < 0.5)  
        discard;
    vec3 albedo= tex.rgb;

    vec3 lighting = CalcDirLight(N, V);

    for (int i = 0; i < uPointLightCount; ++i) {
        lighting += CalcPointLight(N, V, fragPos,
                                   pointLightPositions[i],
                                   pointLightColors[i]);
    }

    vec3 color = albedo * lighting;

    FragColor = vec4(color, 1.0);
}
