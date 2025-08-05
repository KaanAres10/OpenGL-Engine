#version 450 core

out vec4 FragColor;

in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoords;
    vec4 fragPosLightSpace;
} fs_in;


uniform sampler2D diffuseMap;
uniform sampler2D shadowMap;

uniform vec3 viewPos;

uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;

#define NR_POINT_LIGHTS 16
uniform samplerCube shadowCubes[NR_POINT_LIGHTS];
uniform vec3          pointLightPositions[NR_POINT_LIGHTS];
uniform vec3          pointLightColors[NR_POINT_LIGHTS];
uniform float         far_planes[NR_POINT_LIGHTS];
uniform int           uPointLightCount;

const float ambientStrength = 0.1;
const float shininess       = 64.0;

float CalcDirShadow(vec4 fragPosLightSpace, vec3 N)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;


    float closestDepth = texture(shadowMap, projCoords.xy).r;

    float currentDepth = projCoords.z;

    float bias = max(0.1 * (1.0 - dot(N, dirLightDirection)), 0.01);


    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    return shadow;
}

float CalcPointShadow(int index, vec3 fragPos) {


   vec3 sampleOffsetDirections[20] = vec3[]
   (
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
   );   

    vec3 fragToLight = fragPos - pointLightPositions[index];
    float closestDepth = texture(shadowCubes[index], fragToLight).r * far_planes[index];
    float currentDepth = length(fragToLight);
    
    float shadow = 0.0;
    float bias   = 0.15;
    int samples  = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_planes[index])) / 25.0;  
    for(int i = 0; i < samples; i++)
    {
        float closestDepth = texture(shadowCubes[index], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_planes[index];   
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);  
    return  shadow;
}

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

vec3 CalcDirLight(vec3 N, vec3 V, float shadow) {
    vec3 L     = normalize(-dirLightDirection);
    vec3 ambient = ambientStrength * dirLightColor;
    vec3 phong   = BlinnPhong(N, V, L, dirLightColor);
    return ambient + (1 - shadow) * phong;
}

vec3 CalcPointLight(vec3 N, vec3 V, vec3 P, vec3 lightPos, vec3 lightCol) {
    vec3 L    = normalize(lightPos - P);
    vec3 phong = BlinnPhong(N, V, L, lightCol);

    float d    = length(lightPos - P);
    float att  = 1.0 / (1.0 + 0.0000198f*d + 0.0000155f*d*d);
    return phong * att;
}

void main() {
    vec3 N     = normalize(fs_in.normal);
    vec3 V     = normalize(viewPos - fs_in.fragPos);
    vec4 tex = texture(diffuseMap, fs_in.texCoords);
    if (tex.a < 0.5)  
        discard;
    vec3 albedo= tex.rgb;

    float shadow = CalcDirShadow(fs_in.fragPosLightSpace, N);
    float ptFill = 0.0;
    for(int i = 0; i < uPointLightCount; i++) {
        float d = length(pointLightPositions[i] - fs_in.fragPos);
        float att = 1.0 / (1.0 + 0.0000198 * d + 0.0000155 * d * d);
        ptFill += att;
    }
    ptFill = clamp(ptFill, 0.0, 1.0);

    shadow = mix(shadow, 0.0, ptFill);


    vec3 lighting = CalcDirLight(N, V, shadow);

    for (int i = 0; i < uPointLightCount; ++i) {
        float ptShadow = CalcPointShadow(i, fs_in.fragPos);
        vec3 ptLight  = CalcPointLight(N, V, fs_in.fragPos,
                                       pointLightPositions[i],
                                       pointLightColors[i]);
        lighting += (1.0 - ptShadow) * ptLight;
    }

    vec3 color = albedo * lighting;

    FragColor = vec4(color, 1.0);
}
