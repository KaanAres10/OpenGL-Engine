#version 450 core
layout(location=0) out vec4 FragColor;
layout(location=1) out vec4 BrightColor;

in vec2 texCoords;

uniform sampler2DMS gPosition;
uniform sampler2DMS gNormal;
uniform sampler2DMS gAlbedoSpec;
uniform int uSamples;

uniform vec3 viewPos;

// Directional Light
uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// Point Lights
#define NR_POINT_LIGHTS 128
#define NR_POINT_SHADOW_LIGHTS 2
uniform samplerCube shadowCubes[NR_POINT_SHADOW_LIGHTS];
uniform vec3          pointLightPositions[NR_POINT_LIGHTS];
uniform vec3          pointLightColors[NR_POINT_LIGHTS];
uniform float         far_planes[NR_POINT_SHADOW_LIGHTS];
uniform int           uPointLightCount;

const float ambientStrength = 0.1;
const float shininess       = 64.0;



float CalcDirShadow(vec3 fragPos, vec3 N)
{
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
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

vec3 BlinnPhong(vec3 N, vec3 V, vec3 L, vec3 lightCol, float specStrength) {
    // diffuse term
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse  = diff * lightCol;
    // specular term
    vec3 H        = normalize(L + V);
    float spec    = pow(max(dot(N, H), 0.0), shininess) * specStrength;
    vec3 specular = spec * lightCol;
    return diffuse + specular;
}

vec3 CalcDirLight(vec3 N, vec3 V, float shadow, float specStrength) {
    vec3 L     = normalize(-dirLightDirection);
    vec3 ambient = ambientStrength * dirLightColor;
    vec3 phong   = BlinnPhong(N, V, L, dirLightColor, specStrength);

    return ambient + (1 - shadow) * phong;
}

vec3 CalcPointLight(vec3 N, vec3 V, vec3 P, vec3 lightPos, vec3 lightCol, float specStrength) {
    vec3 L    = normalize(lightPos - P);
    vec3 phong = BlinnPhong(N, V, L, lightCol, specStrength);

    float d    = length(lightPos - P);
    float att  = 1.0 / (1.0 + 0.0000198f*d + 0.0000155f*d*d);
    return phong * att;
}

void main() {
    ivec2 ip = ivec2(gl_FragCoord.xy);

    vec3 accum = vec3(0.0);
    float brightAccum = 0.0;
    int covered = 0;


    for (int s = 0; s < uSamples; s++)
    {
        vec3 fragPos = texelFetch(gPosition, ip, s).rgb;
        vec3 N = texelFetch(gNormal, ip, s).rgb;
        vec3 Ngeom = normalize(cross(dFdx(fragPos), dFdy(fragPos)));

        if (dot(N, N) <= 1e-8) continue;   
        N = normalize(N);

        vec4 albedoSpec = texelFetch(gAlbedoSpec, ip, s);

        vec3 albedo = albedoSpec.rgb;

        float specularStrength = albedoSpec.a;

        vec3 V = normalize(viewPos - fragPos);

        float shadow = CalcDirShadow(fragPos, N);
        float ptFill = 0.0;
        for(int i = 0; i < uPointLightCount; i++) {
            float d = length(pointLightPositions[i] - fragPos);
            float att = 1.0 / (1.0 + 0.0000198 * d + 0.0005 * d * d);
            ptFill += att;
        }
        ptFill = clamp(ptFill, 0.0, 1.0);

        shadow = mix(shadow, 0.0, ptFill);


        vec3 lighting = CalcDirLight(N, V, shadow, specularStrength);


        int countOfPointShadow = 0;
        for (int i = 0; i < uPointLightCount; i++) {
            float ptShadow = 0;
            if (countOfPointShadow < NR_POINT_SHADOW_LIGHTS) {
                ptShadow = CalcPointShadow(i, fragPos);
                countOfPointShadow++;
            }
            vec3 ptLight  = CalcPointLight(N, V, fragPos,
                                       pointLightPositions[i],
                                       pointLightColors[i], specularStrength);
            
            lighting += (1.0 - ptShadow) * ptLight;
        }

        vec3 color = albedo * lighting;
        accum += color;

        // gray-scale
        float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

        brightAccum += max(brightness - 1.0, 0.0);

        covered++;
     }

     if (covered == 0) {
        FragColor  = vec4(0.0);
        BrightColor= vec4(0.0);
        return;
     }

     vec3 outColor = accum / float(covered);
     FragColor   = vec4(outColor, 1.0);
     BrightColor = vec4(vec3(brightAccum / float(covered)), 1.0);
}
