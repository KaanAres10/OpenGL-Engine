#version 450 core

out vec4 FragColor;

in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoords;
    mat3 TBN;
} fs_in;


uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D depthMap;    
uniform float     height_scale;


uniform vec3 viewPos;

#define NR_POINT_LIGHTS 16
uniform vec3  pointLightPositions[NR_POINT_LIGHTS];


vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    const float minLayers = 32.0;
    const float maxLayers = 128.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));  
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy * height_scale; 
    vec2 deltaTexCoords = P / numLayers;

    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(depthMap, currentTexCoords).r;
  
    while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(depthMap, currentTexCoords).r;  
        currentLayerDepth += layerDepth;  
     }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(depthMap, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;  
    } 

void main() {
    vec3 viewWorldSpace   = normalize(viewPos - fs_in.fragPos);
    vec3 viewTangentSpace = normalize(transpose(fs_in.TBN) * viewWorldSpace);

    vec2 parallaxUV = ParallaxMapping(fs_in.texCoords, viewTangentSpace);
    if (parallaxUV.x > 1.0 || parallaxUV.y > 1.0 || parallaxUV.x < 0.0 || parallaxUV.y < 0.0)
        discard;

    vec3 normal = texture(normalMap, parallaxUV).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 color = texture(diffuseMap, parallaxUV).rgb;

    vec3 ambient = 0.1 * color;


    vec3 lightDir = normalize(transpose(fs_in.TBN) * (pointLightPositions[0] - fs_in.fragPos));

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color;

    vec3 halfwayDir = normalize(lightDir + viewTangentSpace);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.2) * spec;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
